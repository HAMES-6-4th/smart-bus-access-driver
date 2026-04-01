#ifndef CAN_OPERATION_H
#define CAN_OPERATION_H

#include <stdbool.h>
#include <stdint.h>

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CANOperation
 *
 * purpose
 * - represents one pending unit of work on top of CANCore
 * - owns no transport resource
 * - performs no allocation
 *
 * supported operation kinds
 * - SEND
 * - RECEIVE
 * - POLL
 *
 * intended usage
 * 1) CANOperationPrepare*
 * 2) CANOperationSubmit
 * 3) call CANOperationRunOnce until completed
 *
 * notes
 * - timeout handling is operation-local
 * - actual I/O remains in CANCore
 * - suitable as a building block for executor/context layers
 */


/**
 * @brief 
 * Operation kind represented by CANOperation.
 * 
 * CANOperation이 나타내는 작업 종류입니다.
 */
typedef enum CANOperationTypeEnum
{
    /**
     * @brief 
     * No valid operation is prepared.
     * 
     * 유효한 operation이 준비되지 않은 상태입니다.
     */
    CAN_OPERATION_TYPE_NONE = 0,

    /**
     * @brief 
     * Send operation.
     * 
     * 송신 operation입니다.
     */
    CAN_OPERATION_TYPE_SEND,

    /**
     * @brief 
     * Receive operation.
     * 
     * 수신 operation입니다.
     */
    CAN_OPERATION_TYPE_RECEIVE,

    /**
     * @brief 
     * Poll operation for CAN core events.
     * 
     * CAN core 이벤트를 조회하는 poll operation입니다.
     */
    CAN_OPERATION_TYPE_POLL
} CANOperationType;

/**
 * @brief 
 * Lifecycle state of a CANOperation object.
 * 
 * CANOperation 객체의 생명주기 상태입니다.
 */
typedef enum CANOperationStateEnum
{
    /**
     * @brief 
     * The operation object is idle and not prepared.
     * 
     * operation 객체가 idle 상태이며 아직 준비되지 않았습니다.
     */
    CAN_OPERATION_STATE_IDLE = 0,

    /**
     * @brief 
     * The operation has been prepared and can be submitted.
     * 
     * operation이 준비되었으며 제출할 수 있는 상태입니다.
     */
    CAN_OPERATION_STATE_READY,

    /**
     * @brief 
     * The operation has been submitted and is in progress.
     * 
     * operation이 제출되었고 진행 중인 상태입니다.
     */
    CAN_OPERATION_STATE_PENDING,

    /**
     * @brief 
     * The operation has completed with a final result status.
     * 
     * operation이 최종 결과 상태와 함께 완료된 상태입니다.
     */
    CAN_OPERATION_STATE_COMPLETED
} CANOperationState;

/**
 * @brief 
 * One-step execution result returned by CANOperationRunOnce.
 * 
 * CANOperationRunOnce가 반환하는 1-step 실행 결과입니다.
 */
typedef enum CANOperationRunResultEnum
{
    /**
     * @brief 
     * The operation could not be progressed due to an error.
     * 
     * 오류로 인해 operation을 진행할 수 없었습니다.
     */
    CAN_OPERATION_RUN_ERROR = 0,

    /**
     * @brief 
     * The operation is still pending after this step.
     * 
     * 이번 step 이후에도 operation이 아직 pending 상태입니다.
     */
    CAN_OPERATION_RUN_PENDING,

    /**
     * @brief 
     * The operation completed during this step.
     * 
     * 이번 step에서 operation이 완료되었습니다.
     */
    CAN_OPERATION_RUN_COMPLETED
} CANOperationRunResult;

/**
 * @brief 
 * One caller-owned unit of work built on top of CANCore.
 * The operation performs no allocation, owns no transport resource,
 * and does not take ownership of bound frame or core objects.
 * 
 * CANCore 위에서 동작하는 caller-owned 작업 단위입니다.
 * operation은 동적 할당을 수행하지 않고, transport 자원을 소유하지 않으며,
 * 바인딩된 frame 또는 core 객체의 소유권을 가져오지 않습니다.
 */
typedef struct CANOperationStruct
{
    CANOperationType type;
    CANOperationState state;

    CANCore *core;

    const CANFrame *tx_frame;
    CANFrame *rx_frame;

    uint32_t interest_mask;
    uint32_t *ready_mask;

    uint32_t timeout_ms;
    uint32_t start_ms;
    bool deadline_started;

    CANStatus result_status;
} CANOperation;

/**
 * @brief 
 * Initialize a CANOperation object to its idle empty state.
 * 
 * CANOperation 객체를 idle 빈 상태로 초기화합니다.
 *
 * @param operation 
 * Operation object to initialize.
 * 초기화할 operation 객체입니다.
 */
void CANOperationInit(CANOperation *operation);

/**
 * @brief 
 * Prepare a send operation.
 * This only prepares the operation and does not submit it.
 * timeout_ms = 0 means immediate / non-blocking behavior when the operation is later run.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 송신 operation을 준비합니다.
 * 이 함수는 operation만 준비하며 제출(submit)은 수행하지 않습니다.
 * timeout_ms = 0 은 이후 operation 실행 시 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param operation 
 * Operation object to prepare.
 * 준비할 operation 객체입니다.
 *
 * @param core 
 * Core object used by the operation.
 * operation이 사용할 core 객체입니다.
 *
 * @param frame 
 * Caller-owned frame to transmit.
 * 송신할 caller-owned 프레임입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * operation timeout 시간(ms)입니다.
 */
void CANOperationPrepareSend(CANOperation *operation,
                             CANCore *core,
                             const CANFrame *frame,
                             uint32_t timeout_ms);

/**
 * @brief 
 * Prepare a receive operation.
 * This only prepares the operation and does not submit it.
 * timeout_ms = 0 means immediate / non-blocking behavior when the operation is later run.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 수신 operation을 준비합니다.
 * 이 함수는 operation만 준비하며 제출(submit)은 수행하지 않습니다.
 * timeout_ms = 0 은 이후 operation 실행 시 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param operation 
 * Operation object to prepare.
 * 준비할 operation 객체입니다.
 *
 * @param core 
 * Core object used by the operation.
 * operation이 사용할 core 객체입니다.
 *
 * @param frame 
 * Caller-owned output frame buffer.
 * 수신 프레임을 저장할 caller-owned 출력 버퍼입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * operation timeout 시간(ms)입니다.
 */
void CANOperationPrepareReceive(CANOperation *operation,
                                CANCore *core,
                                CANFrame *frame,
                                uint32_t timeout_ms);

/**
 * @brief 
 * Prepare a poll operation for CAN core events.
 * This only prepares the operation and does not submit it.
 * timeout_ms = 0 means immediate / non-blocking behavior when the operation is later run.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * CAN core 이벤트에 대한 poll operation을 준비합니다.
 * 이 함수는 operation만 준비하며 제출(submit)은 수행하지 않습니다.
 * timeout_ms = 0 은 이후 operation 실행 시 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param operation 
 * Operation object to prepare.
 * 준비할 operation 객체입니다.
 *
 * @param core 
 * Core object used by the operation.
 * operation이 사용할 core 객체입니다.
 *
 * @param interest_mask 
 * Event interest mask to poll for.
 * 조회할 이벤트 관심 마스크입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * operation timeout 시간(ms)입니다.
 *
 * @param ready_mask 
 * Caller-owned output mask storage for ready events.
 * 준비된 이벤트 마스크를 저장할 caller-owned 출력 저장소입니다.
 */
void CANOperationPreparePoll(CANOperation *operation,
                             CANCore *core,
                             uint32_t interest_mask,
                             uint32_t timeout_ms,
                             uint32_t *ready_mask);





/**
 * @brief 
 * Submit a prepared operation and move it to the pending state.
 * 
 * 준비된 operation을 제출하고 pending 상태로 전환합니다.
 *
 * @param operation 
 * Prepared operation object.
 * 준비된 operation 객체입니다.
 *
 * @retval CAN_STATUS_OK 
 * submit succeeded.
 * 제출에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid prepared operation state.
 * 잘못된 인자이거나 준비된 operation 상태가 올바르지 않습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the operation is not ready, or the bound core is not started.
 * operation이 ready 상태가 아니거나 바인딩된 core가 시작되지 않았습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANOperationSubmit(CANOperation *operation);

/**
 * @brief 
 * Progress one submitted operation by one step.
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 제출된 operation을 한 step 진행합니다.
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param operation 
 * Submitted operation object.
 * 제출된 operation 객체입니다.
 *
 * @return 
 * One-step operation progress result.
 * 한 단계 operation 진행 결과를 반환합니다.
 */
CANOperationRunResult CANOperationRunOnce(CANOperation *operation);





/**
 * @brief 
 * Check whether the operation has completed.
 * 
 * operation이 완료되었는지 확인합니다.
 *
 * @param operation 
 * Operation object to query.
 * 조회할 operation 객체입니다.
 *
 * @return 
 * true if the operation is completed, otherwise false.
 * operation이 완료되었으면 true, 아니면 false를 반환합니다.
 */
bool CANOperationIsDone(const CANOperation *operation);

/**
 * @brief 
 * Get the current or final result status of the operation.
 * 
 * operation의 현재 또는 최종 결과 상태를 조회합니다.
 *
 * @param operation 
 * Operation object to query.
 * 조회할 operation 객체입니다.
 *
 * @return 
 * Current or final CANStatus value for the operation.
 * operation의 현재 또는 최종 CANStatus 값을 반환합니다.
 */
CANStatus CANOperationGetResult(const CANOperation *operation);

#ifdef __cplusplus
}
#endif

#endif