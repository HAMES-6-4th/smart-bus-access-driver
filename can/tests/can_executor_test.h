#ifndef CAN_EXECUTOR_TEST_H
#define CAN_EXECUTOR_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANExecutorTestResultStruct
{
    bool init_empty_ok;
    bool empty_run_once_ok;

    bool submit_requires_started_core_ok;
    bool duplicate_submit_busy_ok;
    bool capacity_limit_busy_ok;

    bool round_robin_pending_ok;
    bool round_robin_completion_order_ok;
    bool pending_count_tracking_ok;

    bool failure_completion_removal_ok;
} CANExecutorTestResult;

void CANExecutorRunTest(CANExecutorTestResult *result);
bool CANExecutorTestPassed(const CANExecutorTestResult *result);

#ifdef __cplusplus
}
#endif

#endif