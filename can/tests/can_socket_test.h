#ifndef CAN_SOCKET_TEST_H
#define CAN_SOCKET_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANSocketTestResultStruct
{
    bool init_bind_ok;
    bool unbound_send_einval_ok;
    bool send_ok;
    bool receive_ok;
    bool send_timeout_ok;
    bool poll_ok;
    bool error_state_ok;
    bool recover_ok;
    bool started_state_ok;
} CANSocketTestResult;

void CANSocketRunTest(CANSocketTestResult *result);
bool CANSocketTestPassed(const CANSocketTestResult *result);

#ifdef __cplusplus
}
#endif

#endif