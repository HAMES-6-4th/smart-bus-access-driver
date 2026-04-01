#include "can_socket_sugar_test.h"

#include <string.h>

#include "can_socket.h"

typedef struct CANSocketSugarFakeRuntimeStruct
{
    uint32_t now_ms;
} CANSocketSugarFakeRuntime;

typedef struct CANSocketSugarFakeChannelStruct
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
} CANSocketSugarFakeChannel;

static uint32_t CANSocketSugarFakeGetTickMs(void *user_context)
{
    CANSocketSugarFakeRuntime *runtime = (CANSocketSugarFakeRuntime *)user_context;
    return runtime->now_ms;
}

static void CANSocketSugarFakeRelax(void *user_context)
{
    CANSocketSugarFakeRuntime *runtime = (CANSocketSugarFakeRuntime *)user_context;
    runtime->now_ms += 1U;
}

static CANStatus CANSocketSugarFakeOpen(void *driver_channel,
                                        const void *driver_port,
                                        const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketSugarFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketSugarFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketSugarFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketSugarFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANSocketSugarFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANSocketSugarFakeChannel *)driver_channel;
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

static CANStatus CANSocketSugarFakeReceive(void *driver_channel, CANFrame *frame)
{
    CANSocketSugarFakeChannel *channel;
    uint32_t index;

    channel = (CANSocketSugarFakeChannel *)driver_channel;
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
        frame->id = 0x345U;
        frame->len = 2U;
        frame->data[0] = 0x11U;
        frame->data[1] = 0x22U;
    }

    return channel->receive_script[index];
}

static CANStatus CANSocketSugarFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANSocketSugarFakeChannel *channel;
    uint32_t index;

    channel = (CANSocketSugarFakeChannel *)driver_channel;
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

static const CANCoreDriverOps g_CANSocketSugarFakeDriverOps = {
    .Open = CANSocketSugarFakeOpen,
    .Close = CANSocketSugarFakeClose,
    .Start = CANSocketSugarFakeStart,
    .Stop = CANSocketSugarFakeStop,
    .Send = CANSocketSugarFakeSend,
    .Receive = CANSocketSugarFakeReceive
};

static const CANCoreOptionalDriverOps g_CANSocketSugarFakeOptionalOps = {
    .QueryEvents = CANSocketSugarFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void CANSocketSugarFakeBindingInit(CANCoreBinding *binding,
                                          CANSocketSugarFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_socket_sugar_can";
    binding->ops = &g_CANSocketSugarFakeDriverOps;
    binding->optional_ops = &g_CANSocketSugarFakeOptionalOps;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANSocketSugarRunTest(CANSocketSugarTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANSocket socket;
    CANSocketSugarFakeChannel channel;
    CANSocketSugarFakeRuntime runtime;
    CANFrame frame;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(&frame, 0, sizeof(frame));

    runtime.now_ms = 0U;

    CANSocketInit(&socket);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANSocketSugarFakeGetTickMs;
    open_params.runtime.relax = CANSocketSugarFakeRelax;
    open_params.runtime.user_context = &runtime;

    CANSocketSugarFakeBindingInit(&binding, &channel);

    result->unbound_send_now_einval_ok =
        (CANSocketSendNow(&socket, &frame) == CAN_STATUS_EINVAL);

    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_OK;

    channel.receive_script_length = 1U;
    channel.receive_script[0] = CAN_STATUS_OK;

    channel.query_script_length = 3U;
    channel.query_status_script[0] = CAN_STATUS_OK;
    channel.query_status_script[1] = CAN_STATUS_OK;
    channel.query_status_script[2] = CAN_STATUS_OK;
    channel.query_mask_script[0] = (uint32_t)CAN_CORE_EVENT_TX_READY;
    channel.query_mask_script[1] = 0U;
    channel.query_mask_script[2] = (uint32_t)CAN_CORE_EVENT_RX_READY;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    (void)CANCoreStart(&core);
    CANSocketBindCore(&socket, &core);

    result->send_now_ok =
        (CANSocketSendNow(&socket, &frame) == CAN_STATUS_OK);

    memset(&frame, 0, sizeof(frame));
    result->receive_now_ok =
        (CANSocketReceiveNow(&socket, &frame) == CAN_STATUS_OK) &&
        (frame.id == 0x345U) &&
        (frame.len == 2U) &&
        (frame.data[0] == 0x11U) &&
        (frame.data[1] == 0x22U);

    result->wait_tx_ready_ok =
        (CANSocketWaitTxReady(&socket, 0U) == CAN_STATUS_OK);

    result->wait_rx_ready_zero_timeout_nodata_ok =
        (CANSocketWaitRxReady(&socket, 0U) == CAN_STATUS_ENODATA);

    result->wait_rx_ready_timeout_ok =
        (CANSocketWaitRxReady(&socket, 5U) == CAN_STATUS_OK);

    (void)CANCoreClose(&core);
}

bool CANSocketSugarTestPassed(const CANSocketSugarTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->unbound_send_now_einval_ok &&
        result->send_now_ok &&
        result->receive_now_ok &&
        result->wait_tx_ready_ok &&
        result->wait_rx_ready_zero_timeout_nodata_ok &&
        result->wait_rx_ready_timeout_ok;
}