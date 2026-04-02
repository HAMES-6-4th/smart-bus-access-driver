#ifndef DRIVER_ECU_CAN_H
#define DRIVER_ECU_CAN_H

#include <stdbool.h>
#include <stdint.h>

#include "can_socket.h"
#include "driver_with_door_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file driver_ecu_can.h
 * @brief Driver-side CAN wrapper for door ECU communication.
 */

/**
 * @defgroup driver_ecu_can_api Driver ECU CAN API
 * @brief 운전석 ECU 측 CAN wrapper API입니다.
 * @{
 */

/** @name Data Types */
/** @{ */

/**
 * @brief Open-time configuration for the driver-side door CAN wrapper.
 *
 * 운전석 측 door CAN 래퍼의 open 시점 설정입니다.
 */
typedef struct DriverECUCANConfigStruct
{
    const char *port_name;
    uint32_t nominal_bitrate;
} DriverECUCANConfig;

/**
 * @brief Latest status information received from the door ECU.
 *
 * door ECU로부터 마지막으로 수신한 status 정보입니다.
 */
typedef struct DriverECUCANStatusSnapshotStruct
{
    bool status_received;
    uint32_t last_status_rx_ms;

    bool pinch_detected;
    uint8_t door_state;
    uint8_t ramp_state;
    bool fault_present;
    bool alive_toggle;
} DriverECUCANStatusSnapshot;

/**
 * @brief Driver-side CAN wrapper for sending commands and tracking door status.
 *
 * command 송신과 door status 추적을 위한 운전석 측 CAN 래퍼입니다.
 * 이 객체는 CANSocket을 보관하고 동적 할당을 수행하지 않습니다.
 */
typedef struct DriverECUCANStruct
{
    CANSocket socket;
    DriverECUCANStatusSnapshot status;
    bool is_open;
} DriverECUCAN;

/** @} */

/** @name Initialization and Connection */
/** @{ */

/**
 * @brief Initialize a DriverECUCAN object to its default closed state.
 *
 * DriverECUCAN 객체를 기본 closed 상태로 초기화합니다.
 *
 * @param ecu Object to initialize. / 초기화할 객체입니다.
 */
void driverECUCANInit(DriverECUCAN *ecu);

/**
 * @brief Initialize driver ECU CAN configuration with default values.
 *
 * driver ECU CAN 설정을 기본값으로 초기화합니다.
 *
 * @param config Configuration object to initialize. / 초기화할 설정 객체입니다.
 */
void driverECUCANInitConfig(DriverECUCANConfig *config);

/**
 * @brief Open the driver-side door CAN wrapper.
 *
 * 운전석 측 door CAN 래퍼를 엽니다.
 *
 * @param ecu Wrapper object to open. / 열 대상 래퍼 객체입니다.
 * @param config Open-time configuration. / open 시점 설정입니다.
 *
 * @retval CAN_STATUS_OK open succeeded. / 열기에 성공했습니다.
 * @retval CAN_STATUS_EINVAL invalid argument or configuration. / 잘못된 인자이거나 설정이 유효하지 않습니다.
 * @retval CAN_STATUS_EBUSY the wrapper is already open. / 래퍼가 이미 열린 상태입니다.
 * @retval other other CANStatus values may be returned from the underlying CAN layer.
 *               하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus driverECUCANOpen(DriverECUCAN *ecu, const DriverECUCANConfig *config);

/**
 * @brief Close the driver-side door CAN wrapper.
 *
 * 운전석 측 door CAN 래퍼를 닫습니다.
 *
 * @param ecu Wrapper object to close. / 닫을 래퍼 객체입니다.
 *
 * @retval CAN_STATUS_OK close succeeded. / 닫기에 성공했습니다.
 * @retval CAN_STATUS_EINVAL invalid argument. / 잘못된 인자입니다.
 * @retval other other CANStatus values may be returned from the underlying CAN layer.
 *               하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus driverECUCANClose(DriverECUCAN *ecu);

/** @} */

/** @name Command Transmission */
/** @{ */

/**
 * @brief Pack and send one command frame to the door ECU.
 *
 * door ECU로 보낼 command 프레임 하나를 패킹하여 송신합니다.
 * ramp_manual은 slope 명령이 수동인지 자동인지 구분하기 위한 비트입니다.
 * door command는 기존과 동일하게 별도 mode 없이 사용합니다.
 *
 * @param ecu Wrapper object. / 래퍼 객체입니다.
 * @param door_cmd Door command field value. / door command 필드 값입니다.
 * @param ramp_cmd Ramp command field value. / ramp command 필드 값입니다.
 * @param ramp_manual True when the ramp command is manual, false when automatic.
 *                    ramp command가 수동이면 true, 자동이면 false입니다.
 * @param emergency_stop Emergency-stop flag. / 비상 정지 플래그입니다.
 * @param reset_fault Fault-reset flag. / fault reset 플래그입니다.
 *
 * @retval CAN_STATUS_OK send succeeded. / 송신에 성공했습니다.
 * @retval CAN_STATUS_EINVAL invalid argument or invalid command field value.
 *                           잘못된 인자이거나 command 필드 값이 유효하지 않습니다.
 * @retval other other CANStatus values may be returned from the underlying CAN layer.
 *               하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus driverECUCANSendCommand(DriverECUCAN *ecu,
                                  uint8_t door_cmd,
                                  uint8_t ramp_cmd,
                                  bool ramp_manual,
                                  bool emergency_stop,
                                  bool reset_fault);

/**
 * @brief Poll for one status frame and update the cached snapshot when received.
 *
 * status 프레임 하나를 polling하여 수신 시 캐시된 snapshot을 갱신합니다.
 * 이 함수는 즉시 1회의 수신 시도를 수행합니다.
 * 새 프레임이 없으면 updated를 false로 둔 채 CAN_STATUS_OK를 반환합니다.
 *
 * @param ecu Wrapper object. / 래퍼 객체입니다.
 * @param now_ms Current monotonic time in milliseconds used to timestamp the snapshot.
 *               snapshot 시각 기록에 사용하는 현재 단조 증가 시간(ms)입니다.
 * @param updated Optional output that becomes true when a new status frame is consumed.
 *                새로운 status 프레임을 소비했을 때 true가 되는 선택적 출력 포인터입니다.
 *
 * @retval CAN_STATUS_OK poll completed successfully. / polling이 정상적으로 수행되었습니다.
 * @retval CAN_STATUS_EINVAL invalid argument. / 잘못된 인자입니다.
 * @retval other other CANStatus values may be returned from the underlying CAN layer.
 *               하위 CAN 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus driverECUCANPollStatus(DriverECUCAN *ecu,
                                 uint32_t now_ms,
                                 bool *updated);

/** @} */

/** @name Status Reception */
/** @{ */

/**
 * @brief Get the cached door-status snapshot.
 *
 * 캐시된 door status snapshot을 조회합니다.
 *
 * @param ecu Wrapper object. / 래퍼 객체입니다.
 * @return Pointer to the cached status snapshot, or null if invalid.
 *         캐시된 status snapshot 포인터를 반환하며, 인자가 잘못되었으면 null을 반환합니다.
 */
const DriverECUCANStatusSnapshot *driverECUCANGetStatus(const DriverECUCAN *ecu);

/**
 * @brief Check whether the cached status snapshot is still considered alive.
 *
 * 캐시된 status snapshot이 아직 유효(alive)한지 확인합니다.
 *
 * @param ecu Wrapper object. / 래퍼 객체입니다.
 * @param now_ms Current monotonic time in milliseconds. / 현재 단조 증가 시간(ms)입니다.
 * @param timeout_ms Alive timeout in milliseconds. / alive 판정 timeout(ms)입니다.
 *
 * @return true if a recent status frame was received within timeout_ms, otherwise false.
 *         timeout_ms 이내에 최근 status 프레임이 수신되었으면 true, 아니면 false를 반환합니다.
 */
bool driverECUCANIsStatusAlive(const DriverECUCAN *ecu,
                               uint32_t now_ms,
                               uint32_t timeout_ms);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
