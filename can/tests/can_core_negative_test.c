#include "can_core_negative_test.h"

#include <string.h>

typedef struct CANCoreNegativeFakeChannelStruct
{
    bool is_open;
    bool is_started;
} CANCoreNegativeFakeChannel;

static CANStatus CANCoreNegativeFakeOpen(void *driver_channel,
                                         const void *driver_port,
                                         const CANChannelConfig *config)
{
    CANCoreNegativeFakeChannel *channel = (CANCoreNegativeFakeChannel *)driver_channel;

    (void)driver_port;
    (void)config;

    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    channel->is_open = true;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreNegativeFakeClose(void *driver_channel)
{
    CANCoreNegativeFakeChannel *channel = (CANCoreNegativeFakeChannel *)driver_channel;

    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    channel->is_open = false;
    channel->is_started = false;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreNegativeFakeStart(void *driver_channel)
{
    CANCoreNegativeFakeChannel *channel = (CANCoreNegativeFakeChannel *)driver_channel;

    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EBUSY;
    }

    channel->is_started = true;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreNegativeFakeStop(void *driver_channel)
{
    CANCoreNegativeFakeChannel *channel = (CANCoreNegativeFakeChannel *)driver_channel;

    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EBUSY;
    }

    channel->is_started = false;
    return CAN_STATUS_OK;
}

static CANStatus CANCoreNegativeFakeSend(void *driver_channel, const CANFrame *frame)
{
    CANCoreNegativeFakeChannel *channel = (CANCoreNegativeFakeChannel *)driver_channel;

    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (channel->is_started == false)
    {
        return CAN_STATUS_EBUSY;
    }

    return CAN_STATUS_OK;
}

static CANStatus CANCoreNegativeFakeReceive(void *driver_channel, CANFrame *frame)
{
    CANCoreNegativeFakeChannel *channel = (CANCoreNegativeFakeChannel *)driver_channel;

    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (channel->is_started == false)
    {
        return CAN_STATUS_EBUSY;
    }

    return CAN_STATUS_ENODATA;
}

static const CANCoreDriverOps g_CANCoreNegativeFakeDriverOps =
{
    .Open = CANCoreNegativeFakeOpen,
    .Close = CANCoreNegativeFakeClose,
    .Start = CANCoreNegativeFakeStart,
    .Stop = CANCoreNegativeFakeStop,
    .Send = CANCoreNegativeFakeSend,
    .Receive = CANCoreNegativeFakeReceive
};

static void CANCoreNegativeBindingInit(CANCoreBinding *binding,
                                       CANCoreNegativeFakeChannel *channel)
{
    if ((binding == 0) || (channel == 0))
    {
        return;
    }

    memset(channel, 0, sizeof(*channel));

    *binding = (CANCoreBinding){
        .name = "fake_negative_can",
        .ops = &g_CANCoreNegativeFakeDriverOps,
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

static void CANCoreNegativeInitFrame(CANFrame *frame)
{
    if (frame == 0)
    {
        return;
    }

    memset(frame, 0, sizeof(*frame));
    frame->id_type = CAN_ID_STANDARD;
    frame->mode = CAN_MODE_CLASSIC;
    frame->id = 0x123U;
    frame->len = 1U;
    frame->data[0] = 0x5AU;
}

static void CANCoreNegativeInitLifecycleResult(CANCoreLifecycleMatrixTestResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    result->start_before_open_status = CAN_STATUS_EINVAL;
    result->stop_before_open_status = CAN_STATUS_EINVAL;
    result->send_before_open_status = CAN_STATUS_EINVAL;
    result->receive_before_open_status = CAN_STATUS_EINVAL;
    result->open_status = CAN_STATUS_EINVAL;
    result->double_open_status = CAN_STATUS_EINVAL;
    result->send_before_start_status = CAN_STATUS_EINVAL;
    result->receive_before_start_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->double_start_status = CAN_STATUS_EINVAL;
    result->send_after_start_status = CAN_STATUS_EINVAL;
    result->receive_after_start_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->double_stop_status = CAN_STATUS_EINVAL;
    result->send_after_stop_status = CAN_STATUS_EINVAL;
    result->receive_after_stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;
    result->start_after_close_status = CAN_STATUS_EINVAL;
    result->stop_after_close_status = CAN_STATUS_EINVAL;
    result->close_after_close_status = CAN_STATUS_EINVAL;
    result->last_status = CAN_STATUS_EINVAL;
}

static void CANCoreNegativeInitInvalidBindingResult(CANCoreInvalidBindingTestResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));

    result->null_core_status = CAN_STATUS_EINVAL;
    result->null_binding_status = CAN_STATUS_EINVAL;
    result->null_params_status = CAN_STATUS_EINVAL;
    result->missing_ops_status = CAN_STATUS_EINVAL;
    result->missing_open_status = CAN_STATUS_EINVAL;
    result->missing_close_status = CAN_STATUS_EINVAL;
    result->missing_start_status = CAN_STATUS_EINVAL;
    result->missing_stop_status = CAN_STATUS_EINVAL;
    result->missing_send_status = CAN_STATUS_EINVAL;
    result->missing_receive_status = CAN_STATUS_EINVAL;
}

static CANStatus CANCoreNegativeOpenWithOps(const CANCoreDriverOps *ops)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    CANCoreNegativeFakeChannel channel;

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    CANCoreNegativeBindingInit(&binding, &channel);
    binding.ops = ops;

    return CANCoreOpen(&core, &binding, &params);
}

void CANCoreRunLifecycleMatrixTest(CANCoreLifecycleMatrixTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    CANCoreNegativeFakeChannel channel;
    CANFrame frame;

    if (result == 0)
    {
        return;
    }

    CANCoreNegativeInitLifecycleResult(result);

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    CANCoreNegativeBindingInit(&binding, &channel);
    CANCoreNegativeInitFrame(&frame);

    result->start_before_open_status = CANCoreStart(&core);
    result->stop_before_open_status = CANCoreStop(&core);
    result->send_before_open_status = CANCoreSend(&core, &frame);
    result->receive_before_open_status = CANCoreReceive(&core, &frame);

    result->open_status = CANCoreOpen(&core, &binding, &params);
    if (result->open_status != CAN_STATUS_OK)
    {
        result->last_status = CANCoreGetLastStatus(&core);
        return;
    }

    result->double_open_status = CANCoreOpen(&core, &binding, &params);

    result->send_before_start_status = CANCoreSend(&core, &frame);
    result->receive_before_start_status = CANCoreReceive(&core, &frame);

    result->start_status = CANCoreStart(&core);
    result->double_start_status = CANCoreStart(&core);

    result->send_after_start_status = CANCoreSend(&core, &frame);
    result->receive_after_start_status = CANCoreReceive(&core, &frame);

    result->stop_status = CANCoreStop(&core);
    result->double_stop_status = CANCoreStop(&core);

    result->send_after_stop_status = CANCoreSend(&core, &frame);
    result->receive_after_stop_status = CANCoreReceive(&core, &frame);

    CANCoreGetStats(&core, &result->stats_before_close);

    result->close_status = CANCoreClose(&core);
    result->start_after_close_status = CANCoreStart(&core);
    result->stop_after_close_status = CANCoreStop(&core);
    result->close_after_close_status = CANCoreClose(&core);
    result->last_status = CANCoreGetLastStatus(&core);
}

void CANCoreRunInvalidBindingTest(CANCoreInvalidBindingTestResult *result)
{
    CANCore core;
    CANCoreBinding binding;
    CANCoreOpenParams params;
    CANCoreNegativeFakeChannel channel;
    CANCoreDriverOps missing_open_ops;
    CANCoreDriverOps missing_close_ops;
    CANCoreDriverOps missing_start_ops;
    CANCoreDriverOps missing_stop_ops;
    CANCoreDriverOps missing_send_ops;
    CANCoreDriverOps missing_receive_ops;

    if (result == 0)
    {
        return;
    }

    CANCoreNegativeInitInvalidBindingResult(result);

    CANCoreInit(&core);
    CANCoreInitOpenParams(&params);
    CANCoreNegativeBindingInit(&binding, &channel);

    result->null_core_status = CANCoreOpen(0, &binding, &params);
    result->null_binding_status = CANCoreOpen(&core, 0, &params);
    result->null_params_status = CANCoreOpen(&core, &binding, 0);

    binding.ops = 0;
    result->missing_ops_status = CANCoreOpen(&core, &binding, &params);

    missing_open_ops = g_CANCoreNegativeFakeDriverOps;
    missing_close_ops = g_CANCoreNegativeFakeDriverOps;
    missing_start_ops = g_CANCoreNegativeFakeDriverOps;
    missing_stop_ops = g_CANCoreNegativeFakeDriverOps;
    missing_send_ops = g_CANCoreNegativeFakeDriverOps;
    missing_receive_ops = g_CANCoreNegativeFakeDriverOps;

    missing_open_ops.Open = 0;
    missing_close_ops.Close = 0;
    missing_start_ops.Start = 0;
    missing_stop_ops.Stop = 0;
    missing_send_ops.Send = 0;
    missing_receive_ops.Receive = 0;

    result->missing_open_status = CANCoreNegativeOpenWithOps(&missing_open_ops);
    result->missing_close_status = CANCoreNegativeOpenWithOps(&missing_close_ops);
    result->missing_start_status = CANCoreNegativeOpenWithOps(&missing_start_ops);
    result->missing_stop_status = CANCoreNegativeOpenWithOps(&missing_stop_ops);
    result->missing_send_status = CANCoreNegativeOpenWithOps(&missing_send_ops);
    result->missing_receive_status = CANCoreNegativeOpenWithOps(&missing_receive_ops);
}

bool CANCoreLifecycleMatrixTestPassed(const CANCoreLifecycleMatrixTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->start_before_open_status == CAN_STATUS_EBUSY) &&
        (result->stop_before_open_status == CAN_STATUS_EBUSY) &&
        (result->send_before_open_status == CAN_STATUS_EBUSY) &&
        (result->receive_before_open_status == CAN_STATUS_EBUSY) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->double_open_status == CAN_STATUS_EBUSY) &&
        (result->send_before_start_status == CAN_STATUS_EBUSY) &&
        (result->receive_before_start_status == CAN_STATUS_EBUSY) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->double_start_status == CAN_STATUS_OK) &&
        (result->send_after_start_status == CAN_STATUS_OK) &&
        (result->receive_after_start_status == CAN_STATUS_ENODATA) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->double_stop_status == CAN_STATUS_OK) &&
        (result->send_after_stop_status == CAN_STATUS_EBUSY) &&
        (result->receive_after_stop_status == CAN_STATUS_EBUSY) &&
        (result->close_status == CAN_STATUS_OK) &&
        (result->start_after_close_status == CAN_STATUS_EBUSY) &&
        (result->stop_after_close_status == CAN_STATUS_EBUSY) &&
        (result->close_after_close_status == CAN_STATUS_EINVAL) &&
        (result->last_status == CAN_STATUS_EINVAL) &&
        (result->stats_before_close.tx_calls == 1U) &&
        (result->stats_before_close.tx_ok == 1U) &&
        (result->stats_before_close.tx_busy == 0U) &&
        (result->stats_before_close.tx_timeouts == 0U) &&
        (result->stats_before_close.rx_calls == 1U) &&
        (result->stats_before_close.rx_ok == 0U) &&
        (result->stats_before_close.rx_empty == 1U) &&
        (result->stats_before_close.rx_timeouts == 0U);
}

bool CANCoreInvalidBindingTestPassed(const CANCoreInvalidBindingTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->null_core_status == CAN_STATUS_EINVAL) &&
        (result->null_binding_status == CAN_STATUS_EINVAL) &&
        (result->null_params_status == CAN_STATUS_EINVAL) &&
        (result->missing_ops_status == CAN_STATUS_EINVAL) &&
        (result->missing_open_status == CAN_STATUS_EINVAL) &&
        (result->missing_close_status == CAN_STATUS_EINVAL) &&
        (result->missing_start_status == CAN_STATUS_EINVAL) &&
        (result->missing_stop_status == CAN_STATUS_EINVAL) &&
        (result->missing_send_status == CAN_STATUS_EINVAL) &&
        (result->missing_receive_status == CAN_STATUS_EINVAL);
}
