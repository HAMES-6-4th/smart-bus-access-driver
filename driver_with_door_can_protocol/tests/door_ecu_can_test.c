#include "door_ecu_can_test.h"

#include <string.h>

#include "door_ecu_can.h"

typedef struct DoorEcuCanFakeChannelStruct
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
} DoorEcuCanFakeChannel;

static CANStatus DoorEcuCanFakeOpen(void *driver_channel,
                                    const void *driver_port,
                                    const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus DoorEcuCanFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus DoorEcuCanFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus DoorEcuCanFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus DoorEcuCanFakeSend(void *driver_channel, const CANFrame *frame)
{
    DoorEcuCanFakeChannel *channel;

    channel = (DoorEcuCanFakeChannel *)driver_channel;
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->send_call_count++;
    channel->last_tx_frame = *frame;
    return channel->send_status;
}

static CANStatus DoorEcuCanFakeReceive(void *driver_channel, CANFrame *frame)
{
    DoorEcuCanFakeChannel *channel;

    channel = (DoorEcuCanFakeChannel *)driver_channel;
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->receive_call_count++;
    *frame = channel->scripted_rx_frame;
    return channel->receive_status;
}

static CANStatus DoorEcuCanFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    DoorEcuCanFakeChannel *channel;

    channel = (DoorEcuCanFakeChannel *)driver_channel;
    if ((channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel->query_call_count++;
    *event_mask = channel->query_mask;
    return channel->query_status;
}

static const CANCoreDriverOps g_DoorEcuCanFakeDriverOps = {
    .Open = DoorEcuCanFakeOpen,
    .Close = DoorEcuCanFakeClose,
    .Start = DoorEcuCanFakeStart,
    .Stop = DoorEcuCanFakeStop,
    .Send = DoorEcuCanFakeSend,
    .Receive = DoorEcuCanFakeReceive
};

static const CANCoreOptionalDriverOps g_DoorEcuCanFakeOptionalOps = {
    .QueryEvents = DoorEcuCanFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void DoorEcuCanFakeBindingInit(CANCoreBinding *binding,
                                      DoorEcuCanFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_door_ecu_can";
    binding->ops = &g_DoorEcuCanFakeDriverOps;
    binding->optional_ops = &g_DoorEcuCanFakeOptionalOps;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = false;
    binding->capabilities.supports_brs = false;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = true;
    binding->capabilities.supports_termination_control = false;
}

void DoorEcuCanRunTest(DoorEcuCanTestResult *result)
{
    DoorEcuCan ecu;
    DoorEcuCanConfig config;
    DoorEcuCanFakeChannel channel;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANStatus status;
    bool updated;
    const DoorEcuCanCommandSnapshot *cmd;
    const DoorEcuCanStatusCache *status_cache;
    bool alive_before;
    bool alive_after;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    DoorEcuCanInit(&ecu);
    DoorEcuCanInitConfig(&config);

    result->init_config_ok =
        (config.port_name != 0) &&
        (config.nominal_bitrate == 500000U) &&
        (ecu.command.command_received == false);

    result->invalid_arg_ok =
        (DoorEcuCanPollCommand(0, 0U, &updated) == CAN_STATUS_EINVAL) &&
        (DoorEcuCanPollCommand(&ecu, 0U, 0) == CAN_STATUS_EINVAL) &&
        (DoorEcuCanPublishStatus(0, false, DOOR_STATE_CLOSED, RAMP_STATE_STOWED, false) == CAN_STATUS_EINVAL);

    DoorEcuCanFakeBindingInit(&binding, &channel);

    channel.send_status = CAN_STATUS_OK;
    channel.receive_status = CAN_STATUS_OK;
    channel.query_status = CAN_STATUS_OK;
    channel.query_mask = (uint32_t)CAN_CORE_EVENT_RX_READY;

    memset(&channel.scripted_rx_frame, 0, sizeof(channel.scripted_rx_frame));
    channel.scripted_rx_frame.id_type = CAN_ID_STANDARD;
    channel.scripted_rx_frame.mode = CAN_MODE_CLASSIC;
    channel.scripted_rx_frame.id = DOOR_CAN_ID_COMMAND;
    channel.scripted_rx_frame.len = 1U;
    channel.scripted_rx_frame.data[0] =
        DriverWithDoorCanPackCommand(DOOR_CMD_CLOSE,
                           RAMP_CMD_STOW,
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

    updated = false;
    status = DoorEcuCanPollCommand(&ecu, 2000U, &updated);
    cmd = DoorEcuCanGetCommand(&ecu);

    result->poll_command_ok =
        (status == CAN_STATUS_OK) &&
        (updated == true) &&
        (channel.query_call_count == 1U) &&
        (channel.receive_call_count == 1U) &&
        (cmd != 0) &&
        (cmd->command_received == true) &&
        (cmd->last_command_rx_ms == 2000U) &&
        (cmd->door_command == DOOR_CMD_CLOSE) &&
        (cmd->ramp_command == RAMP_CMD_STOW) &&
        (cmd->emergency_stop == true) &&
        (cmd->reset_fault == true);

    result->command_alive_ok =
        (DoorEcuCanIsCommandAlive(&ecu, 2200U, 300U) == true);

    result->command_timeout_ok =
        (DoorEcuCanIsCommandAlive(&ecu, 2401U, 300U) == false);

    DoorEcuCanForceSafeCommand(&ecu);
    cmd = DoorEcuCanGetCommand(&ecu);

    result->force_safe_ok =
        (cmd != 0) &&
        (cmd->door_command == DOOR_CMD_STOP) &&
        (cmd->ramp_command == RAMP_CMD_STOP) &&
        (cmd->emergency_stop == false) &&
        (cmd->reset_fault == false);

    alive_before = false;
    status_cache = DoorEcuCanGetStatusCache(&ecu);
    if (status_cache != 0)
    {
        alive_before = status_cache->alive_toggle;
    }

    status = DoorEcuCanPublishStatus(&ecu,
                                     true,
                                     DOOR_STATE_MOVING,
                                     RAMP_STATE_MOVING,
                                     true);

    status_cache = DoorEcuCanGetStatusCache(&ecu);
    alive_after = false;
    if (status_cache != 0)
    {
        alive_after = status_cache->alive_toggle;
    }

    result->publish_status_ok =
        (status == CAN_STATUS_OK) &&
        (channel.send_call_count == 1U) &&
        (channel.last_tx_frame.id_type == CAN_ID_STANDARD) &&
        (channel.last_tx_frame.mode == CAN_MODE_CLASSIC) &&
        (channel.last_tx_frame.id == DOOR_CAN_ID_STATUS) &&
        (channel.last_tx_frame.len == 1U) &&
        (DriverWithDoorCanGetPinch(channel.last_tx_frame.data[0]) == true) &&
        (DriverWithDoorCanGetDoorState(channel.last_tx_frame.data[0]) == DOOR_STATE_MOVING) &&
        (DriverWithDoorCanGetRampState(channel.last_tx_frame.data[0]) == RAMP_STATE_MOVING) &&
        (DriverWithDoorCanGetFaultPresent(channel.last_tx_frame.data[0]) == true);

    result->alive_toggle_ok =
        (status_cache != 0) &&
        (alive_before != alive_after) &&
        (DriverWithDoorCanGetAliveToggle(channel.last_tx_frame.data[0]) == alive_after);

    (void)CANCoreClose(&ecu.core);
}

bool DoorEcuCanTestPassed(const DoorEcuCanTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->init_config_ok &&
        result->poll_command_ok &&
        result->command_alive_ok &&
        result->command_timeout_ok &&
        result->force_safe_ok &&
        result->publish_status_ok &&
        result->alive_toggle_ok &&
        result->invalid_arg_ok;
}