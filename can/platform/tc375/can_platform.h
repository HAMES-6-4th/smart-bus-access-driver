#ifndef CAN_PLATFORM_H
#define CAN_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_PLATFORM_MAX_CHANNELS   (8U)

typedef struct CANPlatformStatsStruct
{
    uint32_t total_slots;
    uint32_t used_slots;
} CANPlatformStats;

CANStatus CANPlatformInit(void);
void CANPlatformDeinit(void);

CANStatus CANPlatformOpen(CANCore *core,
                          const char *port_name,
                          const CANCoreOpenParams *params);

CANStatus CANPlatformClose(CANCore *core);

const char *CANPlatformGetBoundPortName(const CANCore *core);
void CANPlatformGetStats(CANPlatformStats *stats);

#ifdef __cplusplus
}
#endif

#endif