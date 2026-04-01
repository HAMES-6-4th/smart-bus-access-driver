#ifndef CAN_TC3XX_CORE_GLUE_H
#define CAN_TC3XX_CORE_GLUE_H

#include "can_core.h"
#include "can_tc3xx.h"
#include "board_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void CANTC3xxCoreBindingInit(CANCoreBinding *binding, const char *name, CANTC3xxChannel *channel, const BoardCANPort *board_port);

#ifdef __cplusplus
}
#endif

#endif