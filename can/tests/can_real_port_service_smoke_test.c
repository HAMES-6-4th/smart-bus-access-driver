#include "can_real_port_service_smoke_test.h"

#include <string.h>

#include "can_platform.h"

void CANRealPortServiceSmokeTestRun(CANRealPortServiceSmokeTestResult *result)
{
    CANCore core;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *executor_slots[2];
    CANOperation service_operations[2];
    bool service_in_use[2];
    CANContext context;
    CANService service;
    CANOperation *poll_op;
    CANOperation *send_op;
    CANFrame tx_frame;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(executor_slots, 0, sizeof(executor_slots));
    memset(service_operations, 0, sizeof(service_operations));
    memset(service_in_use, 0, sizeof(service_in_use));
    memset(&tx_frame, 0, sizeof(tx_frame));

    result->init_status = CAN_STATUS_EINVAL;
    result->open_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->submit_poll_status = CAN_STATUS_EINVAL;
    result->submit_send_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;

    poll_op = 0;
    send_op = 0;

    CANCoreInit(&core);
    CANExecutorInit(&executor, executor_slots, 2U);
    CANContextInit(&context);
    CANServiceInit(&service, service_operations, service_in_use, 2U);

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
    CANServiceBindContext(&service, &context);

    result->bind_ok =
        (CANServiceGetContext(&service) == &context) &&
        (CANServiceGetCapacity(&service) == 2U);

    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = 0x66AU;
    tx_frame.len = 4U;
    tx_frame.data[0] = 0xA1U;
    tx_frame.data[1] = 0xB2U;
    tx_frame.data[2] = 0xC3U;
    tx_frame.data[3] = 0xD4U;

    result->poll_ready_mask = 0U;

    result->submit_poll_status =
        CANServiceSubmitPoll(&service,
                             &poll_op,
                             (uint32_t)CAN_CORE_EVENT_TX_READY,
                             0U,
                             &result->poll_ready_mask);

    result->submit_send_status =
        CANServiceSubmitSend(&service,
                             &send_op,
                             &tx_frame,
                             0U);

    result->in_use_count_after_submit = CANServiceGetInUseCount(&service);
    result->pending_count_after_submit = CANContextGetPendingCount(&context);

    result->poll_one_result = CANServicePollOne(&service);
    result->run_one_result = CANServiceRunOne(&service);

    result->poll_operation_ok =
        (result->submit_poll_status == CAN_STATUS_OK) &&
        (poll_op != 0) &&
        (result->poll_one_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        ((result->poll_ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U) &&
        (CANOperationIsDone(poll_op) == true) &&
        (CANOperationGetResult(poll_op) == CAN_STATUS_OK);

    result->send_operation_ok =
        (result->submit_send_status == CAN_STATUS_OK) &&
        (send_op != 0) &&
        (result->run_one_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
        (CANOperationIsDone(send_op) == true) &&
        (CANOperationGetResult(send_op) == CAN_STATUS_OK);

    CANServiceReleaseOperation(&service, poll_op);
    CANServiceReleaseOperation(&service, send_op);

    result->release_ok =
        (CANServiceGetInUseCount(&service) == 0U) &&
        (CANContextHasPending(&context) == false) &&
        (CANContextGetPendingCount(&context) == 0U);

    result->service_path_ok =
        result->bind_ok &&
        (result->in_use_count_after_submit == 2U) &&
        (result->pending_count_after_submit == 2U) &&
        result->poll_operation_ok &&
        result->send_operation_ok &&
        result->release_ok;

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

bool CANRealPortServiceSmokeTestPassed(const CANRealPortServiceSmokeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        result->service_path_ok &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        result->lifecycle_ok;
}