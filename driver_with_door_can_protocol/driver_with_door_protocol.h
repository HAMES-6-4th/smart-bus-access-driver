#ifndef DRIVER_WITH_DOOR_PROTOCOL_H
#define DRIVER_WITH_DOOR_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file driver_with_door_protocol.h
 * @brief Driver ECU <-> Door ECU CAN protocol definitions and payload helpers.
 */

/**
 * @defgroup driver_with_door_protocol_api Door CAN Protocol API
 * @brief Driver ECU와 Door ECU 사이의 CAN 프로토콜 정의 및 helper입니다.
 * @{
 */

/** @name CAN Identifiers */
/** @{ */

/**
 * @brief CAN identifier used for command frames sent from the driver ECU to the door ECU.
 *
 * 운전석 ECU에서 도어 ECU로 보내는 command 프레임에 사용하는 CAN 식별자입니다.
 */
#define DOOR_CAN_ID_COMMAND   (0x100U)

/**
 * @brief CAN identifier used for status frames sent from the door ECU to the driver ECU.
 *
 * 도어 ECU에서 운전석 ECU로 보내는 status 프레임에 사용하는 CAN 식별자입니다.
 */
#define DOOR_CAN_ID_STATUS    (0x101U)

/** @} */

/** @name Enumerations */
/** @{ */

/**
 * @brief Door command field values carried in a command payload.
 *
 * command payload에 포함되는 door command 필드 값입니다.
 */
typedef enum DoorCommandEnum
{
    /** @brief No door action requested. / 도어 동작을 요청하지 않습니다. */
    DOOR_CMD_NOP   = 0,

    /** @brief Request door opening. / 도어 열림을 요청합니다. */
    DOOR_CMD_OPEN  = 1,

    /** @brief Request door closing. / 도어 닫힘을 요청합니다. */
    DOOR_CMD_CLOSE = 2,

    /** @brief Request door motion stop. / 도어 동작 정지를 요청합니다. */
    DOOR_CMD_STOP  = 3
} DoorCommand;

/**
 * @brief Ramp command field values carried in a command payload.
 *
 * command payload에 포함되는 ramp command 필드 값입니다.
 * 여기서 ramp는 slope와 동일한 의미로 사용합니다.
 */
typedef enum RampCommandEnum
{
    /** @brief No ramp action requested. / 램프 동작을 요청하지 않습니다. */
    RAMP_CMD_NOP    = 0,

    /** @brief Request ramp deployment. / 램프 전개를 요청합니다. */
    RAMP_CMD_DEPLOY = 1,

    /** @brief Request ramp stowing. / 램프 수납을 요청합니다. */
    RAMP_CMD_STOW   = 2,

    /** @brief Request ramp motion stop. / 램프 동작 정지를 요청합니다. */
    RAMP_CMD_STOP   = 3
} RampCommand;

/**
 * @brief Door state field values carried in a status payload.
 *
 * status payload에 포함되는 door state 필드 값입니다.
 */
typedef enum DoorStateEnum
{
    /** @brief The door is closed. / 도어가 닫힌 상태입니다. */
    DOOR_STATE_CLOSED = 0,

    /** @brief The door is opened. / 도어가 열린 상태입니다. */
    DOOR_STATE_OPENED = 1,

    /** @brief The door is moving. / 도어가 이동 중인 상태입니다. */
    DOOR_STATE_MOVING = 2,

    /** @brief The door is in a fault state. / 도어가 fault 상태입니다. */
    DOOR_STATE_FAULT  = 3
} DoorState;

/**
 * @brief Ramp state field values carried in a status payload.
 *
 * status payload에 포함되는 ramp state 필드 값입니다.
 * 여기서 ramp는 slope와 동일한 의미로 사용합니다.
 */
typedef enum RampStateEnum
{
    /** @brief The ramp is stowed. / 램프가 수납된 상태입니다. */
    RAMP_STATE_STOWED   = 0,

    /** @brief The ramp is deployed. / 램프가 전개된 상태입니다. */
    RAMP_STATE_DEPLOYED = 1,

    /** @brief The ramp is moving. / 램프가 이동 중인 상태입니다. */
    RAMP_STATE_MOVING   = 2,

    /** @brief The ramp is in a fault state. / 램프가 fault 상태입니다. */
    RAMP_STATE_FAULT    = 3
} RampState;

/** @} */

/** @name Payload Packing */
/** @{ */

/**
 * @brief Pack door/ramp command fields into one command payload byte.
 *
 * door/ramp command 필드들을 하나의 command payload 바이트로 패킹합니다.
 * command payload의 비트 배치는 아래와 같습니다.
 * - bit 0~1: door command
 * - bit 2~3: ramp command
 * - bit 4  : ramp manual flag
 * - bit 5  : emergency stop flag
 * - bit 6  : reset fault flag
 * - bit 7  : reserved
 *
 * @param door_cmd Door command field value. / door command 필드 값입니다.
 * @param ramp_cmd Ramp command field value. / ramp command 필드 값입니다.
 * @param ramp_manual True when the ramp command is manual, false when it is automatic.
 *                    ramp command가 수동이면 true, 자동이면 false입니다.
 * @param emergency_stop Emergency-stop flag. / 비상 정지 플래그입니다.
 * @param reset_fault Fault-reset flag. / fault reset 플래그입니다.
 *
 * @return Packed command payload byte. / 패킹된 command payload 바이트를 반환합니다.
 */
static inline uint8_t DriverWithDoorCanPackCommand(uint8_t door_cmd,
                                                   uint8_t ramp_cmd,
                                                   bool ramp_manual,
                                                   bool emergency_stop,
                                                   bool reset_fault)
{
    return (uint8_t)(
        ((door_cmd & 0x3u) << 0) |
        ((ramp_cmd & 0x3u) << 2) |
        ((ramp_manual ? 1u : 0u) << 4) |
        ((emergency_stop ? 1u : 0u) << 5) |
        ((reset_fault ? 1u : 0u) << 6));
}

/**
 * @brief Pack door/ramp status fields into one status payload byte.
 *
 * door/ramp status 필드들을 하나의 status payload 바이트로 패킹합니다.
 *
 * @param pinch Pinch-detection flag. / 끼임 감지 플래그입니다.
 * @param door_state Door state field value. / door state 필드 값입니다.
 * @param ramp_state Ramp state field value. / ramp state 필드 값입니다.
 * @param fault_present Fault-present flag. / fault 존재 플래그입니다.
 * @param alive_toggle Alive-toggle flag. / alive toggle 플래그입니다.
 *
 * @return Packed status payload byte. / 패킹된 status payload 바이트를 반환합니다.
 */
static inline uint8_t DriverWithDoorCanPackStatus(bool pinch,
                                                  uint8_t door_state,
                                                  uint8_t ramp_state,
                                                  bool fault_present,
                                                  bool alive_toggle)
{
    return (uint8_t)(
        ((pinch ? 1u : 0u) << 0) |
        ((door_state & 0x3u) << 1) |
        ((ramp_state & 0x3u) << 3) |
        ((fault_present ? 1u : 0u) << 5) |
        ((alive_toggle ? 1u : 0u) << 6));
}

/** @} */

/** @name Payload Unpacking */
/** @{ */

/**
 * @brief Extract the door command field from a command payload byte.
 *
 * command payload 바이트에서 door command 필드를 추출합니다.
 *
 * @param payload Packed command payload byte. / 패킹된 command payload 바이트입니다.
 * @return Door command field value. / door command 필드 값을 반환합니다.
 */
static inline uint8_t DriverWithDoorCanGetDoorCommand(uint8_t payload)
{
    return (uint8_t)((payload >> 0) & 0x3u);
}

/**
 * @brief Extract the ramp command field from a command payload byte.
 *
 * command payload 바이트에서 ramp command 필드를 추출합니다.
 *
 * @param payload Packed command payload byte. / 패킹된 command payload 바이트입니다.
 * @return Ramp command field value. / ramp command 필드 값을 반환합니다.
 */
static inline uint8_t DriverWithDoorCanGetRampCommand(uint8_t payload)
{
    return (uint8_t)((payload >> 2) & 0x3u);
}

/**
 * @brief Extract the ramp manual flag from a command payload byte.
 *
 * command payload 바이트에서 ramp manual 플래그를 추출합니다.
 * false는 자동, true는 수동을 의미합니다.
 *
 * @param payload Packed command payload byte. / 패킹된 command payload 바이트입니다.
 * @return true if the ramp command is manual, otherwise false.
 *         ramp command가 수동이면 true, 아니면 false를 반환합니다.
 */
static inline bool DriverWithDoorCanGetRampManual(uint8_t payload)
{
    return (((payload >> 4) & 0x1u) != 0u);
}

/**
 * @brief Extract the emergency-stop flag from a command payload byte.
 *
 * command payload 바이트에서 emergency-stop 플래그를 추출합니다.
 *
 * @param payload Packed command payload byte. / 패킹된 command payload 바이트입니다.
 * @return true if emergency stop is requested, otherwise false.
 *         emergency stop이 요청되었으면 true, 아니면 false를 반환합니다.
 */
static inline bool DriverWithDoorCanGetEmergencyStop(uint8_t payload)
{
    return (((payload >> 5) & 0x1u) != 0u);
}

/**
 * @brief Extract the fault-reset flag from a command payload byte.
 *
 * command payload 바이트에서 fault-reset 플래그를 추출합니다.
 *
 * @param payload Packed command payload byte. / 패킹된 command payload 바이트입니다.
 * @return true if fault reset is requested, otherwise false.
 *         fault reset이 요청되었으면 true, 아니면 false를 반환합니다.
 */
static inline bool DriverWithDoorCanGetResetFault(uint8_t payload)
{
    return (((payload >> 6) & 0x1u) != 0u);
}

/**
 * @brief Extract the pinch-detection flag from a status payload byte.
 *
 * status payload 바이트에서 끼임 감지 플래그를 추출합니다.
 *
 * @param payload Packed status payload byte. / 패킹된 status payload 바이트입니다.
 * @return true if pinch is reported, otherwise false.
 *         끼임이 보고되었으면 true, 아니면 false를 반환합니다.
 */
static inline bool DriverWithDoorCanGetPinch(uint8_t payload)
{
    return (((payload >> 0) & 0x1u) != 0u);
}

/**
 * @brief Extract the door state field from a status payload byte.
 *
 * status payload 바이트에서 door state 필드를 추출합니다.
 *
 * @param payload Packed status payload byte. / 패킹된 status payload 바이트입니다.
 * @return Door state field value. / door state 필드 값을 반환합니다.
 */
static inline uint8_t DriverWithDoorCanGetDoorState(uint8_t payload)
{
    return (uint8_t)((payload >> 1) & 0x3u);
}

/**
 * @brief Extract the ramp state field from a status payload byte.
 *
 * status payload 바이트에서 ramp state 필드를 추출합니다.
 *
 * @param payload Packed status payload byte. / 패킹된 status payload 바이트입니다.
 * @return Ramp state field value. / ramp state 필드 값을 반환합니다.
 */
static inline uint8_t DriverWithDoorCanGetRampState(uint8_t payload)
{
    return (uint8_t)((payload >> 3) & 0x3u);
}

/**
 * @brief Extract the fault-present flag from a status payload byte.
 *
 * status payload 바이트에서 fault-present 플래그를 추출합니다.
 *
 * @param payload Packed status payload byte. / 패킹된 status payload 바이트입니다.
 * @return true if a fault is reported, otherwise false.
 *         fault가 보고되었으면 true, 아니면 false를 반환합니다.
 */
static inline bool DriverWithDoorCanGetFaultPresent(uint8_t payload)
{
    return (((payload >> 5) & 0x1u) != 0u);
}

/**
 * @brief Extract the alive-toggle flag from a status payload byte.
 *
 * status payload 바이트에서 alive-toggle 플래그를 추출합니다.
 *
 * @param payload Packed status payload byte. / 패킹된 status payload 바이트입니다.
 * @return true if the alive-toggle bit is set, otherwise false.
 *         alive-toggle 비트가 설정되어 있으면 true, 아니면 false를 반환합니다.
 */
static inline bool DriverWithDoorCanGetAliveToggle(uint8_t payload)
{
    return (((payload >> 6) & 0x1u) != 0u);
}

/** @} */

/** @} */

#endif
