#include "ble_uart.h"

enum
{
    DS_UART_PARSER_WAIT_SOF0 = 0,
    DS_UART_PARSER_WAIT_SOF1,
    DS_UART_PARSER_WAIT_TYPE,
    DS_UART_PARSER_WAIT_SEQ,
    DS_UART_PARSER_WAIT_CMD,
    DS_UART_PARSER_WAIT_CRC
};

uint8_t bleUartMakeCmd(uint8_t door_req, uint8_t slope_req)
{
    return (uint8_t)((door_req & 0x03u) | ((slope_req & 0x03u) << 2));
}

uint8_t bleUartCmdGetDoor(uint8_t cmd)
{
    return (uint8_t)(cmd & 0x03u);
}

uint8_t bleUartCmdGetSlope(uint8_t cmd)
{
    return (uint8_t)((cmd >> 2) & 0x03u);
}

uint8_t bleUartCmdIsValid(uint8_t cmd)
{
    uint8_t door;
    uint8_t slope;

    if ((cmd & 0xF0u) != 0u)
    {
        return 0u;
    }

    door = bleUartCmdGetDoor(cmd);
    slope = bleUartCmdGetSlope(cmd);

    if (door > BLE_UART_REQ_CLOSE)
    {
        return 0u;
    }

    if (slope > BLE_UART_REQ_CLOSE)
    {
        return 0u;
    }

    return 1u;
}

uint8_t bleUartCrc8(const uint8_t* data, size_t len)
{
    uint8_t crc = 0x00u;
    size_t i;
    uint8_t bit;

    for (i = 0; i < len; ++i)
    {
        crc ^= data[i];

        for (bit = 0; bit < 8u; ++bit)
        {
            if ((crc & 0x80u) != 0u)
            {
                crc = (uint8_t)((crc << 1) ^ 0x07u);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void bleUartEncodeData(uint8_t seq, uint8_t cmd, BleUartFrame* out_frame)
{
    uint8_t crc_input[3];

    out_frame->bytes[0] = BLE_UART_SOF0;
    out_frame->bytes[1] = BLE_UART_SOF1;
    out_frame->bytes[2] = BLE_UART_TYPE_DATA;
    out_frame->bytes[3] = seq;
    out_frame->bytes[4] = cmd;

    crc_input[0] = out_frame->bytes[2];
    crc_input[1] = out_frame->bytes[3];
    crc_input[2] = out_frame->bytes[4];

    out_frame->bytes[5] = bleUartCrc8(crc_input, 3u);
}

void bleUartEncodeAck(uint8_t seq, BleUartFrame* out_frame)
{
    uint8_t crc_input[3];

    out_frame->bytes[0] = BLE_UART_SOF0;
    out_frame->bytes[1] = BLE_UART_SOF1;
    out_frame->bytes[2] = BLE_UART_TYPE_ACK;
    out_frame->bytes[3] = seq;
    out_frame->bytes[4] = 0x00u;

    crc_input[0] = out_frame->bytes[2];
    crc_input[1] = out_frame->bytes[3];
    crc_input[2] = out_frame->bytes[4];

    out_frame->bytes[5] = bleUartCrc8(crc_input, 3u);
}

void bleUartParserInit(BleUartParser* parser)
{
    parser->state = DS_UART_PARSER_WAIT_SOF0;
    parser->type = 0u;
    parser->seq = 0u;
    parser->cmd = 0u;
}

int bleUartParserFeed(BleUartParser* parser, uint8_t byte, BleUartDecodedFrame* out_frame)
{
    uint8_t crc_input[3];
    uint8_t crc;

    switch (parser->state)
    {
    case DS_UART_PARSER_WAIT_SOF0:
        if (byte == BLE_UART_SOF0)
        {
            parser->state = DS_UART_PARSER_WAIT_SOF1;
        }
        return BLE_UART_PARSE_NONE;

    case DS_UART_PARSER_WAIT_SOF1:
        if (byte == BLE_UART_SOF1)
        {
            parser->state = DS_UART_PARSER_WAIT_TYPE;
        }
        else if (byte == BLE_UART_SOF0)
        {
            parser->state = DS_UART_PARSER_WAIT_SOF1;
        }
        else
        {
            parser->state = DS_UART_PARSER_WAIT_SOF0;
        }
        return BLE_UART_PARSE_NONE;

    case DS_UART_PARSER_WAIT_TYPE:
        parser->type = byte;
        parser->state = DS_UART_PARSER_WAIT_SEQ;
        return BLE_UART_PARSE_NONE;

    case DS_UART_PARSER_WAIT_SEQ:
        parser->seq = byte;
        parser->state = DS_UART_PARSER_WAIT_CMD;
        return BLE_UART_PARSE_NONE;

    case DS_UART_PARSER_WAIT_CMD:
        parser->cmd = byte;
        parser->state = DS_UART_PARSER_WAIT_CRC;
        return BLE_UART_PARSE_NONE;

    case DS_UART_PARSER_WAIT_CRC:
        crc_input[0] = parser->type;
        crc_input[1] = parser->seq;
        crc_input[2] = parser->cmd;
        crc = bleUartCrc8(crc_input, 3u);

        parser->state = DS_UART_PARSER_WAIT_SOF0;

        if (crc != byte)
        {
            return BLE_UART_PARSE_ERROR;
        }

        if ((parser->type != BLE_UART_TYPE_DATA) &&
            (parser->type != BLE_UART_TYPE_ACK))
        {
            return BLE_UART_PARSE_ERROR;
        }

        out_frame->type = parser->type;
        out_frame->seq = parser->seq;
        out_frame->cmd = parser->cmd;

        return BLE_UART_PARSE_OK;

    default:
        parser->state = DS_UART_PARSER_WAIT_SOF0;
        return BLE_UART_PARSE_ERROR;
    }
}

void bleUartSenderInit(BleUartSender* sender, uint32_t ack_timeout_ms, uint8_t retry_limit)
{
    sender->waiting_ack = 0u;
    sender->pending_seq = 0u;
    sender->retry_count = 0u;
    sender->retry_limit = (retry_limit == 0u) ? BLE_UART_DEFAULT_RETRY_LIMIT : retry_limit;
    sender->next_seq = 0u;
    sender->ack_timeout_ms = (ack_timeout_ms == 0u) ? BLE_UART_DEFAULT_ACK_TIMEOUT_MS : ack_timeout_ms;
    sender->last_tx_ms = 0u;
}

uint8_t bleUartSenderIsBusy(const BleUartSender* sender)
{
    return sender->waiting_ack;
}

int bleUartSenderStart(
    BleUartSender* sender,
    uint8_t cmd,
    uint32_t now_ms,
    BleUartFrame* out_frame
)
{
    uint8_t seq;

    if (sender->waiting_ack != 0u)
    {
        return 0;
    }

    if (bleUartCmdIsValid(cmd) == 0u)
    {
        return 0;
    }

    seq = sender->next_seq;
    sender->next_seq = (uint8_t)(sender->next_seq + 1u);

    bleUartEncodeData(seq, cmd, &sender->pending_frame);

    sender->waiting_ack = 1u;
    sender->pending_seq = seq;
    sender->retry_count = 0u;
    sender->last_tx_ms = now_ms;

    *out_frame = sender->pending_frame;
    return 1;
}

int bleUartSenderOnFrame(
    BleUartSender* sender,
    const BleUartDecodedFrame* rx_frame
)
{
    if (sender->waiting_ack == 0u)
    {
        return 0;
    }

    if (rx_frame->type != BLE_UART_TYPE_ACK)
    {
        return 0;
    }

    if (rx_frame->seq != sender->pending_seq)
    {
        return 0;
    }

    sender->waiting_ack = 0u;
    return 1;
}

int bleUartSenderPoll(
    BleUartSender* sender,
    uint32_t now_ms,
    BleUartFrame* out_frame
)
{
    uint32_t elapsed;

    if (sender->waiting_ack == 0u)
    {
        return BLE_UART_SENDER_POLL_NONE;
    }

    elapsed = (uint32_t)(now_ms - sender->last_tx_ms);
    if (elapsed < sender->ack_timeout_ms)
    {
        return BLE_UART_SENDER_POLL_NONE;
    }

    if (sender->retry_count < sender->retry_limit)
    {
        sender->retry_count = (uint8_t)(sender->retry_count + 1u);
        sender->last_tx_ms = now_ms;
        *out_frame = sender->pending_frame;
        return BLE_UART_SENDER_POLL_RESEND;
    }

    sender->waiting_ack = 0u;
    return BLE_UART_SENDER_POLL_GIVEUP;
}

void bleUartReceiverInit(BleUartReceiver* receiver)
{
    receiver->has_last_seq = 0u;
    receiver->last_seq = 0u;
}

uint8_t bleUartReceiverOnFrame(
    BleUartReceiver* receiver,
    const BleUartDecodedFrame* rx_frame,
    BleUartFrame* out_ack_frame,
    uint8_t* out_cmd_to_deliver
)
{
    uint8_t flags = BLE_UART_RX_NONE;

    if (rx_frame->type != BLE_UART_TYPE_DATA)
    {
        return BLE_UART_RX_NONE;
    }

    if (bleUartCmdIsValid(rx_frame->cmd) == 0u)
    {
        return BLE_UART_RX_NONE;
    }

    bleUartEncodeAck(rx_frame->seq, out_ack_frame);
    flags |= BLE_UART_RX_SEND_ACK;

    if ((receiver->has_last_seq == 0u) ||
        (receiver->last_seq != rx_frame->seq))
    {
        receiver->has_last_seq = 1u;
        receiver->last_seq = rx_frame->seq;
        *out_cmd_to_deliver = rx_frame->cmd;
        flags |= BLE_UART_RX_DELIVER;
    }

    return flags;
}