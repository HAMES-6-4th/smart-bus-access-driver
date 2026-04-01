#ifndef CAN_TC3XX_NEGATIVE_TEST_H
#define CAN_TC3XX_NEGATIVE_TEST_H

#include <stdbool.h>
#include <stdint.h>

#include "can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CANTC3xxPlatformBindingNegativeTestResultStruct
{
    CANStatus init_status;
    CANStatus invalid_port_status;
    CANStatus first_open_status;
    CANStatus duplicate_core_status;
    CANStatus duplicate_node_status;
    CANStatus first_close_status;
} CANTC3xxPlatformBindingNegativeTestResult;

typedef struct CANTC3xxOpenConfigNegativeTestResultStruct
{
    CANStatus init_status;

    CANStatus fifo0_with_filter_status;
    CANStatus buffer_without_filter_status;
    CANStatus invalid_rx_path_status;
    CANStatus invalid_standard_filter_status;
    CANStatus invalid_extended_filter_status;
    CANStatus invalid_mode_status;
    CANStatus fd_on_classic_only_port_status;
    CANStatus zero_nominal_bitrate_status;
} CANTC3xxOpenConfigNegativeTestResult;

void CANTC3xxRunPlatformBindingNegativeTest(
    CANTC3xxPlatformBindingNegativeTestResult *result);

void CANTC3xxRunOpenConfigNegativeTest(
    CANTC3xxOpenConfigNegativeTestResult *result);

bool CANTC3xxPlatformBindingNegativeTestPassed(
    const CANTC3xxPlatformBindingNegativeTestResult *result);

bool CANTC3xxOpenConfigNegativeTestPassed(
    const CANTC3xxOpenConfigNegativeTestResult *result);

#ifdef __cplusplus
}
#endif

#endif
