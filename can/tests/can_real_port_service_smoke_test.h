#ifndef CAN_REAL_PORT_SERVICE_SMOKE_TEST_H
#define CAN_REAL_PORT_SERVICE_SMOKE_TEST_H

#include <stdbool.h>

#include "can_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortServiceSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;
    CANStatus start_status;

    bool bind_ok;

    CANStatus submit_poll_status;
    CANStatus submit_send_status;

    uint32_t in_use_count_after_submit;
    uint32_t pending_count_after_submit;

    uint32_t poll_ready_mask;
    CANExecutorRunOnceResult poll_one_result;
    CANExecutorRunOnceResult run_one_result;

    bool poll_operation_ok;
    bool send_operation_ok;
    bool release_ok;
    bool service_path_ok;

    CANStatus stop_status;
    CANStatus close_status;
    bool lifecycle_ok;
} CANRealPortServiceSmokeTestResult;

void CANRealPortServiceSmokeTestRun(CANRealPortServiceSmokeTestResult *result);
bool CANRealPortServiceSmokeTestPassed(const CANRealPortServiceSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif