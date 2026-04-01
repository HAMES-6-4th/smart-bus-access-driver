#include "board_tc375_lite.h"
#include "board_gpio.h"

static bool boardLevelFromControlState(BoardPinActive activeLevel, bool enabled)
{
    if (activeLevel == BOARD_PIN_ACTIVE_HIGH)
    {
        return enabled;
    }
    else
    {
        return !enabled;
    }
}

static BoardStatus boardApplyControlPin(const BoardControlPin *pin, bool enabled)
{
    bool level;

    if (pin == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    if (pin->present == false)
    {
        return BOARD_STATUS_EUNSUPPORTED;
    }

    level = boardLevelFromControlState(pin->active_level, enabled);
    return boardGPIOWrite(pin->gpio, level);
}

static BoardStatus boardInitControlPin(const BoardControlPin *pin, bool enabled)
{
    bool initialLevel;

    if (pin == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    if (pin->present == false)
    {
        return BOARD_STATUS_OK;
    }

    initialLevel = boardLevelFromControlState(pin->active_level, enabled);
    return boardGPIOConfigOutput(pin->gpio, initialLevel);
}

static const BoardCANPort g_can_ports[] =
{
    {
        .name = "can0",
        .mcu_inst = BOARD_MCU_CAN_INST_0,
        .mcu_node = BOARD_MCU_CAN_NODE_0,

        .rx_pin = { .port = 20U, .pin = 7U },
        .tx_pin = { .port = 20U, .pin = 8U },

        .phy_type = BOARD_CAN_PHY_EXTERNAL,
        .fd_support = BOARD_CAN_FD_SUPPORTED,

        .transceiver_enable =
        {
            .present = false,
            .gpio = { .port = 0U, .pin = 0U },
            .active_level = BOARD_PIN_ACTIVE_HIGH
        },

        // TC375 Lite Kit: CAN_STB = P20.6
        // STB high  -> standby
        // STB low   -> normal
        .transceiver_standby =
        {
            .present = true,
            .gpio = { .port = 20U, .pin = 6U },
            .active_level = BOARD_PIN_ACTIVE_HIGH
        },

        // Board manual 기준 software-controlled termination pin은 확인되지 않음
        .termination_enable =
        {
            .present = false,
            .gpio = { .port = 0U, .pin = 0U },
            .active_level = BOARD_PIN_ACTIVE_HIGH
        },

        .default_nominal_bitrate = 500000U,
        .default_data_bitrate = 2000000U
    }
};

// test build용 loopback 포트 정의 (실제 하드웨어에서는 존재하지 않음)
// static const BoardCANPort g_can_ports[] =
// {
//     {
//         .name = "can0",
//         .mcu_inst = BOARD_MCU_CAN_INST_0,
//         .mcu_node = BOARD_MCU_CAN_NODE_0,

//         .rx_pin = { .port = 20U, .pin = 7U },
//         .tx_pin = { .port = 20U, .pin = 8U },

//         .phy_type = BOARD_CAN_PHY_EXTERNAL,
//         .fd_support = BOARD_CAN_FD_SUPPORTED,

//         .transceiver_enable =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .transceiver_standby =
//         {
//             .present = true,
//             .gpio = { .port = 20U, .pin = 6U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .termination_enable =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .default_nominal_bitrate = 500000U,
//         .default_data_bitrate = 2000000U
//     },

//     // test-only: internal loopback source node
//     {
//         .name = "can0_lb0",
//         .mcu_inst = BOARD_MCU_CAN_INST_0,
//         .mcu_node = BOARD_MCU_CAN_NODE_0,

//         .rx_pin = { .port = 0U, .pin = 0U },
//         .tx_pin = { .port = 0U, .pin = 0U },

//         .phy_type = BOARD_CAN_PHY_NONE,
//         .fd_support = BOARD_CAN_FD_SUPPORTED,

//         .transceiver_enable =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .transceiver_standby =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .termination_enable =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .default_nominal_bitrate = 500000U,
//         .default_data_bitrate = 2000000U
//     },

//     // test-only: internal loopback destination node
//     {
//         .name = "can0_lb1",
//         .mcu_inst = BOARD_MCU_CAN_INST_0,
//         .mcu_node = BOARD_MCU_CAN_NODE_1,

//         .rx_pin = { .port = 0U, .pin = 0U },
//         .tx_pin = { .port = 0U, .pin = 0U },

//         .phy_type = BOARD_CAN_PHY_NONE,
//         .fd_support = BOARD_CAN_FD_SUPPORTED,

//         .transceiver_enable =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .transceiver_standby =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .termination_enable =
//         {
//             .present = false,
//             .gpio = { .port = 0U, .pin = 0U },
//             .active_level = BOARD_PIN_ACTIVE_HIGH
//         },

//         .default_nominal_bitrate = 500000U,
//         .default_data_bitrate = 2000000U
//     }
// };

static const BoardDescriptor g_board =
{
    .board_name = "tc375_lite",
    .can_ports = g_can_ports,
    .can_port_count = (uint32_t)(sizeof(g_can_ports) / sizeof(g_can_ports[0]))
};

BoardStatus boardImplInit(void)
{
    BoardStatus status;
    uint32_t i;

    for (i = 0U; i < (uint32_t)(sizeof(g_can_ports) / sizeof(g_can_ports[0])); i++)
    {
        const BoardCANPort *port = &g_can_ports[i];

        // deterministic 하게 transceiver를 standby 상태로 시작
        status = boardInitControlPin(&port->transceiver_standby, true);
        if (status != BOARD_STATUS_OK)
        {
            return status;
        }

        // enable 핀이 있는 보드는 기본적으로 비활성 상태로 시작
        status = boardInitControlPin(&port->transceiver_enable, false);
        if (status != BOARD_STATUS_OK)
        {
            return status;
        }

        // termination 제어 핀이 있더라도 기본은 off
        status = boardInitControlPin(&port->termination_enable, false);
        if (status != BOARD_STATUS_OK)
        {
            return status;
        }
    }

    return BOARD_STATUS_OK;
}

const BoardDescriptor *boardImplGetDescriptor(void)
{
    return &g_board;
}

BoardStatus boardImplCANPortPowerOn(const BoardCANPort *port)
{
    BoardStatus status;

    if (port == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    // powerOn은 PHY 사용 준비까지만 담당한다. 
    // start/stop이 standby 제어를 담당하므로 여기서는 standby 핀을 건드리지 않는다.
    if (port->transceiver_enable.present == true)
    {
        status = boardApplyControlPin(&port->transceiver_enable, true);
        if (status != BOARD_STATUS_OK)
        {
            return status;
        }
    }

    return BOARD_STATUS_OK;
}

BoardStatus boardImplCANPortPowerOff(const BoardCANPort *port)
{
    BoardStatus status;

    if (port == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    if (port->transceiver_standby.present == true)
    {
        status = boardApplyControlPin(&port->transceiver_standby, true);
        if (status != BOARD_STATUS_OK)
        {
            return status;
        }
    }

    if (port->transceiver_enable.present == true)
    {
        status = boardApplyControlPin(&port->transceiver_enable, false);
        if (status != BOARD_STATUS_OK)
        {
            return status;
        }
    }

    return BOARD_STATUS_OK;
}

BoardStatus boardImplCANPortSetStandby(const BoardCANPort *port, bool enable)
{
    if (port == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    if (port->transceiver_standby.present == false)
    {
        return BOARD_STATUS_EUNSUPPORTED;
    }

    return boardApplyControlPin(&port->transceiver_standby, enable);
}

BoardStatus boardImplCANPortSetTermination(const BoardCANPort *port, bool enable)
{
    if (port == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    if (port->termination_enable.present == false)
    {
        return BOARD_STATUS_EUNSUPPORTED;
    }

    return boardApplyControlPin(&port->termination_enable, enable);
}