#include "driver_ecu_can.h"

#include <string.h>

static void DriverEcuCanBuildOpenParams(CANSocketOpenParams *params,
                                        const DriverECUCANConfig *config)
{
    CANSocketInitOpenParamsClassic500k(params);

    params->port_name = config->port_name;
    params->channel_config.timing.nominal_bitrate = config->nominal_bitrate;

    params->channel_config.rx_path = CAN_RX_PATH_BUFFER;
    params->channel_config.rx_filter.enabled = true;
    params->channel_config.rx_filter.id_type = CAN_ID_STANDARD;
    params->channel_config.rx_filter.id = DOOR_CAN_ID_STATUS;
    params->channel_config.rx_filter.mask = 0x7FFU;
}

void driverECUCANInit(DriverECUCAN *ecu)
{
    if (ecu == 0)
    {
        return;
    }

    memset(ecu, 0, sizeof(*ecu));
    CANSocketInit(&ecu->socket);
}

void driverECUCANInitConfig(DriverECUCANConfig *config)
{
    if (config == 0)
    {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->port_name = "can0";
    config->nominal_bitrate = 500000U;
}

CANStatus driverECUCANOpen(DriverECUCAN *ecu, const DriverECUCANConfig *config)
{
    CANSocketOpenParams open_params;
    CANStatus status;

    if ((ecu == 0) || (config == 0) || (config->port_name == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (ecu->is_open)
    {
        return CAN_STATUS_EBUSY;
    }

    memset(&ecu->status, 0, sizeof(ecu->status));
    DriverEcuCanBuildOpenParams(&open_params, config);

    status = CANSocketOpen(&ecu->socket, &open_params);
    if (status == CAN_STATUS_OK)
    {
        ecu->is_open = true;
    }

    return status;
}

CANStatus driverECUCANClose(DriverECUCAN *ecu)
{
    CANStatus status;

    if (ecu == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    status = CANSocketClose(&ecu->socket);
    if (status == CAN_STATUS_OK)
    {
        ecu->is_open = false;
    }

    return status;
}

CANStatus driverECUCANSendCommand(DriverECUCAN *ecu,
                                  uint8_t door_cmd,
                                  uint8_t ramp_cmd,
                                  bool ramp_manual,
                                  bool emergency_stop,
                                  bool reset_fault)
{
    uint8_t payload;

    if (ecu == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if ((door_cmd > DOOR_CMD_STOP) || (ramp_cmd > RAMP_CMD_STOP))
    {
        return CAN_STATUS_EINVAL;
    }

    payload = DriverWithDoorCanPackCommand(door_cmd,
                                           ramp_cmd,
                                           ramp_manual,
                                           emergency_stop,
                                           reset_fault);

    return CANSocketSendClassicStd(&ecu->socket, DOOR_CAN_ID_COMMAND, &payload, 1U);
}

CANStatus driverECUCANPollStatus(DriverECUCAN *ecu,
                                 uint32_t now_ms,
                                 bool *updated)
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

    if ((frame.id_type != CAN_ID_STANDARD) ||
        (frame.id != DOOR_CAN_ID_STATUS) ||
        (frame.len < 1U))
    {
        return CAN_STATUS_EINVAL;
    }

    ecu->status.status_received = true;
    ecu->status.last_status_rx_ms = now_ms;
    ecu->status.pinch_detected = DriverWithDoorCanGetPinch(frame.data[0]);
    ecu->status.door_state = DriverWithDoorCanGetDoorState(frame.data[0]);
    ecu->status.ramp_state = DriverWithDoorCanGetRampState(frame.data[0]);
    ecu->status.fault_present = DriverWithDoorCanGetFaultPresent(frame.data[0]);
    ecu->status.alive_toggle = DriverWithDoorCanGetAliveToggle(frame.data[0]);

    if (updated != 0)
    {
        *updated = true;
    }

    return CAN_STATUS_OK;
}

const DriverECUCANStatusSnapshot *driverECUCANGetStatus(const DriverECUCAN *ecu)
{
    if (ecu == 0)
    {
        return 0;
    }

    return &ecu->status;
}

bool driverECUCANIsStatusAlive(const DriverECUCAN *ecu,
                               uint32_t now_ms,
                               uint32_t timeout_ms)
{
    if (ecu == 0)
    {
        return false;
    }

    if (ecu->status.status_received == false)
    {
        return false;
    }

    return ((uint32_t)(now_ms - ecu->status.last_status_rx_ms) <= timeout_ms);
}
