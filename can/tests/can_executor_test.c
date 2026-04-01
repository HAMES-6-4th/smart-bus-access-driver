#include "can_executor_test.h"

#include <string.h>

#include "can_core.h"
#include "can_executor.h"
#include "can_operation.h"

typedef struct CANExecutorFakeChannelStruct
{
    uint32_t send_call_count;
    CANStatus send_script[8];
    uint32_t send_script_length;
    uint32_t send_script_index;
} CANExecutorFakeChannel;

static CANStatus CANExecutorFakeOpen(void *driver_channel,
                                     const void *driver_port,
                                     const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANExecutorFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANExecutorFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANExecutorFakeChannel *)driver_channel;
    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->send_script_index;
    if (index >= channel->send_script_length)
    {
        index = (channel->send_script_length == 0U) ? 0U : (channel->send_script_length - 1U);
    }

    channel->send_call_count++;

    if (channel->send_script_length == 0U)
    {
        return CAN_STATUS_OK;
    }

    if (channel->send_script_index + 1U < channel->send_script_length)
    {
        channel->send_script_index++;
    }

    return channel->send_script[index];
}

static CANStatus CANExecutorFakeReceive(void *driver_channel, CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_ENODATA;
}

static const CANCoreDriverOps g_CANExecutorFakeDriverOps = {
    .Open = CANExecutorFakeOpen,
    .Close = CANExecutorFakeClose,
    .Start = CANExecutorFakeStart,
    .Stop = CANExecutorFakeStop,
    .Send = CANExecutorFakeSend,
    .Receive = CANExecutorFakeReceive
};

static void CANExecutorFakeBindingInit(CANCoreBinding *binding,
                                       CANExecutorFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));
    memset(binding, 0, sizeof(*binding));

    binding->name = "fake_executor_can";
    binding->ops = &g_CANExecutorFakeDriverOps;
    binding->optional_ops = 0;
    binding->driver_channel = channel;
    binding->driver_port = 0;
    binding->capabilities.supports_fd = true;
    binding->capabilities.supports_brs = true;
    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = false;
    binding->capabilities.supports_termination_control = false;
}

void CANExecutorRunTest(CANExecutorTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANExecutor executor;
    CANOperation *storage[2];
    CANOperation op1;
    CANOperation op2;
    CANOperation op3;
    CANExecutorFakeChannel channel;
    CANFrame frame;
    CANStatus status;
    CANExecutorRunOnceResult run_once_result;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(storage, 0, sizeof(storage));
    memset(&frame, 0, sizeof(frame));

    CANExecutorInit(&executor, storage, 2U);
    result->init_empty_ok =
        (CANExecutorHasPending(&executor) == false) &&
        (CANExecutorGetPendingCount(&executor) == 0U);

    result->empty_run_once_ok =
        (CANExecutorRunOnce(&executor) == CAN_EXECUTOR_RUN_ONCE_IDLE);

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorFakeBindingInit(&binding, &channel);

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        CANOperationPrepareSend(&op1, &core, &frame, 0U);
        result->submit_requires_started_core_ok =
            (CANExecutorSubmit(&executor, &op1) == CAN_STATUS_EBUSY);

        (void)CANCoreClose(&core);
    }

    CANExecutorInit(&executor, storage, 2U);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorFakeBindingInit(&binding, &channel);

    channel.send_script_length = 3U;
    channel.send_script[0] = CAN_STATUS_EBUSY;
    channel.send_script[1] = CAN_STATUS_OK;
    channel.send_script[2] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&op1, &core, &frame, CAN_TIMEOUT_INFINITE);
        CANOperationPrepareSend(&op2, &core, &frame, 0U);

        result->duplicate_submit_busy_ok =
            (CANExecutorSubmit(&executor, &op1) == CAN_STATUS_OK) &&
            (CANExecutorSubmit(&executor, &op1) == CAN_STATUS_EBUSY);

        if (result->duplicate_submit_busy_ok)
        {
            result->duplicate_submit_busy_ok =
                (CANExecutorSubmit(&executor, &op2) == CAN_STATUS_OK);
        }

        result->pending_count_tracking_ok =
            (CANExecutorGetPendingCount(&executor) == 2U) &&
            (CANExecutorHasPending(&executor) == true);

        run_once_result = CANExecutorRunOnce(&executor);
        result->round_robin_pending_ok =
            (run_once_result == CAN_EXECUTOR_RUN_ONCE_PENDING) &&
            (CANExecutorGetPendingCount(&executor) == 2U) &&
            (CANOperationIsDone(&op1) == false);

        run_once_result = CANExecutorRunOnce(&executor);
        result->round_robin_completion_order_ok =
            (run_once_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
            (CANOperationIsDone(&op2) == true) &&
            (CANOperationGetResult(&op2) == CAN_STATUS_OK) &&
            (CANExecutorGetPendingCount(&executor) == 1U);

        run_once_result = CANExecutorRunOnce(&executor);
        result->round_robin_completion_order_ok =
            result->round_robin_completion_order_ok &&
            (run_once_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
            (CANOperationIsDone(&op1) == true) &&
            (CANOperationGetResult(&op1) == CAN_STATUS_OK) &&
            (CANExecutorGetPendingCount(&executor) == 0U) &&
            (CANExecutorHasPending(&executor) == false);

        (void)CANCoreClose(&core);
    }

    CANExecutorInit(&executor, storage, 1U);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorFakeBindingInit(&binding, &channel);

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&op1, &core, &frame, 0U);
        CANOperationPrepareSend(&op2, &core, &frame, 0U);

        result->capacity_limit_busy_ok =
            (CANExecutorSubmit(&executor, &op1) == CAN_STATUS_OK) &&
            (CANExecutorSubmit(&executor, &op2) == CAN_STATUS_EBUSY);

        (void)CANCoreClose(&core);
    }

    CANExecutorInit(&executor, storage, 2U);
    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANExecutorFakeBindingInit(&binding, &channel);

    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_EINVAL;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&op3, &core, &frame, 0U);

        if (CANExecutorSubmit(&executor, &op3) == CAN_STATUS_OK)
        {
            run_once_result = CANExecutorRunOnce(&executor);
            result->failure_completion_removal_ok =
                (run_once_result == CAN_EXECUTOR_RUN_ONCE_COMPLETED) &&
                (CANOperationIsDone(&op3) == true) &&
                (CANOperationGetResult(&op3) == CAN_STATUS_EINVAL) &&
                (CANExecutorGetPendingCount(&executor) == 0U);
        }

        (void)CANCoreClose(&core);
    }
}

bool CANExecutorTestPassed(const CANExecutorTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->init_empty_ok &&
        result->empty_run_once_ok &&
        result->submit_requires_started_core_ok &&
        result->duplicate_submit_busy_ok &&
        result->capacity_limit_busy_ok &&
        result->round_robin_pending_ok &&
        result->round_robin_completion_order_ok &&
        result->pending_count_tracking_ok &&
        result->failure_completion_removal_ok;
}