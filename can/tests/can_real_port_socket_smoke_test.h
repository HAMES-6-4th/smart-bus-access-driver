#ifndef CAN_REAL_PORT_SOCKET_SMOKE_TEST_H
#define CAN_REAL_PORT_SOCKET_SMOKE_TEST_H

#include <stdbool.h>

#include "can_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortSocketSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;
    CANStatus start_status;

    bool bind_ok;

    CANStatus poll_status;
    uint32_t ready_mask;

    CANStatus send_status;
    CANStatus get_error_state_status;
    CANCoreErrorState error_state;
    CANStatus recover_status;

    CANStatus stop_status;
    CANStatus close_status;

    bool socket_path_ok;
    bool lifecycle_ok;
} CANRealPortSocketSmokeTestResult;

void CANRealPortSocketSmokeTestRun(CANRealPortSocketSmokeTestResult *result);
bool CANRealPortSocketSmokeTestPassed(const CANRealPortSocketSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif