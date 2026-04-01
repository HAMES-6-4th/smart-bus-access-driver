#include "can_core_extended_id_test.h"

#include <string.h>

#include "can_core.h"
#include "can_platform.h"

#define CAN_EXT_TEST_CLASSIC_LEN            (8U)
#define CAN_EXT_TEST_FD_LEN                 (16U)

#define CAN_EXT_TEST_CLASSIC_NOMINAL_BITRATE (500000U)
#define CAN_EXT_TEST_FD_NOMINAL_BITRATE      (500000U)
#define CAN_EXT_TEST_FD_DATA_BITRATE         (2000000U)

#define CAN_EXT_TEST_MATCH_ID               (0x18DAF110U)
#define CAN_EXT_TEST_MISS_ID                (0x18DAF111U)
#define CAN_EXT_TEST_MASK                   (0x1FFFFFFFU)

#define CAN_EXT_TEST_POLL_LIMIT             (100000U)

static void CANExtendedIdInitProbe(CANExtendedIdProbeResult *probe)
{
    if (probe == 0)
    {
        return;
    }

    memset(probe, 0, sizeof(*probe));
    probe->send_status = CAN_STATUS_EINVAL;
    probe->receive_status = CAN_STATUS_EINVAL;
}

static void CANExtendedIdInitAcceptAllResult(CANExtendedIdAcceptAllTestResult *result)
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
    result->tx_stop_status = CAN_STATUS_EINVAL;
    result->rx_stop_status = CAN_STATUS_EINVAL;
    result->tx_close_status = CAN_STATUS_EINVAL;
    result->rx_close_status = CAN_STATUS_EINVAL;

    CANExtendedIdInitProbe(&result->probe);
}

static void CANExtendedIdInitExactFilterResult(CANExtendedIdExactFilterTestResult *result)
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
    result->tx_stop_status = CAN_STATUS_EINVAL;
    result->rx_stop_status = CAN_STATUS_EINVAL;
    result->tx_close_status = CAN_STATUS_EINVAL;
    result->rx_close_status = CAN_STATUS_EINVAL;

    CANExtendedIdInitProbe(&result->match_probe);
    CANExtendedIdInitProbe(&result->miss_probe);
}

static uint8_t CANExtendedIdPayloadLengthForMode(CANMode mode)
{
    return (mode == CAN_MODE_CLASSIC) ? CAN_EXT_TEST_CLASSIC_LEN : CAN_EXT_TEST_FD_LEN;
}

static uint32_t CANExtendedIdNominalBitrateForMode(CANMode mode)
{
    (void)mode;
    return CAN_EXT_TEST_CLASSIC_NOMINAL_BITRATE;
}

static uint32_t CANExtendedIdDataBitrateForMode(CANMode mode)
{
    return (mode == CAN_MODE_CLASSIC) ? 0U : CAN_EXT_TEST_FD_DATA_BITRATE;
}

static void CANExtendedIdFillPayload(uint8_t *data, uint8_t len, uint8_t seed)
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

static void CANExtendedIdPrepareOpenParams(CANCoreOpenParams *params,
                                           CANMode mode,
                                           CANRxPath rx_path,
                                           bool filter_enabled,
                                           uint32_t filter_id)
{
    if (params == 0)
    {
        return;
    }

    CANCoreInitOpenParams(params);
    params->channel_config.timing.mode = mode;
    params->channel_config.timing.nominal_bitrate = CANExtendedIdNominalBitrateForMode(mode);
    params->channel_config.timing.data_bitrate = CANExtendedIdDataBitrateForMode(mode);
    params->channel_config.enable_loopback = true;
    params->channel_config.rx_path = rx_path;

    params->channel_config.rx_filter.enabled = filter_enabled;
    params->channel_config.rx_filter.enabled = filter_enabled;
    if (filter_enabled == true)
    {
        params->channel_config.rx_filter.id_type = CAN_ID_EXTENDED;
        params->channel_config.rx_filter.id = filter_id;
        params->channel_config.rx_filter.mask = CAN_EXT_TEST_MASK;
    }
}

static void CANExtendedIdBuildFrame(CANFrame *frame,
                                    CANMode mode,
                                    uint32_t id,
                                    uint8_t seed)
{
    uint8_t len;

    if (frame == 0)
    {
        return;
    }

    memset(frame, 0, sizeof(*frame));

    len = CANExtendedIdPayloadLengthForMode(mode);

    frame->id_type = CAN_ID_EXTENDED;
    frame->mode = mode;
    frame->id = id;
    frame->len = len;

    CANExtendedIdFillPayload(frame->data, len, seed);
}

static void CANExtendedIdCopyRxToProbe(CANExtendedIdProbeResult *probe,
                                       const CANFrame *tx_frame,
                                       const CANFrame *rx_frame)
{
    if ((probe == 0) || (tx_frame == 0) || (rx_frame == 0))
    {
        return;
    }

    probe->received = true;
    probe->rx_id = rx_frame->id;
    probe->rx_len = rx_frame->len;
    probe->rx_mode = rx_frame->mode;
    probe->rx_first_byte = rx_frame->data[0];
    probe->rx_last_byte = rx_frame->data[rx_frame->len - 1U];

    probe->id_match = (rx_frame->id == tx_frame->id);
    probe->mode_match = (rx_frame->mode == tx_frame->mode);
    probe->len_match = (rx_frame->len == tx_frame->len);

    if (probe->len_match == true)
    {
        probe->payload_match = (memcmp(rx_frame->data, tx_frame->data, tx_frame->len) == 0);
    }
}

static void CANExtendedIdRunProbe(CANCore *tx_core,
                                  CANCore *rx_core,
                                  CANExtendedIdProbeResult *probe,
                                  CANMode mode,
                                  uint32_t tx_id,
                                  uint8_t seed)
{
    CANFrame tx_frame;
    CANFrame rx_frame;
    CANStatus status;
    uint32_t i;

    if ((tx_core == 0) || (rx_core == 0) || (probe == 0))
    {
        return;
    }

    CANExtendedIdInitProbe(probe);
    CANExtendedIdBuildFrame(&tx_frame, mode, tx_id, seed);

    probe->tx_id = tx_frame.id;
    probe->tx_len = tx_frame.len;
    probe->tx_mode = tx_frame.mode;

    probe->send_status = CANCoreSend(tx_core, &tx_frame);
    if (probe->send_status != CAN_STATUS_OK)
    {
        return;
    }

    memset(&rx_frame, 0, sizeof(rx_frame));
    probe->receive_status = CAN_STATUS_ENODATA;

    for (i = 0U; i < CAN_EXT_TEST_POLL_LIMIT; ++i)
    {
        status = CANCoreReceive(rx_core, &rx_frame);
        probe->poll_count = i + 1U;

        if (status == CAN_STATUS_OK)
        {
            probe->receive_status = status;
            CANExtendedIdCopyRxToProbe(probe, &tx_frame, &rx_frame);
            return;
        }

        if (status != CAN_STATUS_ENODATA)
        {
            probe->receive_status = status;
            return;
        }
    }
}

static void CANExtendedIdOpenAndStartPair(CANCore *tx_core,
                                          CANCore *rx_core,
                                          CANCoreOpenParams *tx_params,
                                          CANCoreOpenParams *rx_params,
                                          CANStatus *init_status,
                                          CANStatus *tx_open_status,
                                          CANStatus *rx_open_status,
                                          CANStatus *tx_start_status,
                                          CANStatus *rx_start_status)
{
    if ((tx_core == 0) || (rx_core == 0) ||
        (tx_params == 0) || (rx_params == 0) ||
        (init_status == 0) || (tx_open_status == 0) || (rx_open_status == 0) ||
        (tx_start_status == 0) || (rx_start_status == 0))
    {
        return;
    }

    *init_status = CANPlatformInit();
    if (*init_status != CAN_STATUS_OK)
    {
        return;
    }

    *tx_open_status = CANPlatformOpen(tx_core, "can0_lb0", tx_params);
    if (*tx_open_status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        return;
    }

    *rx_open_status = CANPlatformOpen(rx_core, "can0_lb1", rx_params);
    if (*rx_open_status != CAN_STATUS_OK)
    {
        (void)CANPlatformClose(tx_core);
        CANPlatformDeinit();
        return;
    }

    *tx_start_status = CANCoreStart(tx_core);
    if (*tx_start_status != CAN_STATUS_OK)
    {
        (void)CANPlatformClose(rx_core);
        (void)CANPlatformClose(tx_core);
        CANPlatformDeinit();
        return;
    }

    *rx_start_status = CANCoreStart(rx_core);
    if (*rx_start_status != CAN_STATUS_OK)
    {
        (void)CANCoreStop(tx_core);
        (void)CANPlatformClose(rx_core);
        (void)CANPlatformClose(tx_core);
        CANPlatformDeinit();
        return;
    }
}

static void CANExtendedIdStopClosePair(CANCore *tx_core,
                                       CANCore *rx_core,
                                       CANStatus *tx_stop_status,
                                       CANStatus *rx_stop_status,
                                       CANStatus *tx_close_status,
                                       CANStatus *rx_close_status)
{
    if ((tx_core == 0) || (rx_core == 0) ||
        (tx_stop_status == 0) || (rx_stop_status == 0) ||
        (tx_close_status == 0) || (rx_close_status == 0))
    {
        return;
    }

    *rx_stop_status = CANCoreStop(rx_core);
    *tx_stop_status = CANCoreStop(tx_core);

    *rx_close_status = CANPlatformClose(rx_core);
    *tx_close_status = CANPlatformClose(tx_core);

    CANPlatformDeinit();
}

static void CANExtendedIdRunAcceptAllTest(CANExtendedIdAcceptAllTestResult *result,
                                          CANMode mode)
{
    CANCore tx_core;
    CANCore rx_core;
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;

    if (result == 0)
    {
        return;
    }

    CANExtendedIdInitAcceptAllResult(result);

    CANCoreInit(&tx_core);
    CANCoreInit(&rx_core);

    CANExtendedIdPrepareOpenParams(&tx_params,
                                   mode,
                                   CAN_RX_PATH_DEFAULT,
                                   false,
                                   0U);

    CANExtendedIdPrepareOpenParams(&rx_params,
                                   mode,
                                   CAN_RX_PATH_DEFAULT,
                                   false,
                                   0U);

    CANExtendedIdOpenAndStartPair(&tx_core,
                                  &rx_core,
                                  &tx_params,
                                  &rx_params,
                                  &result->init_status,
                                  &result->tx_open_status,
                                  &result->rx_open_status,
                                  &result->tx_start_status,
                                  &result->rx_start_status);

    if ((result->init_status != CAN_STATUS_OK) ||
        (result->tx_open_status != CAN_STATUS_OK) ||
        (result->rx_open_status != CAN_STATUS_OK) ||
        (result->tx_start_status != CAN_STATUS_OK) ||
        (result->rx_start_status != CAN_STATUS_OK))
    {
        return;
    }

    CANExtendedIdRunProbe(&tx_core,
                          &rx_core,
                          &result->probe,
                          mode,
                          CAN_EXT_TEST_MATCH_ID,
                          (mode == CAN_MODE_CLASSIC) ? 0x20U : 0x90U);

    CANExtendedIdStopClosePair(&tx_core,
                               &rx_core,
                               &result->tx_stop_status,
                               &result->rx_stop_status,
                               &result->tx_close_status,
                               &result->rx_close_status);
}

static void CANExtendedIdRunExactFilterTest(CANExtendedIdExactFilterTestResult *result,
                                            CANMode mode)
{
    CANCore tx_core;
    CANCore rx_core;
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;

    if (result == 0)
    {
        return;
    }

    CANExtendedIdInitExactFilterResult(result);

    CANCoreInit(&tx_core);
    CANCoreInit(&rx_core);

    CANExtendedIdPrepareOpenParams(&tx_params,
                                mode,
                                CAN_RX_PATH_DEFAULT,
                                false,
                                0U);

    CANExtendedIdPrepareOpenParams(&rx_params,
                                mode,
                                CAN_RX_PATH_BUFFER,
                                true,
                                CAN_EXT_TEST_MATCH_ID);

    CANExtendedIdOpenAndStartPair(&tx_core,
                                  &rx_core,
                                  &tx_params,
                                  &rx_params,
                                  &result->init_status,
                                  &result->tx_open_status,
                                  &result->rx_open_status,
                                  &result->tx_start_status,
                                  &result->rx_start_status);

    if ((result->init_status != CAN_STATUS_OK) ||
        (result->tx_open_status != CAN_STATUS_OK) ||
        (result->rx_open_status != CAN_STATUS_OK) ||
        (result->tx_start_status != CAN_STATUS_OK) ||
        (result->rx_start_status != CAN_STATUS_OK))
    {
        return;
    }

    CANExtendedIdRunProbe(&tx_core,
                          &rx_core,
                          &result->match_probe,
                          mode,
                          CAN_EXT_TEST_MATCH_ID,
                          (mode == CAN_MODE_CLASSIC) ? 0x30U : 0xA0U);

    CANExtendedIdRunProbe(&tx_core,
                          &rx_core,
                          &result->miss_probe,
                          mode,
                          CAN_EXT_TEST_MISS_ID,
                          (mode == CAN_MODE_CLASSIC) ? 0x40U : 0xB0U);

    CANExtendedIdStopClosePair(&tx_core,
                               &rx_core,
                               &result->tx_stop_status,
                               &result->rx_stop_status,
                               &result->tx_close_status,
                               &result->rx_close_status);
}

static bool CANExtendedIdProbePassExpectedRx(const CANExtendedIdProbeResult *probe,
                                             CANMode expected_mode,
                                             uint32_t expected_id)
{
    if (probe == 0)
    {
        return false;
    }

    return
        (probe->tx_mode == expected_mode) &&
        (probe->tx_id == expected_id) &&
        (probe->send_status == CAN_STATUS_OK) &&
        (probe->receive_status == CAN_STATUS_OK) &&
        (probe->received == true) &&
        (probe->id_match == true) &&
        (probe->mode_match == true) &&
        (probe->len_match == true) &&
        (probe->payload_match == true);
}

static bool CANExtendedIdProbePassExpectedMiss(const CANExtendedIdProbeResult *probe,
                                               CANMode expected_mode,
                                               uint32_t expected_id)
{
    if (probe == 0)
    {
        return false;
    }

    return
        (probe->tx_mode == expected_mode) &&
        (probe->tx_id == expected_id) &&
        (probe->send_status == CAN_STATUS_OK) &&
        (probe->receive_status == CAN_STATUS_ENODATA) &&
        (probe->received == false);
}

void CANExtendedIdRunClassicExactBufferFilterTest(CANExtendedIdExactFilterTestResult *result)
{
    CANExtendedIdRunExactFilterTest(result, CAN_MODE_CLASSIC);
}

void CANExtendedIdRunFdBrsExactBufferFilterTest(CANExtendedIdExactFilterTestResult *result)
{
    CANExtendedIdRunExactFilterTest(result, CAN_MODE_FD_BRS);
}

void CANExtendedIdRunClassicFifo0AcceptAllTest(CANExtendedIdAcceptAllTestResult *result)
{
    CANExtendedIdRunAcceptAllTest(result, CAN_MODE_CLASSIC);
}

void CANExtendedIdRunFdBrsFifo0AcceptAllTest(CANExtendedIdAcceptAllTestResult *result)
{
    CANExtendedIdRunAcceptAllTest(result, CAN_MODE_FD_BRS);
}

bool CANExtendedIdClassicExactBufferFilterTestPassed(const CANExtendedIdExactFilterTestResult *result)
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
        CANExtendedIdProbePassExpectedRx(&result->match_probe,
                                         CAN_MODE_CLASSIC,
                                         CAN_EXT_TEST_MATCH_ID) &&
        CANExtendedIdProbePassExpectedMiss(&result->miss_probe,
                                           CAN_MODE_CLASSIC,
                                           CAN_EXT_TEST_MISS_ID) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}

bool CANExtendedIdFdBrsExactBufferFilterTestPassed(const CANExtendedIdExactFilterTestResult *result)
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
        CANExtendedIdProbePassExpectedRx(&result->match_probe,
                                         CAN_MODE_FD_BRS,
                                         CAN_EXT_TEST_MATCH_ID) &&
        CANExtendedIdProbePassExpectedMiss(&result->miss_probe,
                                           CAN_MODE_FD_BRS,
                                           CAN_EXT_TEST_MISS_ID) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}

bool CANExtendedIdClassicFifo0AcceptAllTestPassed(const CANExtendedIdAcceptAllTestResult *result)
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
        CANExtendedIdProbePassExpectedRx(&result->probe,
                                         CAN_MODE_CLASSIC,
                                         CAN_EXT_TEST_MATCH_ID) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}

bool CANExtendedIdFdBrsFifo0AcceptAllTestPassed(const CANExtendedIdAcceptAllTestResult *result)
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
        CANExtendedIdProbePassExpectedRx(&result->probe,
                                         CAN_MODE_FD_BRS,
                                         CAN_EXT_TEST_MATCH_ID) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}