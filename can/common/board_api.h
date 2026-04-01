#ifndef BOARD_API_H
#define BOARD_API_H

#include "board_types.h"

#ifdef __cplusplus
extern "C" {
#endif

BoardStatus boardInit(void);
const BoardDescriptor *boardGetDescriptor(void);
const BoardCANPort *boardFindCANPort(const char *name);

/* 보드가 가진 외부 트랜시버/종단 제어용 */
BoardStatus boardCANPortPowerOn(const BoardCANPort *port);
BoardStatus boardCANPortPowerOff(const BoardCANPort *port);

BoardStatus boardCANPortSetStandby(const BoardCANPort *port, bool enable);
BoardStatus boardCANPortSetTermination(const BoardCANPort *port, bool enable);

#ifdef __cplusplus
}
#endif

#endif