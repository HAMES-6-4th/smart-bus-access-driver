\mainpage Driver ECU With Door ECU CAN API

운전석 ECU와 도어 ECU 간 CAN 통신 래퍼 및 프로토콜 문서입니다.

# 개요

이 문서는 다음 세 영역으로 구성됩니다.

- \ref driver_ecu_can_api "Driver ECU CAN API"
- \ref door_ecu_can_api "Door ECU CAN API"
- \ref driver_with_door_protocol_api "Door CAN Protocol API"

Driver ECU는 command 프레임을 송신하고, Door ECU는 이를 polling하여 수신합니다.  
Door ECU는 status 프레임을 publish하고, Driver ECU는 이를 polling하여 최신 상태 snapshot을 유지합니다.

# 모듈 구성

## \ref driver_ecu_can_api "Driver ECU CAN API"

운전석 ECU 측 CAN wrapper입니다.

대표 API:
- \ref driverECUCANInit
- \ref driverECUCANInitConfig
- \ref driverECUCANOpen
- \ref driverECUCANClose
- \ref driverECUCANSendCommand
- \ref driverECUCANPollStatus
- \ref driverECUCANGetStatus
- \ref driverECUCANIsStatusAlive

## \ref door_ecu_can_api "Door ECU CAN API"

도어 ECU 측 CAN wrapper입니다.

대표 API:
- \ref doorECUCANInit
- \ref doorECUCANInitConfig
- \ref doorECUCANOpen
- \ref doorECUCANClose
- \ref doorECUCANPollCommand
- \ref doorECUCANGetCommand
- \ref doorECUCANIsCommandAlive
- \ref doorECUCANForceSafeCommand
- \ref doorECUCANPublishStatus
- \ref doorECUCANGetStatusCache

## \ref driver_with_door_protocol_api "Door CAN Protocol API"

CAN ID, command/state enum, payload pack/unpack helper를 정의합니다.

대표 항목:
- \ref DOOR_CAN_ID_COMMAND
- \ref DOOR_CAN_ID_STATUS
- \ref DoorCommand
- \ref RampCommand
- \ref DoorState
- \ref RampState
- \ref DriverWithDoorCanPackCommand
- \ref DriverWithDoorCanPackStatus

# 통신 흐름

1. Driver ECU가 command를 송신합니다.
2. Door ECU가 command를 polling하여 수신합니다.
3. Door ECU가 status를 publish합니다.
4. Driver ECU가 status를 polling하여 갱신합니다.

# CAN 식별자

- Command frame ID: `0x100`
- Status frame ID: `0x101`

# 문서 범위

- 공개 API
- CAN payload bit field helper
- command / state enum 정의
- 상태 snapshot / cache 구조체

# 파일 목록

- `driver_ecu_can.h`
- `door_ecu_can.h`
- `driver_with_door_protocol.h`