#include "board_gpio.h"
#include "IfxPort.h"

#define BOARD_TC3XX_PORT_MAP(port_number, module_name) \
    case (port_number):                               \
        *port_module = &(module_name);                 \
        return BOARD_STATUS_OK

static BoardStatus boardGPIOResolvePort(BoardGPIO gpio, Ifx_P **port_module, uint8 *pin_index)
{
    if ((port_module == 0) || (pin_index == 0))
    {
        return BOARD_STATUS_EINVAL;
    }

    if (gpio.pin > 15U)
    {
        return BOARD_STATUS_EINVAL;
    }

    *pin_index = (uint8)gpio.pin;

    switch (gpio.port)
    {
#ifdef MODULE_P00
        BOARD_TC3XX_PORT_MAP(0U, MODULE_P00);
#endif
#ifdef MODULE_P01
        BOARD_TC3XX_PORT_MAP(1U, MODULE_P01);
#endif
#ifdef MODULE_P02
        BOARD_TC3XX_PORT_MAP(2U, MODULE_P02);
#endif
#ifdef MODULE_P10
        BOARD_TC3XX_PORT_MAP(10U, MODULE_P10);
#endif
#ifdef MODULE_P11
        BOARD_TC3XX_PORT_MAP(11U, MODULE_P11);
#endif
#ifdef MODULE_P12
        BOARD_TC3XX_PORT_MAP(12U, MODULE_P12);
#endif
#ifdef MODULE_P13
        BOARD_TC3XX_PORT_MAP(13U, MODULE_P13);
#endif
#ifdef MODULE_P14
        BOARD_TC3XX_PORT_MAP(14U, MODULE_P14);
#endif
#ifdef MODULE_P15
        BOARD_TC3XX_PORT_MAP(15U, MODULE_P15);
#endif
#ifdef MODULE_P20
        BOARD_TC3XX_PORT_MAP(20U, MODULE_P20);
#endif
#ifdef MODULE_P21
        BOARD_TC3XX_PORT_MAP(21U, MODULE_P21);
#endif
#ifdef MODULE_P22
        BOARD_TC3XX_PORT_MAP(22U, MODULE_P22);
#endif
#ifdef MODULE_P23
        BOARD_TC3XX_PORT_MAP(23U, MODULE_P23);
#endif
#ifdef MODULE_P32
        BOARD_TC3XX_PORT_MAP(32U, MODULE_P32);
#endif
#ifdef MODULE_P33
        BOARD_TC3XX_PORT_MAP(33U, MODULE_P33);
#endif
#ifdef MODULE_P34
        BOARD_TC3XX_PORT_MAP(34U, MODULE_P34);
#endif
#ifdef MODULE_P40
        BOARD_TC3XX_PORT_MAP(40U, MODULE_P40);
#endif
#ifdef MODULE_P41
        BOARD_TC3XX_PORT_MAP(41U, MODULE_P41);
#endif

        default:
            return BOARD_STATUS_ENOTFOUND;
    }
}

BoardStatus boardGPIOConfigOutput(BoardGPIO gpio, bool initialHigh)
{
    BoardStatus status;
    Ifx_P *port_module;
    uint8 pin_index;

    status = boardGPIOResolvePort(gpio, &port_module, &pin_index);
    if (status != BOARD_STATUS_OK)
    {
        return status;
    }

    IfxPort_setPinMode(port_module, pin_index, IfxPort_Mode_outputPushPullGeneral);
    IfxPort_setPinState(port_module,
                        pin_index,
                        initialHigh ? IfxPort_State_high : IfxPort_State_low);

    return BOARD_STATUS_OK;
}

BoardStatus boardGPIOWrite(BoardGPIO gpio, bool high)
{
    BoardStatus status;
    Ifx_P *port_module;
    uint8 pin_index;

    status = boardGPIOResolvePort(gpio, &port_module, &pin_index);
    if (status != BOARD_STATUS_OK)
    {
        return status;
    }

    IfxPort_setPinState(port_module, pin_index, high ? IfxPort_State_high : IfxPort_State_low);

    return BOARD_STATUS_OK;
}

BoardStatus boardGPIORead(BoardGPIO gpio, bool *state)
{
    BoardStatus status;
    Ifx_P *port_module;
    uint8 pin_index;

    if (state == 0)
    {
        return BOARD_STATUS_EINVAL;
    }

    status = boardGPIOResolvePort(gpio, &port_module, &pin_index);
    if (status != BOARD_STATUS_OK)
    {
        return status;
    }

    *state = IfxPort_getPinState(port_module, pin_index) ? true : false;
    return BOARD_STATUS_OK;
}