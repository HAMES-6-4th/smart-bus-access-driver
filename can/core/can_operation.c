#include "can_operation.h"

#include <string.h>

static bool CANOperationHasRuntime(const CANOperation *operation)
{
    return
        (operation != 0) &&
        (operation->core != 0) &&
        (operation->core->runtime.get_tick_ms != 0);
}

static uint32_t CANOperationNowMs(const CANOperation *operation)
{
    return operation->core->runtime.get_tick_ms(operation->core->runtime.user_context);
}

static bool CANOperationFiniteTimeoutEnabled(const CANOperation *operation)
{
    return
        (operation != 0) &&
        (operation->timeout_ms != 0U) &&
        (operation->timeout_ms != CAN_TIMEOUT_INFINITE);
}

static bool CANOperationDeadlineExpired(const CANOperation *operation)
{
    uint32_t now_ms;

    if ((operation == 0) || (operation->deadline_started == false))
    {
        return false;
    }

    if (CANOperationFiniteTimeoutEnabled(operation) == false)
    {
        return false;
    }

    now_ms = CANOperationNowMs(operation);
    return ((uint32_t)(now_ms - operation->start_ms) >= operation->timeout_ms);
}

static void CANOperationComplete(CANOperation *operation, CANStatus status)
{
    if (operation == 0)
    {
        return;
    }

    operation->result_status = status;
    operation->state = CAN_OPERATION_STATE_COMPLETED;
}

void CANOperationInit(CANOperation *operation)
{
    if (operation == 0)
    {
        return;
    }

    memset(operation, 0, sizeof(*operation));
    operation->state = CAN_OPERATION_STATE_IDLE;
    operation->result_status = CAN_STATUS_OK;
}

void CANOperationPrepareSend(CANOperation *operation,
                             CANCore *core,
                             const CANFrame *frame,
                             uint32_t timeout_ms)
{
    if (operation == 0)
    {
        return;
    }

    CANOperationInit(operation);
    operation->type = CAN_OPERATION_TYPE_SEND;
    operation->state = CAN_OPERATION_STATE_READY;
    operation->core = core;
    operation->tx_frame = frame;
    operation->timeout_ms = timeout_ms;
}

void CANOperationPrepareReceive(CANOperation *operation,
                                CANCore *core,
                                CANFrame *frame,
                                uint32_t timeout_ms)
{
    if (operation == 0)
    {
        return;
    }

    CANOperationInit(operation);
    operation->type = CAN_OPERATION_TYPE_RECEIVE;
    operation->state = CAN_OPERATION_STATE_READY;
    operation->core = core;
    operation->rx_frame = frame;
    operation->timeout_ms = timeout_ms;
}

void CANOperationPreparePoll(CANOperation *operation,
                             CANCore *core,
                             uint32_t interest_mask,
                             uint32_t timeout_ms,
                             uint32_t *ready_mask)
{
    if (operation == 0)
    {
        return;
    }

    CANOperationInit(operation);
    operation->type = CAN_OPERATION_TYPE_POLL;
    operation->state = CAN_OPERATION_STATE_READY;
    operation->core = core;
    operation->interest_mask = interest_mask;
    operation->timeout_ms = timeout_ms;
    operation->ready_mask = ready_mask;
}

CANStatus CANOperationSubmit(CANOperation *operation)
{
    const uint32_t valid_mask =
        (uint32_t)(CAN_CORE_EVENT_RX_READY |
                   CAN_CORE_EVENT_TX_READY |
                   CAN_CORE_EVENT_ERROR |
                   CAN_CORE_EVENT_STATE);

    if (operation == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (operation->state != CAN_OPERATION_STATE_READY)
    {
        operation->result_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    if ((operation->core == 0) || (CANCoreIsStarted(operation->core) == false))
    {
        operation->result_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    switch (operation->type)
    {
        case CAN_OPERATION_TYPE_SEND:
            if (operation->tx_frame == 0)
            {
                operation->result_status = CAN_STATUS_EINVAL;
                return CAN_STATUS_EINVAL;
            }
            break;

        case CAN_OPERATION_TYPE_RECEIVE:
            if (operation->rx_frame == 0)
            {
                operation->result_status = CAN_STATUS_EINVAL;
                return CAN_STATUS_EINVAL;
            }
            break;

        case CAN_OPERATION_TYPE_POLL:
            if ((operation->ready_mask == 0) ||
                (operation->interest_mask == 0U) ||
                ((operation->interest_mask & ~valid_mask) != 0U))
            {
                operation->result_status = CAN_STATUS_EINVAL;
                return CAN_STATUS_EINVAL;
            }
            *operation->ready_mask = 0U;
            break;

        case CAN_OPERATION_TYPE_NONE:
        default:
            operation->result_status = CAN_STATUS_EINVAL;
            return CAN_STATUS_EINVAL;
    }

    if (CANOperationFiniteTimeoutEnabled(operation) && (CANOperationHasRuntime(operation) == false))
    {
        operation->result_status = CAN_STATUS_EUNSUPPORTED;
        return CAN_STATUS_EUNSUPPORTED;
    }

    operation->deadline_started = false;
    operation->result_status = CAN_STATUS_OK;
    operation->state = CAN_OPERATION_STATE_PENDING;
    return CAN_STATUS_OK;
}

CANOperationRunResult CANOperationRunOnce(CANOperation *operation)
{
    CANStatus status;
    uint32_t queried_ready_mask;

    if (operation == 0)
    {
        return CAN_OPERATION_RUN_ERROR;
    }

    if (operation->state == CAN_OPERATION_STATE_COMPLETED)
    {
        return CAN_OPERATION_RUN_COMPLETED;
    }

    if (operation->state != CAN_OPERATION_STATE_PENDING)
    {
        operation->result_status = CAN_STATUS_EBUSY;
        return CAN_OPERATION_RUN_ERROR;
    }

    if (CANOperationFiniteTimeoutEnabled(operation) && (operation->deadline_started == false))
    {
        operation->start_ms = CANOperationNowMs(operation);
        operation->deadline_started = true;
    }

    switch (operation->type)
    {
        case CAN_OPERATION_TYPE_SEND:
            status = CANCoreTrySend(operation->core, operation->tx_frame);
            if (status == CAN_STATUS_OK)
            {
                CANOperationComplete(operation, CAN_STATUS_OK);
                return CAN_OPERATION_RUN_COMPLETED;
            }

            if (status == CAN_STATUS_EBUSY)
            {
                if (operation->timeout_ms == 0U)
                {
                    CANOperationComplete(operation, CAN_STATUS_EBUSY);
                    return CAN_OPERATION_RUN_COMPLETED;
                }

                if (CANOperationDeadlineExpired(operation))
                {
                    CANOperationComplete(operation, CAN_STATUS_ETIMEOUT);
                    return CAN_OPERATION_RUN_COMPLETED;
                }

                return CAN_OPERATION_RUN_PENDING;
            }

            CANOperationComplete(operation, status);
            return CAN_OPERATION_RUN_COMPLETED;

        case CAN_OPERATION_TYPE_RECEIVE:
            status = CANCoreTryReceive(operation->core, operation->rx_frame);
            if (status == CAN_STATUS_OK)
            {
                CANOperationComplete(operation, CAN_STATUS_OK);
                return CAN_OPERATION_RUN_COMPLETED;
            }

            if (status == CAN_STATUS_ENODATA)
            {
                if (operation->timeout_ms == 0U)
                {
                    CANOperationComplete(operation, CAN_STATUS_ENODATA);
                    return CAN_OPERATION_RUN_COMPLETED;
                }

                if (CANOperationDeadlineExpired(operation))
                {
                    CANOperationComplete(operation, CAN_STATUS_ETIMEOUT);
                    return CAN_OPERATION_RUN_COMPLETED;
                }

                return CAN_OPERATION_RUN_PENDING;
            }

            CANOperationComplete(operation, status);
            return CAN_OPERATION_RUN_COMPLETED;

        case CAN_OPERATION_TYPE_POLL:
            queried_ready_mask = 0U;
            status = CANCorePoll(operation->core,
                                 operation->interest_mask,
                                 0U,
                                 &queried_ready_mask);

            if (status == CAN_STATUS_OK)
            {
                *operation->ready_mask = queried_ready_mask;
                CANOperationComplete(operation, CAN_STATUS_OK);
                return CAN_OPERATION_RUN_COMPLETED;
            }

            if (status == CAN_STATUS_ENODATA)
            {
                *operation->ready_mask = 0U;

                if (operation->timeout_ms == 0U)
                {
                    CANOperationComplete(operation, CAN_STATUS_ENODATA);
                    return CAN_OPERATION_RUN_COMPLETED;
                }

                if (CANOperationDeadlineExpired(operation))
                {
                    CANOperationComplete(operation, CAN_STATUS_ETIMEOUT);
                    return CAN_OPERATION_RUN_COMPLETED;
                }

                return CAN_OPERATION_RUN_PENDING;
            }

            *operation->ready_mask = 0U;
            CANOperationComplete(operation, status);
            return CAN_OPERATION_RUN_COMPLETED;

        case CAN_OPERATION_TYPE_NONE:
        default:
            operation->result_status = CAN_STATUS_EINVAL;
            return CAN_OPERATION_RUN_ERROR;
    }
}

bool CANOperationIsDone(const CANOperation *operation)
{
    return
        (operation != 0) &&
        (operation->state == CAN_OPERATION_STATE_COMPLETED);
}

CANStatus CANOperationGetResult(const CANOperation *operation)
{
    if (operation == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    return operation->result_status;
}