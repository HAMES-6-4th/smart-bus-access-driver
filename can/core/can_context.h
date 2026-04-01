#ifndef CAN_CONTEXT_H
#define CAN_CONTEXT_H

#include <stdint.h>

#include "can_executor.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CANContext
 *
 * purpose
 * - thin facade over:
 *   CANCore + CANExecutor + CANOperation
 *
 * intended usage tiers
 *
 * tier 1: direct core
 * - CANCoreSend / CANCoreReceive / CANCorePoll
 *
 * tier 2: operation
 * - CANOperationPrepare* + CANOperationSubmit + CANOperationRunOnce
 *
 * tier 3: executor
 * - CANExecutorSubmit + CANExecutorPollOne / CANExecutorPoll
 *
 * tier 4: facade
 * - CANContextSubmitSend / CANContextSubmitReceive / CANContextSubmitPoll
 * - CANContextPollOne / CANContextRunOne / CANContextDispatchOne / CANContextPoll
 *
 * notes
 * - context does not allocate
 * - context does not own bound objects
 * - caller owns CANCore, CANExecutor, and CANOperation storage
 */


/**
 * @brief 
 * Thin facade that binds a CANCore and a CANExecutor together.
 * The context does not allocate memory and does not own the bound objects.
 * 
 * CANCore와 CANExecutor를 함께 묶는 얇은 파사드입니다.
 * context는 동적 메모리를 할당하지 않으며 바인딩된 객체를 소유하지 않습니다.
 */
typedef struct CANContextStruct
{
    CANCore *core;
    CANExecutor *executor;
} CANContext;





/**
 * @brief 
 * Initialize a CANContext object.
 * The object is reset to an unbound empty state.
 * 
 * CANContext 객체를 초기화합니다.
 * 객체는 바인딩되지 않은 빈 상태로 재설정됩니다.
 *
 * @param context 
 * Context object to initialize.
 * 초기화할 context 객체입니다.
 */
void CANContextInit(CANContext *context);

/**
 * @brief 
 * Bind a CANCore and a CANExecutor to a CANContext.
 * Bound object ownership is not transferred.
 * 
 * CANContext에 CANCore와 CANExecutor를 바인딩합니다.
 * 바인딩된 객체의 소유권은 이전되지 않습니다.
 *
 * @param context 
 * Context object to update.
 * 갱신할 context 객체입니다.
 *
 * @param core 
 * Core object to bind.
 * 바인딩할 core 객체입니다.
 *
 * @param executor 
 * Executor object to bind.
 * 바인딩할 executor 객체입니다.
 */
void CANContextBind(CANContext *context, CANCore *core, CANExecutor *executor);





/**
 * @brief 
 * Get the mutable core bound to the context.
 * 
 * context에 바인딩된 수정 가능한 core를 조회합니다.
 *
 * @param context 
 * Context object.
 * 조회할 context 객체입니다.
 *
 * @return 
 * Bound core pointer, or null if unbound or invalid.
 * 바인딩된 core 포인터를 반환하며, 바인딩되지 않았거나 인자가 잘못되었으면 null을 반환합니다.
 */
CANCore *CANContextGetCore(CANContext *context);

/**
 * @brief 
 * Get the mutable executor bound to the context.
 * 
 * context에 바인딩된 수정 가능한 executor를 조회합니다.
 *
 * @param context 
 * Context object.
 * 조회할 context 객체입니다.
 *
 * @return 
 * Bound executor pointer, or null if unbound or invalid.
 * 바인딩된 executor 포인터를 반환하며, 바인딩되지 않았거나 인자가 잘못되었으면 null을 반환합니다.
 */
CANExecutor *CANContextGetExecutor(CANContext *context);






/**
 * @brief 
 * Prepare a send operation using the context-bound core.
 * This only prepares the operation and does not submit it.
 * timeout_ms = 0 means immediate / non-blocking behavior when the operation is later run.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * context에 바인딩된 core를 사용해 송신 operation을 준비합니다.
 * 이 함수는 operation만 준비하며 제출(submit)은 수행하지 않습니다.
 * timeout_ms = 0 은 이후 operation 실행 시 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param context 
 * Context object providing the bound core.
 * 바인딩된 core를 제공하는 context 객체입니다.
 *
 * @param operation 
 * Caller-owned operation storage to prepare.
 * 준비할 caller-owned operation 저장소입니다.
 *
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * timeout 시간(ms)입니다.
 */
void CANContextPrepareSend(CANContext *context,
                           CANOperation *operation,
                           const CANFrame *frame,
                           uint32_t timeout_ms);

/**
 * @brief 
 * Prepare a receive operation using the context-bound core.
 * This only prepares the operation and does not submit it.
 * timeout_ms = 0 means immediate / non-blocking behavior when the operation is later run.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * context에 바인딩된 core를 사용해 수신 operation을 준비합니다.
 * 이 함수는 operation만 준비하며 제출(submit)은 수행하지 않습니다.
 * timeout_ms = 0 은 이후 operation 실행 시 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param context 
 * Context object providing the bound core.
 * 바인딩된 core를 제공하는 context 객체입니다.
 *
 * @param operation 
 * Caller-owned operation storage to prepare.
 * 준비할 caller-owned operation 저장소입니다.
 *
 * @param frame 
 * Output frame buffer.
 * 수신 프레임을 저장할 출력 버퍼입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * timeout 시간(ms)입니다.
 */
void CANContextPrepareReceive(CANContext *context,
                              CANOperation *operation,
                              CANFrame *frame,
                              uint32_t timeout_ms);

                              /**
 * @brief 
 * Prepare a poll operation using the context-bound core.
 * This only prepares the operation and does not submit it.
 * timeout_ms = 0 means immediate / non-blocking behavior when the operation is later run.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * context에 바인딩된 core를 사용해 poll operation을 준비합니다.
 * 이 함수는 operation만 준비하며 제출(submit)은 수행하지 않습니다.
 * timeout_ms = 0 은 이후 operation 실행 시 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param context 
 * Context object providing the bound core.
 * 바인딩된 core를 제공하는 context 객체입니다.
 *
 * @param operation 
 * Caller-owned operation storage to prepare.
 * 준비할 caller-owned operation 저장소입니다.
 *
 * @param interest_mask 
 * Event interest mask to wait for.
 * 대기할 이벤트 관심 마스크입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * timeout 시간(ms)입니다.
 *
 * @param ready_mask 
 * Caller-owned output mask storage for ready events.
 * 준비된 이벤트 마스크를 저장할 caller-owned 출력 저장소입니다.
 */
void CANContextPreparePoll(CANContext *context,
                           CANOperation *operation,
                           uint32_t interest_mask,
                           uint32_t timeout_ms,
                           uint32_t *ready_mask);






/**
 * @brief 
 * Submit a prepared operation to the bound executor.
 * 
 * 준비된 operation을 바인딩된 executor에 제출합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @param operation 
 * Prepared operation to submit.
 * 제출할 준비된 operation입니다.
 *
 * @retval CAN_STATUS_OK 
 * submit succeeded.
 * 제출에 성공했습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or no executor is bound.
 * 잘못된 인자이거나 바인딩된 executor가 없습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the executor cannot accept the operation at this time.
 * executor가 현재 operation을 받을 수 없습니다.
 */
CANStatus CANContextSubmit(CANContext *context, CANOperation *operation);





/**
 * @brief 
 * Progress at most one pending operation through the bound executor.
 * 
 * 바인딩된 executor를 통해 최대 하나의 pending operation을 진행합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANContextPollOne(CANContext *context);

/**
 * @brief 
 * Alias of one-step execution through the bound executor.
 * 
 * 바인딩된 executor를 통한 1-step 실행 별칭 함수입니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANContextRunOne(CANContext *context);

/**
 * @brief 
 * Dispatch at most one pending operation through the bound executor.
 * 
 * 바인딩된 executor를 통해 최대 하나의 pending operation을 dispatch합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANContextDispatchOne(CANContext *context);





/**
 * @brief 
 * Progress pending operations with a bounded step budget.
 * 
 * 제한된 step 예산 안에서 pending operation들을 진행합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @param max_steps 
 * Maximum number of executor steps to run.
 * 수행할 최대 executor step 수입니다.
 *
 * @param completed_count 
 * Optional output for the number of completed operations.
 * 완료된 operation 수를 받는 선택적 출력 포인터입니다.
 *
 * @param remaining_count 
 * Optional output for the number of remaining operations.
 * 남아 있는 operation 수를 받는 선택적 출력 포인터입니다.
 *
 * @retval CAN_STATUS_OK 
 * polling completed successfully.
 * polling이 정상적으로 수행되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or no executor is bound.
 * 잘못된 인자이거나 바인딩된 executor가 없습니다.
 */
CANStatus CANContextPoll(CANContext *context,
                         uint32_t max_steps,
                         uint32_t *completed_count,
                         uint32_t *remaining_count);






/**
 * @brief 
 * Prepare and submit one send operation in a single call.
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 송신 operation 하나를 한 번에 준비하고 제출합니다.
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @param operation 
 * Caller-owned operation storage.
 * caller-owned operation 저장소입니다.
 *
 * @param frame 
 * Frame to transmit.
 * 송신할 프레임입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * timeout 시간(ms)입니다.
 *
 * @retval CAN_STATUS_OK 
 * operation was submitted.
 * operation이 제출되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the executor or core cannot accept the operation.
 * executor 또는 core가 현재 operation을 받을 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANContextSubmitSend(CANContext *context,
                               CANOperation *operation,
                               const CANFrame *frame,
                               uint32_t timeout_ms);

/**
 * @brief 
 * Prepare and submit one receive operation in a single call.
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 수신 operation 하나를 한 번에 준비하고 제출합니다.
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @param operation 
 * Caller-owned operation storage.
 * caller-owned operation 저장소입니다.
 *
 * @param frame 
 * Output frame buffer.
 * 수신 프레임을 저장할 출력 버퍼입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * timeout 시간(ms)입니다.
 *
 * @retval CAN_STATUS_OK 
 * operation was submitted.
 * operation이 제출되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the executor or core cannot accept the operation.
 * executor 또는 core가 현재 operation을 받을 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANContextSubmitReceive(CANContext *context,
                                  CANOperation *operation,
                                  CANFrame *frame,
                                  uint32_t timeout_ms);

/**
 * @brief 
 * Prepare and submit one poll operation in a single call.
 * timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * poll operation 하나를 한 번에 준비하고 제출합니다.
 * timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @param operation 
 * Caller-owned operation storage.
 * caller-owned operation 저장소입니다.
 *
 * @param interest_mask 
 * Event interest mask to wait for.
 * 대기할 이벤트 관심 마스크입니다.
 *
 * @param timeout_ms 
 * Operation timeout in milliseconds.
 * timeout 시간(ms)입니다.
 *
 * @param ready_mask 
 * Caller-owned output mask storage for ready events.
 * 준비된 이벤트 마스크를 저장할 caller-owned 출력 저장소입니다.
 *
 * @retval CAN_STATUS_OK 
 * operation was submitted.
 * operation이 제출되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument.
 * 잘못된 인자입니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the executor or core cannot accept the operation.
 * executor 또는 core가 현재 operation을 받을 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANContextSubmitPoll(CANContext *context,
                               CANOperation *operation,
                               uint32_t interest_mask,
                               uint32_t timeout_ms,
                               uint32_t *ready_mask);

                               



/**
 * @brief 
 * Check whether the bound executor currently has pending operations.
 * 
 * 바인딩된 executor에 현재 pending operation이 있는지 확인합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @return 
 * true if pending operations exist, otherwise false.
 * pending operation이 있으면 true, 없으면 false를 반환합니다.
 */
bool CANContextHasPending(const CANContext *context);

/**
 * @brief 
 * Get the current number of pending operations in the bound executor.
 * 
 * 바인딩된 executor에 현재 남아 있는 pending operation 수를 조회합니다.
 *
 * @param context 
 * Context object.
 * context 객체입니다.
 *
 * @return 
 * Number of pending operations.
 * pending operation 개수를 반환합니다.
 */
uint32_t CANContextGetPendingCount(const CANContext *context);

#ifdef __cplusplus
}
#endif

#endif