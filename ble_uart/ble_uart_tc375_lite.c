#include "ble_uart_tc375_lite.h"

#include "Asclin/Asc/IfxAsclin_Asc.h"
#include "IfxCpu_Irq.h"
#include "IfxPort.h"
#include "IfxSrc.h"

#define BLE_UART_TC375_ASC_BAUDRATE            ((uint32_t)9600u)

#define BLE_UART_TC375_ASC_TX_ISR_PRIORITY     (40u)
#define BLE_UART_TC375_ASC_RX_ISR_PRIORITY     (41u)
#define BLE_UART_TC375_ASC_ER_ISR_PRIORITY     (42u)

#define BLE_UART_TC375_ASC_TX_BUFFER_SIZE      (128u)
#define BLE_UART_TC375_ASC_RX_BUFFER_SIZE      (128u)

/*
 * TC375 Lite Kit - Shield2Go Connector 2 UART
 * RX = P20.3
 * TX = P20.0
 *
 * 필요 시 아래 두 줄만 바꿔서 다른 ASCLIN/pin pair로 교체 가능
 */
#define BLE_UART_TC375_ASC_MODULE              (&MODULE_ASCLIN3)
#define BLE_UART_TC375_ASC_RX_PIN              IfxAsclin3_RXC_P20_3_IN
#define BLE_UART_TC375_ASC_TX_PIN              IfxAsclin3_TX_P20_0_OUT

static IfxAsclin_Asc g_bleUartTc375Asc;

static uint8 g_bleUartTc375AscTxBuffer[BLE_UART_TC375_ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
static uint8 g_bleUartTc375AscRxBuffer[BLE_UART_TC375_ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];

static BleUartParser g_bleUartParser;
static BleUartReceiver g_bleUartReceiver;
static BleUartTc375LiteContext g_bleUartTc375Context;

IFX_INTERRUPT(bleUartTc375AscTxIsr, 0, BLE_UART_TC375_ASC_TX_ISR_PRIORITY);
IFX_INTERRUPT(bleUartTc375AscRxIsr, 0, BLE_UART_TC375_ASC_RX_ISR_PRIORITY);
IFX_INTERRUPT(bleUartTc375AscErIsr, 0, BLE_UART_TC375_ASC_ER_ISR_PRIORITY);

void bleUartTc375AscTxIsr(void)
{
    IfxAsclin_Asc_isrTransmit(&g_bleUartTc375Asc);
}

void bleUartTc375AscRxIsr(void)
{
    IfxAsclin_Asc_isrReceive(&g_bleUartTc375Asc);
}

void bleUartTc375AscErIsr(void)
{
    IfxAsclin_Asc_isrError(&g_bleUartTc375Asc);
}

static void bleUartTc375WriteFrame(const BleUartFrame* frame)
{
    Ifx_SizeT count = BLE_UART_FRAME_SIZE;
    (void)IfxAsclin_Asc_write(&g_bleUartTc375Asc, (void*)frame->bytes, &count, TIME_INFINITE);
}

static void bleUartTc375HandleDecodedFrame(const BleUartDecodedFrame* decoded)
{
    BleUartFrame ack_frame;
    uint8_t cmd_to_deliver = 0u;
    uint8_t flags;

    flags = bleUartReceiverOnFrame(
        &g_bleUartReceiver,
        decoded,
        &ack_frame,
        &cmd_to_deliver
    );

    if ((flags & BLE_UART_RX_SEND_ACK) != 0u)
    {
        bleUartTc375WriteFrame(&ack_frame);
    }

    if ((flags & BLE_UART_RX_DELIVER) != 0u)
    {
        g_bleUartTc375Context.pending_cmd = cmd_to_deliver;
        g_bleUartTc375Context.has_pending_cmd = 1u;
    }
}

void bleUartTc375Init(void)
{
    IfxAsclin_Asc_Config asc_config;

    IfxAsclin_Asc_initModuleConfig(&asc_config, BLE_UART_TC375_ASC_MODULE);

    asc_config.baudrate.baudrate = BLE_UART_TC375_ASC_BAUDRATE;

    asc_config.interrupt.txPriority = BLE_UART_TC375_ASC_TX_ISR_PRIORITY;
    asc_config.interrupt.rxPriority = BLE_UART_TC375_ASC_RX_ISR_PRIORITY;
    asc_config.interrupt.erPriority = BLE_UART_TC375_ASC_ER_ISR_PRIORITY;
    asc_config.interrupt.typeOfService = IfxSrc_Tos_cpu0;

    asc_config.txBuffer = g_bleUartTc375AscTxBuffer;
    asc_config.txBufferSize = BLE_UART_TC375_ASC_TX_BUFFER_SIZE;
    asc_config.rxBuffer = g_bleUartTc375AscRxBuffer;
    asc_config.rxBufferSize = BLE_UART_TC375_ASC_RX_BUFFER_SIZE;

    {
        static const IfxAsclin_Asc_Pins pins =
        {
            NULL_PTR,
            IfxPort_InputMode_pullUp,
            &BLE_UART_TC375_ASC_RX_PIN,
            IfxPort_InputMode_pullUp,
            NULL_PTR,
            IfxPort_OutputMode_pushPull,
            &BLE_UART_TC375_ASC_TX_PIN,
            IfxPort_OutputMode_pushPull,
            IfxPort_PadDriver_cmosAutomotiveSpeed1
        };

        asc_config.pins = &pins;
    }

    IfxAsclin_Asc_initModule(&g_bleUartTc375Asc, &asc_config);

    bleUartParserInit(&g_bleUartParser);
    bleUartReceiverInit(&g_bleUartReceiver);

    g_bleUartTc375Context.initialized = 1u;
    g_bleUartTc375Context.has_pending_cmd = 0u;
    g_bleUartTc375Context.pending_cmd = bleUartMakeCmd(BLE_UART_REQ_NOP, BLE_UART_REQ_NOP);
}

volatile    uint8 g_byte;
void bleUartTc375Poll(void)
{
    Ifx_SizeT count;
    BleUartDecodedFrame decoded;
    int parse_result;

    if (g_bleUartTc375Context.initialized == 0u)
    {
        return;
    }

    for (;;)
    {
        count = 1u;
        if (IfxAsclin_Asc_read(&g_bleUartTc375Asc, &g_byte, &count, 0u) != TRUE)
        {
            break;
        }

        parse_result = bleUartParserFeed(&g_bleUartParser, g_byte, &decoded);
        if (parse_result == BLE_UART_PARSE_OK)
        {
            bleUartTc375HandleDecodedFrame(&decoded);
        }
    }
}

uint8_t bleUartTc375IsInitialized(void)
{
    return g_bleUartTc375Context.initialized;
}

uint8_t bleUartTc375ConsumeCmd(uint8_t* out_cmd)
{
    if (out_cmd == NULL_PTR)
    {
        return 0u;
    }

    if (g_bleUartTc375Context.has_pending_cmd == 0u)
    {
        return 0u;
    }

    *out_cmd = g_bleUartTc375Context.pending_cmd;
    g_bleUartTc375Context.has_pending_cmd = 0u;
    return 1u;
}

const BleUartTc375LiteContext* bleUartTc375GetContext(void)
{
    return &g_bleUartTc375Context;
}
