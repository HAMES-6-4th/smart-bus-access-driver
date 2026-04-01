#ifndef CAN_CORE_FD_LOOPBACK_TEST_H
#define CAN_CORE_FD_LOOPBACK_TEST_H

#include <stdint.h>
#include <stdbool.h>

#include "can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANFDLoopbackOneShotResultStruct
{
    CANMode tx_mode;
    uint32_t tx_id;
    uint8_t tx_len;

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
    bool mode_match;
    bool len_match;
    bool payload_match;

    uint32_t rx_id;
    CANMode rx_mode;
    uint8_t rx_len;
    uint8_t rx_first_byte;
    uint8_t rx_last_byte;
    uint32_t poll_count;
} CANFDLoopbackOneShotResult;

void CANFDLoopbackRunNoBrs64Test(CANFDLoopbackOneShotResult *result);
void CANFDLoopbackRunBrs64Test(CANFDLoopbackOneShotResult *result);

bool CANFDLoopbackNoBrs64TestPassed(const CANFDLoopbackOneShotResult *result);
bool CANFDLoopbackBrs64TestPassed(const CANFDLoopbackOneShotResult *result);

#ifdef __cplusplus
}
#endif

#endif