#include "door_ecu_can.h"

#include <string.h>

static void DoorEcuCanBuildOpenParams(CANSocketOpenParams *params, const DoorECUCANConfig *config)
{
    CANSocketInitOpenParamsClassic500k(params);

    params->port_name = config->port_name;
    params->channel_config.timing.nominal_bitrate = config->nominal_bitrate;

    params->channel_config.rx_path = CAN_RX_PATH_BUFFER;
    params->channel_config.rx_filter.enabled = true;
    params->channel_config.rx_filter.id_type = CAN_ID_STANDARD;
    params->channel_config.rx_filter.id = DOOR_CAN_ID_COMMAND;
    params->channel_config.rx_filter.mask = 0x7FFU;
}

void doorECUCANInit(DoorECUCAN *ecu)
{
    if (ecu == 0)
    {
        return;
    }

    memset(ecu, 0, sizeof(*ecu));
    CANSocketInit(&ecu->socket);
}

void doorECUCANInitConfig(DoorECUCANConfig *config)
{
    if (config == 0)
    {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->port_name = "can0";
    config->nominal_bitrate = 500000U;
}

CANStatus doorECUCANOpen(DoorECUCAN *ecu, const DoorECUCANConfig *config)
{
    CANSocketOpenParams open_params;

    if ((ecu == 0) || (config == 0) || (config->port_name == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (CANSocketGetCoreConst(&ecu->socket) != 0)
    {
        return CAN_STATUS_EBUSY;
    }

    memset(&ecu->command, 0, sizeof(ecu->command));
    memset(&ecu->status_cache, 0, sizeof(ecu->status_cache));
    DoorEcuCanBuildOpenParams(&open_params, config);

    return CANSocketOpen(&ecu->socket, &open_params);
}

CANStatus doorECUCANClose(DoorECUCAN *ecu)
{
    if (ecu == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    return CANSocketClose(&ecu->socket);
}

CANStatus doorECUCANPollCommand(DoorECUCAN *ecu, uint32_t now_ms, bool *updated)
{
    CANFrame frame;
    CANStatus status;

    if (ecu == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (updated != 0)
    {
        *updated = false;
    }

    status = CANSocketReceiveNow(&ecu->socket, &frame);
    if (status == CAN_STATUS_ENODATA)
    {
        return CAN_STATUS_OK;
    }

    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    if ((frame.id_type != CAN_ID_STANDARD) || (frame.id != DOOR_CAN_ID_COMMAND) || (frame.len < 1U))
    {
        return CAN_STATUS_EINVAL;
    }

    ecu->command.command_received = true;
    ecu->command.last_command_rx_ms = now_ms;
    ecu->command.door_command = DriverWithDoorCanGetDoorCommand(frame.data[0]);
    ecu->command.ramp_command = DriverWithDoorCanGetRampCommand(frame.data[0]);
    ecu->command.emergency_stop = DriverWithDoorCanGetEmergencyStop(frame.data[0]);
    ecu->command.reset_fault = DriverWithDoorCanGetResetFault(frame.data[0]);

    if (updated != 0)
    {
        *updated = true;
    }

    return CAN_STATUS_OK;
}

const DoorECUCANCommandSnapshot *doorECUCANGetCommand(const DoorECUCAN *ecu)
{
    if (ecu == 0)
    {
        return 0;
    }

    return &ecu->command;
}

bool doorECUCANIsCommandAlive(const DoorECUCAN *ecu, uint32_t now_ms, uint32_t timeout_ms)
{
    if (ecu == 0)
    {
        return false;
    }

    if (ecu->command.command_received == false)
    {
        return false;
    }

    return ((uint32_t)(now_ms - ecu->command.last_command_rx_ms) <= timeout_ms);
}

void doorECUCANForceSafeCommand(DoorECUCAN *ecu)
{
    if (ecu == 0)
    {
        return;
    }

    ecu->command.door_command = DOOR_CMD_STOP;
    ecu->command.ramp_command = RAMP_CMD_STOP;
    ecu->command.emergency_stop = false;
    ecu->command.reset_fault = false;
}

CANStatus doorECUCANPublishStatus(DoorECUCAN *ecu,
                                  bool pinch_detected,
                                  uint8_t door_state,
                                  uint8_t ramp_state,
                                  bool fault_present)
{
    uint8_t payload;

    if (ecu == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if ((door_state > DOOR_STATE_FAULT) || (ramp_state > RAMP_STATE_FAULT))
    {
        return CAN_STATUS_EINVAL;
    }

    ecu->status_cache.pinch_detected = pinch_detected;
    ecu->status_cache.door_state = door_state;
    ecu->status_cache.ramp_state = ramp_state;
    ecu->status_cache.fault_present = fault_present;
    ecu->status_cache.alive_toggle = !ecu->status_cache.alive_toggle;

    payload = DriverWithDoorCanPackStatus(ecu->status_cache.pinch_detected,
                                          ecu->status_cache.door_state,
                                          ecu->status_cache.ramp_state,
                                          ecu->status_cache.fault_present,
                                          ecu->status_cache.alive_toggle);

    return CANSocketSendClassicStd(&ecu->socket, DOOR_CAN_ID_STATUS, &payload, 1U);
}

const DoorECUCANStatusCache *doorECUCANGetStatusCache(const DoorECUCAN *ecu)
{
    if (ecu == 0)
    {
        return 0;
    }

    return &ecu->status_cache;
}