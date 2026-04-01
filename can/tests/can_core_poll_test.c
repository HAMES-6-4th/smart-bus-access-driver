#include "can_core_poll_test.h"

#include <string.h>

#include "can_core.h"

typedef struct CANCorePollFakeRuntimeStruct
{
    uint32_t now_ms;
} CANCorePollFakeRuntime;

typedef struct CANCorePollFakeChannelStruct
{
    uint32_t query_call_count;
    uint32_t scripted_masks[8];
    CANStatus scripted_status[8];
    uint32_t script_length;
    uint32_t script_index;
} CANCorePollFakeChannel;

static uint32_t CANCorePollFakeGetTickMs(void *user_context)
{
    CANCorePollFakeRuntime *runtime = (CANCorePollFakeRuntime *)user_context;
    return runtime->now_ms;
}

static void CANCorePollFakeRelax(void *user_context)
{
    CANCorePollFakeRuntime *runtime = (CANCorePollFakeRuntime *)user_context;
    runtime->now_ms += 1U;
}

static CANStatus CANCorePollFakeOpen(void *driver_channel,
                                     const void *driver_port,
                                     const CANChannelConfig *config)
{
    (void)driver_channel;
    (void)driver_port;
    (void)config;
    return CAN_STATUS_OK;
}

static CANStatus CANCorePollFakeClose(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANCorePollFakeStart(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANCorePollFakeStop(void *driver_channel)
{
    (void)driver_channel;
    return CAN_STATUS_OK;
}

static CANStatus CANCorePollFakeSend(void *driver_channel, const CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_OK;
}

static CANStatus CANCorePollFakeReceive(void *driver_channel, CANFrame *frame)
{
    (void)driver_channel;
    (void)frame;
    return CAN_STATUS_ENODATA;
}

static CANStatus CANCorePollFakeQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANCorePollFakeChannel *channel;
    uint32_t index;

    channel = (CANCorePollFakeChannel *)driver_channel;
    if ((channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    index = channel->script_index;
    if (index >= channel->script_length)
    {
        index = (channel->script_length == 0U) ? 0U : (channel->script_length - 1U);
    }

    channel->query_call_count++;

    if (channel->script_length == 0U)
    {
        *event_mask = 0U;
        return CAN_STATUS_OK;
    }

    *event_mask = channel->scripted_masks[index];

    if (channel->script_index + 1U < channel->script_length)
    {
        channel->script_index++;
    }

    return channel->scripted_status[index];
}

static const CANCoreDriverOps g_CANCorePollFakeDriverOps = {
    .Open = CANCorePollFakeOpen,
    .Close = CANCorePollFakeClose,
    .Start = CANCorePollFakeStart,
    .Stop = CANCorePollFakeStop,
    .Send = CANCorePollFakeSend,
    .Receive = CANCorePollFakeReceive
};

static const CANCoreOptionalDriverOps g_CANCorePollFakeOptionalOps = {
    .QueryEvents = CANCorePollFakeQueryEvents,
    .GetErrorState = 0,
    .Recover = 0
};

static void CANCorePollFakeBindingInit(CANCoreBinding *binding,
                                       CANCorePollFakeChannel *channel,
                                       const CANCoreOptionalDriverOps *optional_ops)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));

    *binding = (CANCoreBinding){
        .name = "fake_poll_can",
        .ops = &g_CANCorePollFakeDriverOps,
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

void CANCoreRunPollTest(CANCorePollTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams open_params;
    CANCorePollFakeChannel channel;
    CANCorePollFakeRuntime runtime;
    uint32_t ready_mask;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    runtime.now_ms = 0U;
    ready_mask = 0U;

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);

    result->null_arg_core_ok =
        (CANCorePoll(0,
                     (uint32_t)CAN_CORE_EVENT_TX_READY,
                     0U,
                     &ready_mask) == CAN_STATUS_EINVAL);

    result->null_arg_ready_mask_ok =
        (CANCorePoll(&core,
                     (uint32_t)CAN_CORE_EVENT_TX_READY,
                     0U,
                     0) == CAN_STATUS_EINVAL);

    result->zero_interest_mask_ok =
        (CANCorePoll(&core, 0U, 0U, &ready_mask) == CAN_STATUS_EINVAL);

    result->invalid_interest_mask_ok =
        (CANCorePoll(&core, 0x80000000UL, 0U, &ready_mask) == CAN_STATUS_EINVAL);

    result->closed_poll_busy_ok =
        (CANCorePoll(&core,
                     (uint32_t)CAN_CORE_EVENT_TX_READY,
                     0U,
                     &ready_mask) == CAN_STATUS_EBUSY) &&
        (CANCoreGetLastStatus(&core) == CAN_STATUS_EBUSY);

    CANCorePollFakeBindingInit(&binding, &channel, &g_CANCorePollFakeOptionalOps);
    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        result->opened_poll_busy_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_TX_READY,
                         0U,
                         &ready_mask) == CAN_STATUS_EBUSY) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EBUSY);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANCorePollFakeGetTickMs;
    open_params.runtime.relax = CANCorePollFakeRelax;
    open_params.runtime.user_context = &runtime;

    CANCorePollFakeBindingInit(&binding, &channel, 0);
    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        result->no_optional_poll_unsupported_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_TX_READY,
                         0U,
                         &ready_mask) == CAN_STATUS_EUNSUPPORTED) &&
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EUNSUPPORTED);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANCorePollFakeGetTickMs;
    open_params.runtime.relax = CANCorePollFakeRelax;
    open_params.runtime.user_context = &runtime;

    runtime.now_ms = 0U;
    CANCorePollFakeBindingInit(&binding, &channel, &g_CANCorePollFakeOptionalOps);
    channel.script_length = 1U;
    channel.scripted_masks[0] = (uint32_t)(CAN_CORE_EVENT_TX_READY | CAN_CORE_EVENT_RX_READY);
    channel.scripted_status[0] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        ready_mask = 0U;
        result->immediate_ready_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_TX_READY,
                         0U,
                         &ready_mask) == CAN_STATUS_OK);

        result->immediate_ready_mask_ok =
            (ready_mask == (uint32_t)CAN_CORE_EVENT_TX_READY);

        result->immediate_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_OK);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANCorePollFakeGetTickMs;
    open_params.runtime.relax = CANCorePollFakeRelax;
    open_params.runtime.user_context = &runtime;

    runtime.now_ms = 0U;
    CANCorePollFakeBindingInit(&binding, &channel, &g_CANCorePollFakeOptionalOps);
    channel.script_length = 1U;
    channel.scripted_masks[0] = 0U;
    channel.scripted_status[0] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        ready_mask = 0xFFFFFFFFUL;
        result->zero_timeout_no_ready_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_RX_READY,
                         0U,
                         &ready_mask) == CAN_STATUS_ENODATA) &&
            (ready_mask == 0U);

        result->zero_timeout_no_ready_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_ENODATA);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANCorePollFakeGetTickMs;
    open_params.runtime.relax = CANCorePollFakeRelax;
    open_params.runtime.user_context = &runtime;

    runtime.now_ms = 0U;
    CANCorePollFakeBindingInit(&binding, &channel, &g_CANCorePollFakeOptionalOps);
    channel.script_length = 3U;
    channel.scripted_masks[0] = 0U;
    channel.scripted_masks[1] = 0U;
    channel.scripted_masks[2] = (uint32_t)CAN_CORE_EVENT_RX_READY;
    channel.scripted_status[0] = CAN_STATUS_OK;
    channel.scripted_status[1] = CAN_STATUS_OK;
    channel.scripted_status[2] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        ready_mask = 0U;
        result->timeout_ready_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_RX_READY,
                         5U,
                         &ready_mask) == CAN_STATUS_OK);

        result->timeout_ready_mask_ok =
            (ready_mask == (uint32_t)CAN_CORE_EVENT_RX_READY);

        result->timeout_ready_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_OK);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANCorePollFakeGetTickMs;
    open_params.runtime.relax = CANCorePollFakeRelax;
    open_params.runtime.user_context = &runtime;

    runtime.now_ms = 0U;
    CANCorePollFakeBindingInit(&binding, &channel, &g_CANCorePollFakeOptionalOps);
    channel.script_length = 1U;
    channel.scripted_masks[0] = 0U;
    channel.scripted_status[0] = CAN_STATUS_OK;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        ready_mask = 0xFFFFFFFFUL;
        result->timeout_expired_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_RX_READY,
                         3U,
                         &ready_mask) == CAN_STATUS_ETIMEOUT) &&
            (ready_mask == 0U);

        result->timeout_expired_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_ETIMEOUT);

        (void)CANCoreClose(&core);
    }

    CANCoreInit(&core);
    CANCoreInitOpenParams(&open_params);
    open_params.runtime.get_tick_ms = CANCorePollFakeGetTickMs;
    open_params.runtime.relax = CANCorePollFakeRelax;
    open_params.runtime.user_context = &runtime;

    runtime.now_ms = 0U;
    CANCorePollFakeBindingInit(&binding, &channel, &g_CANCorePollFakeOptionalOps);
    channel.script_length = 1U;
    channel.scripted_masks[0] = 0U;
    channel.scripted_status[0] = CAN_STATUS_EBUSY;

    status = CANCoreOpen(&core, &binding, &open_params);
    if (status == CAN_STATUS_OK)
    {
        (void)CANCoreStart(&core);

        ready_mask = 0xFFFFFFFFUL;
        result->query_error_propagation_ok =
            (CANCorePoll(&core,
                         (uint32_t)CAN_CORE_EVENT_RX_READY,
                         0U,
                         &ready_mask) == CAN_STATUS_EBUSY) &&
            (ready_mask == 0U);

        result->query_error_last_status_ok =
            (CANCoreGetLastStatus(&core) == CAN_STATUS_EBUSY);

        (void)CANCoreClose(&core);
    }
}

bool CANCorePollTestPassed(const CANCorePollTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        result->null_arg_core_ok &&
        result->null_arg_ready_mask_ok &&
        result->zero_interest_mask_ok &&
        result->invalid_interest_mask_ok &&
        result->closed_poll_busy_ok &&
        result->opened_poll_busy_ok &&
        result->no_optional_poll_unsupported_ok &&
        result->immediate_ready_ok &&
        result->immediate_ready_mask_ok &&
        result->immediate_last_status_ok &&
        result->zero_timeout_no_ready_ok &&
        result->zero_timeout_no_ready_last_status_ok &&
        result->timeout_ready_ok &&
        result->timeout_ready_mask_ok &&
        result->timeout_ready_last_status_ok &&
        result->timeout_expired_ok &&
        result->timeout_expired_last_status_ok &&
        result->query_error_propagation_ok &&
        result->query_error_last_status_ok;
}