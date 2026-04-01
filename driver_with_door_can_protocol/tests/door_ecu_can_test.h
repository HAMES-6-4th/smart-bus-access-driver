#ifndef DOOR_ECU_CAN_TEST_H
#define DOOR_ECU_CAN_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DoorEcuCanTestResultStruct
{
    bool init_config_ok;
    bool poll_command_ok;
    bool command_alive_ok;
    bool command_timeout_ok;
    bool force_safe_ok;
    bool publish_status_ok;
    bool alive_toggle_ok;
    bool invalid_arg_ok;
} DoorEcuCanTestResult;

void DoorEcuCanRunTest(DoorEcuCanTestResult *result);
bool DoorEcuCanTestPassed(const DoorEcuCanTestResult *result);

#ifdef __cplusplus
}
#endif

#endif