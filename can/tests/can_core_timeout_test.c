#include "can_core_timeout_test.h"

#include <string.h>

typedef struct FakeClockContextStruct
{
    uint32_t now_ms;
    uint32_t relax_step_ms;
    uint32_t relax_call_count;
} FakeClockContext;

typedef struct FakeDriverChannelStruct
{
    bool is_open;
    bool is_started;

    uint32_t send_busy_before_ok;
    uint32_t send_call_count;

    uint32_t receive_empty_before_ok;
    uint32_t receive_call_count;

    CANFrame receive_frame;
} FakeDriverChannel;

static uint32_t FakeGetTickMs(void *user_context)
{
    FakeClockContext *clock = (FakeClockContext *)user_context;

    if (clock == 0)
    {
        return 0U;
    }

    return clock->now_ms;
}

static void FakeRelax(void *user_context)
{
    FakeClockContext *clock = (FakeClockContext *)user_context;

    if (clock == 0)
    {
        return;
    }

    clock->relax_call_count++;
    clock->now_ms += clock->relax_step_ms;
}

static CANStatus FakeOpen(void *driver_channel,
                          const void *driver_port,
                          const CANChannelConfig *config)
{
    FakeDriverChannel *channel = (FakeDriverChannel *)driver_channel;

    (void)driver_port;
    (void)config;

    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    channel->is_open = true;
    return CAN_STATUS_OK;
}

static CANStatus FakeClose(void *driver_channel)
{
    FakeDriverChannel *channel = (FakeDriverChannel *)driver_channel;

    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    channel->is_open = false;
    channel->is_started = false;
    return CAN_STATUS_OK;
}

static CANStatus FakeStart(void *driver_channel)
{
    FakeDriverChannel *channel = (FakeDriverChannel *)driver_channel;

    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EBUSY;
    }

    channel->is_started = true;
    return CAN_STATUS_OK;
}

static CANStatus FakeStop(void *driver_channel)
{
    FakeDriverChannel *channel = (FakeDriverChannel *)driver_channel;

    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EBUSY;
    }

    channel->is_started = false;
    return CAN_STATUS_OK;
}

static CANStatus FakeSend(void *driver_channel, const CANFrame *frame)
{
    FakeDriverChannel *channel = (FakeDriverChannel *)driver_channel;

    (void)frame;

    if ((channel == 0) || (channel->is_started == false))
    {
        return CAN_STATUS_EBUSY;
    }

    channel->send_call_count++;

    if (channel->send_call_count <= channel->send_busy_before_ok)
    {
        return CAN_STATUS_EBUSY;
    }

    return CAN_STATUS_OK;
}

static CANStatus FakeReceive(void *driver_channel, CANFrame *frame)
{
    FakeDriverChannel *channel = (FakeDriverChannel *)driver_channel;

    if ((channel == 0) || (frame == 0) || (channel->is_started == false))
    {
        return CAN_STATUS_EBUSY;
    }

    channel->receive_call_count++;

    if (channel->receive_call_count <= channel->receive_empty_before_ok)
    {
        return CAN_STATUS_ENODATA;
    }

    *frame = channel->receive_frame;
    return CAN_STATUS_OK;
}

static const CANCoreDriverOps g_FakeDriverOps =
{
    .Open = FakeOpen,
    .Close = FakeClose,
    .Start = FakeStart,
    .Stop = FakeStop,
    .Send = FakeSend,
    .Receive = FakeReceive
};

static void FakeBindingInit(CANCoreBinding *binding, FakeDriverChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));

    *binding = (CANCoreBinding){
        .name = "fake_can",
        .ops = &g_FakeDriverOps,
        .optional_ops = 0,
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

static void InitClock(FakeClockContext *clock, uint32_t relax_step_ms)
{
    if (clock == 0)
    {
        return;
    }

    memset(clock, 0, sizeof(*clock));
    clock->relax_step_ms = relax_step_ms;
}

static void InitFrame(CANFrame *frame, uint32_t id, uint8_t first_byte)
{
    if (frame == 0)
    {
        return;
    }

    memset(frame, 0, sizeof(*frame));
    frame->id_type = CAN_ID_STANDARD;
    frame->mode = CAN_MODE_CLASSIC;
    frame->id = id;
    frame->len = 1U;
    frame->data[0] = first_byte;
}

static void InitSendResult(CANCoreTimeoutSendTestResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->open_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->send_timeout_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;
    result->last_status = CAN_STATUS_EINVAL;
}

static void InitReceiveResult(CANCoreTimeoutReceiveTestResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->open_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->receive_timeout_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;
    result->last_status = CAN_STATUS_EINVAL;
}

void CANCoreRunSendTimeoutSuccessTest(CANCoreTimeoutSendTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    FakeDriverChannel channel;
    FakeClockContext clock;
    CANFrame frame;

    if (result == 0)
    {
        return;
    }

    InitSendResult(result);
    InitClock(&clock, 1U);
    FakeBindingInit(&binding, &channel);
    channel.send_busy_before_ok = 3U;

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    params.runtime.get_tick_ms = FakeGetTickMs;
    params.runtime.relax = FakeRelax;
    params.runtime.user_context = &clock;

    InitFrame(&frame, 0x123U, 0x5AU);

    result->open_status = CANCoreOpen(&core, &binding, &params);
    if (result->open_status != CAN_STATUS_OK)
    {
        return;
    }

    result->start_status = CANCoreStart(&core);
    if (result->start_status != CAN_STATUS_OK)
    {
        result->close_status = CANCoreClose(&core);
        return;
    }

    result->send_timeout_status = CANCoreSendTimeout(&core, &frame, 10U);
    result->last_status = CANCoreGetLastStatus(&core);
    CANCoreGetStats(&core, &result->stats);

    result->stop_status = CANCoreStop(&core);
    result->close_status = CANCoreClose(&core);

    result->send_call_count = channel.send_call_count;
    result->relax_call_count = clock.relax_call_count;
}

void CANCoreRunSendTimeoutExpiredTest(CANCoreTimeoutSendTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    FakeDriverChannel channel;
    FakeClockContext clock;
    CANFrame frame;

    if (result == 0)
    {
        return;
    }

    InitSendResult(result);
    InitClock(&clock, 1U);
    FakeBindingInit(&binding, &channel);
    channel.send_busy_before_ok = 1000U;

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    params.runtime.get_tick_ms = FakeGetTickMs;
    params.runtime.relax = FakeRelax;
    params.runtime.user_context = &clock;

    InitFrame(&frame, 0x123U, 0x5AU);

    result->open_status = CANCoreOpen(&core, &binding, &params);
    if (result->open_status != CAN_STATUS_OK)
    {
        return;
    }

    result->start_status = CANCoreStart(&core);
    if (result->start_status != CAN_STATUS_OK)
    {
        result->close_status = CANCoreClose(&core);
        return;
    }

    result->send_timeout_status = CANCoreSendTimeout(&core, &frame, 5U);
    result->last_status = CANCoreGetLastStatus(&core);
    CANCoreGetStats(&core, &result->stats);

    result->stop_status = CANCoreStop(&core);
    result->close_status = CANCoreClose(&core);

    result->send_call_count = channel.send_call_count;
    result->relax_call_count = clock.relax_call_count;
}

void CANCoreRunReceiveTimeoutSuccessTest(CANCoreTimeoutReceiveTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    FakeDriverChannel channel;
    FakeClockContext clock;
    CANFrame frame;

    if (result == 0)
    {
        return;
    }

    InitReceiveResult(result);
    InitClock(&clock, 1U);
    FakeBindingInit(&binding, &channel);
    channel.receive_empty_before_ok = 2U;
    InitFrame(&channel.receive_frame, 0x321U, 0xA5U);

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    params.runtime.get_tick_ms = FakeGetTickMs;
    params.runtime.relax = FakeRelax;
    params.runtime.user_context = &clock;

    memset(&frame, 0, sizeof(frame));

    result->open_status = CANCoreOpen(&core, &binding, &params);
    if (result->open_status != CAN_STATUS_OK)
    {
        return;
    }

    result->start_status = CANCoreStart(&core);
    if (result->start_status != CAN_STATUS_OK)
    {
        result->close_status = CANCoreClose(&core);
        return;
    }

    result->receive_timeout_status = CANCoreReceiveTimeout(&core, &frame, 10U);
    if (result->receive_timeout_status == CAN_STATUS_OK)
    {
        result->received = true;
        result->frame = frame;
    }

    result->last_status = CANCoreGetLastStatus(&core);
    CANCoreGetStats(&core, &result->stats);

    result->stop_status = CANCoreStop(&core);
    result->close_status = CANCoreClose(&core);

    result->receive_call_count = channel.receive_call_count;
    result->relax_call_count = clock.relax_call_count;
}

void CANCoreRunReceiveTimeoutExpiredTest(CANCoreTimeoutReceiveTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    FakeDriverChannel channel;
    FakeClockContext clock;
    CANFrame frame;

    if (result == 0)
    {
        return;
    }

    InitReceiveResult(result);
    InitClock(&clock, 1U);
    FakeBindingInit(&binding, &channel);
    channel.receive_empty_before_ok = 1000U;
    InitFrame(&channel.receive_frame, 0x321U, 0xA5U);

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    params.runtime.get_tick_ms = FakeGetTickMs;
    params.runtime.relax = FakeRelax;
    params.runtime.user_context = &clock;

    memset(&frame, 0, sizeof(frame));

    result->open_status = CANCoreOpen(&core, &binding, &params);
    if (result->open_status != CAN_STATUS_OK)
    {
        return;
    }

    result->start_status = CANCoreStart(&core);
    if (result->start_status != CAN_STATUS_OK)
    {
        result->close_status = CANCoreClose(&core);
        return;
    }

    result->receive_timeout_status = CANCoreReceiveTimeout(&core, &frame, 5U);
    if (result->receive_timeout_status == CAN_STATUS_OK)
    {
        result->received = true;
        result->frame = frame;
    }

    result->last_status = CANCoreGetLastStatus(&core);
    CANCoreGetStats(&core, &result->stats);

    result->stop_status = CANCoreStop(&core);
    result->close_status = CANCoreClose(&core);

    result->receive_call_count = channel.receive_call_count;
    result->relax_call_count = clock.relax_call_count;
}

bool CANCoreSendTimeoutSuccessTestPassed(const CANCoreTimeoutSendTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->send_timeout_status == CAN_STATUS_OK) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        (result->send_call_count == 4U) &&
        (result->relax_call_count == 3U) &&
        (result->last_status == CAN_STATUS_OK) &&
        (result->stats.tx_calls == 4U) &&
        (result->stats.tx_ok == 1U) &&
        (result->stats.tx_busy == 3U) &&
        (result->stats.tx_timeouts == 0U);
}

bool CANCoreSendTimeoutExpiredTestPassed(const CANCoreTimeoutSendTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->send_timeout_status == CAN_STATUS_ETIMEOUT) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        (result->send_call_count == 6U) &&
        (result->relax_call_count == 5U) &&
        (result->last_status == CAN_STATUS_ETIMEOUT) &&
        (result->stats.tx_calls == 6U) &&
        (result->stats.tx_ok == 0U) &&
        (result->stats.tx_busy == 6U) &&
        (result->stats.tx_timeouts == 1U);
}

bool CANCoreReceiveTimeoutSuccessTestPassed(const CANCoreTimeoutReceiveTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->receive_timeout_status == CAN_STATUS_OK) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        (result->receive_call_count == 3U) &&
        (result->relax_call_count == 2U) &&
        (result->last_status == CAN_STATUS_OK) &&
        (result->received == true) &&
        (result->frame.id == 0x321U) &&
        (result->frame.len == 1U) &&
        (result->frame.data[0] == 0xA5U) &&
        (result->stats.rx_calls == 3U) &&
        (result->stats.rx_ok == 1U) &&
        (result->stats.rx_empty == 2U) &&
        (result->stats.rx_timeouts == 0U);
}

bool CANCoreReceiveTimeoutExpiredTestPassed(const CANCoreTimeoutReceiveTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->open_status == CAN_STATUS_OK) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->receive_timeout_status == CAN_STATUS_ETIMEOUT) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK) &&
        (result->receive_call_count == 6U) &&
        (result->relax_call_count == 5U) &&
        (result->last_status == CAN_STATUS_ETIMEOUT) &&
        (result->received == false) &&
        (result->stats.rx_calls == 6U) &&
        (result->stats.rx_ok == 0U) &&
        (result->stats.rx_empty == 6U) &&
        (result->stats.rx_timeouts == 1U);
}