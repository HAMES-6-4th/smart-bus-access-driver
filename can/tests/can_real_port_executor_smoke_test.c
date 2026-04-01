#include "can_real_port_executor_smoke_test.h"

#include <string.h>

#include "can_executor.h"
#include "can_operation.h"
#include "can_platform.h"

void CANRealPortExecutorSmokeTestRun(CANRealPortExecutorSmokeTestResult *result)
{
    CANCore core;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *executor_storage[2];
    CANOperation poll_op;
    CANOperation send_op;
    CANFrame tx_frame;
    uint32_t ready_mask;
    uint32_t completed_count;
    uint32_t remaining_count;
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
    result->poll_submit_status = CAN_STATUS_EINVAL;
    result->send_submit_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;
    result->run_until_idle_status = CAN_STATUS_EINVAL;

    CANCoreInit(&core);
    CANExecutorInit(&executor, executor_storage, 2U);
    CANOperationInit(&poll_op);
    CANOperationInit(&send_op);

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

    ready_mask = 0U;

    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = 0x456U;
    tx_frame.len = 4U;
    tx_frame.data[0] = 0x11U;
    tx_frame.data[1] = 0x22U;
    tx_frame.data[2] = 0x33U;
    tx_frame.data[3] = 0x44U;

    CANOperationPreparePoll(&poll_op,
                            &core,
                            (uint32_t)CAN_CORE_EVENT_TX_READY,
                            0U,
                            &ready_mask);

    CANOperationPrepareSend(&send_op,
                            &core,
                            &tx_frame,
                            0U);

    result->poll_submit_status = CANExecutorSubmit(&executor, &poll_op);
    result->send_submit_status = CANExecutorSubmit(&executor, &send_op);

    result->pending_count_after_submit = CANExecutorGetPendingCount(&executor);

    result->poll_one_result_1 = CANExecutorPollOne(&executor);
    result->poll_ready_mask = ready_mask;
    result->poll_one_result_2 = CANExecutorPollOne(&executor);

    result->poll_operation_ok =
        (result->poll_submit_status == CAN_STATUS_OK) &&
        (result->poll_one_result_1 == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        ((result->poll_ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U) &&
        (CANOperationIsDone(&poll_op) == true) &&
        (CANOperationGetResult(&poll_op) == CAN_STATUS_OK);

    result->send_operation_ok =
        (result->send_submit_status == CAN_STATUS_OK) &&
        (result->poll_one_result_2 == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (CANOperationIsDone(&send_op) == true) &&
        (CANOperationGetResult(&send_op) == CAN_STATUS_OK);

    completed_count = 0U;
    remaining_count = 0U;

    result->run_until_idle_status =
        CANExecutorRunUntilIdle(&executor,
                                4U,
                                &completed_count,
                                &remaining_count);

    result->run_until_idle_completed_count = completed_count;
    result->run_until_idle_remaining_count = remaining_count;

    result->executor_path_ok =
        (result->pending_count_after_submit == 2U) &&
        (result->run_until_idle_status == CAN_STATUS_OK) &&
        (result->run_until_idle_completed_count == 0U) &&
        (result->run_until_idle_remaining_count == 0U) &&
        (CANExecutorHasPending(&executor) == false);

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

bool CANRealPortExecutorSmokeTestPassed(const CANRealPortExecutorSmokeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        result->poll_operation_ok &&
        result->send_operation_ok &&
        result->executor_path_ok &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        result->lifecycle_ok;
}