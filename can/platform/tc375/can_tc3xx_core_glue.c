#include "can_tc3xx_core_glue.h"

#include <string.h>

static CANStatus CANTC3xxCoreOpen(void *driver_channel,
                                  const void *driver_port,
                                  const CANChannelConfig *config)
{
    return canTC3xxOpen((CANTC3xxChannel *)driver_channel,
                        (const BoardCANPort *)driver_port,
                        config);
}

static CANStatus CANTC3xxCoreClose(void *driver_channel)
{
    return canTC3xxClose((CANTC3xxChannel *)driver_channel);
}

static CANStatus CANTC3xxCoreStart(void *driver_channel)
{
    return canTC3xxStart((CANTC3xxChannel *)driver_channel);
}

static CANStatus CANTC3xxCoreStop(void *driver_channel)
{
    return canTC3xxStop((CANTC3xxChannel *)driver_channel);
}

static CANStatus CANTC3xxCoreSend(void *driver_channel, const CANFrame *frame)
{
    return canTC3xxSend((CANTC3xxChannel *)driver_channel, frame);
}

static CANStatus CANTC3xxCoreReceive(void *driver_channel, CANFrame *frame)
{
    return canTC3xxReceive((CANTC3xxChannel *)driver_channel, frame);
}

static CANStatus CANTC3xxCoreQueryEvents(void *driver_channel, uint32_t *event_mask)
{
    CANTC3xxChannel *channel;

    if ((driver_channel == 0) || (event_mask == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    channel = (CANTC3xxChannel *)driver_channel;
    *event_mask = 0U;

    if (channel->is_open == false)
    {
        return CAN_STATUS_EBUSY;
    }

    if (channel->is_started == false)
    {
        return CAN_STATUS_OK;
    }

    /* polling-first v2 준비:
       TX_READY는 started 상태에서 optimistic hint로 제공.
       실제 전송 가능 여부의 최종 판정은 CANCoreTrySend / CANCoreSend 가 담당 */
    *event_mask |= CAN_CORE_EVENT_TX_READY;

    if (channel->use_rx_fifo0 == true)
    {
        if (IfxCan_Node_getRxFifo0FillLevel(channel->can_node.node) > 0U)
        {
            *event_mask |= CAN_CORE_EVENT_RX_READY;
        }
    }
    else
    {
        if (IfxCan_Can_isNewDataReceived(&channel->can_node,
                                         (IfxCan_RxBufferId)channel->rx_buffer_number) != FALSE)
        {
            *event_mask |= CAN_CORE_EVENT_RX_READY;
        }
    }

    return CAN_STATUS_OK;
}

static CANStatus CANTC3xxCoreGetErrorState(void *driver_channel, CANCoreErrorState *state)
{
    return canTC3xxGetErrorState((CANTC3xxChannel *)driver_channel, state);
}

static CANStatus CANTC3xxCoreRecover(void *driver_channel)
{
    return canTC3xxRecover((CANTC3xxChannel *)driver_channel);
}

static const CANCoreDriverOps g_CANTC3xxCoreDriverOps =
{
    .Open = CANTC3xxCoreOpen,
    .Close = CANTC3xxCoreClose,
    .Start = CANTC3xxCoreStart,
    .Stop = CANTC3xxCoreStop,
    .Send = CANTC3xxCoreSend,
    .Receive = CANTC3xxCoreReceive
};

static const CANCoreOptionalDriverOps g_CANTC3xxCoreOptionalDriverOps =
{
    .QueryEvents = CANTC3xxCoreQueryEvents,
    .GetErrorState = CANTC3xxCoreGetErrorState,
    .Recover = CANTC3xxCoreRecover
};

void CANTC3xxCoreBindingInit(CANCoreBinding *binding,
                             const char *name,
                             CANTC3xxChannel *channel,
                             const BoardCANPort *board_port)
{
    if (binding == 0)
    {
        return;
    }

    if (channel != 0)
    {
        canTC3xxChannelInit(channel);
    }

    memset(binding, 0, sizeof(*binding));

    binding->name = name;
    binding->ops = &g_CANTC3xxCoreDriverOps;
    binding->optional_ops = &g_CANTC3xxCoreOptionalDriverOps;
    binding->driver_channel = channel;
    binding->driver_port = board_port;

    binding->capabilities.supports_fd =
        (board_port != 0) && (board_port->fd_support == BOARD_CAN_FD_SUPPORTED);

    binding->capabilities.supports_brs =
        (board_port != 0) && (board_port->fd_support == BOARD_CAN_FD_SUPPORTED);

    binding->capabilities.supports_loopback = true;
    binding->capabilities.supports_hw_filter = true;

    binding->capabilities.supports_termination_control =
        (board_port != 0) && (board_port->termination_enable.present == true);
}

