#include "can_executor_alias_test.h"

#include <string.h>

#include "can_core.h"
#include "can_executor.h"
#include "can_operation.h"

typedef struct CANExecutorAliasFakeChannelStruct
{
    uint32_t send_call_count;
    CANStatus send_script[8];
    uint32_t send_script_length;
    uint32_t send_script_index;
} CANExecutorAliasFakeChannel;

static CANStatus CANExecutorAliasFakeOpen(void *driver_channel,
                                          const void *driver_port,
                                          const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorAliasFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorAliasFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorAliasFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorAliasFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANExecutorAliasFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANExecutorAliasFakeChannel *)driver_channel;
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

static CANStatus CANExecutorAliasFakeReceive(void *driver_channel, CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_ENODATA;
}

static const CANCoreDriverOps g_CANExecutorAliasFakeDriverOps = {
    .Open = CANExecutorAliasFakeOpen,
    .Close = CANExecutorAliasFakeClose,
    .Start = CANExecutorAliasFakeStart,
    .Stop = CANExecutorAliasFakeStop,
    .Send = CANExecutorAliasFakeSend,
    .Receive = CANExecutorAliasFakeReceive
};

static void CANExecutorAliasFakeBindingInit(CANCoreBinding *binding,
                                            CANExecutorAliasFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_executor_alias_can";
    binding->ops = &g_CANExecutorAliasFakeDriverOps;
    binding->optional_ops = 0;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANExecutorAliasRunTest(CANExecutorAliasTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *storage[2];
    CANOperation op1;
    CANOperation op2;
    CANFrame frame;
    CANExecutorAliasFakeChannel channel;
    uint32_t completed_count;
    uint32_t remaining_count;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(storage, 0, sizeof(storage));
    memset(&frame, 0, sizeof(frame));

    CANExecutorInit(&executor, storage, 2U);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorAliasFakeBindingInit(&binding, &channel);

    channel.send_script_length = 3U;
    channel.send_script[0] = CAN_STATUS_EBUSY;
    channel.send_script[1] = CAN_STATUS_OK;
    channel.send_script[2] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    (void)CANCoreStart(&core);

    CANOperationPrepareSend(&op1, &core, &frame, CAN_TIMEOUT_INFINITE);
    CANOperationPrepareSend(&op2, &core, &frame, 0U);

    (void)CANExecutorSubmit(&executor, &op1);
    (void)CANExecutorSubmit(&executor, &op2);

    result->run_one_alias_ok =
        (CANExecutorRunOne(&executor) == CAN_EXECUTOR_RUN_ONCE_PENDING) &&
        (CANExecutorGetPendingCount(&executor) == 2U);

    result->dispatch_one_alias_ok =
        (CANExecutorDispatchOne(&executor) == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (CANOperationIsDone(&op2) == true) &&
        (CANExecutorGetPendingCount(&executor) == 1U);

    completed_count = 0U;
    remaining_count = 0U;

    result->poll_alias_ok =
        (CANExecutorPoll(&executor, 4U, &completed_count, &remaining_count) == CAN_STATUS_OK) &&
        (completed_count == 1U) &&
        (remaining_count == 0U) &&
        (CANOperationIsDone(&op1) == true) &&
        (CANExecutorHasPending(&executor) == false);

    (void)CANCoreClose(&core);
}

bool CANExecutorAliasTestPassed(const CANExecutorAliasTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->run_one_alias_ok &&
        result->dispatch_one_alias_ok &&
        result->poll_alias_ok;
}