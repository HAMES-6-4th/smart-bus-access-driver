#ifndef CAN_REAL_PORT_EXECUTOR_SMOKE_TEST_H
#define CAN_REAL_PORT_EXECUTOR_SMOKE_TEST_H

#include <stdbool.h>

#include "can_types.h"
#include "can_executor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortExecutorSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;
    CANStatus start_status;

    CANStatus poll_submit_status;
    CANStatus send_submit_status;

    uint32_t pending_count_after_submit;

    uint32_t poll_ready_mask;
    CANExecutorRunOnceResult poll_one_result_1;
    CANExecutorRunOnceResult poll_one_result_2;

    CANStatus run_until_idle_status;
    uint32_t run_until_idle_completed_count;
    uint32_t run_until_idle_remaining_count;

    CANStatus stop_status;
    CANStatus close_status;

    bool poll_operation_ok;
    bool send_operation_ok;
    bool executor_path_ok;
    bool lifecycle_ok;
} CANRealPortExecutorSmokeTestResult;

void CANRealPortExecutorSmokeTestRun(CANRealPortExecutorSmokeTestResult *result);
bool CANRealPortExecutorSmokeTestPassed(const CANRealPortExecutorSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif