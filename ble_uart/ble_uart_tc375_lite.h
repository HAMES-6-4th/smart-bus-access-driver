#ifndef BLE_UART_TC375_LITE_H
#define BLE_UART_TC375_LITE_H

#include "Ifx_Types.h"
#include "ble_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BleUartTc375LiteContextStruct
{
    uint8_t initialized;
    uint8_t has_pending_cmd;
    uint8_t pending_cmd;
} BleUartTc375LiteContext;

void bleUartTc375Init(void);
void bleUartTc375Poll(void);

uint8_t bleUartTc375IsInitialized(void);
uint8_t bleUartTc375ConsumeCmd(uint8_t* out_cmd);

const BleUartTc375LiteContext* bleUartTc375GetContext(void);



#ifdef __cplusplus
}
#endif

#endif