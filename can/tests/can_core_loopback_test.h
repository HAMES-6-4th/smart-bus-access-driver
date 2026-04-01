#ifndef CAN_CORE_LOOPBACK_TEST_H
#define CAN_CORE_LOOPBACK_TEST_H

#include <stdint.h>
#include <stdbool.h>

#include "can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANLoopbackOneShotResultStruct
{
    uint32_t tx_id;

    CANStatus init_status;
    CANStatus tx_open_status;
    CANStatus rx_open_status;
    CANStatus tx_start_status;
    CANStatus rx_start_status;
    CANStatus send_status;
    CANStatus receive_status;
    CANStatus tx_stop_status;
    CANStatus rx_stop_status;
    CANStatus tx_close_status;
    CANStatus rx_close_status;

    bool received;
    bool id_match;

    uint32_t rx_id;
    uint8_t rx_len;
    uint8_t rx_first_byte;
    uint32_t poll_count;
} CANLoopbackOneShotResult;

typedef struct CANLoopbackAcceptAllFifo0TestResultStruct
{
    CANLoopbackOneShotResult probe_000;
    CANLoopbackOneShotResult probe_001;
    CANLoopbackOneShotResult probe_123;
    CANLoopbackOneShotResult probe_7ff;
} CANLoopbackAcceptAllFifo0TestResult;

typedef struct CANLoopbackExactBufferFilterTestResultStruct
{
    CANLoopbackOneShotResult match_123;
    CANLoopbackOneShotResult miss_124;
} CANLoopbackExactBufferFilterTestResult;

void CANLoopbackRunAcceptAllFifo0Test(CANLoopbackAcceptAllFifo0TestResult *result);
void CANLoopbackRunExactBufferFilterTest(CANLoopbackExactBufferFilterTestResult *result);

bool CANLoopbackAcceptAllFifo0TestPassed(const CANLoopbackAcceptAllFifo0TestResult *result);
bool CANLoopbackExactBufferFilterTestPassed(const CANLoopbackExactBufferFilterTestResult *result);

#ifdef __cplusplus
}
#endif

#endif