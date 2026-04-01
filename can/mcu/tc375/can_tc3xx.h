#ifndef CAN_TC3XX_H
#define CAN_TC3XX_H

#include "can_core.h"
#include "can_types.h"
#include "board_types.h"
#include "IfxCan_Can.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_TC3XX_MAX_MODULES    (2U)
#define CAN_TC3XX_MAX_NODES      (4U)

typedef struct CANTC3xxModuleContextStruct
{
    bool is_initialized;
    uint8_t ref_count;
    IfxCan_Can can_module;
} CANTC3xxModuleContext;

typedef struct CANTC3xxChannelStruct
{
    bool is_open;
    bool is_started;

    bool use_rx_fifo0;

    const BoardCANPort *board_port;

    BoardMCUCANInst module_inst;
    BoardMCUCANNode node;

    uint8_t module_index;
    CANTC3xxModuleContext *module_context;

    IfxCan_Can_Node can_node;

    uint32_t message_ram_base_address;
    uint16_t message_ram_node_offset;

    uint8_t tx_buffer_number;
    uint8_t rx_buffer_number;
    uint8_t filter_number;

    CANChannelConfig config;
} CANTC3xxChannel;

void canTC3xxChannelInit(CANTC3xxChannel *channel);
CANStatus canTC3xxOpen(CANTC3xxChannel *channel, const BoardCANPort *board_port, const CANChannelConfig *config);

CANStatus canTC3xxClose(CANTC3xxChannel *channel);

CANStatus canTC3xxStart(CANTC3xxChannel *channel);
CANStatus canTC3xxStop(CANTC3xxChannel *channel);

CANStatus canTC3xxSend(CANTC3xxChannel *channel, const CANFrame *frame);
CANStatus canTC3xxReceive(CANTC3xxChannel *channel, CANFrame *frame);

CANStatus canTC3xxGetErrorState(CANTC3xxChannel *channel, CANCoreErrorState *state);
CANStatus canTC3xxRecover(CANTC3xxChannel *channel);

#ifdef __cplusplus
}
#endif

#endif