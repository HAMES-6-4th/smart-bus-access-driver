#ifndef CAN_REAL_PORT_CONTEXT_HELPERS_SMOKE_TEST_H
#define CAN_REAL_PORT_CONTEXT_HELPERS_SMOKE_TEST_H

#include <stdbool.h>

#include "can_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortContextHelpersSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;
    CANStatus start_status;

    bool bind_ok;

    CANStatus submit_poll_status;
    CANStatus submit_send_status;
    uint32_t pending_count_after_submit;

    uint32_t poll_ready_mask;
    CANExecutorRunOnceResult poll_one_result;
    CANExecutorRunOnceResult run_one_result;
    CANExecutorRunOnceResult dispatch_one_result;

    CANStatus drain_status;
    uint32_t drain_completed_count;
    uint32_t drain_remaining_count;

    bool poll_helper_ok;
    bool send_helper_ok;
    bool context_helper_path_ok;

    CANStatus stop_status;
    CANStatus close_status;
    bool lifecycle_ok;
} CANRealPortContextHelpersSmokeTestResult;

void CANRealPortContextHelpersSmokeTestRun(CANRealPortContextHelpersSmokeTestResult *result);
bool CANRealPortContextHelpersSmokeTestPassed(const CANRealPortContextHelpersSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif