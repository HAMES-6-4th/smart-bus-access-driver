#include "can_core.h"

static bool CANCoreHasRuntime(const CANCore *core);
static uint32_t CANCoreNowMs(const CANCore *core);
static void CANCoreRelax(const CANCore *core);
static bool CANCoreDeadlineExpired(uint32_t start_ms, uint32_t timeout_ms, uint32_t now_ms);

static bool CANCoreHasRuntime(const CANCore *core)
{
    return (core != 0) && (core->runtime.get_tick_ms != 0);
}

static uint32_t CANCoreNowMs(const CANCore *core)
{
    return core->runtime.get_tick_ms(core->runtime.user_context);
}

static void CANCoreRelax(const CANCore *core)
{
    if ((core != 0) && (core->runtime.relax != 0))
    {
        core->runtime.relax(core->runtime.user_context);
    }
}

static bool CANCoreDeadlineExpired(uint32_t start_ms, uint32_t timeout_ms, uint32_t now_ms)
{
    return ((uint32_t)(now_ms - start_ms) >= timeout_ms);
}

void CANCoreInit(CANCore *core)
{
    if (core == 0)
    {
        return;
    }

    memset(core, 0, sizeof(*core));
    core->state = CAN_CORE_STATE_CLOSED;
    core->last_status = CAN_STATUS_OK;
    core->optional_ops = 0;
}

void CANCoreInitOpenParams(CANCoreOpenParams *params)
{
    if (params == 0)
    {
        return;
    }

    memset(params, 0, sizeof(*params));
    params->channel_config.timing.mode = CAN_MODE_CLASSIC;
}

CANStatus CANCoreOpen(CANCore *core, const CANCoreBinding *binding, const CANCoreOpenParams *params)
{
    CANStatus status;

    if ((core == 0) || (binding == 0) || (params == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }

        return CAN_STATUS_EINVAL;
    }

    if ((binding->ops == 0) ||
        (binding->ops->Open == 0) ||
        (binding->ops->Close == 0) ||
        (binding->ops->Start == 0) ||
        (binding->ops->Stop == 0) ||
        (binding->ops->Send == 0) ||
        (binding->ops->Receive == 0))
    {
        core->last_status = CAN_STATUS_EINVAL;
        return CAN_STATUS_EINVAL;
    }

    if (core->state != CAN_CORE_STATE_CLOSED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    status = binding->ops->Open(binding->driver_channel, binding->driver_port, &params->channel_config);
    if (status != CAN_STATUS_OK)
    {
        core->last_status = status;
        return status;
    }

    core->state = CAN_CORE_STATE_OPENED;
    core->name = binding->name;
    core->ops = binding->ops;
    core->driver_channel = binding->driver_channel;
    core->driver_port = binding->driver_port;
    core->capabilities = binding->capabilities;
    core->optional_ops = binding->optional_ops;
    core->config = params->channel_config;
    core->hooks = params->hooks;
    core->flags = params->flags;
    core->runtime = params->runtime;
    core->last_status = CAN_STATUS_OK;
    memset(&core->stats, 0, sizeof(core->stats));

    return CAN_STATUS_OK;
}

CANStatus CANCoreClose(CANCore *core)
{
    CANStatus status;

    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (core->state == CAN_CORE_STATE_CLOSED)
    {
        core->last_status = CAN_STATUS_EINVAL;
        return CAN_STATUS_EINVAL;
    }

    if ((core->state == CAN_CORE_STATE_STARTED) && (core->ops != 0) && (core->ops->Stop != 0))
    {
        (void)core->ops->Stop(core->driver_channel);
    }

    status = core->ops->Close(core->driver_channel);
    if (status != CAN_STATUS_OK)
    {
        core->last_status = status;
        return status;
    }

    CANCoreInit(core);
    return CAN_STATUS_OK;
}

CANStatus CANCoreStart(CANCore *core)
{
    CANStatus status;

    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (core->state == CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_OK;
        return CAN_STATUS_OK;
    }

    if (core->state != CAN_CORE_STATE_OPENED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    status = core->ops->Start(core->driver_channel);
    core->last_status = status;
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    core->state = CAN_CORE_STATE_STARTED;
    return CAN_STATUS_OK;
}

CANStatus CANCoreStop(CANCore *core)
{
    CANStatus status;

    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (core->state == CAN_CORE_STATE_OPENED)
    {
        core->last_status = CAN_STATUS_OK;
        return CAN_STATUS_OK;
    }

    if (core->state != CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    status = core->ops->Stop(core->driver_channel);
    core->last_status = status;
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    core->state = CAN_CORE_STATE_OPENED;
    return CAN_STATUS_OK;
}

CANStatus CANCoreSend(CANCore *core, const CANFrame *frame)
{
    CANStatus status;

    if ((core == 0) || (frame == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }

        return CAN_STATUS_EINVAL;
    }

    if (core->state != CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    core->stats.tx_calls++;

    status = core->ops->Send(core->driver_channel, frame);
    core->last_status = status;

    if (status == CAN_STATUS_OK)
    {
        core->stats.tx_ok++;
    }
    else if (status == CAN_STATUS_EBUSY)
    {
        core->stats.tx_busy++;
    }

    return status;
}

CANStatus CANCoreReceive(CANCore *core, CANFrame *frame)
{
    CANStatus status;

    if ((core == 0) || (frame == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }

        return CAN_STATUS_EINVAL;
    }

    if (core->state != CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    core->stats.rx_calls++;

    status = core->ops->Receive(core->driver_channel, frame);
    core->last_status = status;

    if (status == CAN_STATUS_OK)
    {
        core->stats.rx_ok++;
    }
    else if (status == CAN_STATUS_ENODATA)
    {
        core->stats.rx_empty++;
    }

    return status;
}

CANStatus CANCoreTrySend(CANCore *core, const CANFrame *frame)
{
    return CANCoreSend(core, frame);
}

CANStatus CANCoreTryReceive(CANCore *core, CANFrame *frame)
{
    return CANCoreReceive(core, frame);
}

bool CANCoreIsOpen(const CANCore *core)
{
    return (core != 0) && (core->state != CAN_CORE_STATE_CLOSED);
}

bool CANCoreIsStarted(const CANCore *core)
{
    return (core != 0) && (core->state == CAN_CORE_STATE_STARTED);
}

const char *CANCoreGetName(const CANCore *core)
{
    if (core == 0)
    {
        return 0;
    }

    return core->name;
}

const CANChannelConfig *CANCoreGetConfig(const CANCore *core)
{
    if (core == 0)
    {
        return 0;
    }

    return &core->config;
}

const CANCoreCapabilities *CANCoreGetCapabilities(const CANCore *core)
{
    if (core == 0)
    {
        return 0;
    }

    return &core->capabilities;
}

CANStatus CANCoreSendTimeout(CANCore *core, const CANFrame *frame, uint32_t timeout_ms)
{
    CANStatus status;
    uint32_t start_ms;

    if ((core == 0) || (frame == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }

        return CAN_STATUS_EINVAL;
    }

    if (core->state != CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    if (timeout_ms == 0U)
    {
        return CANCoreSend(core, frame);
    }

    if (CANCoreHasRuntime(core) == false)
    {
        core->last_status = CAN_STATUS_EUNSUPPORTED;
        return CAN_STATUS_EUNSUPPORTED;
    }

    start_ms = CANCoreNowMs(core);

    for (;;)
    {
        status = CANCoreSend(core, frame);
        if (status == CAN_STATUS_OK)
        {
            return CAN_STATUS_OK;
        }

        if (status != CAN_STATUS_EBUSY)
        {
            return status;
        }

        if ((timeout_ms != CAN_TIMEOUT_INFINITE) && CANCoreDeadlineExpired(start_ms, timeout_ms, CANCoreNowMs(core)))
        {
            core->stats.tx_timeouts++;
            core->last_status = CAN_STATUS_ETIMEOUT;
            return CAN_STATUS_ETIMEOUT;
        }

        CANCoreRelax(core);
    }
}

CANStatus CANCoreReceiveTimeout(CANCore *core, CANFrame *frame, uint32_t timeout_ms)
{
    CANStatus status;
    uint32_t start_ms;

    if ((core == 0) || (frame == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }
        
        return CAN_STATUS_EINVAL;
    }

    if (core->state != CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        return CAN_STATUS_EBUSY;
    }

    if (timeout_ms == 0U)
    {
        return CANCoreReceive(core, frame);
    }

    if (CANCoreHasRuntime(core) == false)
    {
        core->last_status = CAN_STATUS_EUNSUPPORTED;
        return CAN_STATUS_EUNSUPPORTED;
    }

    start_ms = CANCoreNowMs(core);

    for (;;)
    {
        status = CANCoreReceive(core, frame);
        if (status == CAN_STATUS_OK)
        {
            return CAN_STATUS_OK;
        }

        if (status != CAN_STATUS_ENODATA)
        {
            return status;
        }

        if ((timeout_ms != CAN_TIMEOUT_INFINITE) &&
            CANCoreDeadlineExpired(start_ms, timeout_ms, CANCoreNowMs(core)))
        {
            core->stats.rx_timeouts++;
            core->last_status = CAN_STATUS_ETIMEOUT;
            return CAN_STATUS_ETIMEOUT;
        }

        CANCoreRelax(core);
    }
}

CANStatus CANCoreGetLastStatus(const CANCore *core)
{
    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    return core->last_status;
}

void CANCoreGetStats(const CANCore *core, CANCoreStats *stats)
{
    if ((core == 0) || (stats == 0))
    {
        return;
    }

    *stats = core->stats;
}

void CANCoreResetStats(CANCore *core)
{
    if (core == 0)
    {
        return;
    }

    memset(&core->stats, 0, sizeof(core->stats));
}

CANStatus CANCoreQueryEvents(CANCore *core, uint32_t *event_mask)
{
    CANStatus status;

    if ((core == 0) || (event_mask == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }
        return CAN_STATUS_EINVAL;
    }

    if ((core->state == CAN_CORE_STATE_CLOSED) ||
        (core->optional_ops == 0) ||
        (core->optional_ops->QueryEvents == 0))
    {
        core->last_status = CAN_STATUS_EUNSUPPORTED;
        return CAN_STATUS_EUNSUPPORTED;
    }

    status = core->optional_ops->QueryEvents(core->driver_channel, event_mask);
    core->last_status = status;
    return status;
}

CANStatus CANCorePoll(CANCore *core,
                      uint32_t interest_mask,
                      uint32_t timeout_ms,
                      uint32_t *ready_mask)
{
    CANStatus status;
    uint32_t queried_mask;
    uint32_t start_ms;
    const uint32_t valid_mask =
        (uint32_t)(CAN_CORE_EVENT_RX_READY |
                   CAN_CORE_EVENT_TX_READY |
                   CAN_CORE_EVENT_ERROR |
                   CAN_CORE_EVENT_STATE);

    if ((core == 0) || (ready_mask == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }
        return CAN_STATUS_EINVAL;
    }

    if ((interest_mask == 0U) || ((interest_mask & ~valid_mask) != 0U))
    {
        core->last_status = CAN_STATUS_EINVAL;
        *ready_mask = 0U;
        return CAN_STATUS_EINVAL;
    }

    if (core->state != CAN_CORE_STATE_STARTED)
    {
        core->last_status = CAN_STATUS_EBUSY;
        *ready_mask = 0U;
        return CAN_STATUS_EBUSY;
    }

    if ((timeout_ms != 0U) && (CANCoreHasRuntime(core) == false))
    {
        core->last_status = CAN_STATUS_EUNSUPPORTED;
        *ready_mask = 0U;
        return CAN_STATUS_EUNSUPPORTED;
    }

    start_ms = 0U;
    if (timeout_ms != 0U)
    {
        start_ms = CANCoreNowMs(core);
    }

    for (;;)
    {
        queried_mask = 0U;
        status = CANCoreQueryEvents(core, &queried_mask);
        if (status != CAN_STATUS_OK)
        {
            *ready_mask = 0U;
            return status;
        }

        *ready_mask = (queried_mask & interest_mask);
        if (*ready_mask != 0U)
        {
            core->last_status = CAN_STATUS_OK;
            return CAN_STATUS_OK;
        }

        if (timeout_ms == 0U)
        {
            core->last_status = CAN_STATUS_ENODATA;
            return CAN_STATUS_ENODATA;
        }

        if ((timeout_ms != CAN_TIMEOUT_INFINITE) &&
            CANCoreDeadlineExpired(start_ms, timeout_ms, CANCoreNowMs(core)))
        {
            core->last_status = CAN_STATUS_ETIMEOUT;
            return CAN_STATUS_ETIMEOUT;
        }

        CANCoreRelax(core);
    }
}

CANStatus CANCoreGetErrorState(CANCore *core, CANCoreErrorState *state)
{
    CANStatus status;

    if ((core == 0) || (state == 0))
    {
        if (core != 0)
        {
            core->last_status = CAN_STATUS_EINVAL;
        }
        return CAN_STATUS_EINVAL;
    }

    if ((core->state == CAN_CORE_STATE_CLOSED) ||
        (core->optional_ops == 0) ||
        (core->optional_ops->GetErrorState == 0))
    {
        core->last_status = CAN_STATUS_EUNSUPPORTED;
        return CAN_STATUS_EUNSUPPORTED;
    }

    status = core->optional_ops->GetErrorState(core->driver_channel, state);
    core->last_status = status;
    return status;
}

CANStatus CANCoreRecover(CANCore *core)
{
    CANStatus status;

    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if ((core->state == CAN_CORE_STATE_CLOSED) ||
        (core->optional_ops == 0) ||
        (core->optional_ops->Recover == 0))
    {
        core->last_status = CAN_STATUS_EUNSUPPORTED;
        return CAN_STATUS_EUNSUPPORTED;
    }

    status = core->optional_ops->Recover(core->driver_channel);
    core->last_status = status;
    return status;
}