#ifndef CAN_CONTEXT_SUBMIT_HELPERS_TEST_H
#define CAN_CONTEXT_SUBMIT_HELPERS_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANContextSubmitHelpersTestResultStruct
{
    bool unbound_submit_send_einval_ok;
    bool submit_poll_helper_ok;
    bool submit_send_helper_ok;
    bool submit_receive_helper_ok;
    bool pending_count_ok;
    bool poll_one_ok;
    bool run_one_ok;
    bool dispatch_one_ok;
    bool receive_frame_ok;
    bool drain_ok;
    bool done_ok;
} CANContextSubmitHelpersTestResult;

void CANContextSubmitHelpersRunTest(CANContextSubmitHelpersTestResult *result);
bool CANContextSubmitHelpersTestPassed(const CANContextSubmitHelpersTestResult *result);

#ifdef __cplusplus
}
#endif

#endif