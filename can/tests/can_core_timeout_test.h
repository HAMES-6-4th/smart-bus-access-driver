#ifndef CAN_CORE_TIMEOUT_TEST_H
#define CAN_CORE_TIMEOUT_TEST_H

#include <stdbool.h>
#include <stdint.h>

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANCoreTimeoutSendTestResultStruct
{
    CANStatus open_status;
    CANStatus start_status;
    CANStatus send_timeout_status;
    CANStatus stop_status;
    CANStatus close_status;

    uint32_t send_call_count;
    uint32_t relax_call_count;
    CANStatus last_status;

    CANCoreStats stats;
} CANCoreTimeoutSendTestResult;

typedef struct CANCoreTimeoutReceiveTestResultStruct
{
    CANStatus open_status;
    CANStatus start_status;
    CANStatus receive_timeout_status;
    CANStatus stop_status;
    CANStatus close_status;

    uint32_t receive_call_count;
    uint32_t relax_call_count;
    CANStatus last_status;

    bool received;
    CANFrame frame;
    CANCoreStats stats;
} CANCoreTimeoutReceiveTestResult;

void CANCoreRunSendTimeoutSuccessTest(CANCoreTimeoutSendTestResult *result);
void CANCoreRunSendTimeoutExpiredTest(CANCoreTimeoutSendTestResult *result);

void CANCoreRunReceiveTimeoutSuccessTest(CANCoreTimeoutReceiveTestResult *result);
void CANCoreRunReceiveTimeoutExpiredTest(CANCoreTimeoutReceiveTestResult *result);

bool CANCoreSendTimeoutSuccessTestPassed(const CANCoreTimeoutSendTestResult *result);
bool CANCoreSendTimeoutExpiredTestPassed(const CANCoreTimeoutSendTestResult *result);

bool CANCoreReceiveTimeoutSuccessTestPassed(const CANCoreTimeoutReceiveTestResult *result);
bool CANCoreReceiveTimeoutExpiredTestPassed(const CANCoreTimeoutReceiveTestResult *result);

#ifdef __cplusplus
}
#endif

#endif