#include "can_real_port_context_helpers_smoke_test.h"

#include <string.h>

#include "can_platform.h"

void CANRealPortContextHelpersSmokeTestRun(CANRealPortContextHelpersSmokeTestResult *result)
{
    CANCore core;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *executor_storage[2];
    CANOperation poll_op;
    CANOperation send_op;
    CANContext context;
    CANFrame tx_frame;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(executor_storage, 0, sizeof(executor_storage));
    memset(&tx_frame, 0, sizeof(tx_frame));

    result->init_status = CAN_STATUS_EINVAL;
    result->open_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->submit_poll_status = CAN_STATUS_EINVAL;
    result->submit_send_status = CAN_STATUS_EINVAL;
    result->drain_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;

    CANCoreInit(&core);
    CANExecutorInit(&executor, executor_storage, 2U);
    CANOperationInit(&poll_op);
    CANOperationInit(&send_op);
    CANContextInit(&context);

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

    CANContextBind(&context, &core, &executor);

    result->bind_ok =
        (CANContextGetCore(&context) == &core) &&
        (CANContextGetExecutor(&context) == &executor);

    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = 0x61AU;
    tx_frame.len = 4U;
    tx_frame.data[0] = 0xDEU;
    tx_frame.data[1] = 0xADU;
    tx_frame.data[2] = 0xBEU;
    tx_frame.data[3] = 0xEFU;

    result->poll_ready_mask = 0U;

    result->submit_poll_status =
        CANContextSubmitPoll(&context,
                             &poll_op,
                             (uint32_t)CAN_CORE_EVENT_TX_READY,
                             0U,
                             &result->poll_ready_mask);

    result->submit_send_status =
        CANContextSubmitSend(&context,
                             &send_op,
                             &tx_frame,
                             0U);

    result->pending_count_after_submit = CANContextGetPendingCount(&context);

    result->poll_one_result = CANContextPollOne(&context);
    result->run_one_result = CANContextRunOne(&context);
    result->dispatch_one_result = CANContextDispatchOne(&context);

    result->poll_helper_ok =
        (result->submit_poll_status == CAN_STATUS_OK) &&
        (result->poll_one_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        ((result->poll_ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U) &&
        (CANOperationIsDone(&poll_op) == true) &&
        (CANOperationGetResult(&poll_op) == CAN_STATUS_OK);

    result->send_helper_ok =
        (result->submit_send_status == CAN_STATUS_OK) &&
        (result->run_one_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (CANOperationIsDone(&send_op) == true) &&
        (CANOperationGetResult(&send_op) == CAN_STATUS_OK);

    result->drain_completed_count = 0U;
    result->drain_remaining_count = 0U;

    result->drain_status =
        CANContextPoll(&context,
                       4U,
                       &result->drain_completed_count,
                       &result->drain_remaining_count);

    result->context_helper_path_ok =
        result->bind_ok &&
        (result->pending_count_after_submit == 2U) &&
        (result->dispatch_one_result == CAN_EXECUTOR_RUN_ONCE_IDLE) &&
        (result->drain_status == CAN_STATUS_OK) &&
        (result->drain_completed_count == 0U) &&
        (result->drain_remaining_count == 0U) &&
        (CANContextHasPending(&context) == false) &&
        (CANContextGetPendingCount(&context) == 0U);

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

bool CANRealPortContextHelpersSmokeTestPassed(const CANRealPortContextHelpersSmokeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        result->poll_helper_ok &&
        result->send_helper_ok &&
        result->context_helper_path_ok &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        result->lifecycle_ok;
}