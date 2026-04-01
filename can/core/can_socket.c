#include "can_socket.h"
#include "can_platform.h"

static void CANSocketResetState(CANSocket *socket)
{
    socket->core = 0;
    CANCoreInit(&socket->owned_core);
    socket->owns_core = false;
    socket->platform_opened = false;
}

void CANSocketInit(CANSocket *socket)
{
    if (socket == 0)
    {
        return;
    }

    CANSocketResetState(socket);
}

void CANSocketInitOpenParams(CANSocketOpenParams *params)
{
    CANCoreOpenParams core_params;

    if (params == 0)
    {
        return;
    }

    CANCoreInitOpenParams(&core_params);

    params->port_name = 0;
    params->channel_config = core_params.channel_config;
    params->runtime = core_params.runtime;
    params->hooks = core_params.hooks;
    params->flags = core_params.flags;
}

void CANSocketInitOpenParamsClassic500k(CANSocketOpenParams *params)
{
    CANSocketInitOpenParams(params);

    if (params == 0)
    {
        return;
    }

    params->channel_config.timing.mode = CAN_MODE_CLASSIC;
    params->channel_config.timing.nominal_bitrate = 500000U;
    params->channel_config.timing.data_bitrate = 0U;

    params->channel_config.enable_loopback = false;
    params->channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    params->channel_config.rx_filter.enabled = false;
    params->channel_config.rx_filter.id_type = CAN_ID_STANDARD;
    params->channel_config.rx_filter.id = 0U;
    params->channel_config.rx_filter.mask = 0U;
}

void CANSocketInitOpenParamsFd500k2M(CANSocketOpenParams *params)
{
    CANSocketInitOpenParams(params);

    if (params == 0)
    {
        return;
    }

    params->channel_config.timing.mode = CAN_MODE_FD_BRS;
    params->channel_config.timing.nominal_bitrate = 500000U;
    params->channel_config.timing.data_bitrate = 2000000U;

    params->channel_config.enable_loopback = false;
    params->channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    params->channel_config.rx_filter.enabled = false;
    params->channel_config.rx_filter.id_type = CAN_ID_STANDARD;
    params->channel_config.rx_filter.id = 0U;
    params->channel_config.rx_filter.mask = 0U;
}

CANStatus CANSocketOpen(CANSocket *socket, const CANSocketOpenParams *params)
{
    CANCoreOpenParams core_params;
    CANStatus status;

    if ((socket == 0) || (params == 0) || (params->port_name == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if ((socket->core != 0) ||
        (socket->owns_core == true) ||
        (socket->platform_opened == true))
    {
        return CAN_STATUS_EBUSY;
    }

    CANSocketResetState(socket);
    CANCoreInitOpenParams(&core_params);

    core_params.channel_config = params->channel_config;
    core_params.runtime = params->runtime;
    core_params.hooks = params->hooks;
    core_params.flags = params->flags;

    status = CANPlatformInit();
    if (status != CAN_STATUS_OK)
    {
        CANSocketResetState(socket);
        return status;
    }

    status = CANPlatformOpen(&socket->owned_core, params->port_name, &core_params);
    if (status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        CANSocketResetState(socket);
        return status;
    }

    status = CANCoreStart(&socket->owned_core);
    if (status != CAN_STATUS_OK)
    {
        (void)CANPlatformClose(&socket->owned_core);
        CANPlatformDeinit();
        CANSocketResetState(socket);
        return status;
    }

    socket->core = &socket->owned_core;
    socket->owns_core = true;
    socket->platform_opened = true;

    return CAN_STATUS_OK;
}

CANStatus CANSocketClose(CANSocket *socket)
{
    CANStatus stop_status;
    CANStatus close_status;
    bool need_platform_deinit;

    if (socket == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (socket->core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (socket->owns_core == false)
    {
        return CAN_STATUS_EUNSUPPORTED;
    }

    need_platform_deinit = socket->platform_opened;

    stop_status = CANCoreStop(socket->core);
    close_status = CANPlatformClose(socket->core);

    if (need_platform_deinit == true)
    {
        CANPlatformDeinit();
    }

    CANSocketResetState(socket);

    if ((stop_status != CAN_STATUS_OK) && (stop_status != CAN_STATUS_EBUSY))
    {
        return stop_status;
    }

    return close_status;
}

CANStatus CANSocketBindCore(CANSocket *socket, CANCore *core)
{
    if ((socket == 0) || (core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if ((socket->owns_core == true) ||
        (socket->platform_opened == true))
    {
        return CAN_STATUS_EBUSY;
    }

    if ((socket->core != 0) && (socket->core != core))
    {
        return CAN_STATUS_EBUSY;
    }

    socket->core = core;
    socket->owns_core = false;
    socket->platform_opened = false;

    return CAN_STATUS_OK;
}

CANStatus CANSocketUnbind(CANSocket *socket)
{
    if (socket == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if ((socket->owns_core == true) || (socket->platform_opened == true))
    {
        return CAN_STATUS_EBUSY;
    }

    CANSocketResetState(socket);
    return CAN_STATUS_OK;
}

CANCore *CANSocketGetCore(CANSocket *socket)
{
    if (socket == 0)
    {
        return 0;
    }

    return socket->core;
}

const CANCore *CANSocketGetCoreConst(const CANSocket *socket)
{
    if (socket == 0)
    {
        return 0;
    }

    return socket->core;
}

CANStatus CANSocketSend(CANSocket *socket, const CANFrame *frame)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreSend(socket->core, frame);
}

CANStatus CANSocketReceive(CANSocket *socket, CANFrame *frame)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreReceive(socket->core, frame);
}

CANStatus CANSocketTrySend(CANSocket *socket, const CANFrame *frame)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreTrySend(socket->core, frame);
}

CANStatus CANSocketTryReceive(CANSocket *socket, CANFrame *frame)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreTryReceive(socket->core, frame);
}

CANStatus CANSocketSendTimeout(CANSocket *socket, const CANFrame *frame, uint32_t timeout_ms)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreSendTimeout(socket->core, frame, timeout_ms);
}

CANStatus CANSocketReceiveTimeout(CANSocket *socket, CANFrame *frame, uint32_t timeout_ms)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreReceiveTimeout(socket->core, frame, timeout_ms);
}

CANStatus CANSocketSendNow(CANSocket *socket, const CANFrame *frame)
{
    return CANSocketSend(socket, frame);
}

CANStatus CANSocketSendClassicStd(CANSocket *socket,
                                  uint32_t id,
                                  const void *data,
                                  uint8_t len)
{
    CANFrame frame;
    CANStatus status;

    CANFrameInitClassicStd(&frame, id, len);

    status = CANFrameSetData(&frame, data, len);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    return CANSocketSend(socket, &frame);
}

CANStatus CANSocketSendClassicExt(CANSocket *socket,
                                  uint32_t id,
                                  const void *data,
                                  uint8_t len)
{
    CANFrame frame;
    CANStatus status;

    CANFrameInitClassicExt(&frame, id, len);

    status = CANFrameSetData(&frame, data, len);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    return CANSocketSend(socket, &frame);
}

CANStatus CANSocketSendFdStd(CANSocket *socket,
                             uint32_t id,
                             const void *data,
                             uint8_t len,
                             bool enable_brs)
{
    CANFrame frame;
    CANStatus status;

    CANFrameInitFdStd(&frame, id, len, enable_brs);

    status = CANFrameSetData(&frame, data, len);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    return CANSocketSend(socket, &frame);
}

CANStatus CANSocketSendFdExt(CANSocket *socket,
                             uint32_t id,
                             const void *data,
                             uint8_t len,
                             bool enable_brs)
{
    CANFrame frame;
    CANStatus status;

    CANFrameInitFdExt(&frame, id, len, enable_brs);

    status = CANFrameSetData(&frame, data, len);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    return CANSocketSend(socket, &frame);
}

CANStatus CANSocketReceiveNow(CANSocket *socket, CANFrame *frame)
{
    return CANSocketReceive(socket, frame);
}

CANStatus CANSocketReceiveMatch(CANSocket *socket,
                                CANFrame *frame,
                                CANIdType id_type,
                                uint32_t id)
{
    CANStatus status;
    CANFrame local_frame;

    if ((socket == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    while (1)
    {
        status = CANSocketReceiveNow(socket, &local_frame);
        if (status != CAN_STATUS_OK)
        {
            return status;
        }

        if ((local_frame.id_type == id_type) &&
            (local_frame.id == id))
        {
            *frame = local_frame;
            return CAN_STATUS_OK;
        }
    }
}

CANStatus CANSocketReceiveTimeoutMatch(CANSocket *socket,
                                       CANFrame *frame,
                                       CANIdType id_type,
                                       uint32_t id,
                                       uint32_t timeout_ms)
{
    CANStatus status;
    CANFrame local_frame;
    uint32_t start_ms;
    CANCore *core;
    const CANCoreRuntime *runtime;

    if ((socket == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (timeout_ms == 0U)
    {
        return CANSocketReceiveMatch(socket, frame, id_type, id);
    }

    core = CANSocketGetCore(socket);
    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    runtime = &core->runtime;
    if (runtime->get_tick_ms == 0)
    {
        return CAN_STATUS_EUNSUPPORTED;
    }

    start_ms = runtime->get_tick_ms(runtime->user_context);

    while (1)
    {
        status = CANSocketReceiveNow(socket, &local_frame);
        if (status == CAN_STATUS_OK)
        {
            if ((local_frame.id_type == id_type) &&
                (local_frame.id == id))
            {
                *frame = local_frame;
                return CAN_STATUS_OK;
            }
        }
        else if (status != CAN_STATUS_ENODATA)
        {
            return status;
        }

        if ((uint32_t)(runtime->get_tick_ms(runtime->user_context) - start_ms) >= timeout_ms)
        {
            return CAN_STATUS_ETIMEOUT;
        }

        if (runtime->relax != 0)
        {
            runtime->relax(runtime->user_context);
        }
    }
}

CANStatus CANSocketWaitTxReady(CANSocket *socket, uint32_t timeout_ms)
{
    uint32_t ready_mask;

    ready_mask = 0U;
    return CANSocketPoll(socket,
                         (uint32_t)CAN_CORE_EVENT_TX_READY,
                         timeout_ms,
                         &ready_mask);
}

CANStatus CANSocketWaitRxReady(CANSocket *socket, uint32_t timeout_ms)
{
    uint32_t ready_mask;

    ready_mask = 0U;
    return CANSocketPoll(socket,
                         (uint32_t)CAN_CORE_EVENT_RX_READY,
                         timeout_ms,
                         &ready_mask);
}

CANStatus CANSocketQueryEvents(CANSocket *socket, uint32_t *event_mask)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreQueryEvents(socket->core, event_mask);
}

CANStatus CANSocketPoll(CANSocket *socket,
                        uint32_t interest_mask,
                        uint32_t timeout_ms,
                        uint32_t *ready_mask)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCorePoll(socket->core, interest_mask, timeout_ms, ready_mask);
}

CANStatus CANSocketGetErrorState(CANSocket *socket, CANCoreErrorState *state)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreGetErrorState(socket->core, state);
}

CANStatus CANSocketRecover(CANSocket *socket)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreRecover(socket->core);
}

CANStatus CANSocketGetLastStatus(const CANSocket *socket)
{
    if ((socket == 0) || (socket->core == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    return CANCoreGetLastStatus(socket->core);
}

bool CANSocketIsOpen(const CANSocket *socket)
{
    return
        (socket != 0) &&
        (socket->core != 0) &&
        CANCoreIsOpen(socket->core);
}

bool CANSocketIsStarted(const CANSocket *socket)
{
    return
        (socket != 0) &&
        (socket->core != 0) &&
        CANCoreIsStarted(socket->core);
}
