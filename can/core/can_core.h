/**
 * @file can_core.h
 * @brief 
 * Stable core CAN API for static-resource, polling-first usage.
 * 
 * This header defines the minimal stable CAN core surface, including open/close,
 * start/stop, send/receive, timeout-aware operations, and optional extension hooks.
 * 
 * The core does not allocate memory and does not own platform resources.
 * 
 * 
 * 정적 자원 기반, polling 우선 사용을 위한 안정적인 CAN 코어 API입니다.
 * 이 헤더는 open/close, start/stop, send/receive, timeout 기반 동작,
 * 그리고 선택적 확장 훅을 포함한 최소 안정 CAN 코어 표면을 정의합니다.
 * 코어는 동적 메모리를 할당하지 않으며 플랫폼 자원을 소유하지 않습니다.
 */

#ifndef CAN_CORE_H
#define CAN_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "can_types.h"

/**
 * @brief 
 * Sentinel timeout value meaning wait indefinitely.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 무한 대기를 의미하는 sentinel timeout 값입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 */
#define CAN_TIMEOUT_INFINITE   (0xFFFFFFFFUL)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CAN Core stability boundary
 *
 * v1 stable surface
 * - CANCoreInit / CANCoreInitOpenParams
 * - CANCoreOpen / CANCoreClose
 * - CANCoreStart / CANCoreStop
 * - CANCoreSend / CANCoreReceive
 * - CANCoreTrySend / CANCoreTryReceive
 * - CANCoreSendTimeout / CANCoreReceiveTimeout
 * - CANCoreGetLastStatus / CANCoreGetStats / CANCoreResetStats
 *
 * runtime model
 * - no dynamic allocation
 * - static resource binding
 * - polling-first
 *
 * RX path policy
 * - CAN_RX_PATH_DEFAULT == CAN_RX_PATH_FIFO0
 * - FIFO0  : accept-all oriented path
 * - BUFFER : exact / explicit filter oriented path
 *
 * intended combinations
 * - FIFO0  + filter disabled : supported
 * - FIFO0  + filter enabled  : unsupported
 * - BUFFER + filter enabled  : supported
 * - BUFFER + filter disabled : unsupported
 *
 * additive extension surface
 * - CANCoreQueryEvents
 * - CANCoreGetErrorState
 * - CANCoreRecover
 *
 * if optional driver ops are not provided:
 * - CAN_STATUS_EUNSUPPORTED is returned
 *
 * higher layers built above core:
 * - CANOperation
 * - CANExecutor
 * - CANContext
 */

/**
 * @brief 
 * State of a CANCore instance.
 * The core transitions through closed, opened, and started states.
 * 
 * CANCore 인스턴스의 상태를 나타냅니다.
 * 코어는 closed, opened, started 상태를 순차적으로 전이합니다.
 */
typedef enum CANCoreStateEnum
{
    CAN_CORE_STATE_CLOSED = 0,
    CAN_CORE_STATE_OPENED,
    CAN_CORE_STATE_STARTED
} CANCoreState;

struct CANCoreStruct;

/**
 * @brief 
 * Optional callback invoked when receive-side progress is reported by the core.
 * This hook is intended for upper-layer integration and does not transfer ownership.
 * 
 * core가 수신 측 진행을 보고할 때 호출될 수 있는 선택적 콜백입니다.
 * 이 훅은 상위 계층 연동을 위한 것이며 소유권은 이전되지 않습니다.
 */
typedef void (*CANCoreOnRxFn)(struct CANCoreStruct *core, void *user_context);

/**
 * @brief 
 * Optional callback invoked when transmit-side progress is reported by the core.
 * This hook is intended for upper-layer integration and does not transfer ownership.
 * 
 * core가 송신 측 진행을 보고할 때 호출될 수 있는 선택적 콜백입니다.
 * 이 훅은 상위 계층 연동을 위한 것이며 소유권은 이전되지 않습니다.
 */
typedef void (*CANCoreOnTxFn)(struct CANCoreStruct *core, void *user_context);

/**
 * @brief 
 * Optional callback invoked when the core reports an error status.
 * This hook is intended for upper-layer integration and does not transfer ownership.
 * 
 * core가 오류 상태를 보고할 때 호출될 수 있는 선택적 콜백입니다.
 * 이 훅은 상위 계층 연동을 위한 것이며 소유권은 이전되지 않습니다.
 */
typedef void (*CANCoreOnErrorFn)(struct CANCoreStruct *core, CANStatus status, void *user_context);

/**
 * @brief 
 * Optional user hook callbacks associated with a CANCore instance.
 * These hooks are intended for upper-layer integration and are not required
 * for basic polling-oriented CAN operation.
 * 
 * CANCore 인스턴스에 연결할 수 있는 선택적 사용자 훅 콜백 집합입니다.
 * 이 훅들은 상위 계층 연동을 위한 것이며, 기본적인 polling 기반 CAN 동작에는 필수는 아닙니다.
 */
typedef struct CANCoreHooksStruct
{
    CANCoreOnRxFn on_rx;
    CANCoreOnTxFn on_tx;
    CANCoreOnErrorFn on_error;
    void *user_context;
} CANCoreHooks;

/** 
 * @brief 
 * Callback used to obtain a monotonic tick in milliseconds.
 * 
 * 밀리초 단위 단조 증가 tick 값을 얻기 위한 콜백입니다.
 */
typedef uint32_t (*CANCoreGetTickMsFn)(void *user_context);

/**
 * @brief 
 * Optional callback used to relax or yield between timeout-aware polling attempts.
 * This hook may be omitted when immediate / non-blocking behavior is sufficient.
 * 
 * timeout 기반 polling 재시도 사이에서 완화(relax) 또는 양보(yield)를 수행하기 위한 선택적 콜백입니다.
 * immediate / non-blocking 동작만으로 충분한 경우 이 훅은 생략할 수 있습니다.
 */
typedef void (*CANCoreRelaxFn)(void *user_context);





/**
 * @brief 
 * Runtime hooks used for timeout-aware core operations.
 * Real time-based timeout behavior requires a valid tick source.
 * 
 * timeout 기반 코어 동작에 사용되는 런타임 훅입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 유효한 tick 소스가 필요합니다.
 */
typedef struct CANCoreRuntimeStruct
{
    CANCoreGetTickMsFn get_tick_ms;
    CANCoreRelaxFn relax;
    void *user_context;
} CANCoreRuntime;

/**
 * @brief 
 * Runtime statistics collected by a CANCore instance.
 * This structure stores transmit/receive call counts and common result counters.
 * 
 * CANCore 인스턴스가 수집하는 런타임 통계 정보입니다.
 * 이 구조체는 송수신 호출 횟수와 주요 결과 카운터를 저장합니다.
 */
typedef struct CANCoreStatsStruct
{
    uint32_t tx_calls;
    uint32_t tx_ok;
    uint32_t tx_busy;
    uint32_t tx_timeouts;

    uint32_t rx_calls;
    uint32_t rx_ok;
    uint32_t rx_empty;
    uint32_t rx_timeouts;
} CANCoreStats;

/**
 * @brief 
 * Capability flags of a bound CAN driver or port.
 * These flags describe what the current binding can support.
 * 
 * 바인딩된 CAN 드라이버 또는 포트의 기능 지원 여부를 나타내는 플래그입니다.
 * 이 플래그들은 현재 바인딩이 어떤 기능을 지원할 수 있는지 설명합니다.
 */
typedef struct CANCoreCapabilitiesStruct
{
    bool supports_fd;
    bool supports_brs;
    bool supports_loopback;
    bool supports_hw_filter;
    bool supports_termination_control;
} CANCoreCapabilities;

/**
 * @brief Event hint mask returned by optional event query operations.
 * These bits are intended for polling and readiness-style progression.
 * 
 * 선택적 이벤트 조회 동작이 반환하는 이벤트 힌트 마스크입니다.
 * 이 비트들은 polling 및 readiness 기반 진행을 위해 사용됩니다.
 */
typedef enum CANCoreEventMaskEnum
{
    CAN_CORE_EVENT_NONE      = 0u,
    CAN_CORE_EVENT_RX_READY  = (1u << 0),
    CAN_CORE_EVENT_TX_READY  = (1u << 1),
    CAN_CORE_EVENT_ERROR     = (1u << 2),
    CAN_CORE_EVENT_STATE     = (1u << 3)
} CANCoreEventMask;

/**
 * @brief 
 * Snapshot of driver-reported CAN error state.
 * This structure is used by optional error-state query operations.
 * 
 * 드라이버가 보고한 CAN 오류 상태 스냅샷입니다.
 * 이 구조체는 선택적 오류 상태 조회 동작에서 사용됩니다.
 */
typedef struct CANCoreErrorStateStruct
{
    bool bus_off;
    bool error_passive;
    bool warning;
    uint32_t tx_error_count;
    uint32_t rx_error_count;
} CANCoreErrorState;

/**
 * @brief 
 * Optional driver operations for extended core functionality.
 * If an optional operation is not provided, the core returns CAN_STATUS_EUNSUPPORTED.
 * 
 * 코어의 확장 기능을 위한 선택적 드라이버 연산 집합입니다.
 * 선택 연산이 제공되지 않으면 코어는 CAN_STATUS_EUNSUPPORTED를 반환합니다.
 */
typedef struct CANCoreOptionalDriverOpsStruct
{
    CANStatus (*QueryEvents)(void *driver_channel, uint32_t *event_mask);
    CANStatus (*GetErrorState)(void *driver_channel, CANCoreErrorState *state);
    CANStatus (*Recover)(void *driver_channel);
} CANCoreOptionalDriverOps;

/**
 * @brief 
 * Mandatory low-level driver operations required by CANCore.
 * A valid binding must provide all mandatory operations.
 * 
 * CANCore가 동작하기 위해 필요한 필수 저수준 드라이버 연산 집합입니다.
 * 유효한 바인딩은 모든 필수 연산을 제공해야 합니다.
 */
typedef struct CANCoreDriverOpsStruct
{
    CANStatus (*Open)(void *driver_channel,
                      const void *driver_port,
                      const CANChannelConfig *config);

    CANStatus (*Close)(void *driver_channel);

    CANStatus (*Start)(void *driver_channel);
    CANStatus (*Stop)(void *driver_channel);

    CANStatus (*Send)(void *driver_channel, const CANFrame *frame);
    CANStatus (*Receive)(void *driver_channel, CANFrame *frame);
} CANCoreDriverOps;

/**
 * @brief 
 * Static binding information used to open a CANCore instance.
 * The binding connects the core to driver operations, driver storage, and capability metadata.
 * 
 * CANCore 인스턴스를 열 때 사용하는 정적 바인딩 정보입니다.
 * 이 바인딩은 코어를 드라이버 연산, 드라이버 저장소, 기능 메타데이터와 연결합니다.
 */
typedef struct CANCoreBindingStruct
{
    const char *name;
    const CANCoreDriverOps *ops;
    const CANCoreOptionalDriverOps *optional_ops;

    void *driver_channel;
    const void *driver_port;

    CANCoreCapabilities capabilities;
} CANCoreBinding;

/**
 * @brief 
 * Open-time configuration parameters for CANCore.
 * This structure contains channel configuration, optional hooks, runtime hooks, and flags.
 * 
 * CANCore open 시점에 사용하는 설정 파라미터입니다.
 * 이 구조체는 채널 설정, 선택적 훅, 런타임 훅, 그리고 플래그를 포함합니다.
 */
typedef struct CANCoreOpenParamsStruct
{
    CANChannelConfig channel_config;
    const CANCoreHooks *hooks;
    CANCoreRuntime runtime;
    uint32_t flags;
} CANCoreOpenParams;

/**
 * @brief 
 * Main CAN core object.
 * The core stores driver binding state, runtime hooks, statistics, and current status.
 * 
 * 메인 CAN 코어 객체입니다.
 * 코어는 드라이버 바인딩 상태, 런타임 훅, 통계 정보, 현재 상태를 저장합니다.
 */
typedef struct CANCoreStruct
{
    CANCoreState state;

    const char *name;
    const CANCoreDriverOps *ops;
    const CANCoreOptionalDriverOps *optional_ops;

    void *driver_channel;
    const void *driver_port;

    CANCoreCapabilities capabilities;
    CANChannelConfig config;

    const CANCoreHooks *hooks;
    uint32_t flags;

    CANCoreRuntime runtime;
    CANCoreStats stats;
    CANStatus last_status;
} CANCore;

/**
 * @brief 
 * Initialize a CANCore object to its default closed state.
 * CANCore 객체를 기본 closed 상태로 초기화합니다.
 *
 * @param core 
 * Core instance to initialize.
 * 초기화할 core 인스턴스입니다.
 */
void CANCoreInit(CANCore *core);

/**
 * @brief 
 * Initialize open parameters with default values.
 * open 파라미터를 기본값으로 초기화합니다.
 *
 * @param params 
 * Open parameter structure to initialize.
 * 초기화할 open 파라미터 구조체입니다.
 */
void CANCoreInitOpenParams(CANCoreOpenParams *params);




/**
 * @brief
 * Open a CANCore instance using a static binding and open parameters.
 * 정적 바인딩과 open 파라미터를 사용하여 CANCore 인스턴스를 엽니다.
 *
 * @param core
 * Core instance to open.
 * 열 대상 core 인스턴스입니다.
 * @param binding
 * Static driver binding to attach.
 * 연결할 정적 드라이버 바인딩입니다.
 * @param params
 * Open-time configuration parameters.
 * open 시점 설정 파라미터입니다.
 *
 * @retval CAN_STATUS_OK
 * Open succeeded.
 * 열기에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * An argument or binding is invalid.
 * 잘못된 인자이거나 바인딩이 유효하지 않습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * The core is not in the closed state.
 * core가 closed 상태가 아닙니다.
 */
CANStatus CANCoreOpen(CANCore *core, const CANCoreBinding *binding, const CANCoreOpenParams *params);

/**
 * @brief 
 * Close a CANCore instance and release its bound driver state.
 * CANCore 인스턴스를 닫고 바인딩된 드라이버 상태를 해제합니다.
 *
 * @param core 
 * Core instance to close.
 * 닫을 core 인스턴스입니다.
 * 
 * @retval CAN_STATUS_OK 
 * Close succeeded.
 * 닫기에 성공했습니다.
 * @retval CAN_STATUS_EINVAL
 * Invalid argument such as null pointer, or the core is already closed.
 * 잘못된 인자 (예: null 포인터) 또는 core가 이미 closed 상태입니다.
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANCoreClose(CANCore *core);





/**
 * @brief 
 * Start CAN communication on an opened core.
 * 열린 core에서 CAN 통신을 시작합니다.
 *
 * @param core 
 * Core instance to start.
 * 시작할 core 인스턴스입니다.
 * @retval CAN_STATUS_OK 
 * success, including the case where the core is already started.
 * 성공했거나 core가 이미 started 상태인 경우입니다.
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer
 * 잘못된 인자(예: null 포인터)입니다.
 * @retval CAN_STATUS_EBUSY
 * the core is not in the opened state.
 * core가 opened 상태가 아닙니다.
 * @retval other
 * other CANStatus values may be returned, for example due to driver errors or invalid state.
 * 드라이버 오류나 잘못된 상태 등으로 다른 CANStatus 값이 반환될 수 있습니다.
 */
CANStatus CANCoreStart(CANCore *core);

/**
 * @brief 
 * Stop CAN communication on a started core.
 * started 상태의 core에서 CAN 통신을 중지합니다.
 *
 * @param core 
 * Core instance to stop.
 * 중지할 core 인스턴스입니다.
 * @retval CAN_STATUS_OK 
 * success, including the case where the core is already stopped and remains opened.
 * 성공했거나 core가 이미 중지된 상태로 opened에 머물러 있는 경우입니다.
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as a null pointer.
 * 잘못된 인자(예: null 포인터)입니다.
 * @retval CAN_STATUS_EBUSY
 * the core is not in the started state.
 * core가 started 상태가 아닙니다.
 */
CANStatus CANCoreStop(CANCore *core);

/**
 * @brief Transmit one CAN frame.
 * CAN 프레임 하나를 송신합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
 * 
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 * 
 * @retval CAN_STATUS_OK 
 * success.
 * 성공
 * 
 * @retval CAN_STATUS_EINVAL
 * invalid argument such as null pointer.
 * 잘못된 인자 (예: null 포인터)입니다.
 * 
 * @retval CAN_STATUS_EBUSY
 * the core is not started or transmit is not currently possible.
 * core가 started 상태가 아니거나 현재 송신이 불가능합니다.
 */
CANStatus CANCoreSend(CANCore *core, const CANFrame *frame);

/**
 * @brief 
 * Receive one CAN frame.
 * CAN 프레임 하나를 수신합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
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
CANStatus CANCoreReceive(CANCore *core, CANFrame *frame);





/**
 * @brief Attempt to transmit one CAN frame without waiting.
 * 대기 없이 CAN 프레임 하나를 송신 시도합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
 * @param frame Frame to transmit.
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
CANStatus CANCoreTrySend(CANCore *core, const CANFrame *frame);

/**
 * @brief 
 * Attempt to receive one CAN frame without waiting.
 * 대기 없이 CAN 프레임 하나를 수신 시도합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
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
CANStatus CANCoreTryReceive(CANCore *core, CANFrame *frame);





/**
 * @brief 
 * Transmit one CAN frame with timeout-aware retry behavior.
 * timeout 기반 재시도를 사용하여 CAN 프레임 하나를 송신합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
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
CANStatus CANCoreSendTimeout(CANCore *core, const CANFrame *frame, uint32_t timeout_ms);

/**
 * @brief 
 * Receive one CAN frame with timeout-aware retry behavior.
 * timeout 기반 재시도를 사용하여 CAN 프레임 하나를 수신합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
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
CANStatus CANCoreReceiveTimeout(CANCore *core, CANFrame *frame, uint32_t timeout_ms);





/**
 * @brief 
 * Get the last status recorded by the core.
 * core에 기록된 마지막 상태 코드를 조회합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
 * 
 * @return 
 * Last recorded CANStatus value.
 * 마지막으로 기록된 CANStatus 값을 반환합니다.
 */
CANStatus CANCoreGetLastStatus(const CANCore *core);

/**
 * @brief 
 * Copy runtime statistics out of the core.
 * core의 런타임 통계를 외부로 복사합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
 * @param stats 
 * Output statistics buffer.
 * 통계를 저장할 출력 버퍼입니다.
 */
void CANCoreGetStats(const CANCore *core, CANCoreStats *stats);

/**
 * @brief 
 * Reset runtime statistics stored in the core.
 * core에 저장된 런타임 통계를 초기화합니다.
 *
 * @param core 
 * Core instance to reset.
 * 통계를 초기화할 core 인스턴스입니다.
 */
void CANCoreResetStats(CANCore *core);





/**
 * @brief 
 * Check whether the core is opened.
 * core가 열린 상태인지 확인합니다.
 *
 * @param core 
 * Core instance to inspect.
 * 확인할 core 인스턴스입니다.
 * @return 
 * true if the core is not closed.
 * core가 closed 상태가 아니면 true를 반환합니다.
 */
bool CANCoreIsOpen(const CANCore *core);

/**
 * @brief 
 * Check whether the core is started.
 * core가 started 상태인지 확인합니다.
 *
 * @param core 
 * Core instance to inspect.
 * 확인할 core 인스턴스입니다.
 * @return 
 * true if the core is started.
 * core가 started 상태이면 true를 반환합니다.
 */
bool CANCoreIsStarted(const CANCore *core);





/**
 * @brief 
 * Get the binding name of the core.
 * core의 바인딩 이름을 조회합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
 * @return 
 * Bound name string, or null if unavailable.
 * 바인딩된 이름 문자열을 반환하며, 없으면 null을 반환합니다.
 */
const char *CANCoreGetName(const CANCore *core);

/**
 * @brief 
 * Get the applied channel configuration of the core.
 * core에 적용된 채널 설정을 조회합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
 * @return 
 * Pointer to the applied configuration, or null if unavailable.
 * 적용된 설정에 대한 포인터를 반환하며, 없으면 null을 반환합니다.
 */
const CANChannelConfig *CANCoreGetConfig(const CANCore *core);

/**
 * @brief 
 * Get capability information of the bound core.
 * 바인딩된 core의 기능 정보를 조회합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
 * @return 
 * Pointer to capability information, or null if unavailable.
 * 기능 정보에 대한 포인터를 반환하며, 없으면 null을 반환합니다.
 */
const CANCoreCapabilities *CANCoreGetCapabilities(const CANCore *core);





/**
 * @brief 
 * Query optional readiness/event hints from the bound driver.
 * 바인딩된 드라이버로부터 선택적 readiness/event 힌트를 조회합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
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
CANStatus CANCoreQueryEvents(CANCore *core, uint32_t *event_mask);

/**
 * @brief 
 * Query optional CAN error-state information from the bound driver.
 * 바인딩된 드라이버로부터 선택적 CAN 오류 상태 정보를 조회합니다.
 *
 * @param core 
 * Core instance.
 * 조회할 core 인스턴스입니다.
 * @param state 
 * Output error-state buffer.
 * 오류 상태를 저장할 출력 버퍼입니다.
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
CANStatus CANCoreGetErrorState(CANCore *core, CANCoreErrorState *state);

/**
 * @brief 
 * Invoke optional recovery behavior on the bound driver.
 * 바인딩된 드라이버의 선택적 복구 동작을 호출합니다.
 *
 * @param core 
 * Core instance.
 * 복구를 요청할 core 인스턴스입니다.
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
CANStatus CANCoreRecover(CANCore *core);

/**
 * @brief 
 * Wait for one or more requested readiness events.
 * 요청한 readiness 이벤트 중 하나 이상이 준비될 때까지 대기합니다.
 *
 * @param core 
 * Started core instance.
 * started 상태의 core 인스턴스입니다.
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
CANStatus CANCorePoll(CANCore *core,
                      uint32_t interest_mask,
                      uint32_t timeout_ms,
                      uint32_t *ready_mask);

#ifdef __cplusplus
}
#endif

#endif