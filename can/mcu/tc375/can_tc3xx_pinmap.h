#ifndef CAN_TC3XX_PINMAP_H
#define CAN_TC3XX_PINMAP_H

#include "can_types.h"
#include "board_types.h"
#include "IfxCan_Can.h"

#ifdef __cplusplus
extern "C" {
#endif

CANStatus canTC3xxResolveModule(BoardMCUCANInst inst, Ifx_CAN **module);
CANStatus canTC3xxResolveNodeId(BoardMCUCANNode node, IfxCan_NodeId *node_id);
CANStatus canTC3xxResolvePins(const BoardCANPort *board_port, IfxCan_Can_Pins *pins);

#ifdef __cplusplus
}
#endif

#endif