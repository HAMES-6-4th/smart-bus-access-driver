#include "can_platform.h"

#include <string.h>

#include "board_api.h"
#include "board_types.h"
#include "can_tc3xx.h"
#include "can_tc3xx_core_glue.h"

typedef struct CANPlatformSlotStruct
{
    bool in_use;
    CANCore *owner_core;

    const BoardCANPort *board_port;
    CANCoreBinding binding;
    CANTC3xxChannel tc3xx_channel;
} CANPlatformSlot;

static bool g_CANPlatformInitialized = false;
static CANPlatformSlot g_CANPlatformSlots[CAN_PLATFORM_MAX_CHANNELS];

static void CANPlatformClearSlots(void)
{
    uint32_t i;

    for (i = 0U; i < CAN_PLATFORM_MAX_CHANNELS; ++i)
    {
        memset(&g_CANPlatformSlots[i], 0, sizeof(g_CANPlatformSlots[i]));
        canTC3xxChannelInit(&g_CANPlatformSlots[i].tc3xx_channel);
    }
}

static CANPlatformSlot *CANPlatformFindSlotByCore(const CANCore *core)
{
    uint32_t i;

    if (core == 0)
    {
        return 0;
    }

    for (i = 0U; i < CAN_PLATFORM_MAX_CHANNELS; ++i)
    {
        if ((g_CANPlatformSlots[i].in_use == true) &&
            (g_CANPlatformSlots[i].owner_core == core))
        {
            return &g_CANPlatformSlots[i];
        }
    }

    return 0;
}

static CANPlatformSlot *CANPlatformAllocateSlot(void)
{
    uint32_t i;

    for (i = 0U; i < CAN_PLATFORM_MAX_CHANNELS; ++i)
    {
        if (g_CANPlatformSlots[i].in_use == false)
        {
            g_CANPlatformSlots[i].in_use = true;
            return &g_CANPlatformSlots[i];
        }
    }

    return 0;
}

static void CANPlatformReleaseSlot(CANPlatformSlot *slot)
{
    if (slot == 0)
    {
        return;
    }

    memset(slot, 0, sizeof(*slot));
    canTC3xxChannelInit(&slot->tc3xx_channel);
}

CANStatus CANPlatformInit(void)
{
    BoardStatus board_status;

    board_status = boardInit();
    if (board_status != BOARD_STATUS_OK)
    {
        return CAN_STATUS_EIO;
    }

    CANPlatformClearSlots();
    g_CANPlatformInitialized = true;
    return CAN_STATUS_OK;
}

void CANPlatformDeinit(void)
{
    CANPlatformClearSlots();
    g_CANPlatformInitialized = false;
}

CANStatus CANPlatformOpen(CANCore *core,
                          const char *port_name,
                          const CANCoreOpenParams *params)
{
    const BoardCANPort *board_port;
    CANPlatformSlot *slot;
    CANStatus status;

    if ((core == 0) || (port_name == 0) || (params == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (g_CANPlatformInitialized == false)
    {
        return CAN_STATUS_EBUSY;
    }

    if (CANPlatformFindSlotByCore(core) != 0)
    {
        return CAN_STATUS_EBUSY;
    }

    board_port = boardFindCANPort(port_name);
    if (board_port == 0)
    {
        return CAN_STATUS_ENOTFOUND;
    }

    slot = CANPlatformAllocateSlot();
    if (slot == 0)
    {
        return CAN_STATUS_ENOSPC;
    }

    slot->owner_core = core;
    slot->board_port = board_port;

    CANTC3xxCoreBindingInit(&slot->binding,
                            board_port->name,
                            &slot->tc3xx_channel,
                            board_port);

    CANCoreInit(core);

    status = CANCoreOpen(core, &slot->binding, params);
    if (status != CAN_STATUS_OK)
    {
        CANPlatformReleaseSlot(slot);
        return status;
    }

    return CAN_STATUS_OK;
}

CANStatus CANPlatformClose(CANCore *core)
{
    CANPlatformSlot *slot;
    CANStatus status;

    if (core == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    slot = CANPlatformFindSlotByCore(core);
    if (slot == 0)
    {
        return CAN_STATUS_ENOTFOUND;
    }

    status = CANCoreClose(core);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    CANPlatformReleaseSlot(slot);
    return CAN_STATUS_OK;
}

const char *CANPlatformGetBoundPortName(const CANCore *core)
{
    CANPlatformSlot *slot = CANPlatformFindSlotByCore(core);

    if ((slot == 0) || (slot->board_port == 0))
    {
        return 0;
    }

    return slot->board_port->name;
}

void CANPlatformGetStats(CANPlatformStats *stats)
{
    uint32_t i;
    uint32_t used = 0U;

    if (stats == 0)
    {
        return;
    }

    for (i = 0U; i < CAN_PLATFORM_MAX_CHANNELS; ++i)
    {
        if (g_CANPlatformSlots[i].in_use == true)
        {
            used++;
        }
    }

    stats->total_slots = CAN_PLATFORM_MAX_CHANNELS;
    stats->used_slots = used;
}