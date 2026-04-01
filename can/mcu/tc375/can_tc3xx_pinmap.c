#include "can_tc3xx_pinmap.h"
#include "IfxPort.h"

CANStatus canTC3xxResolveModule(BoardMCUCANInst inst, Ifx_CAN **module)
{
    if (module == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (inst)
    {
        case BOARD_MCU_CAN_INST_0:
            *module = &MODULE_CAN0;
            return CAN_STATUS_OK;

#ifdef MODULE_CAN1
        case BOARD_MCU_CAN_INST_1:
            *module = &MODULE_CAN1;
            return CAN_STATUS_OK;
#endif

        default:
            return CAN_STATUS_ENOTFOUND;
    }
}

CANStatus canTC3xxResolveNodeId(BoardMCUCANNode node, IfxCan_NodeId *node_id)
{
    if (node_id == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (node)
    {
        case BOARD_MCU_CAN_NODE_0:
            *node_id = IfxCan_NodeId_0;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_NODE_1:
            *node_id = IfxCan_NodeId_1;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_NODE_2:
            *node_id = IfxCan_NodeId_2;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_NODE_3:
            *node_id = IfxCan_NodeId_3;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_ENOTFOUND;
    }
}

typedef struct CANTC3xxPinBindingStruct
{
    BoardMCUCANInst inst;
    BoardMCUCANNode node;
    uint16_t tx_port;
    uint16_t tx_pin;
    uint16_t rx_port;
    uint16_t rx_pin;
    const IfxCan_Can_Pins *pins;
} CANTC3xxPinBinding;

static const IfxCan_Can_Pins g_can00_p20_8_p20_7 =
{
    &IfxCan_TXD00_P20_8_OUT,
    IfxPort_OutputMode_pushPull,
    &IfxCan_RXD00B_P20_7_IN,
    IfxPort_InputMode_pullUp,
    IfxPort_PadDriver_cmosAutomotiveSpeed2
};

static const CANTC3xxPinBinding g_can_pin_bindings[] =
{
    {
        BOARD_MCU_CAN_INST_0,
        BOARD_MCU_CAN_NODE_0,
        20U, 8U,
        20U, 7U,
        &g_can00_p20_8_p20_7
    }
};

CANStatus canTC3xxResolvePins(const BoardCANPort *board_port, IfxCan_Can_Pins *pins)
{
    uint32_t i;

    if ((board_port == 0) || (pins == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    for (i = 0U; i < (uint32_t)(sizeof(g_can_pin_bindings) / sizeof(g_can_pin_bindings[0])); ++i)
    {
        const CANTC3xxPinBinding *binding = &g_can_pin_bindings[i];

        if ((board_port->mcu_inst == binding->inst) &&
            (board_port->mcu_node == binding->node) &&
            (board_port->tx_pin.port == binding->tx_port) &&
            (board_port->tx_pin.pin == binding->tx_pin) &&
            (board_port->rx_pin.port == binding->rx_port) &&
            (board_port->rx_pin.pin == binding->rx_pin))
        {
            *pins = *binding->pins;
            return CAN_STATUS_OK;
        }
    }

    return CAN_STATUS_ENOTFOUND;
}