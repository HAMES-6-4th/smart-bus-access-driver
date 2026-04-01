#ifndef DOOR_ECU_CAN_H
#define DOOR_ECU_CAN_H

#include <stdbool.h>
#include <stdint.h>

#include "can_socket.h"
#include "driver_with_door_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file door_ecu_can.h
 * @brief Door-side CAN wrapper for command reception and status publication.
 */

/**
 * @defgroup door_ecu_can_api Door ECU CAN API
 * @brief 도어 ECU 측 CAN wrapper API
 * @{
 */

/** @name Data Types */
/** @{ */

/**
 * @brief 
 * Open-time configuration for the door-side CAN wrapper.
 * 
 * 도어 측 CAN 래퍼의 open 시점 설정입니다.
 */
typedef struct DoorECUCANConfigStruct
{
    const char *port_name;
    uint32_t nominal_bitrate;
} DoorECUCANConfig;

/**
 * @brief 
 * Latest command information received from the driver ECU.
 * 
 * 운전석 ECU로부터 마지막으로 수신한 command 정보입니다.
 */
typedef struct DoorECUCANCommandSnapshotStruct
{
    bool command_received;
    uint32_t last_command_rx_ms;

    uint8_t door_command;
    uint8_t ramp_command;
    bool emergency_stop;
    bool reset_fault;
} DoorECUCANCommandSnapshot;

/**
 * @brief 
 * Cached status fields that will be published toward the driver ECU.
 * 
 * 운전석 ECU 쪽으로 publish할 status 필드를 캐시하는 구조체입니다.
 */
typedef struct DoorECUCANStatusCacheStruct
{
    bool pinch_detected;
    uint8_t door_state;
    uint8_t ramp_state;
    bool fault_present;
    bool alive_toggle;
} DoorECUCANStatusCache;

/**
 * @brief 
 * Door-side CAN wrapper for receiving commands and publishing status.
 * The object stores a CANSocket, performs no allocation,
 * and opens/closes the underlying CAN connection through CANSocket.
 * 
 * command 수신과 status publish를 위한 도어 측 CAN 래퍼입니다.
 * 이 객체는 CANSocket을 보관하고 동적 할당을 수행하지 않으며,
 * CANSocket을 통해 하위 CAN 연결을 열고 닫습니다.
 */
typedef struct DoorECUCANStruct
{
    CANSocket socket;

    DoorECUCANCommandSnapshot command;
    DoorECUCANStatusCache status_cache;
} DoorECUCAN;

/** @} */

/** @name Initialization and Connection */
/** @{ */


/**
 * @brief 
 * Initialize a DoorECUCAN object to its default closed state.
 * 
 * DoorECUCAN 객체를 기본 closed 상태로 초기화합니다.
 *
 * @param ecu 
 * Object to initialize.
 * 초기화할 객체입니다.
 */
void doorECUCANInit(DoorECUCAN *ecu);

/**
 * @brief 
 * Initialize door ECU CAN configuration with default values.
 * 
 * door ECU CAN 설정을 기본값으로 초기화합니다.
 *
 * @param config 
 * Configuration object to initialize.
 * 초기화할 설정 객체입니다.
 */
void doorECUCANInitConfig(DoorECUCANConfig *config);





/**
 * @brief 
 * Open the door-side CAN wrapper.
 * 
 * 도어 측 CAN 래퍼를 엽니다.
 *
 * @param ecu 
 * Wrapper object to open.
 * 열 대상 래퍼 객체입니다.
 *
 * @param config 
 * Open-time configuration.
 * open 시점 설정입니다.
 *
 * @retval CAN_STATUS_OK
 * open succeeded.
 * 열기에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid configuration.
 * 잘못된 인자이거나 설정이 유효하지 않습니다.
 * 
 * @retval CAN_STATUS_EBUSY
 * the wrapper is already open.
 * 래퍼가 이미 열린 상태입니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying CAN layer.
 * 하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus doorECUCANOpen(DoorECUCAN *ecu, const DoorECUCANConfig *config);

/**
 * @brief 
 * Close the door-side CAN wrapper.
 * 
 * 도어 측 CAN 래퍼를 닫습니다.
 *
 * @param ecu 
 * Wrapper object to close.
 * 닫을 래퍼 객체입니다.
 *
 * @retval CAN_STATUS_OK
 * close succeeded.
 * 닫기에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying CAN layer.
 * 하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus doorECUCANClose(DoorECUCAN *ecu);



/** @} */

/** @name Command Reception */
/** @{ */

/**
 * @brief 
 * Poll for one command frame and update the cached snapshot when received.
 * This function performs one immediate receive attempt.
 * If no new frame is available, it returns CAN_STATUS_OK with updated left false.
 * 
 * command 프레임 하나를 polling하여 수신 시 캐시된 snapshot을 갱신합니다.
 * 이 함수는 즉시 1회의 수신 시도를 수행합니다.
 * 새 프레임이 없으면 updated를 false로 둔 채 CAN_STATUS_OK를 반환합니다.
 *
 * @param ecu 
 * Wrapper object.
 * 래퍼 객체입니다.
 *
 * @param now_ms 
 * Current monotonic time in milliseconds used to timestamp the snapshot.
 * snapshot 시각 기록에 사용하는 현재 단조 증가 시간(ms)입니다.
 *
 * @param updated 
 * Optional output that becomes true when a new command frame is consumed.
 * 새로운 command 프레임을 소비했을 때 true가 되는 선택적 출력 포인터입니다.
 *
 * @retval CAN_STATUS_OK
 * poll completed successfully, including the case where no new frame was consumed.
 * polling이 정상적으로 수행되었으며, 새 프레임이 없었던 경우도 포함합니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying CAN layer.
 * 하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus doorECUCANPollCommand(DoorECUCAN *ecu,
                                uint32_t now_ms,
                                bool *updated);





/**
 * @brief 
 * Get the cached command snapshot.
 * 
 * 캐시된 command snapshot을 조회합니다.
 *
 * @param ecu 
 * Wrapper object.
 * 래퍼 객체입니다.
 *
 * @return 
 * Pointer to the cached command snapshot, or null if the argument is invalid.
 * 캐시된 command snapshot 포인터를 반환하며, 인자가 잘못되었으면 null을 반환합니다.
 */
const DoorECUCANCommandSnapshot *doorECUCANGetCommand(const DoorECUCAN *ecu);





/**
 * @brief 
 * Check whether the cached command snapshot is still considered alive.
 * 
 * 캐시된 command snapshot이 아직 유효(alive)한지 확인합니다.
 *
 * @param ecu 
 * Wrapper object.
 * 래퍼 객체입니다.
 *
 * @param now_ms 
 * Current monotonic time in milliseconds.
 * 현재 단조 증가 시간(ms)입니다.
 *
 * @param timeout_ms 
 * Alive timeout in milliseconds.
 * alive 판정 timeout(ms)입니다.
 *
 * @return 
 * true if a recent command frame was received within timeout_ms, otherwise false.
 * timeout_ms 이내에 최근 command 프레임이 수신되었으면 true, 아니면 false를 반환합니다.
 */
bool doorECUCANIsCommandAlive(const DoorECUCAN *ecu,
                              uint32_t now_ms,
                              uint32_t timeout_ms);





/**
 * @brief 
 * Overwrite the cached command snapshot with a safe fallback command.
 * 
 * 캐시된 command snapshot을 안전한 fallback command로 덮어씁니다.
 *
 * @param ecu 
 * Wrapper object.
 * 래퍼 객체입니다.
 */
void doorECUCANForceSafeCommand(DoorECUCAN *ecu);



/** @} */

/** @name Status Publication */
/** @{ */

/**
 * @brief 
 * Update the status cache and publish one status frame to the driver ECU.
 * 
 * status cache를 갱신하고 운전석 ECU로 status 프레임 하나를 publish합니다.
 *
 * @param ecu 
 * Wrapper object.
 * 래퍼 객체입니다.
 *
 * @param pinch_detected 
 * Pinch-detection flag to publish.
 * publish할 끼임 감지 플래그입니다.
 *
 * @param door_state 
 * Door state field value to publish.
 * publish할 door state 필드 값입니다.
 *
 * @param ramp_state 
 * Ramp state field value to publish.
 * publish할 ramp state 필드 값입니다.
 *
 * @param fault_present 
 * Fault-present flag to publish.
 * publish할 fault 존재 플래그입니다.
 *
 * @retval CAN_STATUS_OK
 * publish succeeded.
 * publish에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid state field value.
 * 잘못된 인자이거나 state 필드 값이 유효하지 않습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying CAN layer.
 * 하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus doorECUCANPublishStatus(DoorECUCAN *ecu,
                                  bool pinch_detected,
                                  uint8_t door_state,
                                  uint8_t ramp_state,
                                  bool fault_present);

                                  



/**
 * @brief 
 * Get the cached status fields that will be published.
 * 
 * publish 대상인 캐시된 status 필드를 조회합니다.
 *
 * @param ecu 
 * Wrapper object.
 * 래퍼 객체입니다.
 *
 * @return 
 * Pointer to the cached status cache, or null if the argument is invalid.
 * 캐시된 status cache 포인터를 반환하며, 인자가 잘못되었으면 null을 반환합니다.
 */
const DoorECUCANStatusCache *doorECUCANGetStatusCache(const DoorECUCAN *ecu);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif