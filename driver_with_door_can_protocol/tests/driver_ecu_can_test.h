#ifndef DRIVER_ECU_CAN_TEST_H
#define DRIVER_ECU_CAN_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DriverEcuCanTestResultStruct
{
    bool init_config_ok;
    bool send_command_ok;
    bool poll_status_ok;
    bool status_alive_ok;
    bool status_timeout_ok;
    bool invalid_arg_ok;
} DriverEcuCanTestResult;

void DriverEcuCanRunTest(DriverEcuCanTestResult *result);
bool DriverEcuCanTestPassed(const DriverEcuCanTestResult *result);

#ifdef __cplusplus
}
#endif

#endif