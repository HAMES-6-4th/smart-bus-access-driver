#include "can_socket_test.h"

#include <string.h>

#include "can_socket.h"

typedef struct CANSocketFakeRuntimeStruct
{
    uint32_t now_ms;
} CANSocketFakeRuntime;

typedef struct CANSocketFakeChannelStruct
{
    uint32_t send_call_count;
    uint32_t receive_call_count;
    uint32_t query_call_count;
    uint32_t recover_call_count;

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

    CANStatus get_error_state_status;
    CANCoreErrorState error_state_value;

    CANStatus recover_status;
} CANSocketFakeChannel;

static uint32_t CANSocketFakeGetTickMs(void *user_context)
{
    CANSocketFakeRuntime *runtime = (CANSocketFakeRuntime *)user_context;
    return runtime->now_ms;
}

static void CANSocketFakeRelax(void *user_context)
{
    CANSocketFakeRuntime *runtime = (CANSocketFakeRuntime *)user_context;
    runtime->now_ms += 1U;
}

static CANStatus CANSocketFakeOpen(void *driver_channel,
                                   const void *driver_port,
                                   const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANSocketFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANSocketFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANSocketFakeChannel *)driver_channel;
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

static CANStatus CANSocketFakeReceive(void *driver_channel, CANFrame *frame)
{
    CANSocketFakeChannel *channel;
    uint32_t index;

    channel = (CANSocketFakeChannel *)driver_channel;
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
        frame->id = 0x234U;
        frame->len = 2U;
        frame->data[0] = 0xAAU;
        frame->data[1] = 0x55U;
    }

    return channel->receive_script[index];
}

static CANStatus CANSocketFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANSocketFakeChannel *channel;
    uint32_t index;

    channel = (CANSocketFakeChannel *)driver_channel;
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

static CANStatus CANSocketFakeGetErrorState(void *driver_channel, CANCoreErrorState *state)
{
    CANSocketFakeChannel *channel;

    channel = (CANSocketFakeChannel *)driver_channel;
    if ((channel == 0) || (state == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    *state = channel->error_state_value;
    return channel->get_error_state_status;
}

static CANStatus CANSocketFakeRecover(void *driver_channel)
{
    CANSocketFakeChannel *channel;

    channel = (CANSocketFakeChannel *)driver_channel;
    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    channel->recover_call_count++;
    return channel->recover_status;
}

static const CANCoreDriverOps g_CANSocketFakeDriverOps = {
    .Open = CANSocketFakeOpen,
    .Close = CANSocketFakeClose,
    .Start = CANSocketFakeStart,
    .Stop = CANSocketFakeStop,
    .Send = CANSocketFakeSend,
    .Receive = CANSocketFakeReceive
};

static const CANCoreOptionalDriverOps g_CANSocketFakeOptionalOps = {
    .QueryEvents = CANSocketFakeQueryEvents,
    .GetErrorState = CANSocketFakeGetErrorState,
    .Recover = CANSocketFakeRecover
};

static void CANSocketFakeBindingInit(CANCoreBinding *binding,
                                     CANSocketFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_socket_can";
    binding->ops = &g_CANSocketFakeDriverOps;
    binding->optional_ops = &g_CANSocketFakeOptionalOps;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANSocketRunTest(CANSocketTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANSocket socket;
    CANSocketFakeChannel channel;
    CANSocketFakeRuntime runtime;
    CANFrame frame;
    uint32_t ready_mask;
    CANCoreErrorState error_state;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(&frame, 0, sizeof(frame));
    memset(&error_state, 0, sizeof(error_state));

    runtime.now_ms = 0U;

    CANSocketInit(&socket);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANSocketFakeGetTickMs;
    open_params.runtime.relax = CANSocketFakeRelax;
    open_params.runtime.user_context = &runtime;

    CANSocketFakeBindingInit(&binding, &channel);

    result->unbound_send_einval_ok =
        (CANSocketSend(&socket, &frame) == CAN_STATUS_EINVAL);

    channel.send_script_length = 2U;
    channel.send_script[0] = CAN_STATUS_EBUSY;
    channel.send_script[1] = CAN_STATUS_OK;

    channel.receive_script_length = 1U;
    channel.receive_script[0] = CAN_STATUS_OK;

    channel.query_script_length = 1U;
    channel.query_status_script[0] = CAN_STATUS_OK;
    channel.query_mask_script[0] = (uint32_t)CAN_CORE_EVENT_TX_READY;

    channel.get_error_state_status = CAN_STATUS_OK;
    channel.error_state_value.bus_off = false;
    channel.error_state_value.error_passive = false;
    channel.error_state_value.warning = true;
    channel.error_state_value.tx_error_count = 0U;
    channel.error_state_value.rx_error_count = 0U;

    channel.recover_status = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    (void)CANCoreStart(&core);
    CANSocketBindCore(&socket, &core);

    result->init_bind_ok =
        (CANSocketGetCore(&socket) == &core) &&
        (CANSocketGetCoreConst(&socket) == &core);

    result->started_state_ok =
        (CANSocketIsOpen(&socket) == true) &&
        (CANSocketIsStarted(&socket) == true);

    result->send_ok =
        (CANSocketSend(&socket, &frame) == CAN_STATUS_EBUSY);

    result->send_timeout_ok =
        (CANSocketSendTimeout(&socket, &frame, 5U) == CAN_STATUS_OK);

    memset(&frame, 0, sizeof(frame));
    result->receive_ok =
        (CANSocketReceive(&socket, &frame) == CAN_STATUS_OK) &&
        (frame.id == 0x234U) &&
        (frame.len == 2U) &&
        (frame.data[0] == 0xAAU) &&
        (frame.data[1] == 0x55U);

    ready_mask = 0U;
    result->poll_ok =
        (CANSocketQueryEvents(&socket, &ready_mask) == CAN_STATUS_OK) &&
        ((ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U) &&
        (CANSocketPoll(&socket,
                       (uint32_t)CAN_CORE_EVENT_TX_READY,
                       0U,
                       &ready_mask) == CAN_STATUS_OK) &&
        ((ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U);

    memset(&error_state, 0, sizeof(error_state));
    result->error_state_ok =
        (CANSocketGetErrorState(&socket, &error_state) == CAN_STATUS_OK) &&
        (error_state.warning == true);

    result->recover_ok =
        (CANSocketRecover(&socket) == CAN_STATUS_OK) &&
        (channel.recover_call_count == 1U);

    (void)CANCoreClose(&core);
}

bool CANSocketTestPassed(const CANSocketTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->init_bind_ok &&
        result->unbound_send_einval_ok &&
        result->send_ok &&
        result->receive_ok &&
        result->send_timeout_ok &&
        result->poll_ok &&
        result->error_state_ok &&
        result->recover_ok &&
        result->started_state_ok;
}