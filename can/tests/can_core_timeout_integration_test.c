#include "can_core_timeout_integration_test.h"

#include <string.h>

#include "can_platform.h"

#define CAN_TIMEOUT_ITG_TEST_ID               (0x123U)
#define CAN_TIMEOUT_ITG_TEST_LEN              (8U)
#define CAN_TIMEOUT_ITG_TEST_NOMINAL_BITRATE  (500000U)

#define CAN_TIMEOUT_ITG_SEND_TIMEOUT_MS       (10U)
#define CAN_TIMEOUT_ITG_RECV_TIMEOUT_MS       (10U)
#define CAN_TIMEOUT_ITG_RECV_EXPIRE_MS        (5U)

typedef struct CANTimeoutIntegrationRuntimeContextStruct
{
    uint32_t now_ms;
    uint32_t relax_step_ms;
    uint32_t relax_call_count;
} CANTimeoutIntegrationRuntimeContext;

static uint32_t CANTimeoutIntegrationGetTickMs(void *user_context)
{
    CANTimeoutIntegrationRuntimeContext *context =
        (CANTimeoutIntegrationRuntimeContext *)user_context;

    if (context == 0)
    {
        return 0U;
    }

    return context->now_ms;
}

static void CANTimeoutIntegrationRelax(void *user_context)
{
    CANTimeoutIntegrationRuntimeContext *context =
        (CANTimeoutIntegrationRuntimeContext *)user_context;

    if (context == 0)
    {
        return;
    }

    context->relax_call_count++;
    context->now_ms += context->relax_step_ms;
}

static void CANTimeoutIntegrationInitRuntimeContext(
    CANTimeoutIntegrationRuntimeContext *context,
    uint32_t relax_step_ms)
{
    if (context == 0)
    {
        return;
    }

    memset(context, 0, sizeof(*context));
    context->relax_step_ms = relax_step_ms;
}

static void CANTimeoutIntegrationPrepareOpenParams(
    CANCoreOpenParams *params,
    CANTimeoutIntegrationRuntimeContext *runtime_context)
{
    if ((params == 0) || (runtime_context == 0))
    {
        return;
    }

    CANCoreInitOpenParams(params);
    params->channel_config.timing.mode = CAN_MODE_CLASSIC;
    params->channel_config.timing.nominal_bitrate = CAN_TIMEOUT_ITG_TEST_NOMINAL_BITRATE;
    params->channel_config.enable_loopback = true;
    params->channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    params->channel_config.rx_filter.enabled = false;

    params->runtime.get_tick_ms = CANTimeoutIntegrationGetTickMs;
    params->runtime.relax = CANTimeoutIntegrationRelax;
    params->runtime.user_context = runtime_context;
}

static void CANTimeoutIntegrationBuildFrame(CANFrame *frame)
{
    uint32_t i;

    if (frame == 0)
    {
        return;
    }

    memset(frame, 0, sizeof(*frame));
    frame->id_type = CAN_ID_STANDARD;
    frame->mode = CAN_MODE_CLASSIC;
    frame->id = CAN_TIMEOUT_ITG_TEST_ID;
    frame->len = CAN_TIMEOUT_ITG_TEST_LEN;

    for (i = 0U; i < frame->len; ++i)
    {
        frame->data[i] = (uint8_t)(0xA0U + (uint8_t)i);
    }
}

static void CANTimeoutIntegrationInitSendImmediateResult(
    CANTimeoutIntegrationSendImmediateTestResult *result)
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
    result->send_timeout_status = CAN_STATUS_EINVAL;
    result->tx_stop_status = CAN_STATUS_EINVAL;
    result->rx_stop_status = CAN_STATUS_EINVAL;
    result->tx_close_status = CAN_STATUS_EINVAL;
    result->rx_close_status = CAN_STATUS_EINVAL;
    result->tx_last_status = CAN_STATUS_EINVAL;
}

static void CANTimeoutIntegrationInitReceiveExpiredResult(
    CANTimeoutIntegrationReceiveExpiredTestResult *result)
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
    result->receive_timeout_status = CAN_STATUS_EINVAL;
    result->tx_stop_status = CAN_STATUS_EINVAL;
    result->rx_stop_status = CAN_STATUS_EINVAL;
    result->tx_close_status = CAN_STATUS_EINVAL;
    result->rx_close_status = CAN_STATUS_EINVAL;
    result->rx_last_status = CAN_STATUS_EINVAL;
}

static bool CANTimeoutIntegrationOpenStartPair(
    CANCore *tx_core,
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
        (init_status == 0) || (tx_open_status == 0) ||
        (rx_open_status == 0) || (tx_start_status == 0) ||
        (rx_start_status == 0))
    {
        return false;
    }

    *init_status = CANPlatformInit();
    if (*init_status != CAN_STATUS_OK)
    {
        return false;
    }

    *tx_open_status = CANPlatformOpen(tx_core, "can0_lb0", tx_params);
    if (*tx_open_status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        return false;
    }

    *rx_open_status = CANPlatformOpen(rx_core, "can0_lb1", rx_params);
    if (*rx_open_status != CAN_STATUS_OK)
    {
        (void)CANPlatformClose(tx_core);
        CANPlatformDeinit();
        return false;
    }

    *tx_start_status = CANCoreStart(tx_core);
    if (*tx_start_status != CAN_STATUS_OK)
    {
        (void)CANPlatformClose(rx_core);
        (void)CANPlatformClose(tx_core);
        CANPlatformDeinit();
        return false;
    }

    *rx_start_status = CANCoreStart(rx_core);
    if (*rx_start_status != CAN_STATUS_OK)
    {
        (void)CANCoreStop(tx_core);
        (void)CANPlatformClose(rx_core);
        (void)CANPlatformClose(tx_core);
        CANPlatformDeinit();
        return false;
    }

    return true;
}

static void CANTimeoutIntegrationStopClosePair(
    CANCore *tx_core,
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

void CANTimeoutIntegrationRunSendImmediateTest(
    CANTimeoutIntegrationSendImmediateTestResult *result)
{
    CANCore tx_core;
    CANCore rx_core;
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;
    CANTimeoutIntegrationRuntimeContext tx_runtime;
    CANTimeoutIntegrationRuntimeContext rx_runtime;
    CANFrame tx_frame;

    if (result == 0)
    {
        return;
    }

    CANTimeoutIntegrationInitSendImmediateResult(result);

    CANCoreInit(&tx_core);
    CANCoreInit(&rx_core);

    CANTimeoutIntegrationInitRuntimeContext(&tx_runtime, 1U);
    CANTimeoutIntegrationInitRuntimeContext(&rx_runtime, 1U);

    CANTimeoutIntegrationPrepareOpenParams(&tx_params, &tx_runtime);
    CANTimeoutIntegrationPrepareOpenParams(&rx_params, &rx_runtime);

    if (CANTimeoutIntegrationOpenStartPair(&tx_core,
                                           &rx_core,
                                           &tx_params,
                                           &rx_params,
                                           &result->init_status,
                                           &result->tx_open_status,
                                           &result->rx_open_status,
                                           &result->tx_start_status,
                                           &result->rx_start_status) == false)
    {
        return;
    }

    CANTimeoutIntegrationBuildFrame(&tx_frame);

    result->send_timeout_status =
        CANCoreSendTimeout(&tx_core, &tx_frame, CAN_TIMEOUT_ITG_SEND_TIMEOUT_MS);

    result->tx_last_status = CANCoreGetLastStatus(&tx_core);
    CANCoreGetStats(&tx_core, &result->tx_stats);
    result->tx_relax_call_count = tx_runtime.relax_call_count;

    CANTimeoutIntegrationStopClosePair(&tx_core,
                                       &rx_core,
                                       &result->tx_stop_status,
                                       &result->rx_stop_status,
                                       &result->tx_close_status,
                                       &result->rx_close_status);
}

void CANTimeoutIntegrationRunReceiveExpiredTest(
    CANTimeoutIntegrationReceiveExpiredTestResult *result)
{
    CANCore tx_core;
    CANCore rx_core;
    CANCoreOpenParams tx_params;
    CANCoreOpenParams rx_params;
    CANTimeoutIntegrationRuntimeContext tx_runtime;
    CANTimeoutIntegrationRuntimeContext rx_runtime;
    CANFrame rx_frame;

    if (result == 0)
    {
        return;
    }

    CANTimeoutIntegrationInitReceiveExpiredResult(result);

    CANCoreInit(&tx_core);
    CANCoreInit(&rx_core);

    CANTimeoutIntegrationInitRuntimeContext(&tx_runtime, 1U);
    CANTimeoutIntegrationInitRuntimeContext(&rx_runtime, 1U);

    CANTimeoutIntegrationPrepareOpenParams(&tx_params, &tx_runtime);
    CANTimeoutIntegrationPrepareOpenParams(&rx_params, &rx_runtime);

    if (CANTimeoutIntegrationOpenStartPair(&tx_core,
                                           &rx_core,
                                           &tx_params,
                                           &rx_params,
                                           &result->init_status,
                                           &result->tx_open_status,
                                           &result->rx_open_status,
                                           &result->tx_start_status,
                                           &result->rx_start_status) == false)
    {
        return;
    }

    memset(&rx_frame, 0, sizeof(rx_frame));

    result->receive_timeout_status =
        CANCoreReceiveTimeout(&rx_core, &rx_frame, CAN_TIMEOUT_ITG_RECV_EXPIRE_MS);

    result->rx_last_status = CANCoreGetLastStatus(&rx_core);
    CANCoreGetStats(&rx_core, &result->rx_stats);
    result->rx_relax_call_count = rx_runtime.relax_call_count;

    CANTimeoutIntegrationStopClosePair(&tx_core,
                                       &rx_core,
                                       &result->tx_stop_status,
                                       &result->rx_stop_status,
                                       &result->tx_close_status,
                                       &result->rx_close_status);
}

bool CANTimeoutIntegrationSendImmediateTestPassed(
    const CANTimeoutIntegrationSendImmediateTestResult *result)
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
        (result->send_timeout_status == CAN_STATUS_OK) &&
        (result->tx_last_status == CAN_STATUS_OK) &&
        (result->tx_relax_call_count == 0U) &&
        (result->tx_stats.tx_calls == 1U) &&
        (result->tx_stats.tx_ok == 1U) &&
        (result->tx_stats.tx_busy == 0U) &&
        (result->tx_stats.tx_timeouts == 0U) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK);
}

bool CANTimeoutIntegrationReceiveExpiredTestPassed(
    const CANTimeoutIntegrationReceiveExpiredTestResult *result)
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
        (result->receive_timeout_status == CAN_STATUS_ETIMEOUT) &&
        (result->rx_last_status == CAN_STATUS_ETIMEOUT) &&
        (result->rx_stats.rx_timeouts == 1U) &&
        (result->rx_stats.rx_ok == 0U) &&
        (result->rx_stop_status == CAN_STATUS_OK) &&
        (result->tx_stop_status == CAN_STATUS_OK) &&
        (result->rx_close_status == CAN_STATUS_OK) &&
        (result->tx_close_status == CAN_STATUS_OK);
}