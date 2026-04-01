#include "can_service_test.h"

#include <string.h>

#include "can_service.h"

typedef struct CANServiceFakeChannelStruct
{
    uint32_t send_call_count;
    uint32_t receive_call_count;
    uint32_t query_call_count;

    CANStatus send_script[8];
    uint32_t send_script_length;
    uint32_t send_script_index;

    CANStatus receive_script[8];
    uint32_t receive_script_length;
    uint32_t receive_script_index;

    CANStatus query_status_script[8];
    uint32_t query_mask_script[8];
    uint32_t query_script_length;
    uint32_t query_script_index;
} CANServiceFakeChannel;

static CANStatus CANServiceFakeOpen(void *driver_channel,
                                    const void *driver_port,
                                    const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANServiceFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANServiceFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANServiceFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANServiceFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANServiceFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANServiceFakeChannel *)driver_channel;
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

static CANStatus CANServiceFakeReceive(void *driver_channel, CANFrame *frame)
{
    CANServiceFakeChannel *channel;
    uint32_t index;

    channel = (CANServiceFakeChannel *)driver_channel;
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->receive_script_index;
    if (index >= channel->receive_script_length)
    {
        index = (channel->receive_script_length == 0U) ? 0U : (channel->receive_script_length - 1U);
    }

    channel->receive_call_count++;

    if (channel->receive_script_length == 0U)
    {
        return CAN_STATUS_ENODATA;
    }

    if (channel->receive_script_index + 1U < channel->receive_script_length)
    {
        channel->receive_script_index++;
    }

    if (channel->receive_script[index] == CAN_STATUS_OK)
    {
        memset(frame, 0, sizeof(*frame));
        frame->id_type = CAN_ID_STANDARD;
        frame->mode = CAN_MODE_CLASSIC;
        frame->id = 0x432U;
        frame->len = 2U;
        frame->data[0] = 0x12U;
        frame->data[1] = 0x34U;
    }

    return channel->receive_script[index];
}

static CANStatus CANServiceFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANServiceFakeChannel *channel;
    uint32_t index;

    channel = (CANServiceFakeChannel *)driver_channel;
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

static const CANCoreDriverOps g_CANServiceFakeDriverOps = {
    .Open = CANServiceFakeOpen,
    .Close = CANServiceFakeClose,
    .Start = CANServiceFakeStart,
    .Stop = CANServiceFakeStop,
    .Send = CANServiceFakeSend,
    .Receive = CANServiceFakeReceive
};

static const CANCoreOptionalDriverOps g_CANServiceFakeOptionalOps = {
    .QueryEvents = CANServiceFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void CANServiceFakeBindingInit(CANCoreBinding *binding,
                                      CANServiceFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_service_can";
    binding->ops = &g_CANServiceFakeDriverOps;
    binding->optional_ops = &g_CANServiceFakeOptionalOps;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANServiceRunTest(CANServiceTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation executor_storage[3];
    CANOperation service_ops[3];
    bool service_in_use[3];
    CANContext context;
    CANService service;
    CANServiceFakeChannel channel;
    CANOperation *poll_op;
    CANOperation *send_op;
    CANOperation *receive_op;
    CANOperation *extra_op;
    CANFrame tx_frame;
    CANFrame rx_frame;
    uint32_t ready_mask;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(executor_storage, 0, sizeof(executor_storage));
    memset(service_ops, 0, sizeof(service_ops));
    memset(service_in_use, 0, sizeof(service_in_use));
    memset(&tx_frame, 0, sizeof(tx_frame));
    memset(&rx_frame, 0, sizeof(rx_frame));

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorInit(&executor, (CANOperation **)0, 0U);
    CANContextInit(&context);
    CANServiceInit(&service, service_ops, service_in_use, 3U);

    result->init_ok =
        (CANServiceGetCapacity(&service) == 3U) &&
        (CANServiceGetInUseCount(&service) == 0U) &&
        (CANServiceGetContext(&service) == 0);

    CANExecutorInit(&executor, (CANOperation **)&executor_storage, 3U);
    CANContextBind(&context, &core, &executor);
    CANServiceBindContext(&service, &context);

    result->bind_ok =
        (CANServiceGetContext(&service) == &context);

    CANServiceFakeBindingInit(&binding, &channel);
    channel.query_script_length = 1U;
    channel.query_status_script[0] = CAN_STATUS_OK;
    channel.query_mask_script[0] = (uint32_t)CAN_CORE_EVENT_TX_READY;

    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_OK;

    channel.receive_script_length = 1U;
    channel.receive_script[0] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    (void)CANCoreStart(&core);

    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = 0x111U;
    tx_frame.len = 1U;
    tx_frame.data[0] = 0x5AU;

    ready_mask = 0U;
    poll_op = 0;
    send_op = 0;
    receive_op = 0;
    extra_op = 0;

    result->submit_poll_ok =
        (CANServiceSubmitPoll(&service,
                              &poll_op,
                              (uint32_t)CAN_CORE_EVENT_TX_READY,
                              0U,
                              &ready_mask) == CAN_STATUS_OK) &&
        (poll_op != 0);

    result->submit_send_ok =
        (CANServiceSubmitSend(&service,
                              &send_op,
                              &tx_frame,
                              0U) == CAN_STATUS_OK) &&
        (send_op != 0);

    result->submit_receive_ok =
        (CANServiceSubmitReceive(&service,
                                 &receive_op,
                                 &rx_frame,
                                 0U) == CAN_STATUS_OK) &&
        (receive_op != 0);

    result->in_use_count_ok =
        (CANServiceGetInUseCount(&service) == 3U);

    result->capacity_busy_ok =
        (CANServiceSubmitSend(&service,
                              &extra_op,
                              &tx_frame,
                              0U) == CAN_STATUS_EBUSY) &&
        (extra_op == 0);

    result->poll_one_ok =
        (CANServicePollOne(&service) == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (poll_op != 0) &&
        (CANOperationIsDone(poll_op) == true) &&
        (CANOperationGetResult(poll_op) == CAN_STATUS_OK) &&
        ((ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U);

    result->run_one_ok =
        (CANServiceRunOne(&service) == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (send_op != 0) &&
        (CANOperationIsDone(send_op) == true) &&
        (CANOperationGetResult(send_op) == CAN_STATUS_OK);

    result->dispatch_one_ok =
        (CANServiceDispatchOne(&service) == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (receive_op != 0) &&
        (CANOperationIsDone(receive_op) == true) &&
        (CANOperationGetResult(receive_op) == CAN_STATUS_OK);

    result->receive_frame_ok =
        (rx_frame.id == 0x432U) &&
        (rx_frame.len == 2U) &&
        (rx_frame.data[0] == 0x12U) &&
        (rx_frame.data[1] == 0x34U);

    CANServiceReleaseOperation(&service, poll_op);
    CANServiceReleaseOperation(&service, send_op);
    CANServiceReleaseOperation(&service, receive_op);

    result->release_ok =
        (CANServiceGetInUseCount(&service) == 0U);

    (void)CANCoreClose(&core);
}

bool CANServiceTestPassed(const CANServiceTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->init_ok &&
        result->bind_ok &&
        result->submit_poll_ok &&
        result->submit_send_ok &&
        result->submit_receive_ok &&
        result->capacity_busy_ok &&
        result->poll_one_ok &&
        result->run_one_ok &&
        result->dispatch_one_ok &&
        result->receive_frame_ok &&
        result->in_use_count_ok &&
        result->release_ok;
}