#include "can_service.h"

#include <string.h>

static int32_t CANServiceFindIndex(const CANService *service, const CANOperation *operation)
{
    uint32_t i;

    if ((service == 0) || (operation == 0) || (service->operations == 0) || (service->in_use == 0))
    {
        return -1;
    }

    for (i = 0U; i < service->capacity; ++i)
    {
        if (&service->operations[i] == operation)
        {
            return (int32_t)i;
        }
    }

    return -1;
}

void CANServiceInit(CANService *service,
                    CANOperation *operations,
                    bool *in_use,
                    uint32_t capacity)
{
    uint32_t i;

    if (service == 0)
    {
        return;
    }

    memset(service, 0, sizeof(*service));
    service->operations = operations;
    service->in_use = in_use;
    service->capacity = capacity;

    if ((operations != 0) && (in_use != 0))
    {
        for (i = 0U; i < capacity; ++i)
        {
            in_use[i] = false;
            CANOperationInit(&operations[i]);
        }
    }
}

void CANServiceBindContext(CANService *service, CANContext *context)
{
    if (service == 0)
    {
        return;
    }

    service->context = context;
}

CANContext *CANServiceGetContext(CANService *service)
{
    if (service == 0)
    {
        return 0;
    }

    return service->context;
}

uint32_t CANServiceGetCapacity(const CANService *service)
{
    if (service == 0)
    {
        return 0U;
    }

    return service->capacity;
}

uint32_t CANServiceGetInUseCount(const CANService *service)
{
    uint32_t i;
    uint32_t count;

    if ((service == 0) || (service->in_use == 0))
    {
        return 0U;
    }

    count = 0U;
    for (i = 0U; i < service->capacity; ++i)
    {
        if (service->in_use[i] == true)
        {
            count++;
        }
    }

    return count;
}

CANOperation *CANServiceAcquireOperation(CANService *service)
{
    uint32_t i;

    if ((service == 0) ||
        (service->operations == 0) ||
        (service->in_use == 0) ||
        (service->capacity == 0U))
    {
        return 0;
    }

    for (i = 0U; i < service->capacity; ++i)
    {
        if (service->in_use[i] == false)
        {
            service->in_use[i] = true;
            CANOperationInit(&service->operations[i]);
            return &service->operations[i];
        }
    }

    return 0;
}

void CANServiceReleaseOperation(CANService *service, CANOperation *operation)
{
    int32_t index;

    if ((service == 0) || (operation == 0))
    {
        return;
    }

    index = CANServiceFindIndex(service, operation);
    if (index < 0)
    {
        return;
    }

    service->in_use[(uint32_t)index] = false;
    CANOperationInit(operation);
}

CANStatus CANServiceSubmitSend(CANService *service,
                               CANOperation **out_operation,
                               const CANFrame *frame,
                               uint32_t timeout_ms)
{
    CANOperation *operation;
    CANStatus status;

    if ((service == 0) || (out_operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    *out_operation = 0;

    if (service->context == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    operation = CANServiceAcquireOperation(service);
    if (operation == 0)
    {
        return CAN_STATUS_EBUSY;
    }

    status = CANContextSubmitSend(service->context, operation, frame, timeout_ms);
    if (status != CAN_STATUS_OK)
    {
        CANServiceReleaseOperation(service, operation);
        return status;
    }

    *out_operation = operation;
    return CAN_STATUS_OK;
}

CANStatus CANServiceSubmitReceive(CANService *service,
                                  CANOperation **out_operation,
                                  CANFrame *frame,
                                  uint32_t timeout_ms)
{
    CANOperation *operation;
    CANStatus status;

    if ((service == 0) || (out_operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    *out_operation = 0;

    if (service->context == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    operation = CANServiceAcquireOperation(service);
    if (operation == 0)
    {
        return CAN_STATUS_EBUSY;
    }

    status = CANContextSubmitReceive(service->context, operation, frame, timeout_ms);
    if (status != CAN_STATUS_OK)
    {
        CANServiceReleaseOperation(service, operation);
        return status;
    }

    *out_operation = operation;
    return CAN_STATUS_OK;
}

CANStatus CANServiceSubmitPoll(CANService *service,
                               CANOperation **out_operation,
                               uint32_t interest_mask,
                               uint32_t timeout_ms,
                               uint32_t *ready_mask)
{
    CANOperation *operation;
    CANStatus status;

    if ((service == 0) || (out_operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    *out_operation = 0;

    if (service->context == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    operation = CANServiceAcquireOperation(service);
    if (operation == 0)
    {
        return CAN_STATUS_EBUSY;
    }

    status = CANContextSubmitPoll(service->context,
                                  operation,
                                  interest_mask,
                                  timeout_ms,
                                  ready_mask);
    if (status != CAN_STATUS_OK)
    {
        CANServiceReleaseOperation(service, operation);
        return status;
    }

    *out_operation = operation;
    return CAN_STATUS_OK;
}

CANExecutorRunOnceResult CANServicePollOne(CANService *service)
{
    if ((service == 0) || (service->context == 0))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    return CANContextPollOne(service->context);
}

CANExecutorRunOnceResult CANServiceRunOne(CANService *service)
{
    if ((service == 0) || (service->context == 0))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    return CANContextRunOne(service->context);
}

CANExecutorRunOnceResult CANServiceDispatchOne(CANService *service)
{
    if ((service == 0) || (service->context == 0))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    return CANContextDispatchOne(service->context);
}

CANStatus CANServicePoll(CANService *service,
                         uint32_t max_steps,
                         uint32_t *completed_count,
                         uint32_t *remaining_count)
{
    if ((service == 0) || (service->context == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANContextPoll(service->context,
                          max_steps,
                          completed_count,
                          remaining_count);
}