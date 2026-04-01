#ifndef CAN_SERVICE_TEST_H
#define CAN_SERVICE_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANServiceTestResultStruct
{
    bool init_ok;
    bool bind_ok;
    bool submit_poll_ok;
    bool submit_send_ok;
    bool submit_receive_ok;
    bool capacity_busy_ok;
    bool poll_one_ok;
    bool run_one_ok;
    bool dispatch_one_ok;
    bool receive_frame_ok;
    bool in_use_count_ok;
    bool release_ok;
} CANServiceTestResult;

void CANServiceRunTest(CANServiceTestResult *result);
bool CANServiceTestPassed(const CANServiceTestResult *result);

#ifdef __cplusplus
}
#endif

#endif