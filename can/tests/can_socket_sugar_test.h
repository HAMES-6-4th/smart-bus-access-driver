#ifndef CAN_SOCKET_SUGAR_TEST_H
#define CAN_SOCKET_SUGAR_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANSocketSugarTestResultStruct
{
    bool unbound_send_now_einval_ok;
    bool send_now_ok;
    bool receive_now_ok;
    bool wait_tx_ready_ok;
    bool wait_rx_ready_zero_timeout_nodata_ok;
    bool wait_rx_ready_timeout_ok;
} CANSocketSugarTestResult;

void CANSocketSugarRunTest(CANSocketSugarTestResult *result);
bool CANSocketSugarTestPassed(const CANSocketSugarTestResult *result);

#ifdef __cplusplus
}
#endif

#endif