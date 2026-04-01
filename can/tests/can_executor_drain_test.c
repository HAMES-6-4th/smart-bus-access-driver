#include "can_executor_drain_test.h"

#include <string.h>

#include "can_core.h"
#include "can_executor.h"
#include "can_operation.h"

typedef struct CANExecutorDrainFakeChannelStruct
{
    uint32_t send_call_count;
    CANStatus send_script[8];
    uint32_t send_script_length;
    uint32_t send_script_index;
} CANExecutorDrainFakeChannel;

static CANStatus CANExecutorDrainFakeOpen(void *driver_channel,
                                          const void *driver_port,
                                          const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorDrainFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorDrainFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorDrainFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorDrainFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANExecutorDrainFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANExecutorDrainFakeChannel *)driver_channel;
    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->send_script_index;
    if (index >= channel->send_script_length)
    {
        index = (channel->send_script_length == 0U) ? 0U : (channel->send_script_length - 1U);
    }

    channel->send_call_count++;

    if (channel->send_script_length == 0U)
    {
        return CAN_STATUS_OK;
    }

    if (channel->send_script_index + 1U < channel->send_script_length)
    {
        channel->send_script_index++;
    }

    return channel->send_script[index];
}

static CANStatus CANExecutorDrainFakeReceive(void *driver_channel, CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_ENODATA;
}

static const CANCoreDriverOps g_CANExecutorDrainFakeDriverOps = {
    .Open = CANExecutorDrainFakeOpen,
    .Close = CANExecutorDrainFakeClose,
    .Start = CANExecutorDrainFakeStart,
    .Stop = CANExecutorDrainFakeStop,
    .Send = CANExecutorDrainFakeSend,
    .Receive = CANExecutorDrainFakeReceive
};

static void CANExecutorDrainFakeBindingInit(CANCoreBinding *binding,
                                            CANExecutorDrainFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_executor_drain_can";
    binding->ops = &g_CANExecutorDrainFakeDriverOps;
    binding->optional_ops = 0;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANExecutorDrainRunTest(CANExecutorDrainTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *storage[2];
    CANOperation op1;
    CANOperation op2;
    CANExecutorDrainFakeChannel channel;
    CANFrame frame;
    CANStatus status;
    uint32_t completed_count;
    uint32_t remaining_count;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(storage, 0, sizeof(storage));
    memset(&frame, 0, sizeof(frame));

    CANExecutorInit(&executor, storage, 2U);

    result->poll_one_alias_ok =
        (CANExecutorPollOne(&executor) == CAN_EXECUTOR_RUN_ONCE_IDLE);

    result->run_until_idle_zero_budget_einval_ok =
        (CANExecutorRunUntilIdle(&executor, 0U, &completed_count, &remaining_count) == CAN_STATUS_EINVAL);

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorDrainFakeBindingInit(&binding, &channel);

    channel.send_script_length = 3U;
    channel.send_script[0] = CAN_STATUS_EBUSY;
    channel.send_script[1] = CAN_STATUS_OK;
    channel.send_script[2] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&op1, &core, &frame, CAN_TIMEOUT_INFINITE);
        CANOperationPrepareSend(&op2, &core, &frame, 0U);

        if ((CANExecutorSubmit(&executor, &op1) == CAN_STATUS_OK) &&
            (CANExecutorSubmit(&executor, &op2) == CAN_STATUS_OK))
        {
            completed_count = 0U;
            remaining_count = 0U;

            result->run_until_idle_complete_ok =
                (CANExecutorRunUntilIdle(&executor,
                                         4U,
                                         &completed_count,
                                         &remaining_count) == CAN_STATUS_OK);

            result->run_until_idle_completed_count_ok =
                (completed_count == 2U);

            result->run_until_idle_remaining_count_ok =
                (remaining_count == 0U) &&
                (CANExecutorGetPendingCount(&executor) == 0U) &&
                (CANOperationIsDone(&op1) == true) &&
                (CANOperationIsDone(&op2) == true) &&
                (CANOperationGetResult(&op1) == CAN_STATUS_OK) &&
                (CANOperationGetResult(&op2) == CAN_STATUS_OK);
        }

        (void)CANCoreClose(&core);
    }

    CANExecutorInit(&executor, storage, 1U);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorDrainFakeBindingInit(&binding, &channel);

    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_EBUSY;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&op1, &core, &frame, CAN_TIMEOUT_INFINITE);

        if (CANExecutorSubmit(&executor, &op1) == CAN_STATUS_OK)
        {
            completed_count = 0U;
            remaining_count = 0U;

            result->run_until_idle_budget_exhausted_ok =
                (CANExecutorRunUntilIdle(&executor,
                                         3U,
                                         &completed_count,
                                         &remaining_count) == CAN_STATUS_EBUSY);

            result->run_until_idle_budget_remaining_ok =
                (completed_count == 0U) &&
                (remaining_count == 1U) &&
                (CANExecutorGetPendingCount(&executor) == 1U) &&
                (CANOperationIsDone(&op1) == false);
        }

        (void)CANCoreClose(&core);
    }

    CANExecutorInit(&executor, storage, 1U);
    executor.count = 1U;
    executor.slots[0] = 0;

    completed_count = 0U;
    remaining_count = 0U;

    result->run_until_idle_error_path_ok =
        (CANExecutorRunUntilIdle(&executor,
                                 1U,
                                 &completed_count,
                                 &remaining_count) == CAN_STATUS_EINVAL) &&
        (completed_count == 0U) &&
        (remaining_count == 0U);
}

bool CANExecutorDrainTestPassed(const CANExecutorDrainTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->poll_one_alias_ok &&
        result->run_until_idle_zero_budget_einval_ok &&
        result->run_until_idle_complete_ok &&
        result->run_until_idle_completed_count_ok &&
        result->run_until_idle_remaining_count_ok &&
        result->run_until_idle_budget_exhausted_ok &&
        result->run_until_idle_budget_remaining_ok &&
        result->run_until_idle_error_path_ok;
}