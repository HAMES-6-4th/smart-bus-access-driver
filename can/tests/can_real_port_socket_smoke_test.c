#include "can_real_port_socket_smoke_test.h"

#include <string.h>

#include "can_platform.h"

void CANRealPortSocketSmokeTestRun(CANRealPortSocketSmokeTestResult *result)
{
    CANCore core;
    CANCoreOpenParams open_params;
    CANSocket socket;
    CANFrame tx_frame;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(&tx_frame, 0, sizeof(tx_frame));

    result->init_status = CAN_STATUS_EINVAL;
    result->open_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->poll_status = CAN_STATUS_EINVAL;
    result->send_status = CAN_STATUS_EINVAL;
    result->get_error_state_status = CAN_STATUS_EINVAL;
    result->recover_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;

    CANCoreInit(&core);
    CANSocketInit(&socket);

    status = CANPlatformInit();
    result->init_status = status;
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    CANCoreInitOpenParams(&open_params);
    open_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    open_params.channel_config.timing.nominal_bitrate = 500000U;
    open_params.channel_config.enable_loopback = false;
    open_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    open_params.channel_config.rx_filter.enabled = false;

    status = CANPlatformOpen(&core, "can0", &open_params);
    result->open_status = status;
    if (status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        return;
    }

    status = CANCoreStart(&core);
    result->start_status = status;
    if (status != CAN_STATUS_OK)
    {
        result->close_status = CANPlatformClose(&core);
        CANPlatformDeinit();
        return;
    }

    CANSocketBindCore(&socket, &core);

    result->bind_ok =
        (CANSocketGetCore(&socket) == &core) &&
        (CANSocketIsOpen(&socket) == true) &&
        (CANSocketIsStarted(&socket) == true);

    result->ready_mask = 0U;
    result->poll_status =
        CANSocketPoll(&socket,
                      (uint32_t)CAN_CORE_EVENT_TX_READY,
                      0U,
                      &result->ready_mask);

    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = 0x71AU;
    tx_frame.len = 4U;
    tx_frame.data[0] = 0x01U;
    tx_frame.data[1] = 0x23U;
    tx_frame.data[2] = 0x45U;
    tx_frame.data[3] = 0x67U;

    result->send_status =
        CANSocketSend(&socket, &tx_frame);

    memset(&result->error_state, 0, sizeof(result->error_state));
    result->get_error_state_status =
        CANSocketGetErrorState(&socket, &result->error_state);

    result->recover_status =
        CANSocketRecover(&socket);

    result->socket_path_ok =
        result->bind_ok &&
        (result->poll_status == CAN_STATUS_OK) &&
        ((result->ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U) &&
        (result->send_status == CAN_STATUS_OK) &&
        (result->get_error_state_status == CAN_STATUS_OK) &&
        (result->recover_status == CAN_STATUS_OK);

    result->stop_status = CANCoreStop(&core);
    result->close_status = CANPlatformClose(&core);

    result->lifecycle_ok =
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK);

    CANPlatformDeinit();
}

bool CANRealPortSocketSmokeTestPassed(const CANRealPortSocketSmokeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        result->socket_path_ok &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        result->lifecycle_ok;
}