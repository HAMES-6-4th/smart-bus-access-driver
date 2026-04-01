#include "can_real_port_error_recover_smoke_test.h"

#include <string.h>

#include "can_platform.h"

void CANRealPortErrorRecoverSmokeTestRun(CANRealPortErrorRecoverSmokeTestResult *result)
{
    CANCore core;
    CANCoreOpenParams open_params;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->init_status = CAN_STATUS_EINVAL;
    result->open_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->get_error_state_status = CAN_STATUS_EINVAL;
    result->recover_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;

    CANCoreInit(&core);

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

    memset(&result->error_state, 0, sizeof(result->error_state));

    result->get_error_state_status =
        CANCoreGetErrorState(&core, &result->error_state);

    result->error_state_path_ok =
        (result->get_error_state_status == CAN_STATUS_OK);

    result->recover_status =
        CANCoreRecover(&core);

    result->recover_path_ok =
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

bool CANRealPortErrorRecoverSmokeTestPassed(const CANRealPortErrorRecoverSmokeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        result->error_state_path_ok &&
        result->recover_path_ok &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        result->lifecycle_ok;
}