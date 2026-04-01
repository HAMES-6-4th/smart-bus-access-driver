#ifndef CAN_EXECUTOR_ALIAS_TEST_H
#define CAN_EXECUTOR_ALIAS_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANExecutorAliasTestResultStruct
{
    bool run_one_alias_ok;
    bool dispatch_one_alias_ok;
    bool poll_alias_ok;
} CANExecutorAliasTestResult;

void CANExecutorAliasRunTest(CANExecutorAliasTestResult *result);
bool CANExecutorAliasTestPassed(const CANExecutorAliasTestResult *result);

#ifdef __cplusplus
}
#endif

#endif