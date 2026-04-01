#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * Common status codes used across the CAN library.
 * 
 * CAN 라이브러리 전반에서 공통으로 사용하는 상태 코드입니다.
 */
typedef enum CANStatusEnum
{
    /**
     * @brief 
     * Operation completed successfully.
     * 
     * 동작이 성공적으로 완료되었습니다.
     */
    CAN_STATUS_OK = 0,

    /**
     * @brief 
     * An argument, configuration, or state is invalid.
     * 
     * 인자, 설정, 또는 상태가 유효하지 않습니다.
     */
    CAN_STATUS_EINVAL,

    /**
     * @brief 
     * A requested object or resource was not found.
     * 
     * 요청한 객체 또는 자원을 찾을 수 없습니다.
     */
    CAN_STATUS_ENOTFOUND,

    /**
     * @brief 
     * The requested operation is not supported by the current binding or driver.
     * 
     * 요청한 동작이 현재 바인딩 또는 드라이버에서 지원되지 않습니다.
     */
    CAN_STATUS_EUNSUPPORTED,

    /**
     * @brief 
     * The object is busy or the requested operation cannot proceed right now.
     * 
     * 객체가 busy 상태이거나 요청한 동작을 현재 진행할 수 없습니다.
     */
    CAN_STATUS_EBUSY,

    /**
     * @brief 
     * No space or free slot is available.
     * 
     * 사용할 수 있는 공간 또는 빈 슬롯이 없습니다.
     */
    CAN_STATUS_ENOSPC,

    /**
     * @brief 
     * No data is currently available.
     * 
     * 현재 사용 가능한 데이터가 없습니다.
     */
    CAN_STATUS_ENODATA,

    /**
     * @brief 
     * The operation did not complete before the timeout expired.
     * 
     * timeout이 만료되기 전에 동작이 완료되지 못했습니다.
     */
    CAN_STATUS_ETIMEOUT,

    /**
     * @brief 
     * A low-level I/O or driver-related failure occurred.
     * 
     * 저수준 I/O 또는 드라이버 관련 오류가 발생했습니다.
     */
    CAN_STATUS_EIO
} CANStatus;

/**
 * @brief 
 * CAN frame transmission mode.
 * 
 * CAN 프레임 전송 모드입니다.
 */
typedef enum CANModeEnum
{
    /**
     * @brief 
     * Classical CAN mode.
     * 
     * Classical CAN 모드입니다.
     */
    CAN_MODE_CLASSIC = 0,

    /**
     * @brief 
     * CAN FD mode without bit-rate switching.
     * 
     * bit-rate switching 없이 동작하는 CAN FD 모드입니다.
     */
    CAN_MODE_FD_NO_BRS,
    
    /**
     * @brief 
     * CAN FD mode with bit-rate switching.
     * 
     * bit-rate switching을 사용하는 CAN FD 모드입니다.
     */
    CAN_MODE_FD_BRS
} CANMode;

/**
 * @brief 
 * CAN identifier format.
 * 
 * CAN 식별자 형식입니다.
 */
typedef enum CANIdTypeEnum
{
    /**
     * @brief 
     * Standard 11-bit CAN identifier.
     * 
     * 표준 11비트 CAN 식별자입니다.
     */
    CAN_ID_STANDARD = 0,

    /**
     * @brief 
     * Extended 29-bit CAN identifier.
     * 
     * 확장 29비트 CAN 식별자입니다.
     */
    CAN_ID_EXTENDED
} CANIdType;

/**
 * @brief 
 * Receive path selection for an opened CAN channel.
 * 
 * 열린 CAN 채널에서 사용할 수신 경로 선택값입니다.
 */
typedef enum CANRxPathEnum
{
    /**
     * @brief 
     * Use the library default receive path.
     * 
     * 라이브러리 기본 수신 경로를 사용합니다.
     */
    CAN_RX_PATH_DEFAULT = 0,

    /**
     * @brief 
     * Use FIFO0 as the receive path.
     * 
     * 수신 경로로 FIFO0를 사용합니다.
     */
    CAN_RX_PATH_FIFO0,

    /**
     * @brief 
     * Use a dedicated receive buffer as the receive path.
     * 
     * 수신 경로로 전용 receive buffer를 사용합니다.
     */
    CAN_RX_PATH_BUFFER
} CANRxPath;

/**
 * @brief 
 * Bitrate and mode configuration for a CAN channel.
 * 
 * CAN 채널의 bitrate 및 모드 설정입니다.
 */
typedef struct CANTimingConfigStruct
{
    /**
     * @brief 
     * Nominal bitrate in bits per second.
     * Used for arbitration phase timing.
     * 
     * 초당 비트 수(bps) 단위의 nominal bitrate입니다.
     * arbitration 구간 타이밍에 사용됩니다.
     */
    uint32_t nominal_bitrate;

    /**
     * @brief 
     * Data-phase bitrate in bits per second.
     * Used only for CAN FD data phase.
     * 
     * 초당 비트 수(bps) 단위의 data-phase bitrate입니다.
     * CAN FD의 data 구간에서만 사용됩니다.
     */
    uint32_t data_bitrate;

    /**
     * @brief 
     * Requested CAN transmission mode.
     * 
     * 사용할 CAN 전송 모드입니다.
     */
    CANMode mode;
} CANTimingConfig;

/**
 * @brief 
 * Receive acceptance filter configuration.
 * 
 * 수신 acceptance filter 설정입니다.
 */
typedef struct CANAcceptanceFilterStruct
{
    /**
     * @brief 
     * Enables or disables the receive filter.
     * 
     * 수신 필터 사용 여부를 나타냅니다.
     */
    bool enabled;

    /**
     * @brief 
     * Identifier type matched by the filter.
     * 
     * 필터가 매칭할 식별자 형식입니다.
     */
    CANIdType id_type;

    /**
     * @brief 
     * Base identifier value used by the filter.
     * 
     * 필터에서 사용하는 기준 식별자 값입니다.
     */
    uint32_t id;

    /**
     * @brief 
     * Identifier mask used for filter matching.
     * 
     * 필터 매칭에 사용하는 식별자 마스크입니다.
     */
    uint32_t mask;
} CANAcceptanceFilter;

/**
 * @brief 
 * Open-time channel configuration for a CAN core or wrapper.
 * 
 * CAN core 또는 래퍼의 open 시점 채널 설정입니다.
 */
typedef struct CANChannelConfigStruct
{
    /**
     * @brief 
     * Timing and bitrate configuration.
     * 
     * 타이밍 및 bitrate 설정입니다.
     */
    CANTimingConfig timing;

    /**
     * @brief 
     * Enables loopback mode when supported by the binding.
     * 
     * 바인딩이 지원하는 경우 loopback 모드를 활성화합니다.
     */
    bool enable_loopback;

    /**
     * @brief 
     * Receive path selection.
     * 
     * 수신 경로 선택값입니다.
     */
    CANRxPath rx_path;

    /**
     * @brief 
     * Receive filter configuration.
     * 
     * 수신 필터 설정입니다.
     */
    CANAcceptanceFilter rx_filter;
} CANChannelConfig;

/**
 * @brief 
 * Generic CAN / CAN FD frame container used by the public API.
 * The frame uses caller-owned storage and does not allocate memory.
 * 
 * 공개 API에서 사용하는 범용 CAN / CAN FD 프레임 컨테이너입니다.
 * 이 프레임은 caller-owned 저장소를 사용하며 동적 할당을 수행하지 않습니다.
 */
typedef struct CANFrameStruct
{
    /**
     * @brief 
     * Identifier format of the frame.
     * 
     * 프레임의 식별자 형식입니다.
     */
    CANIdType id_type;

    /**
     * @brief 
     * Transmission mode of the frame.
     * 
     * 프레임의 전송 모드입니다.
     */
    CANMode mode;

    /**
     * @brief 
     * CAN identifier value.
     * 
     * CAN 식별자 값입니다.
     */
    uint32_t id;

    /**
     * @brief 
     * Payload length in bytes.
     * 
     * 바이트 단위 payload 길이입니다.
     */
    uint8_t len;

    /**
     * @brief 
     * Frame payload buffer.
     * Supports up to 64 bytes for CAN FD.
     * 
     * 프레임 payload 버퍼입니다.
     * CAN FD 기준 최대 64바이트를 지원합니다.
     */
    uint8_t data[64];
} CANFrame;

/**
 * @brief 
 * Initialize a CANFrame with the requested identifier type, mode, ID, and payload length.
 * The frame payload buffer is zero-initialized.
 * No dynamic allocation is performed.
 * 
 * 요청한 식별자 형식, 모드, ID, payload 길이로 CANFrame을 초기화합니다.
 * 프레임 payload 버퍼는 0으로 초기화됩니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param frame 
 * Frame object to initialize.
 * 초기화할 프레임 객체입니다.
 *
 * @param id_type 
 * Identifier format of the frame.
 * 프레임의 식별자 형식입니다.
 *
 * @param mode 
 * Transmission mode of the frame.
 * 프레임의 전송 모드입니다.
 *
 * @param id 
 * CAN identifier value.
 * CAN 식별자 값입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 */
void CANFrameInit(CANFrame *frame,
                  CANIdType id_type,
                  CANMode mode,
                  uint32_t id,
                  uint8_t len);

/**
 * @brief 
 * Copy payload data into a CANFrame and update the frame length.
 * Data is copied into the caller-owned frame buffer.
 * No dynamic allocation is performed.
 * 
 * payload 데이터를 CANFrame에 복사하고 프레임 길이를 갱신합니다.
 * 데이터는 caller-owned 프레임 버퍼에 복사됩니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param frame 
 * Destination frame object.
 * 목적지 프레임 객체입니다.
 *
 * @param data 
 * Source payload buffer.
 * 원본 payload 버퍼입니다.
 *
 * @param len 
 * Number of bytes to copy.
 * 복사할 바이트 수입니다.
 *
 * @retval CAN_STATUS_OK
 * data copy succeeded.
 * 데이터 복사에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid payload length.
 * 잘못된 인자이거나 payload 길이가 유효하지 않습니다.
 */
CANStatus CANFrameSetData(CANFrame *frame,
                          const void *data,
                          uint8_t len);

/**
 * @brief 
 * Initialize a standard-ID Classical CAN frame with zeroed payload.
 * 
 * 표준 ID Classical CAN 프레임을 payload 0 초기화 상태로 초기화합니다.
 *
 * @param frame 
 * Frame object to initialize.
 * 초기화할 프레임 객체입니다.
 *
 * @param id 
 * Standard CAN identifier.
 * 표준 CAN 식별자입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 */
void CANFrameInitClassicStd(CANFrame *frame,
                            uint32_t id,
                            uint8_t len);

/**
 * @brief 
 * Initialize an extended-ID Classical CAN frame with zeroed payload.
 * 
 * 확장 ID Classical CAN 프레임을 payload 0 초기화 상태로 초기화합니다.
 *
 * @param frame 
 * Frame object to initialize.
 * 초기화할 프레임 객체입니다.
 *
 * @param id 
 * Extended CAN identifier.
 * 확장 CAN 식별자입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 */
void CANFrameInitClassicExt(CANFrame *frame,
                            uint32_t id,
                            uint8_t len);

/**
 * @brief 
 * Initialize a standard-ID CAN FD frame with zeroed payload.
 * 
 * 표준 ID CAN FD 프레임을 payload 0 초기화 상태로 초기화합니다.
 *
 * @param frame 
 * Frame object to initialize.
 * 초기화할 프레임 객체입니다.
 *
 * @param id 
 * Standard CAN identifier.
 * 표준 CAN 식별자입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 *
 * @param enable_brs 
 * true to use bit-rate switching, otherwise false.
 * bit-rate switching을 사용하면 true, 아니면 false입니다.
 */
void CANFrameInitFdStd(CANFrame *frame,
                       uint32_t id,
                       uint8_t len,
                       bool enable_brs);

/**
 * @brief 
 * Initialize an extended-ID CAN FD frame with zeroed payload.
 * 
 * 확장 ID CAN FD 프레임을 payload 0 초기화 상태로 초기화합니다.
 *
 * @param frame 
 * Frame object to initialize.
 * 초기화할 프레임 객체입니다.
 *
 * @param id 
 * Extended CAN identifier.
 * 확장 CAN 식별자입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 *
 * @param enable_brs 
 * true to use bit-rate switching, otherwise false.
 * bit-rate switching을 사용하면 true, 아니면 false입니다.
 */
void CANFrameInitFdExt(CANFrame *frame,
                       uint32_t id,
                       uint8_t len,
                       bool enable_brs);

#ifdef __cplusplus
}
#endif

#endif