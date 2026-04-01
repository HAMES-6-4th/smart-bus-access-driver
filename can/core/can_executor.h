#ifndef CAN_EXECUTOR_H
#define CAN_EXECUTOR_H

#include <stdbool.h>
#include <stdint.h>

#include "can_operation.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CANExecutor
 *
 * purpose
 * - progresses multiple CANOperation objects over caller-provided static slots
 * - no dynamic allocation
 *
 * current execution model
 * - submit operation pointers into fixed slot storage
 * - PollOne / RunOne / DispatchOne advance at most one operation
 * - Poll / RunUntilIdle perform bounded draining with max_steps budget
 *
 * notes
 * - fairness is simple round-robin
 * - executor does not own CANCore
 * - event demultiplexing remains shallow at this stage
 */

/**
 * @brief 
 * One-step executor result returned by CANExecutorRunOnce and its aliases.
 * 
 * CANExecutorRunOnce 및 그 별칭 함수들이 반환하는 1-step executor 결과입니다.
 */
typedef enum CANExecutorRunOnceResultEnum
{
    /**
     * @brief 
     * No pending operation was available to run.
     * 
     * 실행할 pending operation이 없었습니다.
     */
    CAN_EXECUTOR_RUN_ONCE_IDLE = 0,

    /**
     * @brief 
     * One operation was progressed but remains pending.
     * 
     * operation 하나를 진행했지만 아직 pending 상태입니다.
     */
    CAN_EXECUTOR_RUN_ONCE_PENDING,

    /**
     * @brief 
     * One operation completed during this step.
     * 
     * 이번 step에서 operation 하나가 완료되었습니다.
     */
    CAN_EXECUTOR_RUN_ONCE_COMPLETED,

    /**
     * @brief 
     * Executor progression failed due to an error.
     * 
     * 오류로 인해 executor 진행에 실패했습니다.
     */
    CAN_EXECUTOR_RUN_ONCE_ERROR
} CANExecutorRunOnceResult;

/**
 * @brief 
 * Fixed-slot executor for multiple caller-owned CANOperation objects.
 * The executor performs no allocation and does not take ownership of
 * the bound operations, cores, or slot storage.
 * 
 * 여러 caller-owned CANOperation 객체를 위한 고정 슬롯 executor입니다.
 * executor는 동적 할당을 수행하지 않으며,
 * 바인딩된 operation, core, 또는 슬롯 저장소의 소유권을 가져오지 않습니다.
 */
typedef struct CANExecutorStruct
{
    CANOperation **slots;
    uint32_t capacity;
    uint32_t count;
    uint32_t next_index;
} CANExecutor;

/**
 * @brief 
 * Initialize a CANExecutor with caller-provided slot storage.
 * No allocation is performed.
 * 
 * caller가 제공한 슬롯 저장소로 CANExecutor를 초기화합니다.
 * 동적 할당은 수행되지 않습니다.
 *
 * @param executor 
 * Executor object to initialize.
 * 초기화할 executor 객체입니다.
 *
 * @param storage 
 * Caller-owned CANOperation pointer array used as executor slot storage.
 * executor 슬롯 저장소로 사용할 caller-owned CANOperation 포인터 배열입니다.
 *
 * @param capacity 
 * Number of entries in the provided storage array.
 * 제공된 저장소 배열의 엔트리 개수입니다.
 */
void CANExecutorInit(CANExecutor *executor, CANOperation **storage, uint32_t capacity);





/**
 * @brief 
 * Submit one prepared operation into the executor.
 * The executor stores the operation pointer only.
 * Bound object ownership is not transferred.
 * 
 * 준비된 operation 하나를 executor에 제출합니다.
 * executor는 operation 포인터만 저장합니다.
 * 바인딩된 객체의 소유권은 이전되지 않습니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
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
 * invalid argument or invalid executor storage configuration.
 * 잘못된 인자이거나 executor 저장소 구성이 올바르지 않습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the executor is full, the operation is already present,
 * or the operation itself could not be submitted.
 * executor가 가득 찼거나, operation이 이미 존재하거나,
 * operation 자체를 제출할 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested by the operation without required runtime hooks.
 * operation이 필요한 runtime hooks 없이 finite timeout을 요청했습니다.
 */
CANStatus CANExecutorSubmit(CANExecutor *executor, CANOperation *operation);

/**
 * @brief 
 * Progress at most one pending operation by one step.
 * 
 * 최대 하나의 pending operation을 한 step 진행합니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANExecutorRunOnce(CANExecutor *executor);

/**
 * @brief 
 * Alias of CANExecutorRunOnce.
 * 
 * CANExecutorRunOnce의 별칭 함수입니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANExecutorPollOne(CANExecutor *executor);





/**
 * @brief 
 * Progress pending operations with a bounded step budget until idle or budget exhaustion.
 * 
 * pending operation들을 제한된 step 예산 안에서 idle 상태가 되거나
 * 예산을 모두 사용할 때까지 진행합니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @param max_steps 
 * Maximum number of executor steps to run.
 * 수행할 최대 executor step 수입니다.
 *
 * @param completed_count 
 * Optional output for the number of operations completed during this call.
 * 이번 호출 동안 완료된 operation 수를 받는 선택적 출력 포인터입니다.
 *
 * @param remaining_count 
 * Optional output for the number of operations still pending after this call.
 * 이번 호출 이후에도 남아 있는 pending operation 수를 받는 선택적 출력 포인터입니다.
 *
 * @retval CAN_STATUS_OK 
 * draining completed successfully or the executor became idle.
 * draining이 정상적으로 수행되었거나 executor가 idle 상태가 되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid executor storage configuration.
 * 잘못된 인자이거나 executor 저장소 구성이 올바르지 않습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the step budget was exhausted while pending operations still remain.
 * pending operation이 남아 있는 상태에서 step 예산을 모두 사용했습니다.
 */
CANStatus CANExecutorRunUntilIdle(CANExecutor *executor,
                                  uint32_t max_steps,
                                  uint32_t *completed_count,
                                  uint32_t *remaining_count);





                                  /**
 * @brief 
 * Check whether the executor currently has pending operations.
 * 
 * executor에 현재 pending operation이 있는지 확인합니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @return 
 * true if pending operations exist, otherwise false.
 * pending operation이 있으면 true, 없으면 false를 반환합니다.
 */
bool CANExecutorHasPending(const CANExecutor *executor);

/**
 * @brief 
 * Get the number of currently pending operations in the executor.
 * 
 * executor에 현재 남아 있는 pending operation 수를 조회합니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @return 
 * Number of pending operations.
 * pending operation 개수를 반환합니다.
 */
uint32_t CANExecutorGetPendingCount(const CANExecutor *executor);





/**
 * @brief 
 * Alias of CANExecutorPollOne.
 * 
 * CANExecutorPollOne의 별칭 함수입니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANExecutorRunOne(CANExecutor *executor);





/**
 * @brief 
 * Alias of CANExecutorRunUntilIdle.
 * 
 * CANExecutorRunUntilIdle의 별칭 함수입니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @param max_steps 
 * Maximum number of executor steps to run.
 * 수행할 최대 executor step 수입니다.
 *
 * @param completed_count 
 * Optional output for the number of operations completed during this call.
 * 이번 호출 동안 완료된 operation 수를 받는 선택적 출력 포인터입니다.
 *
 * @param remaining_count 
 * Optional output for the number of operations still pending after this call.
 * 이번 호출 이후에도 남아 있는 pending operation 수를 받는 선택적 출력 포인터입니다.
 *
 * @retval CAN_STATUS_OK 
 * draining completed successfully or the executor became idle.
 * draining이 정상적으로 수행되었거나 executor가 idle 상태가 되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or invalid executor storage configuration.
 * 잘못된 인자이거나 executor 저장소 구성이 올바르지 않습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * the step budget was exhausted while pending operations still remain.
 * pending operation이 남아 있는 상태에서 step 예산을 모두 사용했습니다.
 */
CANStatus CANExecutorPoll(CANExecutor *executor,
                          uint32_t max_steps,
                          uint32_t *completed_count,
                          uint32_t *remaining_count);





/**
 * @brief 
 * Alias of CANExecutorPollOne.
 * 
 * CANExecutorPollOne의 별칭 함수입니다.
 *
 * @param executor 
 * Executor object.
 * executor 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANExecutorDispatchOne(CANExecutor *executor);

#ifdef __cplusplus
}
#endif

#endif