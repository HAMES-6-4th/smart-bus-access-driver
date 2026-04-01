#ifndef BOARD_GPIO_H
#define BOARD_GPIO_H

#include <stdbool.h>
#include "board_types.h"

#ifdef __cplusplus
extern "C" {
#endif

BoardStatus boardGPIOConfigOutput(BoardGPIO gpio, bool initialHigh);
BoardStatus boardGPIOWrite(BoardGPIO gpio, bool high);
BoardStatus boardGPIORead(BoardGPIO gpio, bool *state);

#ifdef __cplusplus
}
#endif

#endif