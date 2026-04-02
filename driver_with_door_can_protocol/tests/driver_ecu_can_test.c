#include "driver_ecu_can_test.h"

#include <string.h>

#include "driver_ecu_can.h"

typedef struct DriverEcuCanFakeChannelStruct
{
    CANFrame last_tx_frame;
    CANFrame scripted_rx_frame;

    CANStatus send_status;
    CANStatus receive_status;

    uint32_t query_mask;
    CANStatus query_status;

    uint32_t send_call_count;
    uint32_t receive_call_count;
    uint32_t query_call_count;
} DriverEcuCanFakeChannel;

static CANStatus DriverEcuCanFakeOpen(void *driver_channel,
                                      const void *driver_port,
                                      const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus DriverEcuCanFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus DriverEcuCanFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus DriverEcuCanFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus DriverEcuCanFakeSend(void *driver_channel, const CANFrame *frame)
{
    DriverEcuCanFakeChannel *channel;

    channel = (DriverEcuCanFakeChannel *)driver_channel;
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->send_call_count++;
    channel->last_tx_frame = *frame;
    return channel->send_status;
}

static CANStatus DriverEcuCanFakeReceive(void *driver_channel, CANFrame *frame)
{
    DriverEcuCanFakeChannel *channel;

    channel = (DriverEcuCanFakeChannel *)driver_channel;
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->receive_call_count++;
    *frame = channel->scripted_rx_frame;
    return channel->receive_status;
}

static CANStatus DriverEcuCanFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    DriverEcuCanFakeChannel *channel;

    channel = (DriverEcuCanFakeChannel *)driver_channel;
    if ((channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->query_call_count++;
    *event_mask = channel->query_mask;
    return channel->query_status;
}

static const CANCoreDriverOps g_DriverEcuCanFakeDriverOps = {
    .Open = DriverEcuCanFakeOpen,
    .Close = DriverEcuCanFakeClose,
    .Start = DriverEcuCanFakeStart,
    .Stop = DriverEcuCanFakeStop,
    .Send = DriverEcuCanFakeSend,
    .Receive = DriverEcuCanFakeReceive
};

static const CANCoreOptionalDriverOps g_DriverEcuCanFakeOptionalOps = {
    .QueryEvents = DriverEcuCanFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void DriverEcuCanFakeBindingInit(CANCoreBinding *binding,
                                        DriverEcuCanFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_driver_ecu_can";
    binding->ops = &g_DriverEcuCanFakeDriverOps;
    binding->optional_ops = &g_DriverEcuCanFakeOptionalOps;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = false;
    binding->capabilities.supports_brs = false;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = true;
    binding->capabilities.supports_termination_control = false;
}

void DriverEcuCanRunTest(DriverEcuCanTestResult *result)
{
    DriverEcuCan ecu;
    DriverEcuCanConfig config;
    DriverEcuCanFakeChannel channel;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANStatus status;
    bool updated;
    const DriverEcuCanStatusSnapshot *snapshot;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    DriverEcuCanInit(&ecu);
    DriverEcuCanInitConfig(&config);

    result->init_config_ok =
        (config.port_name != 0) &&
        (config.nominal_bitrate == 500000U) &&
        (ecu.status.status_received == false);

    result->invalid_arg_ok =
        (DriverEcuCanSendCommand(0, DOOR_CMD_OPEN, RAMP_CMD_NOP, false, false) == CAN_STATUS_EINVAL) &&
        (DriverEcuCanPollStatus(0, 0U, &updated) == CAN_STATUS_EINVAL) &&
        (DriverEcuCanPollStatus(&ecu, 0U, 0) == CAN_STATUS_EINVAL);

    DriverEcuCanFakeBindingInit(&binding, &channel);

    channel.send_status = CAN_STATUS_OK;
    channel.receive_status = CAN_STATUS_OK;
    channel.query_status = CAN_STATUS_OK;
    channel.query_mask = (uint32_t)CAN_CORE_EVENT_RX_READY;

    memset(&channel.scripted_rx_frame, 0, sizeof(channel.scripted_rx_frame));
    channel.scripted_rx_frame.id_type = CAN_ID_STANDARD;
    channel.scripted_rx_frame.mode = CAN_MODE_CLASSIC;
    channel.scripted_rx_frame.id = DOOR_CAN_ID_STATUS;
    channel.scripted_rx_frame.len = 1U;
    channel.scripted_rx_frame.data[0] =
        DriverWithDoorCanPackStatus(true,
                          DOOR_STATE_MOVING,
                          RAMP_STATE_DEPLOYED,
                          true,
                          true);

    CANCoreInit(&ecu.core);
    CANSocketInit(&ecu.socket);
    CANCoreInitOpenParams(&open_params);
    open_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    open_params.channel_config.timing.nominal_bitrate = 500000U;

    status = CANCoreOpen(&ecu.core, &binding, &open_params);
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    status = CANCoreStart(&ecu.core);
    if (status != CAN_STATUS_OK)
    {
        (void)CANCoreClose(&ecu.core);
        return;
    }

    CANSocketBindCore(&ecu.socket, &ecu.core);

    status = DriverEcuCanSendCommand(&ecu,
                                     DOOR_CMD_OPEN,
                                     RAMP_CMD_DEPLOY,
                                     true,
                                     false);

    result->send_command_ok =
        (status == CAN_STATUS_OK) &&
        (channel.send_call_count == 1U) &&
        (channel.last_tx_frame.id_type == CAN_ID_STANDARD) &&
        (channel.last_tx_frame.mode == CAN_MODE_CLASSIC) &&
        (channel.last_tx_frame.id == DOOR_CAN_ID_COMMAND) &&
        (channel.last_tx_frame.len == 1U) &&
        (DriverWithDoorCanGetDoorCommand(channel.last_tx_frame.data[0]) == DOOR_CMD_OPEN) &&
        (DriverWithDoorCanGetRampCommand(channel.last_tx_frame.data[0]) == RAMP_CMD_DEPLOY) &&
        (DriverWithDoorCanGetEmergencyStop(channel.last_tx_frame.data[0]) == true) &&
        (DriverWithDoorCanGetResetFault(channel.last_tx_frame.data[0]) == false);

    updated = false;
    status = DriverEcuCanPollStatus(&ecu, 1234U, &updated);
    snapshot = DriverEcuCanGetStatus(&ecu);

    result->poll_status_ok =
        (status == CAN_STATUS_OK) &&
        (updated == true) &&
        (channel.query_call_count == 1U) &&
        (channel.receive_call_count == 1U) &&
        (snapshot != 0) &&
        (snapshot->status_received == true) &&
        (snapshot->last_status_rx_ms == 1234U) &&
        (snapshot->pinch_detected == true) &&
        (snapshot->door_state == DOOR_STATE_MOVING) &&
        (snapshot->ramp_state == RAMP_STATE_DEPLOYED) &&
        (snapshot->fault_present == true) &&
        (snapshot->alive_toggle == true);

    result->status_alive_ok =
        (DriverEcuCanIsStatusAlive(&ecu, 1400U, 300U) == true);

    result->status_timeout_ok =
        (DriverEcuCanIsStatusAlive(&ecu, 1600U, 300U) == false);

    (void)CANCoreClose(&ecu.core);
}

bool DriverEcuCanTestPassed(const DriverEcuCanTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->init_config_ok &&
        result->send_command_ok &&
        result->poll_status_ok &&
        result->status_alive_ok &&
        result->status_timeout_ok &&
        result->invalid_arg_ok;
}