#ifndef CAN_CORE_POLL_TEST_H
#define CAN_CORE_POLL_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANCorePollTestResultStruct
{
    bool null_arg_core_ok;
    bool null_arg_ready_mask_ok;
    bool zero_interest_mask_ok;
    bool invalid_interest_mask_ok;

    bool closed_poll_busy_ok;
    bool opened_poll_busy_ok;

    bool no_optional_poll_unsupported_ok;

    bool immediate_ready_ok;
    bool immediate_ready_mask_ok;
    bool immediate_last_status_ok;

    bool zero_timeout_no_ready_ok;
    bool zero_timeout_no_ready_last_status_ok;

    bool timeout_ready_ok;
    bool timeout_ready_mask_ok;
    bool timeout_ready_last_status_ok;

    bool timeout_expired_ok;
    bool timeout_expired_last_status_ok;

    bool query_error_propagation_ok;
    bool query_error_last_status_ok;
} CANCorePollTestResult;

void CANCoreRunPollTest(CANCorePollTestResult *result);
bool CANCorePollTestPassed(const CANCorePollTestResult *result);

#ifdef __cplusplus
}
#endif

#endif