#ifndef CAN_EXECUTOR_DRAIN_TEST_H
#define CAN_EXECUTOR_DRAIN_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANExecutorDrainTestResultStruct
{
    bool poll_one_alias_ok;
    bool run_until_idle_zero_budget_einval_ok;

    bool run_until_idle_complete_ok;
    bool run_until_idle_completed_count_ok;
    bool run_until_idle_remaining_count_ok;

    bool run_until_idle_budget_exhausted_ok;
    bool run_until_idle_budget_remaining_ok;

    bool run_until_idle_error_path_ok;
} CANExecutorDrainTestResult;

void CANExecutorDrainRunTest(CANExecutorDrainTestResult *result);
bool CANExecutorDrainTestPassed(const CANExecutorDrainTestResult *result);

#ifdef __cplusplus
}
#endif

#endif