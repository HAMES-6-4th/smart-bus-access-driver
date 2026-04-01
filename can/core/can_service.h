#ifndef CAN_SERVICE_H
#define CAN_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "can_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CANService
 *
 * purpose
 * - thin multi-operation helper above CANContext
 * - uses caller-provided static CANOperation storage
 * - performs no dynamic allocation
 *
 * intended usage
 * 1) CANServiceInit(...)
 * 2) CANServiceBindContext(...)
 * 3) CANServiceSubmitSend / Receive / Poll
 * 4) CANServicePollOne / RunOne / DispatchOne / Poll
 * 5) inspect returned CANOperation result
 * 6) CANServiceReleaseOperation(...)
 */

/**
 * @brief 
 * Thin multi-operation helper built on top of CANContext.
 * The service uses caller-owned static CANOperation storage, performs no allocation,
 * and does not take ownership of the bound context or operation pool.
 * 
 * CANContext 위에 구축된 얇은 multi-operation helper입니다.
 * service는 caller-owned 정적 CANOperation 저장소를 사용하고, 동적 할당을 수행하지 않으며,
 * 바인딩된 context나 operation 풀의 소유권을 가져오지 않습니다.
 */
typedef struct CANServiceStruct
{
    CANContext *context;
    CANOperation *operations;
    bool *in_use;
    uint32_t capacity;
} CANService;





/**
 * @brief 
 * Initialize a CANService with caller-provided operation pool storage.
 * No allocation is performed.
 * 
 * caller가 제공한 operation 풀 저장소로 CANService를 초기화합니다.
 * 동적 할당은 수행되지 않습니다.
 *
 * @param service 
 * Service object to initialize.
 * 초기화할 service 객체입니다.
 *
 * @param operations 
 * Caller-owned CANOperation array used as the operation pool.
 * operation 풀로 사용할 caller-owned CANOperation 배열입니다.
 *
 * @param in_use 
 * Caller-owned usage bitmap array corresponding to the operation pool.
 * operation 풀과 대응되는 caller-owned 사용 상태 배열입니다.
 *
 * @param capacity 
 * Number of entries in the provided arrays.
 * 제공된 배열의 엔트리 개수입니다.
 */
void CANServiceInit(CANService *service,
                    CANOperation *operations,
                    bool *in_use,
                    uint32_t capacity);





/**
 * @brief 
 * Bind a CANContext to a CANService.
 * Bound object ownership is not transferred.
 * 
 * CANService에 CANContext를 바인딩합니다.
 * 바인딩된 객체의 소유권은 이전되지 않습니다.
 *
 * @param service 
 * Service object to update.
 * 갱신할 service 객체입니다.
 *
 * @param context 
 * Context object to bind.
 * 바인딩할 context 객체입니다.
 */
void CANServiceBindContext(CANService *service, CANContext *context);

/**
 * @brief 
 * Get the mutable context bound to the service.
 * 
 * service에 바인딩된 수정 가능한 context를 조회합니다.
 *
 * @param service 
 * Service object.
 * 조회할 service 객체입니다.
 *
 * @return 
 * Bound context pointer, or null if unbound or invalid.
 * 바인딩된 context 포인터를 반환하며, 바인딩되지 않았거나 인자가 잘못되었으면 null을 반환합니다.
 */
CANContext *CANServiceGetContext(CANService *service);





/**
 * @brief 
 * Get the total operation slot capacity of the service.
 * 
 * service가 보유한 전체 operation 슬롯 용량을 조회합니다.
 *
 * @param service 
 * Service object.
 * 조회할 service 객체입니다.
 *
 * @return 
 * Configured operation pool capacity.
 * 설정된 operation 풀 용량을 반환합니다.
 */
uint32_t CANServiceGetCapacity(const CANService *service);

/**
 * @brief 
 * Get the number of operation slots currently marked as in use.
 * 
 * 현재 사용 중으로 표시된 operation 슬롯 수를 조회합니다.
 *
 * @param service 
 * Service object.
 * 조회할 service 객체입니다.
 *
 * @return 
 * Number of slots currently in use.
 * 현재 사용 중인 슬롯 수를 반환합니다.
 */
uint32_t CANServiceGetInUseCount(const CANService *service);





/**
 * @brief 
 * Acquire one free operation slot from the service pool.
 * The returned operation refers to caller-owned storage and must be released
 * back with CANServiceReleaseOperation when no longer needed.
 * 
 * service 풀에서 사용 가능한 operation 슬롯 하나를 확보합니다.
 * 반환된 operation은 caller-owned 저장소를 가리키며, 더 이상 필요 없으면
 * CANServiceReleaseOperation으로 반환해야 합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @return 
 * Acquired operation pointer, or null if no free slot is available.
 * 확보한 operation 포인터를 반환하며, 사용 가능한 슬롯이 없으면 null을 반환합니다.
 */
CANOperation *CANServiceAcquireOperation(CANService *service);

/**
 * @brief 
 * Release a previously acquired operation slot back to the service pool.
 * The slot is marked free again and the operation object is reinitialized.
 * 
 * 이전에 확보한 operation 슬롯을 service 풀로 반환합니다.
 * 슬롯은 다시 free 상태로 표시되며 operation 객체는 재초기화됩니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @param operation 
 * Operation pointer previously acquired from this service.
 * 이 service에서 이전에 확보한 operation 포인터입니다.
 */
void CANServiceReleaseOperation(CANService *service, CANOperation *operation);





/**
 * @brief 
 * Acquire, prepare, and submit one send operation in a single call.
 * No allocation is performed. The returned operation pointer refers to
 * caller-owned pool storage. timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 송신 operation 하나를 한 번에 확보, 준비, 제출합니다.
 * 동적 할당은 수행되지 않습니다. 반환되는 operation 포인터는
 * caller-owned 풀 저장소를 가리킵니다. timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @param out_operation 
 * Output pointer that receives the acquired operation on success.
 * 성공 시 확보된 operation을 받는 출력 포인터입니다.
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
 * operation was acquired and submitted.
 * operation이 확보되고 제출되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or no context is bound.
 * 잘못된 인자이거나 바인딩된 context가 없습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * no free operation slot is available, or submission could not proceed.
 * 사용 가능한 operation 슬롯이 없거나 제출을 진행할 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANServiceSubmitSend(CANService *service,
                               CANOperation **out_operation,
                               const CANFrame *frame,
                               uint32_t timeout_ms);

/**
 * @brief 
 * Acquire, prepare, and submit one receive operation in a single call.
 * No allocation is performed. The returned operation pointer refers to
 * caller-owned pool storage. timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * 수신 operation 하나를 한 번에 확보, 준비, 제출합니다.
 * 동적 할당은 수행되지 않습니다. 반환되는 operation 포인터는
 * caller-owned 풀 저장소를 가리킵니다. timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @param out_operation 
 * Output pointer that receives the acquired operation on success.
 * 성공 시 확보된 operation을 받는 출력 포인터입니다.
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
 * operation was acquired and submitted.
 * operation이 확보되고 제출되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or no context is bound.
 * 잘못된 인자이거나 바인딩된 context가 없습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * no free operation slot is available, or submission could not proceed.
 * 사용 가능한 operation 슬롯이 없거나 제출을 진행할 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANServiceSubmitReceive(CANService *service,
                                  CANOperation **out_operation,
                                  CANFrame *frame,
                                  uint32_t timeout_ms);

/**
 * @brief 
 * Acquire, prepare, and submit one poll operation in a single call.
 * No allocation is performed. The returned operation pointer refers to
 * caller-owned pool storage. timeout_ms = 0 means immediate / non-blocking behavior.
 * Real time-based timeout behavior requires runtime hooks.
 * 
 * poll operation 하나를 한 번에 확보, 준비, 제출합니다.
 * 동적 할당은 수행되지 않습니다. 반환되는 operation 포인터는
 * caller-owned 풀 저장소를 가리킵니다. timeout_ms = 0 은 immediate / non-blocking 의미입니다.
 * 실제 시간 기반 timeout 동작을 위해서는 runtime hooks가 필요합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @param out_operation 
 * Output pointer that receives the acquired operation on success.
 * 성공 시 확보된 operation을 받는 출력 포인터입니다.
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
 * operation was acquired and submitted.
 * operation이 확보되고 제출되었습니다.
 *
 * @retval CAN_STATUS_EINVAL
 * invalid argument or no context is bound.
 * 잘못된 인자이거나 바인딩된 context가 없습니다.
 *
 * @retval CAN_STATUS_EBUSY
 * no free operation slot is available, or submission could not proceed.
 * 사용 가능한 operation 슬롯이 없거나 제출을 진행할 수 없습니다.
 *
 * @retval CAN_STATUS_EUNSUPPORTED
 * finite timeout was requested without required runtime hooks.
 * 필요한 runtime hooks 없이 finite timeout이 요청되었습니다.
 */
CANStatus CANServiceSubmitPoll(CANService *service,
                               CANOperation **out_operation,
                               uint32_t interest_mask,
                               uint32_t timeout_ms,
                               uint32_t *ready_mask);





/**
 * @brief 
 * Progress at most one pending operation through the bound context.
 * 
 * 바인딩된 context를 통해 최대 하나의 pending operation을 진행합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANServicePollOne(CANService *service);

/**
 * @brief 
 * Alias of one-step execution through the bound context.
 * 
 * 바인딩된 context를 통한 1-step 실행 별칭 함수입니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANServiceRunOne(CANService *service);

/**
 * @brief 
 * Dispatch at most one pending operation through the bound context.
 * 
 * 바인딩된 context를 통해 최대 하나의 pending operation을 dispatch합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
 *
 * @return 
 * One-step executor progress result.
 * 한 단계 executor 진행 결과를 반환합니다.
 */
CANExecutorRunOnceResult CANServiceDispatchOne(CANService *service);





/**
 * @brief 
 * Progress pending service operations with a bounded step budget.
 * 
 * 제한된 step 예산 안에서 service의 pending operation들을 진행합니다.
 *
 * @param service 
 * Service object.
 * service 객체입니다.
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
 * invalid argument or no context is bound.
 * 잘못된 인자이거나 바인딩된 context가 없습니다.
 */
CANStatus CANServicePoll(CANService *service,
                         uint32_t max_steps,
                         uint32_t *completed_count,
                         uint32_t *remaining_count);

#ifdef __cplusplus
}
#endif

#endif