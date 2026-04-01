#include "can_core_loopback_test.h"

#include <string.h>

#include "can_core.h"
#include "can_platform.h"

#define CAN_LOOPBACK_POLL_LIMIT    (100000U)

static void CANLoopbackInitOneShotResult(CANLoopbackOneShotResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    result->init_status = CAN_STATUS_EINVAL;
    result->tx_open_status = CAN_STATUS_EINVAL;
    result->rx_open_status = CAN_STATUS_EINVAL;
    result->tx_start_status = CAN_STATUS_EINVAL;
    result->rx_start_status = CAN_STATUS_EINVAL;
    result->send_status = CAN_STATUS_EINVAL;
    result->receive_status = CAN_STATUS_EINVAL;
    result->tx_stop_status = CAN_STATUS_EINVAL;
    result->rx_stop_status = CAN_STATUS_EINVAL;
    result->tx_close_status = CAN_STATUS_EINVAL;
    result->rx_close_status = CAN_STATUS_EINVAL;
}

static void CANLoopbackRunOneShot(CANLoopbackOneShotResult *result,
                                  const CANCoreOpenParams *tx_params,
                                  const CANCoreOpenParams *rx_params,
                                  uint32_t tx_id)
{
    CANCore tx_core;
    CANCore rx_core;
    CANFrame tx_frame;
    CANFrame rx_frame;
    CANStatus status;
    uint32_t i;

    if ((result == 0) || (tx_params == 0) || (rx_params == 0))
    {
        return;
    }

    CANLoopbackInitOneShotResult(result);
    result->tx_id = tx_id;

    CANCoreInit(&tx_core);
    CANCoreInit(&rx_core);

    status = CANPlatformInit();
    result->init_status = status;
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    status = CANPlatformOpen(&tx_core, "can0_lb0", tx_params);
    result->tx_open_status = status;
    if (status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        return;
    }

    status = CANPlatformOpen(&rx_core, "can0_lb1", rx_params);
    result->rx_open_status = status;
    if (status != CAN_STATUS_OK)
    {
        result->tx_close_status = CANPlatformClose(&tx_core);
        CANPlatformDeinit();
        return;
    }

    status = CANCoreStart(&tx_core);
    result->tx_start_status = status;
    if (status != CAN_STATUS_OK)
    {
        result->rx_close_status = CANPlatformClose(&rx_core);
        result->tx_close_status = CANPlatformClose(&tx_core);
        CANPlatformDeinit();
        return;
    }

    status = CANCoreStart(&rx_core);
    result->rx_start_status = status;
    if (status != CAN_STATUS_OK)
    {
        result->tx_stop_status = CANCoreStop(&tx_core);
        result->rx_close_status = CANPlatformClose(&rx_core);
        result->tx_close_status = CANPlatformClose(&tx_core);
        CANPlatformDeinit();
        return;
    }

    memset(&tx_frame, 0, sizeof(tx_frame));
    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = tx_id;
    tx_frame.len = 1U;
    tx_frame.data[0] = 0x5AU;

    status = CANCoreSend(&tx_core, &tx_frame);
    result->send_status = status;

    memset(&rx_frame, 0, sizeof(rx_frame));
    result->receive_status = CAN_STATUS_ENODATA;

    for (i = 0U; i < CAN_LOOPBACK_POLL_LIMIT; ++i)
    {
        status = CANCoreReceive(&rx_core, &rx_frame);
        result->poll_count = i + 1U;

        if (status == CAN_STATUS_OK)
        {
            result->receive_status = status;
            result->received = true;
            result->rx_id = rx_frame.id;
            result->rx_len = rx_frame.len;
            result->rx_first_byte = rx_frame.data[0];
            result->id_match = (rx_frame.id == tx_frame.id);
            break;
        }

        if (status != CAN_STATUS_ENODATA)
        {
            result->receive_status = status;
            break;
        }
    }

    result->rx_stop_status = CANCoreStop(&rx_core);
    result->tx_stop_status = CANCoreStop(&tx_core);

    result->rx_close_status = CANPlatformClose(&rx_core);
    result->tx_close_status = CANPlatformClose(&tx_core);

    CANPlatformDeinit();
}

void CANLoopbackRunAcceptAllFifo0Test(CANLoopbackAcceptAllFifo0TestResult *result)
{
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    CANCoreInitOpenParams(&tx_params);
    tx_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    tx_params.channel_config.timing.nominal_bitrate = 500000U;
    tx_params.channel_config.enable_loopback = true;
    tx_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    tx_params.channel_config.rx_filter.enabled = false;

    CANCoreInitOpenParams(&rx_params);
    rx_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    rx_params.channel_config.timing.nominal_bitrate = 500000U;
    rx_params.channel_config.enable_loopback = true;
    rx_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    rx_params.channel_config.rx_filter.enabled = false;

    CANLoopbackRunOneShot(&result->probe_000, &tx_params, &rx_params, 0x000U);
    CANLoopbackRunOneShot(&result->probe_001, &tx_params, &rx_params, 0x001U);
    CANLoopbackRunOneShot(&result->probe_123, &tx_params, &rx_params, 0x123U);
    CANLoopbackRunOneShot(&result->probe_7ff, &tx_params, &rx_params, 0x7FFU);
}

void CANLoopbackRunExactBufferFilterTest(CANLoopbackExactBufferFilterTestResult *result)
{
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    CANCoreInitOpenParams(&tx_params);
    tx_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    tx_params.channel_config.timing.nominal_bitrate = 500000U;
    tx_params.channel_config.enable_loopback = true;
    tx_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    tx_params.channel_config.rx_filter.enabled = false;

    CANCoreInitOpenParams(&rx_params);
    rx_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    rx_params.channel_config.timing.nominal_bitrate = 500000U;
    rx_params.channel_config.enable_loopback = true;
    rx_params.channel_config.rx_path = CAN_RX_PATH_BUFFER;
    rx_params.channel_config.rx_filter.enabled = true;
    rx_params.channel_config.rx_filter.id_type = CAN_ID_STANDARD;
    rx_params.channel_config.rx_filter.id = 0x123U;
    rx_params.channel_config.rx_filter.mask = 0x7FFU;

    CANLoopbackRunOneShot(&result->match_123, &tx_params, &rx_params, 0x123U);
    CANLoopbackRunOneShot(&result->miss_124, &tx_params, &rx_params, 0x124U);
}

static bool CANLoopbackOneShotSucceeded(const CANLoopbackOneShotResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->tx_open_status == CAN_STATUS_OK) &&
        (result->rx_open_status == CAN_STATUS_OK) &&
        (result->tx_start_status == CAN_STATUS_OK) &&
        (result->rx_start_status == CAN_STATUS_OK) &&
        (result->send_status == CAN_STATUS_OK) &&
        (result->receive_status == CAN_STATUS_OK) &&
        (result->received == true) &&
        (result->id_match == true) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}

bool CANLoopbackAcceptAllFifo0TestPassed(const CANLoopbackAcceptAllFifo0TestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        CANLoopbackOneShotSucceeded(&result->probe_000) &&
        CANLoopbackOneShotSucceeded(&result->probe_001) &&
        CANLoopbackOneShotSucceeded(&result->probe_123) &&
        CANLoopbackOneShotSucceeded(&result->probe_7ff);
}

bool CANLoopbackExactBufferFilterTestPassed(const CANLoopbackExactBufferFilterTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    if (CANLoopbackOneShotSucceeded(&result->match_123) == false)
    {
        return false;
    }

    return
        (result->miss_124.init_status == CAN_STATUS_OK) &&
        (result->miss_124.tx_open_status == CAN_STATUS_OK) &&
        (result->miss_124.rx_open_status == CAN_STATUS_OK) &&
        (result->miss_124.tx_start_status == CAN_STATUS_OK) &&
        (result->miss_124.rx_start_status == CAN_STATUS_OK) &&
        (result->miss_124.send_status == CAN_STATUS_OK) &&
        (result->miss_124.receive_status == CAN_STATUS_ENODATA) &&
        (result->miss_124.received == false) &&
        (result->miss_124.tx_stop_status == CAN_STATUS_OK) &&
        (result->miss_124.rx_stop_status == CAN_STATUS_OK) &&
        (result->miss_124.tx_close_status == CAN_STATUS_OK) &&
        (result->miss_124.rx_close_status == CAN_STATUS_OK);
}