#include "can_core_fd_loopback_test.h"

#include <string.h>

#include "can_core.h"
#include "can_platform.h"

#define CAN_FD_LOOPBACK_POLL_LIMIT   (100000U)
#define CAN_FD_TEST_ID               (0x123U)
#define CAN_FD_TEST_LEN              (64U)
#define CAN_FD_TEST_NOMINAL_BITRATE  (500000U)
#define CAN_FD_TEST_DATA_BITRATE     (2000000U)

static void CANFDLoopbackInitOneShotResult(CANFDLoopbackOneShotResult *result)
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

static void CANFDLoopbackFillPayload(uint8_t *data, uint8_t len, uint8_t seed)
{
    uint32_t i;

    if (data == 0)
    {
        return;
    }

    for (i = 0U; i < len; ++i)
    {
        data[i] = (uint8_t)(seed + (uint8_t)i);
    }
}

static void CANFDLoopbackRunOneShot(CANFDLoopbackOneShotResult *result, CANMode fd_mode)
{
    CANCore tx_core;
    CANCore rx_core;
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;
    CANFrame tx_frame;
    CANFrame rx_frame;
    CANStatus status;
    uint32_t i;
    uint8_t seed;

    if (result == 0)
    {
        return;
    }

    CANFDLoopbackInitOneShotResult(result);

    result->tx_mode = fd_mode;
    result->tx_id = CAN_FD_TEST_ID;
    result->tx_len = CAN_FD_TEST_LEN;

    CANCoreInit(&tx_core);
    CANCoreInit(&rx_core);

    CANCoreInitOpenParams(&tx_params);
    tx_params.channel_config.timing.mode = fd_mode;
    tx_params.channel_config.timing.nominal_bitrate = CAN_FD_TEST_NOMINAL_BITRATE;
    tx_params.channel_config.timing.data_bitrate = CAN_FD_TEST_DATA_BITRATE;
    tx_params.channel_config.enable_loopback = true;
    tx_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    tx_params.channel_config.rx_filter.enabled = false;

    CANCoreInitOpenParams(&rx_params);
    rx_params.channel_config.timing.mode = fd_mode;
    rx_params.channel_config.timing.nominal_bitrate = CAN_FD_TEST_NOMINAL_BITRATE;
    rx_params.channel_config.timing.data_bitrate = CAN_FD_TEST_DATA_BITRATE;
    rx_params.channel_config.enable_loopback = true;
    rx_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    rx_params.channel_config.rx_filter.enabled = false;

    status = CANPlatformInit();
    result->init_status = status;
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    status = CANPlatformOpen(&tx_core, "can0_lb0", &tx_params);
    result->tx_open_status = status;
    if (status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        return;
    }

    status = CANPlatformOpen(&rx_core, "can0_lb1", &rx_params);
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
    tx_frame.mode = fd_mode;
    tx_frame.id = CAN_FD_TEST_ID;
    tx_frame.len = CAN_FD_TEST_LEN;

    seed = (fd_mode == CAN_MODE_FD_BRS) ? 0x80U : 0x10U;
    CANFDLoopbackFillPayload(tx_frame.data, tx_frame.len, seed);

    status = CANCoreSend(&tx_core, &tx_frame);
    result->send_status = status;

    memset(&rx_frame, 0, sizeof(rx_frame));
    result->receive_status = CAN_STATUS_ENODATA;

    for (i = 0U; i < CAN_FD_LOOPBACK_POLL_LIMIT; ++i)
    {
        status = CANCoreReceive(&rx_core, &rx_frame);
        result->poll_count = i + 1U;

        if (status == CAN_STATUS_OK)
        {
            result->receive_status = status;
            result->received = true;
            result->rx_id = rx_frame.id;
            result->rx_mode = rx_frame.mode;
            result->rx_len = rx_frame.len;
            result->rx_first_byte = rx_frame.data[0];
            result->rx_last_byte = rx_frame.data[rx_frame.len - 1U];

            result->id_match = (rx_frame.id == tx_frame.id);
            result->mode_match = (rx_frame.mode == tx_frame.mode);
            result->len_match = (rx_frame.len == tx_frame.len);

            if (result->len_match == true)
            {
                result->payload_match =
                    (memcmp(rx_frame.data, tx_frame.data, tx_frame.len) == 0);
            }

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

static bool CANFDLoopbackOneShotPassed(const CANFDLoopbackOneShotResult *result, CANMode expected_mode)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->tx_mode == expected_mode) &&
        (result->tx_len == CAN_FD_TEST_LEN) &&
        (result->init_status == CAN_STATUS_OK) &&
        (result->tx_open_status == CAN_STATUS_OK) &&
        (result->rx_open_status == CAN_STATUS_OK) &&
        (result->tx_start_status == CAN_STATUS_OK) &&
        (result->rx_start_status == CAN_STATUS_OK) &&
        (result->send_status == CAN_STATUS_OK) &&
        (result->receive_status == CAN_STATUS_OK) &&
        (result->received == true) &&
        (result->id_match == true) &&
        (result->mode_match == true) &&
        (result->len_match == true) &&
        (result->payload_match == true) &&
        (result->rx_len == CAN_FD_TEST_LEN) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}

void CANFDLoopbackRunNoBrs64Test(CANFDLoopbackOneShotResult *result)
{
    CANFDLoopbackRunOneShot(result, CAN_MODE_FD_NO_BRS);
}

void CANFDLoopbackRunBrs64Test(CANFDLoopbackOneShotResult *result)
{
    CANFDLoopbackRunOneShot(result, CAN_MODE_FD_BRS);
}

bool CANFDLoopbackNoBrs64TestPassed(const CANFDLoopbackOneShotResult *result)
{
    return CANFDLoopbackOneShotPassed(result, CAN_MODE_FD_NO_BRS);
}

bool CANFDLoopbackBrs64TestPassed(const CANFDLoopbackOneShotResult *result)
{
    return CANFDLoopbackOneShotPassed(result, CAN_MODE_FD_BRS);
}