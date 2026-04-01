#ifndef CAN_CORE_TIMEOUT_INTEGRATION_TEST_H
#define CAN_CORE_TIMEOUT_INTEGRATION_TEST_H

#include <stdint.h>
#include <stdbool.h>

#include "can_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANTimeoutIntegrationSendImmediateTestResultStruct
{
    CANStatus init_status;
    CANStatus tx_open_status;
    CANStatus rx_open_status;
    CANStatus tx_start_status;
    CANStatus rx_start_status;

    CANStatus send_timeout_status;

    CANStatus tx_stop_status;
    CANStatus rx_stop_status;
    CANStatus tx_close_status;
    CANStatus rx_close_status;

    uint32_t tx_relax_call_count;
    CANStatus tx_last_status;
    CANCoreStats tx_stats;
} CANTimeoutIntegrationSendImmediateTestResult;

typedef struct CANTimeoutIntegrationReceiveExpiredTestResultStruct
{
    CANStatus init_status;
    CANStatus tx_open_status;
    CANStatus rx_open_status;
    CANStatus tx_start_status;
    CANStatus rx_start_status;

    CANStatus receive_timeout_status;

    CANStatus tx_stop_status;
    CANStatus rx_stop_status;
    CANStatus tx_close_status;
    CANStatus rx_close_status;

    uint32_t rx_relax_call_count;
    CANStatus rx_last_status;
    CANCoreStats rx_stats;
} CANTimeoutIntegrationReceiveExpiredTestResult;

void CANTimeoutIntegrationRunSendImmediateTest(
    CANTimeoutIntegrationSendImmediateTestResult *result);

void CANTimeoutIntegrationRunReceiveExpiredTest(
    CANTimeoutIntegrationReceiveExpiredTestResult *result);

bool CANTimeoutIntegrationSendImmediateTestPassed(
    const CANTimeoutIntegrationSendImmediateTestResult *result);

bool CANTimeoutIntegrationReceiveExpiredTestPassed(
    const CANTimeoutIntegrationReceiveExpiredTestResult *result);

#ifdef __cplusplus
}
#endif

#endif