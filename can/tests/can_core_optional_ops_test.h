#ifndef CAN_CORE_OPTIONAL_OPS_TEST_H
#define CAN_CORE_OPTIONAL_OPS_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANCoreOptionalOpsTestResultStruct
{
    bool null_arg_query_ok;
    bool null_arg_error_state_ok;
    bool null_arg_recover_ok;

    bool closed_query_unsupported_ok;
    bool closed_error_state_unsupported_ok;
    bool closed_recover_unsupported_ok;

    bool no_optional_query_unsupported_ok;
    bool no_optional_error_state_unsupported_ok;
    bool no_optional_recover_unsupported_ok;

    bool ok_query_forward_ok;
    bool ok_error_state_forward_ok;
    bool ok_recover_ok;

    bool query_status_propagation_ok;
    bool error_state_status_propagation_ok;
    bool recover_status_propagation_ok;

    bool query_last_status_ok;
    bool error_state_last_status_ok;
    bool recover_last_status_ok;
} CANCoreOptionalOpsTestResult;

void CANCoreRunOptionalOpsTest(CANCoreOptionalOpsTestResult *result);
bool CANCoreOptionalOpsTestPassed(const CANCoreOptionalOpsTestResult *result);

#ifdef __cplusplus
}
#endif

#endif