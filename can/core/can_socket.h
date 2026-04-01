/**
 * @file can_socket.h
 * @brief 
 * Minimal socket-like facade over CANCore.
 * This header provides the simplest public polling-oriented API for
 * CAN send/receive/poll style usage, including optional owning open/close helpers.
 * A socket may either bind to an external core or manage an internally owned core.
 * 
 * CANCore 위에 제공되는 최소 소켓형 파사드입니다.
 * 이 헤더는 단순한 CAN send/receive/poll 스타일 사용을 위한
 * 가장 간단한 공개 polling 기반 API와 선택적인 owning open/close helper를 제공합니다.
 * socket은 외부 core에 바인딩될 수도 있고 내부적으로 소유한 core를 관리할 수도 있습니다.
 */

#ifndef CAN_SOCKET_H
#define CAN_SOCKET_H

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CANSocket
 *
 * purpose
 * - minimal socket-like facade over CANCore
 * - thin forwarding layer for simple synchronous / poll-oriented usage
 * - no allocation
 * - supports both:
 *   1) non-owning bind to an external CANCore
 *   2) simple owning open/close using embedded core storage
 *
 * recommended public shorthand surface
 * - shortest synchronous / poll-oriented API
 *
 * intended usage
 * 1) CANSocketInit
 * 2) either:
 *    - CANSocketBindCore
 *    - or CANSocketOpen
 * 3) CANSocketSend / Receive / Poll / Timeout APIs
 * 4) optionally use sugar helpers:
 *    CANSocketSendNow / ReceiveNow / WaitTxReady / WaitRxReady
 */




/**
 * @brief 
 * Open-time parameter bundle used by CANSocketOpen().
 * This structure carries port selection, channel configuration,
 * runtime callbacks, hooks, and flags for the owning open path.
 * 
 * CANSocketOpen()에서 사용하는 open 시점 파라미터 묶음입니다.
 * 이 구조체는 owning open 경로를 위한 포트 선택, 채널 설정,
 * runtime callback, hook, flag를 담습니다.
 */
typedef struct CANSocketOpenParamsStruct
{
    const char *port_name;
    CANChannelConfig channel_config;
    CANCoreRuntime runtime;
    const CANCoreHooks *hooks;
    uint32_t flags;
} CANSocketOpenParams;

/**
 * @brief 
 * Minimal socket-like wrapper over CANCore.
 * The socket may either reference an externally bound core
 * or manage an internally owned core opened through CANSocketOpen().
 * No dynamic allocation is performed.
 * 
 * CANCore 위의 최소 소켓형 래퍼입니다.
 * socket은 외부에서 바인딩된 core를 참조할 수도 있고,
 * CANSocketOpen()을 통해 내부적으로 소유한 core를 관리할 수도 있습니다.
 * 동적 할당은 수행하지 않습니다.
 */
typedef struct CANSocketStruct
{
    CANCore *core;

    CANCore owned_core;
    bool owns_core;
    bool platform_opened;
} CANSocket;





/**
 * @brief 
 * Initialize a CANSocket object to its default unbound state.
 * This resets both external-binding state and internally owned core state.
 * No dynamic allocation is performed.
 * 
 * CANSocket 객체를 기본 바인딩되지 않은 상태로 초기화합니다.
 * 이 함수는 외부 바인딩 상태와 내부 소유 core 상태를 모두 초기화합니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object to initialize.
 * 초기화할 socket 객체입니다.
 */
void CANSocketInit(CANSocket *socket);





/**
 * @brief 
 * Initialize CANSocket open parameters with default values.
 * 
 * CANSocket open 파라미터를 기본값으로 초기화합니다.
 *
 * @param params 
 * Open parameter structure to initialize.
 * 초기화할 open 파라미터 구조체입니다.
 */
void CANSocketInitOpenParams(CANSocketOpenParams *params);

/**
 * @brief 
 * Initialize CANSocket open parameters for a common Classical CAN 500 kbps setup.
 * This preset configures Classical CAN, 500 kbps nominal bitrate,
 * loopback disabled, and default accept-all receive path settings.
 * 
 * 일반적인 Classical CAN 500 kbps 설정으로 CANSocket open 파라미터를 초기화합니다.
 * 이 preset은 Classical CAN, 500 kbps nominal bitrate,
 * loopback 비활성화, 그리고 기본 accept-all 수신 경로 설정을 구성합니다.
 *
 * @param params 
 * Open parameter structure to initialize.
 * 초기화할 open 파라미터 구조체입니다.
 */
void CANSocketInitOpenParamsClassic500k(CANSocketOpenParams *params);

/**
 * @brief 
 * Initialize CANSocket open parameters for a common CAN FD 500 kbps / 2 Mbps setup.
 * This preset configures CAN FD with BRS enabled, 500 kbps nominal bitrate,
 * 2 Mbps data bitrate, loopback disabled, and default accept-all receive path settings.
 * 
 * 일반적인 CAN FD 500 kbps / 2 Mbps 설정으로 CANSocket open 파라미터를 초기화합니다.
 * 이 preset은 BRS가 활성화된 CAN FD, 500 kbps nominal bitrate,
 * 2 Mbps data bitrate, loopback 비활성화, 그리고 기본 accept-all 수신 경로 설정을 구성합니다.
 *
 * @param params 
 * Open parameter structure to initialize.
 * 초기화할 open 파라미터 구조체입니다.
 */
void CANSocketInitOpenParamsFd500k2M(CANSocketOpenParams *params);






/**
 * @brief 
 * Open and start a CANSocket using embedded core storage.
 * This is a convenience helper for simple owning usage.
 * The socket opens the requested platform port, starts CAN communication,
 * and binds itself to the internally owned core.
 * No dynamic allocation is performed.
 * 
 * 내장된 core 저장소를 사용하여 CANSocket을 열고 시작합니다.
 * 이 함수는 단순한 owning 사용을 위한 convenience helper입니다.
 * socket은 요청한 플랫폼 포트를 열고 CAN 통신을 시작한 뒤,
 * 내부적으로 소유한 core에 자신을 바인딩합니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object to open.
 * 열 대상 socket 객체입니다.
 *
 * @param params 
 * Open-time parameters.
 * open 시점 파라미터입니다.
 *
 * @retval CAN_STATUS_OK
 * open and start succeeded.
 * 열기와 시작에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid open parameters.
 * 잘못된 인자이거나 open 파라미터가 유효하지 않습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the socket is already bound or already opened.
 * socket이 이미 바인딩되어 있거나 이미 열린 상태입니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying platform or core layer.
 * 하위 platform 또는 core 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketOpen(CANSocket *socket, const CANSocketOpenParams *params);

/**
 * @brief 
 * Stop and close a CANSocket previously opened by CANSocketOpen().
 * This releases the internally owned core binding and resets the socket
 * back to an unbound state.
 * No dynamic allocation is performed.
 * 
 * 이전에 CANSocketOpen()으로 연 CANSocket을 중지하고 닫습니다.
 * 이 함수는 내부적으로 소유한 core 바인딩을 해제하고,
 * socket을 다시 바인딩되지 않은 상태로 되돌립니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object to close.
 * 닫을 socket 객체입니다.
 *
 * @retval CAN_STATUS_OK
 * close succeeded.
 * 닫기에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or the socket is not open.
 * 잘못된 인자이거나 socket이 열린 상태가 아닙니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * the socket is bound to an external core and is not owning the current binding.
 * socket이 외부 core에 바인딩되어 있으며 현재 바인딩을 소유하지 않습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying platform or core layer.
 * 하위 platform 또는 core 계층으로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketClose(CANSocket *socket);





/**
 * @brief 
 * Bind an external CANCore to a CANSocket without transferring ownership.
 * This function is intended for a fresh socket or an already non-owning socket
 * bound to the same external core.
 * It must not be used to overwrite an active owning-open socket or to rebind
 * a socket that is already bound to a different core.
 * 
 * 외부 CANCore를 소유권 이전 없이 CANSocket에 바인딩합니다.
 * 이 함수는 fresh socket 또는 이미 동일한 외부 core에 바인딩된
 * non-owning socket 용도로 사용해야 합니다.
 * active owning-open socket을 덮어쓰거나,
 * 이미 다른 core에 바인딩된 socket을 재바인딩하는 용도로 사용하면 안 됩니다.
 *
 * @param socket 
 * Socket object to update.
 * 갱신할 socket 객체입니다.
 * 
 * @param core 
 * External core object to bind. Ownership is not transferred.
 * 바인딩할 외부 core 객체입니다. 소유권은 이전되지 않습니다.
 *
 * @retval CAN_STATUS_OK
 * bind succeeded.
 * 바인딩에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the socket is in owning-open state or already bound to a different core.
 * socket이 owning-open 상태이거나 이미 다른 core에 바인딩되어 있습니다.
 */
CANStatus CANSocketBindCore(CANSocket *socket, CANCore *core);

/**
 * @brief 
 * Remove a non-owning external binding from a CANSocket and return it to the default unbound state.
 * This function only detaches the socket wrapper and does not close or stop the external core.
 * It must not be used on an owning-open socket.
 * 
 * CANSocket의 non-owning 외부 바인딩을 해제하고 기본 unbound 상태로 되돌립니다.
 * 이 함수는 socket wrapper만 분리하며 외부 core를 close하거나 stop하지 않습니다.
 * owning-open socket에는 사용하면 안 됩니다.
 *
 * @param socket
 * Socket object to update.
 * 갱신할 socket 객체입니다.
 *
 * @retval CAN_STATUS_OK
 * unbind succeeded.
 * 바인딩 해제에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the socket is currently in owning-open state.
 * socket이 현재 owning-open 상태입니다.
 */
CANStatus CANSocketUnbind(CANSocket *socket);





/**
 * @brief 
 * Get the mutable core currently associated with the socket.
 * The returned pointer may refer to an externally bound core or an internally owned core.
 * 
 * socket에 현재 연결된 수정 가능한 core를 조회합니다.
 * 반환되는 포인터는 외부 바인딩 core일 수도 있고 내부 소유 core일 수도 있습니다.
 *
 * @param socket 
 * Socket object.
 * 조회할 socket 객체입니다.
 *
 * @return
 * Associated core pointer, or null if unbound or invalid.
 * 연결된 core 포인터를 반환하며, 바인딩되지 않았거나 인자가 잘못되었으면 null을 반환합니다.
 */
CANCore *CANSocketGetCore(CANSocket *socket);

/**
 * @brief 
 * Get the const core currently associated with the socket.
 * The returned pointer may refer to an externally bound core or an internally owned core.
 * 
 * socket에 현재 연결된 const core를 조회합니다.
 * 반환되는 포인터는 외부 바인딩 core일 수도 있고 내부 소유 core일 수도 있습니다.
 *
 * @param socket 
 * Socket object.
 * 조회할 socket 객체입니다.
 *
 * @return
 * Associated const core pointer, or null if unbound or invalid.
 * 연결된 const core 포인터를 반환하며, 바인딩되지 않았거나 인자가 잘못되었으면 null을 반환합니다.
 */
const CANCore *CANSocketGetCoreConst(const CANSocket *socket);





/**
 * @brief 
 * Send one CAN frame through the bound core.
 * 바인딩된 core를 통해 CAN 프레임 하나를 송신합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_EBUSY
 * the core is not started or transmit is not currently possible.
 * core가 started 상태가 아니거나 현재 송신이 불가능합니다.
 */
CANStatus CANSocketSend(CANSocket *socket, const CANFrame *frame);

/**
 * @brief 
 * Build and send one standard-ID Classical CAN frame in a single call.
 * The helper initializes a temporary frame, copies the payload, and sends it immediately.
 * No dynamic allocation is performed.
 * 
 * 표준 ID Classical CAN 프레임 하나를 한 번에 구성하고 송신합니다.
 * 이 helper는 임시 프레임을 초기화하고 payload를 복사한 뒤 즉시 송신합니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object used for transmission.
 * 송신에 사용할 socket 객체입니다.
 *
 * @param id 
 * Standard CAN identifier.
 * 표준 CAN 식별자입니다.
 *
 * @param data 
 * Source payload buffer.
 * 원본 payload 버퍼입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 *
 * @retval CAN_STATUS_OK
 * frame construction and send succeeded.
 * 프레임 구성과 송신에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid payload length.
 * 잘못된 인자이거나 payload 길이가 유효하지 않습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying socket send path.
 * 하위 socket 송신 경로로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketSendClassicStd(CANSocket *socket,
                                  uint32_t id,
                                  const void *data,
                                  uint8_t len);

/**
 * @brief 
 * Build and send one extended-ID Classical CAN frame in a single call.
 * The helper initializes a temporary frame, copies the payload, and sends it immediately.
 * No dynamic allocation is performed.
 * 
 * 확장 ID Classical CAN 프레임 하나를 한 번에 구성하고 송신합니다.
 * 이 helper는 임시 프레임을 초기화하고 payload를 복사한 뒤 즉시 송신합니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object used for transmission.
 * 송신에 사용할 socket 객체입니다.
 *
 * @param id 
 * Extended CAN identifier.
 * 확장 CAN 식별자입니다.
 *
 * @param data 
 * Source payload buffer.
 * 원본 payload 버퍼입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 *
 * @retval CAN_STATUS_OK
 * frame construction and send succeeded.
 * 프레임 구성과 송신에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid payload length.
 * 잘못된 인자이거나 payload 길이가 유효하지 않습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying socket send path.
 * 하위 socket 송신 경로로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketSendClassicExt(CANSocket *socket,
                                  uint32_t id,
                                  const void *data,
                                  uint8_t len);

/**
 * @brief 
 * Build and send one standard-ID CAN FD frame in a single call.
 * The helper initializes a temporary frame, copies the payload,
 * selects FD mode based on enable_brs, and sends it immediately.
 * No dynamic allocation is performed.
 * 
 * 표준 ID CAN FD 프레임 하나를 한 번에 구성하고 송신합니다.
 * 이 helper는 임시 프레임을 초기화하고 payload를 복사한 뒤,
 * enable_brs에 따라 FD 모드를 선택하고 즉시 송신합니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object used for transmission.
 * 송신에 사용할 socket 객체입니다.
 *
 * @param id 
 * Standard CAN identifier.
 * 표준 CAN 식별자입니다.
 *
 * @param data 
 * Source payload buffer.
 * 원본 payload 버퍼입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 *
 * @param enable_brs 
 * true to use bit-rate switching, otherwise false.
 * bit-rate switching을 사용하면 true, 아니면 false입니다.
 *
 * @retval CAN_STATUS_OK
 * frame construction and send succeeded.
 * 프레임 구성과 송신에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid payload length.
 * 잘못된 인자이거나 payload 길이가 유효하지 않습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying socket send path.
 * 하위 socket 송신 경로로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketSendFdStd(CANSocket *socket,
                             uint32_t id,
                             const void *data,
                             uint8_t len,
                             bool enable_brs);

/**
 * @brief 
 * Build and send one extended-ID CAN FD frame in a single call.
 * The helper initializes a temporary frame, copies the payload,
 * selects FD mode based on enable_brs, and sends it immediately.
 * No dynamic allocation is performed.
 * 
 * 확장 ID CAN FD 프레임 하나를 한 번에 구성하고 송신합니다.
 * 이 helper는 임시 프레임을 초기화하고 payload를 복사한 뒤,
 * enable_brs에 따라 FD 모드를 선택하고 즉시 송신합니다.
 * 동적 할당은 수행하지 않습니다.
 *
 * @param socket 
 * Socket object used for transmission.
 * 송신에 사용할 socket 객체입니다.
 *
 * @param id 
 * Extended CAN identifier.
 * 확장 CAN 식별자입니다.
 *
 * @param data 
 * Source payload buffer.
 * 원본 payload 버퍼입니다.
 *
 * @param len 
 * Payload length in bytes.
 * 바이트 단위 payload 길이입니다.
 *
 * @param enable_brs 
 * true to use bit-rate switching, otherwise false.
 * bit-rate switching을 사용하면 true, 아니면 false입니다.
 *
 * @retval CAN_STATUS_OK
 * frame construction and send succeeded.
 * 프레임 구성과 송신에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid payload length.
 * 잘못된 인자이거나 payload 길이가 유효하지 않습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying socket send path.
 * 하위 socket 송신 경로로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketSendFdExt(CANSocket *socket,
                             uint32_t id,
                             const void *data,
                             uint8_t len,
                             bool enable_brs);

/**
 * @brief 
 * Receive one CAN frame through the bound core.
 * 바인딩된 core를 통해 CAN 프레임 하나를 수신합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Output frame buffer.
 * 수신 프레임을 저장할 출력 버퍼입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_ENODATA
 * no frame is currently available.
 * 현재 수신 가능한 프레임이 없습니다.
 */
CANStatus CANSocketReceive(CANSocket *socket, CANFrame *frame);

/**
 * @brief 
 * Attempt to send one CAN frame without waiting.
 * 대기 없이 CAN 프레임 하나를 송신 시도합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_EBUSY
 * the core is not started or transmit is not currently possible.
 * core가 started 상태가 아니거나 현재 송신이 불가능합니다.
 */
CANStatus CANSocketTrySend(CANSocket *socket, const CANFrame *frame);

/**
 * @brief 
 * Attempt to receive one CAN frame without waiting.
 * 대기 없이 CAN 프레임 하나를 수신 시도합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * 
 * @param frame 
 * Output frame buffer.
 * 수신 프레임을 저장할 출력 버퍼입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_ENODATA
 * no frame is currently available.
 * 현재 수신 가능한 프레임이 없습니다.
 */
CANStatus CANSocketTryReceive(CANSocket *socket, CANFrame *frame);





/**
 * @brief 
 * Send one CAN frame with timeout-aware behavior.
 * timeout 기반 동작으로 CAN 프레임 하나를 송신합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 * @param timeout_ms 
 * Timeout in milliseconds.
 * 밀리초 단위 timeout 값입니다.
 *
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 *
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 * 
 * @retval CAN_STATUS_OK
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_ETIMEOUT
 * timeout expired before the frame could be transmitted.
 * 프레임이 송신되기 전에 timeout이 만료되었습니다.
 * 
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketSendTimeout(CANSocket *socket, const CANFrame *frame, uint32_t timeout_ms);

/**
 * @brief 
 * Receive one CAN frame with timeout-aware behavior.
 * timeout 기반 동작으로 CAN 프레임 하나를 수신합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Output frame buffer.
 * 수신 프레임을 저장할 출력 버퍼입니다.
 * @param timeout_ms 
 * Timeout in milliseconds.
 * 밀리초 단위 timeout 값입니다.
 *
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 *
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 * 
 * @retval CAN_STATUS_OK
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_ETIMEOUT
 * timeout expired before the frame could be received.
 * 프레임이 수신되기 전에 timeout이 만료되었습니다.
 * 
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketReceiveTimeout(CANSocket *socket, CANFrame *frame, uint32_t timeout_ms);





/**
 * @brief 
 * Send one CAN frame immediately using the shortest socket-like API.
 * 가장 짧은 socket형 API를 사용하여 CAN 프레임 하나를 즉시 송신합니다.
 *
 * This is a shorthand wrapper over CANSocketSend().
 * 이 함수는 CANSocketSend()를 감싼 shorthand 래퍼입니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 * 
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_EBUSY
 * the core is not started or transmit is not currently possible.
 * core가 started 상태가 아니거나 현재 송신이 불가능합니다.
 */
CANStatus CANSocketSendNow(CANSocket *socket, const CANFrame *frame);

/**
 * @brief 
 * Receive one CAN frame immediately using the shortest socket-like API.
 * 가장 짧은 socket형 API를 사용하여 CAN 프레임 하나를 즉시 수신합니다.
 *
 * This is a shorthand wrapper over CANSocketReceive().
 * 이 함수는 CANSocketReceive()를 감싼 shorthand 래퍼입니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param frame 
 * Output frame buffer.
 * 수신 프레임을 저장할 출력 버퍼입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_ENODATA
 * no frame is currently available.
 * 현재 수신 가능한 프레임이 없습니다.
 */
CANStatus CANSocketReceiveNow(CANSocket *socket, CANFrame *frame);





/**
 * @brief 
 * Receive frames until one matching the requested identifier type and ID is found.
 * Non-matching frames are consumed while searching for a match.
 * If no frame is currently available, CAN_STATUS_ENODATA may be returned.
 * 
 * 요청한 식별자 형식과 ID에 일치하는 프레임이 나올 때까지 프레임을 수신합니다.
 * 일치하지 않는 프레임은 매칭 탐색 과정에서 소비됩니다.
 * 현재 즉시 수신 가능한 프레임이 없으면 CAN_STATUS_ENODATA가 반환될 수 있습니다.
 *
 * @param socket 
 * Socket object used for reception.
 * 수신에 사용할 socket 객체입니다.
 *
 * @param frame 
 * Output frame buffer for the matched frame.
 * 매칭된 프레임을 저장할 출력 버퍼입니다.
 *
 * @param id_type 
 * Identifier type to match.
 * 매칭할 식별자 형식입니다.
 *
 * @param id 
 * Identifier value to match.
 * 매칭할 식별자 값입니다.
 *
 * @retval CAN_STATUS_OK
 * a matching frame was received.
 * 매칭되는 프레임을 수신했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying receive path.
 * 하위 수신 경로로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketReceiveMatch(CANSocket *socket,
                                CANFrame *frame,
                                CANIdType id_type,
                                uint32_t id);

/**
 * @brief 
 * Receive frames until one matching the requested identifier type and ID is found,
 * or until the timeout expires.
 * Non-matching frames are consumed while searching for a match.
 * When timeout_ms is zero, this function falls back to CANSocketReceiveMatch().
 * In that case, no elapsed-time waiting is performed.
 * Finite timeout behavior requires runtime->get_tick_ms.
 * If runtime->relax is not provided, the function may busy-poll.
 * 
 * 요청한 식별자 형식과 ID에 일치하는 프레임이 나올 때까지 프레임을 수신하거나,
 * timeout이 만료될 때까지 대기합니다.
 * 일치하지 않는 프레임은 매칭 탐색 과정에서 소비됩니다.
 * timeout_ms가 0이면 이 함수는 CANSocketReceiveMatch()로 동작이 넘어갑니다.
 * 이 경우 경과 시간 기반 대기는 수행하지 않습니다.
 * 유한 timeout 동작을 위해서는 runtime->get_tick_ms가 필요합니다.
 * runtime->relax가 없으면 busy-poll 형태로 동작할 수 있습니다.
 *
 * @param socket 
 * Socket object used for reception.
 * 수신에 사용할 socket 객체입니다.
 *
 * @param frame 
 * Output frame buffer for the matched frame.
 * 매칭된 프레임을 저장할 출력 버퍼입니다.
 *
 * @param id_type 
 * Identifier type to match.
 * 매칭할 식별자 형식입니다.
 *
 * @param id 
 * Identifier value to match.
 * 매칭할 식별자 값입니다.
 *
 * @param timeout_ms 
 * Timeout in milliseconds.
 * 밀리초 단위 timeout 값입니다.
 *
 * @retval CAN_STATUS_OK
 * a matching frame was received before timeout.
 * timeout 전에 매칭되는 프레임을 수신했습니다.
 *
 * @retval CAN_STATUS_ETIMEOUT
 * timeout expired before a matching frame was received.
 * 매칭되는 프레임을 수신하기 전에 timeout이 만료되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 *
 * @retval other
 * other CANStatus values may be returned from the underlying receive path.
 * 하위 수신 경로로부터 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketReceiveTimeoutMatch(CANSocket *socket,
                                       CANFrame *frame,
                                       CANIdType id_type,
                                       uint32_t id,
                                       uint32_t timeout_ms);





/**
 * @brief 
 * Wait until transmit readiness is reported by the bound core.
 * 바인딩된 core가 송신 가능 상태를 보고할 때까지 대기합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param timeout_ms 
 * Timeout in milliseconds.
 * 밀리초 단위 timeout 값입니다.
 * 
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 *
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 * 
 * @retval CAN_STATUS_OK
 * TX readiness is observed.
 * TX readiness가 관찰되었습니다.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_ETIMEOUT
 * timeout expired before any requested event became ready.
 * 요청한 이벤트가 준비되기 전에 timeout이 만료되었습니다.
 * 
 * @retval CAN_STATUS_EUNSUPPORTED
 * the bound driver does not provide this operation.
 * 바인딩된 드라이버가 이 연산을 제공하지 않습니다.
 * 
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketWaitTxReady(CANSocket *socket, uint32_t timeout_ms);

/**
 * @brief 
 * Wait until receive readiness is reported by the bound core.
 * 바인딩된 core가 수신 가능 상태를 보고할 때까지 대기합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param timeout_ms 
 * Timeout in milliseconds.
 * 밀리초 단위 timeout 값입니다.
 * 
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 *
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 * 
 * @retval CAN_STATUS_OK
 * RX readiness is observed.
 * RX readiness가 관찰되었습니다.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_ETIMEOUT
 * timeout expired before any requested event became ready.
 * 요청한 이벤트가 준비되기 전에 timeout이 만료되었습니다.
 * 
 * @retval CAN_STATUS_EUNSUPPORTED
 * the bound driver does not provide this operation.
 * 바인딩된 드라이버가 이 연산을 제공하지 않습니다.
 * 
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketWaitRxReady(CANSocket *socket, uint32_t timeout_ms);





/**
 * @brief 
 * Query readiness/event hints from the bound core.
 * 바인딩된 core로부터 readiness/event 힌트를 조회합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param event_mask 
 * Output event mask buffer.
 * 이벤트 마스크를 저장할 출력 버퍼입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * @retval CAN_STATUS_EUNSUPPORTED 
 * the bound driver does not provide this operation.
 * 바인딩된 드라이버가 이 연산을 제공하지 않습니다.
 */
CANStatus CANSocketQueryEvents(CANSocket *socket, uint32_t *event_mask);

/**
 * @brief 
 * Wait for one or more requested readiness events through the socket facade.
 * socket 파사드를 통해 요청한 readiness 이벤트 중 하나 이상이 준비될 때까지 대기합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param interest_mask 
 * Requested event mask.
 * 요청할 이벤트 마스크입니다.
 * @param timeout_ms 
 * Timeout in milliseconds.
 * 밀리초 단위 timeout 값입니다.
 * @param ready_mask 
 * Output ready event mask.
 * 준비된 이벤트 마스크를 저장할 출력 버퍼입니다.
 *
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 *
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 * 
 * @retval CAN_STATUS_OK
 * one or more requested events are ready, with ready_mask updated to indicate which ones.
 * 하나 이상 요청한 이벤트가 준비되었으며, ready_mask가 어떤 이벤트인지 표시하도록 업데이트되었습니다.
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_ETIMEOUT
 * timeout expired before any requested event became ready.
 * 요청한 이벤트가 준비되기 전에 timeout이 만료되었습니다.
 * 
 * @retval CAN_STATUS_EUNSUPPORTED
 * runtime hooks or event query support are not available for this operation.
 * 이 동작에 필요한 runtime hooks 또는 event query 지원이 제공되지 않습니다.
 * 
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANSocketPoll(CANSocket *socket,
                        uint32_t interest_mask,
                        uint32_t timeout_ms,
                        uint32_t *ready_mask);





/**
 * @brief 
 * Query optional CAN error-state information through the socket facade.
 * socket 파사드를 통해 선택적 CAN 오류 상태 정보를 조회합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * @param state 
 * Output error-state buffer.
 * 오류 상태를 저장할 출력 버퍼입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * @retval CAN_STATUS_EUNSUPPORTED 
 * the bound driver does not provide this operation.
 * 바인딩된 드라이버가 이 연산을 제공하지 않습니다.
 */
CANStatus CANSocketGetErrorState(CANSocket *socket, CANCoreErrorState *state);

/**
 * @brief 
 * Invoke optional recovery behavior through the socket facade.
 * socket 파사드를 통해 선택적 복구 동작을 호출합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공.
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * @retval CAN_STATUS_EUNSUPPORTED 
 * the bound driver does not provide this operation.
 * 바인딩된 드라이버가 이 연산을 제공하지 않습니다.
 */
CANStatus CANSocketRecover(CANSocket *socket);

/**
 * @brief 
 * Get the last status from the bound core through the socket facade.
 * socket 파사드를 통해 바인딩된 core의 마지막 상태 코드를 조회합니다.
 *
 * @param socket 
 * Bound socket object.
 * 바인딩된 socket 객체입니다.
 * 
 * @return 
 * Last recorded CANStatus value.
 * 마지막으로 기록된 CANStatus 값을 반환합니다.
 */
CANStatus CANSocketGetLastStatus(const CANSocket *socket);

/**
 * @brief 
 * Check whether the bound core is open.
 * 바인딩된 core가 열린 상태인지 확인합니다.
 *
 * @param socket 
 * Socket object to inspect.
 * 확인할 socket 객체입니다.
 * 
 * @return 
 * true if the bound core is open, otherwise false.
 * 바인딩된 core가 열린 상태이면 true, 아니면 false를 반환합니다.
 */
bool CANSocketIsOpen(const CANSocket *socket);

/**
 * @brief 
 * Check whether the bound core is started.
 * 바인딩된 core가 started 상태인지 확인합니다.
 *
 * @param socket 
 * Socket object to inspect.
 * 확인할 socket 객체입니다.
 * @return 
 * true if the bound core is started, otherwise false.
 * 바인딩된 core가 started 상태이면 true, 아니면 false를 반환합니다.
 */
bool CANSocketIsStarted(const CANSocket *socket);

#ifdef __cplusplus
}
#endif

#endif