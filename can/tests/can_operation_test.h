#ifndef CAN_OPERATION_TEST_H
#define CAN_OPERATION_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANOperationTestResultStruct
{
    bool send_immediate_ok;
    bool send_busy_zero_timeout_ok;

    bool receive_pending_then_ok;
    bool receive_timeout_ok;

    bool poll_pending_then_ok;
    bool poll_ready_mask_ok;

    bool finite_timeout_without_runtime_unsupported_ok;

    bool submit_requires_started_core_ok;
    bool completed_run_once_stable_ok;
} CANOperationTestResult;

void CANOperationRunTest(CANOperationTestResult *result);
bool CANOperationTestPassed(const CANOperationTestResult *result);

#ifdef __cplusplus
}
#endif

#endif