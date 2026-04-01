#include "can_executor.h"

#include <string.h>

static bool CANExecutorContains(const CANExecutor *executor, const CANOperation *operation)
{
    uint32_t i;

    if ((executor == 0) || (operation == 0))
    {
        return false;
    }

    for (i = 0U; i < executor->count; ++i)
    {
        if (executor->slots[i] == operation)
        {
            return true;
        }
    }

    return false;
}

static void CANExecutorRemoveAt(CANExecutor *executor, uint32_t index)
{
    uint32_t i;

    if ((executor == 0) || (index >= executor->count))
    {
        return;
    }

    for (i = index; (i + 1U) < executor->count; ++i)
    {
        executor->slots[i] = executor->slots[i + 1U];
    }

    executor->slots[executor->count - 1U] = 0;
    executor->count--;

    if (executor->count == 0U)
    {
        executor->next_index = 0U;
    }
    else if (index >= executor->count)
    {
        executor->next_index = 0U;
    }
    else
    {
        executor->next_index = index;
    }
}

void CANExecutorInit(CANExecutor *executor, CANOperation **storage, uint32_t capacity)
{
    if (executor == 0)
    {
        return;
    }

    memset(executor, 0, sizeof(*executor));
    executor->slots = storage;
    executor->capacity = capacity;
}

CANStatus CANExecutorSubmit(CANExecutor *executor, CANOperation *operation)
{
    CANStatus status;

    if ((executor == 0) || (operation == 0) || (executor->slots == 0) || (executor->capacity == 0U))
    {
        return CAN_STATUS_EINVAL;
    }

    if (executor->count >= executor->capacity)
    {
        return CAN_STATUS_EBUSY;
    }

    if (CANExecutorContains(executor, operation))
    {
        return CAN_STATUS_EBUSY;
    }

    status = CANOperationSubmit(operation);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    executor->slots[executor->count] = operation;
    executor->count++;

    return CAN_STATUS_OK;
}

CANExecutorRunOnceResult CANExecutorRunOnce(CANExecutor *executor)
{
    CANOperation *operation;
    CANOperationRunResult run_result;
    uint32_t index;

    if ((executor == 0) || (executor->slots == 0) || (executor->capacity == 0U))
    {
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    if (executor->count == 0U)
    {
        return CAN_EXECUTOR_RUN_ONCE_IDLE;
    }

    index = executor->next_index;
    if (index >= executor->count)
    {
        index = 0U;
        executor->next_index = 0U;
    }

    operation = executor->slots[index];
    if (operation == 0)
    {
        CANExecutorRemoveAt(executor, index);
        return CAN_EXECUTOR_RUN_ONCE_ERROR;
    }

    run_result = CANOperationRunOnce(operation);

    if (run_result == CAN_OPERATION_RUN_COMPLETED)
    {
        CANExecutorRemoveAt(executor, index);
        return CAN_EXECUTOR_RUN_ONCE_COMPLETED;
    }

    if (run_result == CAN_OPERATION_RUN_PENDING)
    {
        if (executor->count > 0U)
        {
            executor->next_index = (index + 1U) % executor->count;
        }
        else
        {
            executor->next_index = 0U;
        }

        return CAN_EXECUTOR_RUN_ONCE_PENDING;
    }

    CANExecutorRemoveAt(executor, index);
    return CAN_EXECUTOR_RUN_ONCE_ERROR;
}

CANExecutorRunOnceResult CANExecutorPollOne(CANExecutor *executor)
{
    return CANExecutorRunOnce(executor);
}

CANStatus CANExecutorRunUntilIdle(CANExecutor *executor,
                                  uint32_t max_steps,
                                  uint32_t *completed_count,
                                  uint32_t *remaining_count)
{
    CANExecutorRunOnceResult run_result;
    uint32_t local_completed;
    uint32_t step;

    if ((executor == 0) || (executor->slots == 0) || (executor->capacity == 0U) || (max_steps == 0U))
    {
        return CAN_STATUS_EINVAL;
    }

    local_completed = 0U;

    if (completed_count != 0)
    {
        *completed_count = 0U;
    }

    if (remaining_count != 0)
    {
        *remaining_count = executor->count;
    }

    for (step = 0U; step < max_steps; ++step)
    {
        if (CANExecutorHasPending(executor) == false)
        {
            if (completed_count != 0)
            {
                *completed_count = local_completed;
            }

            if (remaining_count != 0)
            {
                *remaining_count = 0U;
            }

            return CAN_STATUS_OK;
        }

        run_result = CANExecutorRunOnce(executor);

        if (run_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED)
        {
            local_completed++;
            continue;
        }

        if (run_result == CAN_EXECUTOR_RUN_ONCE_PENDING)
        {
            continue;
        }

        if (run_result == CAN_EXECUTOR_RUN_ONCE_IDLE)
        {
            if (completed_count != 0)
            {
                *completed_count = local_completed;
            }

            if (remaining_count != 0)
            {
                *remaining_count = 0U;
            }

            return CAN_STATUS_OK;
        }

        if (completed_count != 0)
        {
            *completed_count = local_completed;
        }

        if (remaining_count != 0)
        {
            *remaining_count = executor->count;
        }

        return CAN_STATUS_EINVAL;
    }

    if (completed_count != 0)
    {
        *completed_count = local_completed;
    }

    if (remaining_count != 0)
    {
        *remaining_count = executor->count;
    }

    if (CANExecutorHasPending(executor) == true)
    {
        return CAN_STATUS_EBUSY;
    }

    return CAN_STATUS_OK;
}

bool CANExecutorHasPending(const CANExecutor *executor)
{
    return (executor != 0) && (executor->count > 0U);
}

uint32_t CANExecutorGetPendingCount(const CANExecutor *executor)
{
    if (executor == 0)
    {
        return 0U;
    }

    return executor->count;
}

CANExecutorRunOnceResult CANExecutorRunOne(CANExecutor *executor)
{
    return CANExecutorPollOne(executor);
}

CANStatus CANExecutorPoll(CANExecutor *executor,
                          uint32_t max_steps,
                          uint32_t *completed_count,
                          uint32_t *remaining_count)
{
    return CANExecutorRunUntilIdle(executor,
                                   max_steps,
                                   completed_count,
                                   remaining_count);
}

CANExecutorRunOnceResult CANExecutorDispatchOne(CANExecutor *executor)
{
    return CANExecutorPollOne(executor);
}