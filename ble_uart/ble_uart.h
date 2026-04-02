#ifndef BLE_UART_H
#define BLE_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define BLE_UART_SOF0              ((uint8_t)0xA5u)
#define BLE_UART_SOF1              ((uint8_t)0x5Au)
#define BLE_UART_FRAME_SIZE        ((uint8_t)6u)

#define BLE_UART_TYPE_DATA         ((uint8_t)0x01u)
#define BLE_UART_TYPE_ACK          ((uint8_t)0x02u)

/* request encoding: 2bit */
#define BLE_UART_REQ_NOP           ((uint8_t)0x00u)
#define BLE_UART_REQ_OPEN          ((uint8_t)0x01u)
#define BLE_UART_REQ_CLOSE         ((uint8_t)0x02u)

#define BLE_UART_DEFAULT_ACK_TIMEOUT_MS  ((uint32_t)20u)
#define BLE_UART_DEFAULT_RETRY_LIMIT     ((uint8_t)5u)

#define BLE_UART_PARSE_NONE        (0)
#define BLE_UART_PARSE_OK          (1)
#define BLE_UART_PARSE_ERROR       (-1)

#define BLE_UART_RX_NONE           ((uint8_t)0x00u)
#define BLE_UART_RX_SEND_ACK       ((uint8_t)0x01u)
#define BLE_UART_RX_DELIVER        ((uint8_t)0x02u)

#define BLE_UART_SENDER_POLL_NONE    (0)
#define BLE_UART_SENDER_POLL_RESEND  (1)
#define BLE_UART_SENDER_POLL_GIVEUP  (2)

typedef struct BleUartFrameStruct
{
    uint8_t bytes[BLE_UART_FRAME_SIZE];
} BleUartFrame;

typedef struct BleUartDecodedFrameStruct
{
    uint8_t type;
    uint8_t seq;
    uint8_t cmd;
} BleUartDecodedFrame;

typedef struct BleUartParserStruct
{
    uint8_t state;
    uint8_t type;
    uint8_t seq;
    uint8_t cmd;
} BleUartParser;

typedef struct BleUartSenderStruct
{
    uint8_t waiting_ack;
    uint8_t pending_seq;
    uint8_t retry_count;
    uint8_t retry_limit;
    uint8_t next_seq;
    uint32_t ack_timeout_ms;
    uint32_t last_tx_ms;
    BleUartFrame pending_frame;
} BleUartSender;

typedef struct BleUartReceiverStruct
{
    uint8_t has_last_seq;
    uint8_t last_seq;
} BleUartReceiver;

/* cmd layout:
 * bits[1:0] = door request
 * bits[3:2] = slope request
 */
uint8_t bleUartMakeCmd(uint8_t door_req, uint8_t slope_req);
uint8_t bleUartCmdGetDoor(uint8_t cmd);
uint8_t bleUartCmdGetSlope(uint8_t cmd);
uint8_t bleUartCmdIsValid(uint8_t cmd);

uint8_t bleUartCrc8(const uint8_t* data, size_t len);
void bleUartEncodeData(uint8_t seq, uint8_t cmd, BleUartFrame* out_frame);
void bleUartEncodeAck(uint8_t seq, BleUartFrame* out_frame);

void bleUartParserInit(BleUartParser* parser);
int bleUartParserFeed(BleUartParser* parser, uint8_t byte, BleUartDecodedFrame* out_frame);

void bleUartSenderInit(BleUartSender* sender, uint32_t ack_timeout_ms, uint8_t retry_limit);
uint8_t bleUartSenderIsBusy(const BleUartSender* sender);

int bleUartSenderStart(BleUartSender* sender, uint8_t cmd, uint32_t now_ms, BleUartFrame* out_frame);

int bleUartSenderOnFrame(
    BleUartSender* sender,
    const BleUartDecodedFrame* rx_frame
);

int bleUartSenderPoll(
    BleUartSender* sender,
    uint32_t now_ms,
    BleUartFrame* out_frame
);

void bleUartReceiverInit(BleUartReceiver* receiver);

uint8_t bleUartReceiverOnFrame(
    BleUartReceiver* receiver,
    const BleUartDecodedFrame* rx_frame,
    BleUartFrame* out_ack_frame,
    uint8_t* out_cmd_to_deliver
);

#ifdef __cplusplus
}
#endif

#endif