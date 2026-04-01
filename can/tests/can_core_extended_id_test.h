#ifndef CAN_CORE_EXTENDED_ID_TEST_H
#define CAN_CORE_EXTENDED_ID_TEST_H

#include <stdint.h>
#include <stdbool.h>

#include "can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANExtendedIdProbeResultStruct
{
    uint32_t tx_id;
    uint8_t tx_len;
    CANMode tx_mode;

    CANStatus send_status;
    CANStatus receive_status;

    bool received;
    bool id_match;
    bool mode_match;
    bool len_match;
    bool payload_match;

    uint32_t rx_id;
    uint8_t rx_len;
    CANMode rx_mode;
    uint8_t rx_first_byte;
    uint8_t rx_last_byte;
    uint32_t poll_count;
} CANExtendedIdProbeResult;

typedef struct CANExtendedIdAcceptAllTestResultStruct
{
    CANStatus init_status;
    CANStatus tx_open_status;
    CANStatus rx_open_status;
    CANStatus tx_start_status;
    CANStatus rx_start_status;
    CANStatus tx_stop_status;
    CANStatus rx_stop_status;
    CANStatus tx_close_status;
    CANStatus rx_close_status;

    CANExtendedIdProbeResult probe;
} CANExtendedIdAcceptAllTestResult;

typedef struct CANExtendedIdExactFilterTestResultStruct
{
    CANStatus init_status;
    CANStatus tx_open_status;
    CANStatus rx_open_status;
    CANStatus tx_start_status;
    CANStatus rx_start_status;
    CANStatus tx_stop_status;
    CANStatus rx_stop_status;
    CANStatus tx_close_status;
    CANStatus rx_close_status;

    CANExtendedIdProbeResult match_probe;
    CANExtendedIdProbeResult miss_probe;
} CANExtendedIdExactFilterTestResult;

void CANExtendedIdRunClassicExactBufferFilterTest(CANExtendedIdExactFilterTestResult *result);
void CANExtendedIdRunFdBrsExactBufferFilterTest(CANExtendedIdExactFilterTestResult *result);

void CANExtendedIdRunClassicFifo0AcceptAllTest(CANExtendedIdAcceptAllTestResult *result);
void CANExtendedIdRunFdBrsFifo0AcceptAllTest(CANExtendedIdAcceptAllTestResult *result);

bool CANExtendedIdClassicExactBufferFilterTestPassed(const CANExtendedIdExactFilterTestResult *result);
bool CANExtendedIdFdBrsExactBufferFilterTestPassed(const CANExtendedIdExactFilterTestResult *result);

bool CANExtendedIdClassicFifo0AcceptAllTestPassed(const CANExtendedIdAcceptAllTestResult *result);
bool CANExtendedIdFdBrsFifo0AcceptAllTestPassed(const CANExtendedIdAcceptAllTestResult *result);

#ifdef __cplusplus
}
#endif

#endif