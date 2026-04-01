#ifndef CAN_REAL_PORT_ERROR_RECOVER_SMOKE_TEST_H
#define CAN_REAL_PORT_ERROR_RECOVER_SMOKE_TEST_H

#include <stdbool.h>

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortErrorRecoverSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;
    CANStatus start_status;

    CANStatus get_error_state_status;
    CANCoreErrorState error_state;

    CANStatus recover_status;

    CANStatus stop_status;
    CANStatus close_status;

    bool error_state_path_ok;
    bool recover_path_ok;
    bool lifecycle_ok;
} CANRealPortErrorRecoverSmokeTestResult;

void CANRealPortErrorRecoverSmokeTestRun(CANRealPortErrorRecoverSmokeTestResult *result);
bool CANRealPortErrorRecoverSmokeTestPassed(const CANRealPortErrorRecoverSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif