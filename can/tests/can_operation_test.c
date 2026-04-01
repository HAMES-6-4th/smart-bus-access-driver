#include "can_operation_test.h"

#include <string.h>

#include "can_core.h"
#include "can_operation.h"

typedef struct CANOperationFakeRuntimeStruct
{
    uint32_t now_ms;
} CANOperationFakeRuntime;

typedef struct CANOperationFakeChannelStruct
{
    uint32_t send_call_count;
    uint32_t receive_call_count;
    uint32_t query_call_count;

    CANStatus send_script[8];
    uint32_t send_script_length;
    uint32_t send_script_index;

    CANStatus receive_script[8];
    uint32_t receive_script_length;
    uint32_t receive_script_index;

    CANStatus query_status_script[8];
    uint32_t query_mask_script[8];
    uint32_t query_script_length;
    uint32_t query_script_index;
} CANOperationFakeChannel;

static uint32_t CANOperationFakeGetTickMs(void *user_context)
{
    CANOperationFakeRuntime *runtime = (CANOperationFakeRuntime *)user_context;
    return runtime->now_ms;
}

static CANStatus CANOperationFakeOpen(void *driver_channel,
                                      const void *driver_port,
                                      const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANOperationFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANOperationFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANOperationFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANOperationFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANOperationFakeChannel *channel;
    uint32_t index;

    (void)frame;

    channel = (CANOperationFakeChannel *)driver_channel;
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

static CANStatus CANOperationFakeReceive(void *driver_channel, CANFrame *frame)
{
    CANOperationFakeChannel *channel;
    uint32_t index;

    channel = (CANOperationFakeChannel *)driver_channel;
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->receive_script_index;
    if (index >= channel->receive_script_length)
    {
        index = (channel->receive_script_length == 0U) ? 0U : (channel->receive_script_length - 1U);
    }

    channel->receive_call_count++;

    if (channel->receive_script_length == 0U)
    {
        return CAN_STATUS_ENODATA;
    }

    if (channel->receive_script_index + 1U < channel->receive_script_length)
    {
        channel->receive_script_index++;
    }

    if (channel->receive_script[index] == CAN_STATUS_OK)
    {
        memset(frame, 0, sizeof(*frame));
        frame->id_type = CAN_ID_STANDARD;
        frame->mode = CAN_MODE_CLASSIC;
        frame->id = 0x123U;
        frame->len = 1U;
        frame->data[0] = 0x5AU;
    }

    return channel->receive_script[index];
}

static CANStatus CANOperationFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANOperationFakeChannel *channel;
    uint32_t index;

    channel = (CANOperationFakeChannel *)driver_channel;
    if ((channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->query_script_index;
    if (index >= channel->query_script_length)
    {
        index = (channel->query_script_length == 0U) ? 0U : (channel->query_script_length - 1U);
    }

    channel->query_call_count++;

    if (channel->query_script_length == 0U)
    {
        *event_mask = 0U;
        return CAN_STATUS_OK;
    }

    *event_mask = channel->query_mask_script[index];

    if (channel->query_script_index + 1U < channel->query_script_length)
    {
        channel->query_script_index++;
    }

    return channel->query_status_script[index];
}

static const CANCoreDriverOps g_CANOperationFakeDriverOps = {
    .Open = CANOperationFakeOpen,
    .Close = CANOperationFakeClose,
    .Start = CANOperationFakeStart,
    .Stop = CANOperationFakeStop,
    .Send = CANOperationFakeSend,
    .Receive = CANOperationFakeReceive
};

static const CANCoreOptionalDriverOps g_CANOperationFakeOptionalOps = {
    .QueryEvents = CANOperationFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void CANOperationFakeBindingInit(CANCoreBinding *binding,
                                        CANOperationFakeChannel *channel,
                                        const CANCoreOptionalDriverOps *optional_ops)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));

    *binding = (CANCoreBinding){
        .name = "fake_operation_can",
        .ops = &g_CANOperationFakeDriverOps,
        .optional_ops = optional_ops,
        .driver_channel = channel,
        .driver_port = 0,
        .capabilities = {
            .supports_fd = true,
            .supports_brs = true,
            .supports_loopback = true,
            .supports_hw_filter = false,
            .supports_termination_control = false
        }
    };
}

void CANOperationRunTest(CANOperationTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANOperation operation;
    CANOperationFakeChannel channel;
    CANOperationFakeRuntime runtime;
    CANFrame frame;
    uint32_t ready_mask;
    CANStatus status;
    CANOperationRunResult run_result;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    memset(&frame, 0, sizeof(frame));
    ready_mask = 0U;
    runtime.now_ms = 0U;

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        CANOperationPrepareSend(&operation, &core, &frame, 0U);
        result->submit_requires_started_core_ok =
            (CANOperationSubmit(&operation) == CAN_STATUS_EBUSY);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANOperationFakeGetTickMs;
    open_params.runtime.user_context = &runtime;
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);
    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&operation, &core, &frame, 0U);
        if (CANOperationSubmit(&operation) == CAN_STATUS_OK)
        {
            run_result = CANOperationRunOnce(&operation);
            result->send_immediate_ok =
                (run_result == CAN_OPERATION_RUN_COMPLETED) &&
                (CANOperationIsDone(&operation) == true) &&
                (CANOperationGetResult(&operation) == CAN_STATUS_OK);
        }

        result->completed_run_once_stable_ok =
            (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_COMPLETED) &&
            (CANOperationGetResult(&operation) == CAN_STATUS_OK);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANOperationFakeGetTickMs;
    open_params.runtime.user_context = &runtime;
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);
    channel.send_script_length = 1U;
    channel.send_script[0] = CAN_STATUS_EBUSY;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareSend(&operation, &core, &frame, 0U);
        if (CANOperationSubmit(&operation) == CAN_STATUS_OK)
        {
            run_result = CANOperationRunOnce(&operation);
            result->send_busy_zero_timeout_ok =
                (run_result == CAN_OPERATION_RUN_COMPLETED) &&
                (CANOperationGetResult(&operation) == CAN_STATUS_EBUSY);
        }

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANOperationFakeGetTickMs;
    open_params.runtime.user_context = &runtime;
    runtime.now_ms = 0U;
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);
    channel.receive_script_length = 2U;
    channel.receive_script[0] = CAN_STATUS_ENODATA;
    channel.receive_script[1] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareReceive(&operation, &core, &frame, CAN_TIMEOUT_INFINITE);
        if (CANOperationSubmit(&operation) == CAN_STATUS_OK)
        {
            result->receive_pending_then_ok =
                (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_PENDING);

            result->receive_pending_then_ok =
                result->receive_pending_then_ok &&
                (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_COMPLETED) &&
                (CANOperationGetResult(&operation) == CAN_STATUS_OK) &&
                (frame.id == 0x123U) &&
                (frame.data[0] == 0x5AU);
        }

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANOperationFakeGetTickMs;
    open_params.runtime.user_context = &runtime;
    runtime.now_ms = 0U;
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);
    channel.receive_script_length = 1U;
    channel.receive_script[0] = CAN_STATUS_ENODATA;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareReceive(&operation, &core, &frame, 3U);
        if (CANOperationSubmit(&operation) == CAN_STATUS_OK)
        {
            result->receive_timeout_ok =
                (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_PENDING);

            runtime.now_ms = 4U;

            result->receive_timeout_ok =
                result->receive_timeout_ok &&
                (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_COMPLETED) &&
                (CANOperationGetResult(&operation) == CAN_STATUS_ETIMEOUT);
        }

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANOperationFakeGetTickMs;
    open_params.runtime.user_context = &runtime;
    runtime.now_ms = 0U;
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);
    channel.query_script_length = 2U;
    channel.query_status_script[0] = CAN_STATUS_OK;
    channel.query_status_script[1] = CAN_STATUS_OK;
    channel.query_mask_script[0] = 0U;
    channel.query_mask_script[1] = (uint32_t)CAN_CORE_EVENT_TX_READY;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        ready_mask = 0U;
        CANOperationPreparePoll(&operation,
                                &core,
                                (uint32_t)CAN_CORE_EVENT_TX_READY,
                                CAN_TIMEOUT_INFINITE,
                                &ready_mask);

        if (CANOperationSubmit(&operation) == CAN_STATUS_OK)
        {
            result->poll_pending_then_ok =
                (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_PENDING);

            result->poll_pending_then_ok =
                result->poll_pending_then_ok &&
                (CANOperationRunOnce(&operation) == CAN_OPERATION_RUN_COMPLETED) &&
                (CANOperationGetResult(&operation) == CAN_STATUS_OK);

            result->poll_ready_mask_ok =
                (ready_mask == (uint32_t)CAN_CORE_EVENT_TX_READY);
        }

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    CANOperationFakeBindingInit(&binding, &channel, &g_CANOperationFakeOptionalOps);

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        CANOperationPrepareReceive(&operation, &core, &frame, 5U);
        result->finite_timeout_without_runtime_unsupported_ok =
            (CANOperationSubmit(&operation) == CAN_STATUS_EUNSUPPORTED);

        (void)CANCoreClose(&core);
    }
}

bool CANOperationTestPassed(const CANOperationTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->send_immediate_ok &&
        result->send_busy_zero_timeout_ok &&
        result->receive_pending_then_ok &&
        result->receive_timeout_ok &&
        result->poll_pending_then_ok &&
        result->poll_ready_mask_ok &&
        result->finite_timeout_without_runtime_unsupported_ok &&
        result->submit_requires_started_core_ok &&
        result->completed_run_once_stable_ok;
}