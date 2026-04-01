#ifndef CAN_REAL_PORT_SMOKE_TEST_H
#define CAN_REAL_PORT_SMOKE_TEST_H

#include "can_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANRealPortSmokeTestResultStruct
{
    CANStatus init_status;
    CANStatus open_status;

    CANStatus pre_start_query_status;
    uint32_t pre_start_event_mask;

    CANStatus pre_start_send_status;
    CANStatus pre_start_receive_status;

    CANStatus start_status;

    CANStatus post_start_query_status;
    uint32_t post_start_event_mask;

    CANStatus receive_empty_status;
    CANStatus send_status;
    CANStatus stop_status;
    CANStatus close_status;

    CANStatus poll_tx_ready_status;
    uint32_t poll_tx_ready_mask;

    bool pre_start_query_behavior_ok;
    bool pre_start_behavior_ok;
    bool post_start_query_behavior_ok;
    bool empty_receive_behavior_ok;
    bool lifecycle_ok;
    bool poll_tx_ready_behavior_ok;
} CANRealPortSmokeTestResult;

void CANRealPortSmokeTestRun(CANRealPortSmokeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif