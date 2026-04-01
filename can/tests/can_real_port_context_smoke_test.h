#ifndef CAN_REAL_PORT_CONTEXT_SMOKE_TEST_H
#define CAN_REAL_PORT_CONTEXT_SMOKE_TEST_H

#include <stdbool.h>

#include "can_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortContextSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;
    CANStatus start_status;

    bool bind_ok;

    CANStatus poll_submit_status;
    CANStatus send_submit_status;
    uint32_t pending_count_after_submit;

    uint32_t poll_ready_mask;
    CANExecutorRunOnceResult poll_one_result;
    CANExecutorRunOnceResult run_one_result;
    CANExecutorRunOnceResult dispatch_one_result;

    CANStatus drain_status;
    uint32_t drain_completed_count;
    uint32_t drain_remaining_count;

    bool poll_operation_ok;
    bool send_operation_ok;
    bool context_path_ok;

    CANStatus stop_status;
    CANStatus close_status;
    bool lifecycle_ok;
} CANRealPortContextSmokeTestResult;

void CANRealPortContextSmokeTestRun(CANRealPortContextSmokeTestResult *result);
bool CANRealPortContextSmokeTestPassed(const CANRealPortContextSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif