#include "can_context_test.h"

#include <string.h>

#include "can_context.h"

typedef struct CANContextFakeChannelStruct
{
    uint32_t send_call_count;
    uint32_t query_call_count;

    CANStatus send_script[8];
    uint32_t send_script_length;
    uint32_t send_script_index;

    CANStatus query_status_script[8];
    uint32_t query_mask_script[8];
    uint32_t query_script_length;
    uint32_t query_script_index;
} CANContextFakeChannel;

static CANStatus CANContextFakeOpen(void *driver_channel,
                                    const void *driver_port,
                                    const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANContextFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANContextFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANContextFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANContextFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANContextFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANContextFakeChannel *)driver_channel;
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

static CANStatus CANContextFakeReceive(void *driver_channel, CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_ENODATA;
}

static CANStatus CANContextFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANContextFakeChannel *channel;
    uint32_t index;

    channel = (CANContextFakeChannel *)driver_channel;
    if ((channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->query_script_index;
    if (index >= channel->query_script_length)
    {
        index = (channel->query_script_length == 0U) ? 0U : (channel->query_script_length - 1U);
    }

    channel->query_call_count++;

    if (channel->query_script_length == 0U)
    {
        *event_mask = 0U;
        return CAN_STATUS_OK;
    }

    *event_mask = channel->query_mask_script[index];

    if (channel->query_script_index + 1U < channel->query_script_length)
    {
        channel->query_script_index++;
    }

    return channel->query_status_script[index];
}

static const CANCoreDriverOps g_CANContextFakeDriverOps = {
    .Open = CANContextFakeOpen,
    .Close = CANContextFakeClose,
    .Start = CANContextFakeStart,
    .Stop = CANContextFakeStop,
    .Send = CANContextFakeSend,
    .Receive = CANContextFakeReceive
};

static const CANCoreOptionalDriverOps g_CANContextFakeOptionalOps = {
    .QueryEvents = CANContextFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void CANContextFakeBindingInit(CANCoreBinding *binding,
                                      CANContextFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_context_can";
    binding->ops = &g_CANContextFakeDriverOps;
    binding->optional_ops = &g_CANContextFakeOptionalOps;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANContextRunTest(CANContextTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *storage[2];
    CANOperation poll_op;
    CANOperation send_op;
    CANContext context;
    CANContextFakeChannel channel;
    CANFrame frame;
    uint32_t ready_mask;
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

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorInit(&executor, storage, 2U);
    CANOperationInit(&poll_op);
    CANOperationInit(&send_op);
    CANContextInit(&context);
    CANContextFakeBindingInit(&binding, &channel);

    channel.query_script_length = 1U;
    channel.query_status_script[0] = CAN_STATUS_OK;
    channel.query_mask_script[0] = (uint32_t)CAN_CORE_EVENT_TX_READY;

    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    (void)CANCoreStart(&core);

    CANContextBind(&context, &core, &executor);

    result->bind_ok =
        (CANContextGetCore(&context) == &core) &&
        (CANContextGetExecutor(&context) == &executor);

    ready_mask = 0U;

    CANContextPreparePoll(&context,
                          &poll_op,
                          (uint32_t)CAN_CORE_EVENT_TX_READY,
                          0U,
                          &ready_mask);

    CANContextPrepareSend(&context,
                          &send_op,
                          &frame,
                          0U);

    result->prepare_poll_submit_ok =
        (CANContextSubmit(&context, &poll_op) == CAN_STATUS_OK);

    result->prepare_send_submit_ok =
        (CANContextSubmit(&context, &send_op) == CAN_STATUS_OK);

    result->pending_count_ok =
        (CANContextGetPendingCount(&context) == 2U) &&
        (CANContextHasPending(&context) == true);

    result->poll_one_ok =
        (CANContextPollOne(&context) == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        ((ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U) &&
        (CANOperationIsDone(&poll_op) == true) &&
        (CANOperationGetResult(&poll_op) == CAN_STATUS_OK);

    result->run_one_ok =
        (CANContextRunOne(&context) == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (CANOperationIsDone(&send_op) == true) &&
        (CANOperationGetResult(&send_op) == CAN_STATUS_OK);

    result->dispatch_one_ok =
        (CANContextDispatchOne(&context) == CAN_EXECUTOR_RUN_ONCE_IDLE);

    completed_count = 0U;
    remaining_count = 0U;

    result->poll_ok =
        (CANContextPoll(&context, 2U, &completed_count, &remaining_count) == CAN_STATUS_OK) &&
        (completed_count == 0U) &&
        (remaining_count == 0U);

    result->done_ok =
        (CANContextHasPending(&context) == false) &&
        (CANContextGetPendingCount(&context) == 0U);

    (void)CANCoreClose(&core);
}

bool CANContextTestPassed(const CANContextTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->bind_ok &&
        result->prepare_send_submit_ok &&
        result->prepare_poll_submit_ok &&
        result->pending_count_ok &&
        result->poll_one_ok &&
        result->run_one_ok &&
        result->dispatch_one_ok &&
        result->poll_ok &&
        result->done_ok;
}