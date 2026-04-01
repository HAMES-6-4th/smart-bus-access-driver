#include "can_context.h"

#include <string.h>

void CANContextInit(CANContext *context)
{
    if (context == 0)
    {
        return;
    }

    memset(context, 0, sizeof(*context));
}

void CANContextBind(CANContext *context, CANCore *core, CANExecutor *executor)
{
    if (context == 0)
    {
        return;
    }

    context->core = core;
    context->executor = executor;
}

CANCore *CANContextGetCore(CANContext *context)
{
    if (context == 0)
    {
        return 0;
    }

    return context->core;
}

CANExecutor *CANContextGetExecutor(CANContext *context)
{
    if (context == 0)
    {
        return 0;
    }

    return context->executor;
}

void CANContextPrepareSend(CANContext *context,
                           CANOperation *operation,
                           const CANFrame *frame,
                           uint32_t timeout_ms)
{
    if ((context == 0) || (operation == 0))
    {
        return;
    }

    CANOperationPrepareSend(operation,
                            context->core,
                            frame,
                            timeout_ms);
}

void CANContextPrepareReceive(CANContext *context,
                              CANOperation *operation,
                              CANFrame *frame,
                              uint32_t timeout_ms)
{
    if ((context == 0) || (operation == 0))
    {
        return;
    }

    CANOperationPrepareReceive(operation,
                               context->core,
                               frame,
                               timeout_ms);
}

void CANContextPreparePoll(CANContext *context,
                           CANOperation *operation,
                           uint32_t interest_mask,
                           uint32_t timeout_ms,
                           uint32_t *ready_mask)
{
    if ((context == 0) || (operation == 0))
    {
        return;
    }

    CANOperationPreparePoll(operation,
                            context->core,
                            interest_mask,
                            timeout_ms,
                            ready_mask);
}

CANStatus CANContextSubmit(CANContext *context, CANOperation *operation)
{
    if ((context == 0) || (context->executor == 0) || (operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANExecutorSubmit(context->executor, operation);
}

CANExecutorRunOnceResult CANContextPollOne(CANContext *context)
{
    if ((context == 0) || (context->executor == 0))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    return CANExecutorPollOne(context->executor);
}

CANExecutorRunOnceResult CANContextRunOne(CANContext *context)
{
    if ((context == 0) || (context->executor == 0))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    return CANExecutorRunOne(context->executor);
}

CANExecutorRunOnceResult CANContextDispatchOne(CANContext *context)
{
    if ((context == 0) || (context->executor == 0))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    return CANExecutorDispatchOne(context->executor);
}

CANStatus CANContextPoll(CANContext *context,
                         uint32_t max_steps,
                         uint32_t *completed_count,
                         uint32_t *remaining_count)
{
    if ((context == 0) || (context->executor == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANExecutorPoll(context->executor,
                           max_steps,
                           completed_count,
                           remaining_count);
}

CANStatus CANContextSubmitSend(CANContext *context,
                               CANOperation *operation,
                               const CANFrame *frame,
                               uint32_t timeout_ms)
{
    if ((context == 0) || (operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    CANContextPrepareSend(context, operation, frame, timeout_ms);
    return CANContextSubmit(context, operation);
}

CANStatus CANContextSubmitReceive(CANContext *context,
                                  CANOperation *operation,
                                  CANFrame *frame,
                                  uint32_t timeout_ms)
{
    if ((context == 0) || (operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    CANContextPrepareReceive(context, operation, frame, timeout_ms);
    return CANContextSubmit(context, operation);
}

CANStatus CANContextSubmitPoll(CANContext *context,
                               CANOperation *operation,
                               uint32_t interest_mask,
                               uint32_t timeout_ms,
                               uint32_t *ready_mask)
{
    if ((context == 0) || (operation == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    CANContextPreparePoll(context,
                          operation,
                          interest_mask,
                          timeout_ms,
                          ready_mask);

    return CANContextSubmit(context, operation);
}

bool CANContextHasPending(const CANContext *context)
{
    if ((context == 0) || (context->executor == 0))
    {
        return false;
    }

    return CANExecutorHasPending(context->executor);
}

uint32_t CANContextGetPendingCount(const CANContext *context)
{
    if ((context == 0) || (context->executor == 0))
    {
        return 0U;
    }

    return CANExecutorGetPendingCount(context->executor);
}