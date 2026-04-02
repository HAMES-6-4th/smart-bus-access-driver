#ifndef STATE_H_
#define STATE_H_

typedef enum
{
    STATE_STOP_BTN_OFF = 0,
    STATE_STOP_BTN_ON,
} StopBtnState;

typedef enum
{
    STATE_DISABLED_STOP_BTN_OFF = 0,
    STATE_DISABLED_STOP_BTN_ON,
} DisabledStopBtnState;

typedef enum
{
    STATE_DOOR_NONE = 0,
    STATE_DOOR_OPEN,
    STATE_DOOR_CLOSE
} DoorState_t;

typedef enum
{
    STATE_SLOPE_NONE = 0,
    STATE_SLOPE_OPEN,
    STATE_SLOPE_CLOSE,
} SlopeState;

#endif
