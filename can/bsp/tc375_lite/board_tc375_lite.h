#ifndef BOARD_TC375_LITE_H
#define BOARD_TC375_LITE_H

#include "board_types.h"

BoardStatus boardImplInit(void);
const BoardDescriptor *boardImplGetDescriptor(void);

BoardStatus boardImplCANPortPowerOn(const BoardCANPort *port);
BoardStatus boardImplCANPortPowerOff(const BoardCANPort *port);
BoardStatus boardImplCANPortSetStandby(const BoardCANPort *port, bool enable);
BoardStatus boardImplCANPortSetTermination(const BoardCANPort *port, bool enable);

#endif