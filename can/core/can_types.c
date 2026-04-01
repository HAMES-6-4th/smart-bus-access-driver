#include <string.h>
#include "can_types.h"

void CANFrameInit(CANFrame *frame,
                  CANIdType id_type,
                  CANMode mode,
                  uint32_t id,
                  uint8_t len)
{
    if (frame == 0)
    {
        return;
    }

    memset(frame, 0, sizeof(*frame));

    frame->id_type = id_type;
    frame->mode = mode;
    frame->id = id;

    if (len > 64U)
    {
        len = 64U;
    }

    frame->len = len;
}

CANStatus CANFrameSetData(CANFrame *frame,
                          const void *data,
                          uint8_t len)
{
    if (frame == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if ((data == 0) && (len > 0U))
    {
        return CAN_STATUS_EINVAL;
    }

    if (len > 64U)
    {
        return CAN_STATUS_EINVAL;
    }

    if (len > 0U)
    {
        memcpy(frame->data, data, len);
    }

    if (len < 64U)
    {
        memset(&frame->data[len], 0, (uint32_t)(64U - len));
    }

    frame->len = len;
    return CAN_STATUS_OK;
}

void CANFrameInitClassicStd(CANFrame *frame,
                            uint32_t id,
                            uint8_t len)
{
    CANFrameInit(frame,
                 CAN_ID_STANDARD,
                 CAN_MODE_CLASSIC,
                 id,
                 len);
}

void CANFrameInitClassicExt(CANFrame *frame,
                            uint32_t id,
                            uint8_t len)
{
    CANFrameInit(frame,
                 CAN_ID_EXTENDED,
                 CAN_MODE_CLASSIC,
                 id,
                 len);
}

void CANFrameInitFdStd(CANFrame *frame,
                       uint32_t id,
                       uint8_t len,
                       bool enable_brs)
{
    CANFrameInit(frame,
                 CAN_ID_STANDARD,
                 (enable_brs == true) ? CAN_MODE_FD_BRS : CAN_MODE_FD_NO_BRS,
                 id,
                 len);
}

void CANFrameInitFdExt(CANFrame *frame,
                       uint32_t id,
                       uint8_t len,
                       bool enable_brs)
{
    CANFrameInit(frame,
                 CAN_ID_EXTENDED,
                 (enable_brs == true) ? CAN_MODE_FD_BRS : CAN_MODE_FD_NO_BRS,
                 id,
                 len);
}