#ifndef CAN_CONTEXT_TEST_H
#define CAN_CONTEXT_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANContextTestResultStruct
{
    bool bind_ok;
    bool prepare_send_submit_ok;
    bool prepare_poll_submit_ok;
    bool pending_count_ok;
    bool poll_one_ok;
    bool run_one_ok;
    bool dispatch_one_ok;
    bool poll_ok;
    bool done_ok;
} CANContextTestResult;

void CANContextRunTest(CANContextTestResult *result);
bool CANContextTestPassed(const CANContextTestResult *result);

#ifdef __cplusplus
}
#endif

#endif