#ifndef CAN_CORE_NEGATIVE_TEST_H
#define CAN_CORE_NEGATIVE_TEST_H

#include <stdbool.h>
#include <stdint.h>

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANCoreLifecycleMatrixTestResultStruct
{
    CANStatus start_before_open_status;
    CANStatus stop_before_open_status;
    CANStatus send_before_open_status;
    CANStatus receive_before_open_status;

    CANStatus open_status;
    CANStatus double_open_status;

    CANStatus send_before_start_status;
    CANStatus receive_before_start_status;

    CANStatus start_status;
    CANStatus double_start_status;

    CANStatus send_after_start_status;
    CANStatus receive_after_start_status;

    CANStatus stop_status;
    CANStatus double_stop_status;

    CANStatus send_after_stop_status;
    CANStatus receive_after_stop_status;

    CANStatus close_status;
    CANStatus start_after_close_status;
    CANStatus stop_after_close_status;
    CANStatus close_after_close_status;

    CANStatus last_status;
    CANCoreStats stats_before_close;
} CANCoreLifecycleMatrixTestResult;

typedef struct CANCoreInvalidBindingTestResultStruct
{
    CANStatus null_core_status;
    CANStatus null_binding_status;
    CANStatus null_params_status;

    CANStatus missing_ops_status;
    CANStatus missing_open_status;
    CANStatus missing_close_status;
    CANStatus missing_start_status;
    CANStatus missing_stop_status;
    CANStatus missing_send_status;
    CANStatus missing_receive_status;
} CANCoreInvalidBindingTestResult;

void CANCoreRunLifecycleMatrixTest(CANCoreLifecycleMatrixTestResult *result);
void CANCoreRunInvalidBindingTest(CANCoreInvalidBindingTestResult *result);

bool CANCoreLifecycleMatrixTestPassed(const CANCoreLifecycleMatrixTestResult *result);
bool CANCoreInvalidBindingTestPassed(const CANCoreInvalidBindingTestResult *result);

#ifdef __cplusplus
}
#endif

#endif
