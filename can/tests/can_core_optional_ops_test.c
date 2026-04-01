#include "can_core_optional_ops_test.h"

#include <string.h>

#include "can_core.h"

typedef struct CANCoreOptionalOpsFakeChannelStruct
{
    uint32_t query_events_call_count;
    uint32_t get_error_state_call_count;
    uint32_t recover_call_count;

    uint32_t query_event_mask_to_return;
    CANCoreErrorState error_state_to_return;

    CANStatus query_status_to_return;
    CANStatus error_state_status_to_return;
    CANStatus recover_status_to_return;
} CANCoreOptionalOpsFakeChannel;

static CANStatus CANCoreOptionalOpsFakeOpen(void *driver_channel,
                                            const void *driver_port,
                                            const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreOptionalOpsFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreOptionalOpsFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreOptionalOpsFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreOptionalOpsFakeSend(void *driver_channel, const CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreOptionalOpsFakeReceive(void *driver_channel, CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_ENODATA;
}

static CANStatus CANCoreOptionalOpsFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANCoreOptionalOpsFakeChannel *channel;

    channel = (CANCoreOptionalOpsFakeChannel *)driver_channel;
    if ((channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->query_events_call_count++;
    *event_mask = channel->query_event_mask_to_return;
    return channel->query_status_to_return;
}

static CANStatus CANCoreOptionalOpsFakeGetErrorState(void *driver_channel, CANCoreErrorState *state)
{
    CANCoreOptionalOpsFakeChannel *channel;

    channel = (CANCoreOptionalOpsFakeChannel *)driver_channel;
    if ((channel == 0) || (state == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->get_error_state_call_count++;
    *state = channel->error_state_to_return;
    return channel->error_state_status_to_return;
}

static CANStatus CANCoreOptionalOpsFakeRecover(void *driver_channel)
{
    CANCoreOptionalOpsFakeChannel *channel;

    channel = (CANCoreOptionalOpsFakeChannel *)driver_channel;
    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    channel->recover_call_count++;
    return channel->recover_status_to_return;
}

static const CANCoreDriverOps g_CANCoreOptionalOpsFakeDriverOps = {
    .Open = CANCoreOptionalOpsFakeOpen,
    .Close = CANCoreOptionalOpsFakeClose,
    .Start = CANCoreOptionalOpsFakeStart,
    .Stop = CANCoreOptionalOpsFakeStop,
    .Send = CANCoreOptionalOpsFakeSend,
    .Receive = CANCoreOptionalOpsFakeReceive
};

static const CANCoreOptionalDriverOps g_CANCoreOptionalOpsFakeOptionalDriverOps = {
    .QueryEvents = CANCoreOptionalOpsFakeQueryEvents,
    .GetErrorState = CANCoreOptionalOpsFakeGetErrorState,
    .Recover = CANCoreOptionalOpsFakeRecover
};

static void CANCoreOptionalOpsFakeBindingInit(CANCoreBinding *binding,
                                              CANCoreOptionalOpsFakeChannel *channel,
                                              const CANCoreOptionalDriverOps *optional_ops)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));

    *binding = (CANCoreBinding){
        .name = "fake_optional_can",
        .ops = &g_CANCoreOptionalOpsFakeDriverOps,
        .optional_ops = optional_ops,
        .driver_channel = channel,
        .driver_port = 0,
        .capabilities = {
            .supports_fd = true,
            .supports_brs = true,
            .supports_loopback = true,
            .supports_hw_filter = false,
            .supports_termination_control = false
        }
    };
}

void CANCoreRunOptionalOpsTest(CANCoreOptionalOpsTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANCoreOptionalOpsFakeChannel channel;
    CANCoreErrorState error_state;
    uint32_t event_mask;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(&error_state, 0, sizeof(error_state));
    event_mask = 0u;

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);

    result->null_arg_query_ok =
        (CANCoreQueryEvents(0, &event_mask) == CAN_STATUS_EINVAL);

    result->null_arg_error_state_ok =
        (CANCoreGetErrorState(0, &error_state) == CAN_STATUS_EINVAL);

    result->null_arg_recover_ok =
        (CANCoreRecover(0) == CAN_STATUS_EINVAL);

    result->closed_query_unsupported_ok =
        (CANCoreQueryEvents(&core, &event_mask) == CAN_STATUS_EUNSUPPORTED) &&
        (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

    result->closed_error_state_unsupported_ok =
        (CANCoreGetErrorState(&core, &error_state) == CAN_STATUS_EUNSUPPORTED) &&
        (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

    result->closed_recover_unsupported_ok =
        (CANCoreRecover(&core) == CAN_STATUS_EUNSUPPORTED) &&
        (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

    CANCoreOptionalOpsFakeBindingInit(&binding, &channel, 0);
    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        result->no_optional_query_unsupported_ok =
            (CANCoreQueryEvents(&core, &event_mask) == CAN_STATUS_EUNSUPPORTED) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

        result->no_optional_error_state_unsupported_ok =
            (CANCoreGetErrorState(&core, &error_state) == CAN_STATUS_EUNSUPPORTED) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

        result->no_optional_recover_unsupported_ok =
            (CANCoreRecover(&core) == CAN_STATUS_EUNSUPPORTED) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANCoreOptionalOpsFakeBindingInit(&binding, &channel, &g_CANCoreOptionalOpsFakeOptionalDriverOps);

    channel.query_event_mask_to_return = (uint32_t)(CAN_CORE_EVENT_RX_READY | CAN_CORE_EVENT_ERROR);
    channel.error_state_to_return.bus_off = true;
    channel.error_state_to_return.error_passive = false;
    channel.error_state_to_return.warning = true;
    channel.error_state_to_return.tx_error_count = 7u;
    channel.error_state_to_return.rx_error_count = 11u;
    channel.query_status_to_return = CAN_STATUS_OK;
    channel.error_state_status_to_return = CAN_STATUS_OK;
    channel.recover_status_to_return = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        event_mask = 0u;
        memset(&error_state, 0, sizeof(error_state));

        result->ok_query_forward_ok =
            (CANCoreQueryEvents(&core, &event_mask) == CAN_STATUS_OK) &&
            (event_mask == channel.query_event_mask_to_return) &&
            (channel.query_events_call_count == 1u);

        result->query_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_OK);

        result->ok_error_state_forward_ok =
            (CANCoreGetErrorState(&core, &error_state) == CAN_STATUS_OK) &&
            (error_state.bus_off == channel.error_state_to_return.bus_off) &&
            (error_state.error_passive == channel.error_state_to_return.error_passive) &&
            (error_state.warning == channel.error_state_to_return.warning) &&
            (error_state.tx_error_count == channel.error_state_to_return.tx_error_count) &&
            (error_state.rx_error_count == channel.error_state_to_return.rx_error_count) &&
            (channel.get_error_state_call_count == 1u);

        result->error_state_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_OK);

        result->ok_recover_ok =
            (CANCoreRecover(&core) == CAN_STATUS_OK) &&
            (channel.recover_call_count == 1u);

        result->recover_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_OK);

        channel.query_status_to_return = CAN_STATUS_EBUSY;
        channel.error_state_status_to_return = CAN_STATUS_EBUSY;
        channel.recover_status_to_return = CAN_STATUS_ETIMEOUT;

        result->query_status_propagation_ok =
            (CANCoreQueryEvents(&core, &event_mask) == CAN_STATUS_EBUSY) &&
            (channel.query_events_call_count == 2u) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EBUSY);

        result->error_state_status_propagation_ok =
            (CANCoreGetErrorState(&core, &error_state) == CAN_STATUS_EBUSY) &&
            (channel.get_error_state_call_count == 2u) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EBUSY);

        result->recover_status_propagation_ok =
            (CANCoreRecover(&core) == CAN_STATUS_ETIMEOUT) &&
            (channel.recover_call_count == 2u) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_ETIMEOUT);

        (void)CANCoreClose(&core);
    }
}

bool CANCoreOptionalOpsTestPassed(const CANCoreOptionalOpsTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->null_arg_query_ok &&
        result->null_arg_error_state_ok &&
        result->null_arg_recover_ok &&
        result->closed_query_unsupported_ok &&
        result->closed_error_state_unsupported_ok &&
        result->closed_recover_unsupported_ok &&
        result->no_optional_query_unsupported_ok &&
        result->no_optional_error_state_unsupported_ok &&
        result->no_optional_recover_unsupported_ok &&
        result->ok_query_forward_ok &&
        result->ok_error_state_forward_ok &&
        result->ok_recover_ok &&
        result->query_status_propagation_ok &&
        result->error_state_status_propagation_ok &&
        result->recover_status_propagation_ok &&
        result->query_last_status_ok &&
        result->error_state_last_status_ok &&
        result->recover_last_status_ok;
}