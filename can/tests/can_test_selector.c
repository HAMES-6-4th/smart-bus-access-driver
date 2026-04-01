#include "can_test_selector.h"

#include <string.h>

#include "can_core_extended_id_test.h"
#include "can_core_fd_loopback_test.h"
#include "can_core_loopback_test.h"
#include "can_core_negative_test.h"
#include "can_core_timeout_integration_test.h"
#include "can_core_timeout_test.h"
#include "can_real_port_smoke_test.h"
#include "can_tc3xx_negative_test.h"
#include "can_core_optional_ops_test.h"
#include "can_core_poll_test.h"
#include "can_operation_test.h"
#include "can_executor_test.h"
#include "can_executor_drain_test.h"
#include "can_real_port_executor_smoke_test.h"
#include "can_executor_alias_test.h"
#include "can_context_test.h"
#include "can_real_port_context_smoke_test.h"
#include "can_context_submit_helpers_test.h"
#include "can_real_port_context_helpers_smoke_test.h"
#include "can_real_port_error_recover_smoke_test.h"
#include "can_service_test.h"
#include "can_real_port_service_smoke_test.h"
#include "can_socket_test.h"
#include "can_socket_sugar_test.h"
#include "can_real_port_socket_smoke_test.h"

const char *CANTestSelectorGetName(CANTestId id)
{
    switch (id)
    {
        case CAN_TEST_CORE_TIMEOUT_SEND_SUCCESS:
            return "core_timeout_send_success";

        case CAN_TEST_CORE_TIMEOUT_SEND_EXPIRED:
            return "core_timeout_send_expired";

        case CAN_TEST_CORE_TIMEOUT_RECEIVE_SUCCESS:
            return "core_timeout_receive_success";

        case CAN_TEST_CORE_TIMEOUT_RECEIVE_EXPIRED:
            return "core_timeout_receive_expired";

        case CAN_TEST_LOOPBACK_CLASSIC_FIFO0_ACCEPT_ALL:
            return "loopback_classic_fifo0_accept_all";

        case CAN_TEST_LOOPBACK_CLASSIC_BUFFER_EXACT:
            return "loopback_classic_buffer_exact";

        case CAN_TEST_LOOPBACK_FD_64_NO_BRS:
            return "loopback_fd_64_no_brs";

        case CAN_TEST_LOOPBACK_FD_64_BRS:
            return "loopback_fd_64_brs";

        case CAN_TEST_EXT_CLASSIC_BUFFER_EXACT:
            return "ext_classic_buffer_exact";

        case CAN_TEST_EXT_FD_BRS_BUFFER_EXACT:
            return "ext_fd_brs_buffer_exact";

        case CAN_TEST_EXT_CLASSIC_FIFO0_ACCEPT_ALL:
            return "ext_classic_fifo0_accept_all";

        case CAN_TEST_EXT_FD_BRS_FIFO0_ACCEPT_ALL:
            return "ext_fd_brs_fifo0_accept_all";

        case CAN_TEST_TIMEOUT_ITG_SEND_IMMEDIATE:
            return "timeout_itg_send_immediate";

        case CAN_TEST_TIMEOUT_ITG_RECEIVE_EXPIRED:
            return "timeout_itg_receive_expired";

        case CAN_TEST_CORE_LIFECYCLE_MATRIX:
            return "core_lifecycle_matrix";

        case CAN_TEST_CORE_INVALID_BINDING:
            return "core_invalid_binding";

        case CAN_TEST_TC3XX_PLATFORM_NEGATIVE:
            return "tc3xx_platform_negative";

        case CAN_TEST_TC3XX_OPEN_CONFIG_NEGATIVE:
            return "tc3xx_open_config_negative";

        case CAN_TEST_REAL_PORT_SMOKE:
            return "real_port_smoke";

        case CAN_TEST_CORE_OPTIONAL_OPS:
            return "core_optional_ops";

        case CAN_TEST_CORE_POLL:
            return "core_poll";
        
        case CAN_TEST_CORE_OPERATION:
            return "core_operation";

        case CAN_TEST_CORE_EXECUTOR:
            return "core_executor";
        
        case CAN_TEST_CORE_EXECUTOR_DRAIN:
            return "core_executor_drain";
        
        case CAN_TEST_REAL_PORT_EXECUTOR_SMOKE:
            return "real_port_executor_smoke";
        
        case CAN_TEST_CORE_EXECUTOR_ALIAS:
            return "core_executor_alias";

        case CAN_TEST_CORE_CONTEXT:
            return "core_context";

        case CAN_TEST_CORE_SERVICE:
            return "core_service";

        case CAN_TEST_REAL_PORT_CONTEXT_SMOKE:
            return "real_port_context_smoke";

        case CAN_TEST_CORE_CONTEXT_HELPERS:
            return "core_context_helpers";

        case CAN_TEST_REAL_PORT_CONTEXT_HELPERS_SMOKE:
            return "real_port_context_helpers_smoke";

        case CAN_TEST_REAL_PORT_ERROR_RECOVER_SMOKE:
            return "real_port_error_recover_smoke";
        
        case CAN_TEST_REAL_PORT_SERVICE_SMOKE:
            return "real_port_service_smoke";

        case CAN_TEST_CORE_SOCKET:
            return "core_socket";
        
        case CAN_TEST_CORE_SOCKET_SUGAR:
            return "core_socket_sugar";

        case CAN_TEST_REAL_PORT_SOCKET_SMOKE:
            return "real_port_socket_smoke";

        case CAN_TEST_NONE:
        default:
            return "none";
    }
}

CANTestCategory CANTestSelectorGetCategory(CANTestId id)
{
    switch (id)
    {
        case CAN_TEST_CORE_TIMEOUT_SEND_SUCCESS:
        case CAN_TEST_CORE_TIMEOUT_SEND_EXPIRED:
        case CAN_TEST_CORE_TIMEOUT_RECEIVE_SUCCESS:
        case CAN_TEST_CORE_TIMEOUT_RECEIVE_EXPIRED:
        case CAN_TEST_CORE_LIFECYCLE_MATRIX:
        case CAN_TEST_CORE_INVALID_BINDING:
        case CAN_TEST_CORE_OPTIONAL_OPS:
        case CAN_TEST_CORE_POLL:
        case CAN_TEST_CORE_OPERATION:
        case CAN_TEST_CORE_EXECUTOR:
        case CAN_TEST_CORE_EXECUTOR_DRAIN:
        case CAN_TEST_CORE_EXECUTOR_ALIAS:
        case CAN_TEST_CORE_CONTEXT:
        case CAN_TEST_CORE_CONTEXT_HELPERS:
        case CAN_TEST_CORE_SERVICE:
        case CAN_TEST_CORE_SOCKET:
        case CAN_TEST_CORE_SOCKET_SUGAR:
            return CAN_TEST_CATEGORY_CORE_UNIT;

        case CAN_TEST_LOOPBACK_CLASSIC_FIFO0_ACCEPT_ALL:
        case CAN_TEST_LOOPBACK_CLASSIC_BUFFER_EXACT:
        case CAN_TEST_LOOPBACK_FD_64_NO_BRS:
        case CAN_TEST_LOOPBACK_FD_64_BRS:
        case CAN_TEST_EXT_CLASSIC_BUFFER_EXACT:
        case CAN_TEST_EXT_FD_BRS_BUFFER_EXACT:
        case CAN_TEST_EXT_CLASSIC_FIFO0_ACCEPT_ALL:
        case CAN_TEST_EXT_FD_BRS_FIFO0_ACCEPT_ALL:
        case CAN_TEST_TIMEOUT_ITG_SEND_IMMEDIATE:
        case CAN_TEST_TIMEOUT_ITG_RECEIVE_EXPIRED:
            return CAN_TEST_CATEGORY_CORE_INTEGRATION;

        case CAN_TEST_TC3XX_PLATFORM_NEGATIVE:
        case CAN_TEST_TC3XX_OPEN_CONFIG_NEGATIVE:
            return CAN_TEST_CATEGORY_TC3XX_NEGATIVE;

        case CAN_TEST_REAL_PORT_SMOKE:
        case CAN_TEST_REAL_PORT_EXECUTOR_SMOKE:
        case CAN_TEST_REAL_PORT_CONTEXT_SMOKE:
        case CAN_TEST_REAL_PORT_CONTEXT_HELPERS_SMOKE:
        case CAN_TEST_REAL_PORT_ERROR_RECOVER_SMOKE:
        case CAN_TEST_REAL_PORT_SERVICE_SMOKE:
        case CAN_TEST_REAL_PORT_SOCKET_SMOKE:
            return CAN_TEST_CATEGORY_REAL_PORT;

        case CAN_TEST_NONE:
        default:
            return CAN_TEST_CATEGORY_NONE;
    }
}

bool CANTestSelectorRun(CANTestId id, CANTestRunResult *result)
{
    bool passed = false;

    if (result != 0)
    {
        memset(result, 0, sizeof(*result));
        result->id = id;
        result->name = CANTestSelectorGetName(id);
        result->category = CANTestSelectorGetCategory(id);
    }

    switch (id)
    {
        case CAN_TEST_CORE_TIMEOUT_SEND_SUCCESS:
        {
            CANCoreTimeoutSendTestResult test_result;
            CANCoreRunSendTimeoutSuccessTest(&test_result);
            passed = CANCoreSendTimeoutSuccessTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_TIMEOUT_SEND_EXPIRED:
        {
            CANCoreTimeoutSendTestResult test_result;
            CANCoreRunSendTimeoutExpiredTest(&test_result);
            passed = CANCoreSendTimeoutExpiredTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_TIMEOUT_RECEIVE_SUCCESS:
        {
            CANCoreTimeoutReceiveTestResult test_result;
            CANCoreRunReceiveTimeoutSuccessTest(&test_result);
            passed = CANCoreReceiveTimeoutSuccessTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_TIMEOUT_RECEIVE_EXPIRED:
        {
            CANCoreTimeoutReceiveTestResult test_result;
            CANCoreRunReceiveTimeoutExpiredTest(&test_result);
            passed = CANCoreReceiveTimeoutExpiredTestPassed(&test_result);
            break;
        }

        case CAN_TEST_LOOPBACK_CLASSIC_FIFO0_ACCEPT_ALL:
        {
            CANLoopbackAcceptAllFifo0TestResult test_result;
            CANLoopbackRunAcceptAllFifo0Test(&test_result);
            passed = CANLoopbackAcceptAllFifo0TestPassed(&test_result);
            break;
        }

        case CAN_TEST_LOOPBACK_CLASSIC_BUFFER_EXACT:
        {
            CANLoopbackExactBufferFilterTestResult test_result;
            CANLoopbackRunExactBufferFilterTest(&test_result);
            passed = CANLoopbackExactBufferFilterTestPassed(&test_result);
            break;
        }

        case CAN_TEST_LOOPBACK_FD_64_NO_BRS:
        {
            CANFDLoopbackOneShotResult test_result;
            CANFDLoopbackRunNoBrs64Test(&test_result);
            passed = CANFDLoopbackNoBrs64TestPassed(&test_result);
            break;
        }

        case CAN_TEST_LOOPBACK_FD_64_BRS:
        {
            CANFDLoopbackOneShotResult test_result;
            CANFDLoopbackRunBrs64Test(&test_result);
            passed = CANFDLoopbackBrs64TestPassed(&test_result);
            break;
        }

        case CAN_TEST_EXT_CLASSIC_BUFFER_EXACT:
        {
            CANExtendedIdExactFilterTestResult test_result;
            CANExtendedIdRunClassicExactBufferFilterTest(&test_result);
            passed = CANExtendedIdClassicExactBufferFilterTestPassed(&test_result);
            break;
        }

        case CAN_TEST_EXT_FD_BRS_BUFFER_EXACT:
        {
            CANExtendedIdExactFilterTestResult test_result;
            CANExtendedIdRunFdBrsExactBufferFilterTest(&test_result);
            passed = CANExtendedIdFdBrsExactBufferFilterTestPassed(&test_result);
            break;
        }

        case CAN_TEST_EXT_CLASSIC_FIFO0_ACCEPT_ALL:
        {
            CANExtendedIdAcceptAllTestResult test_result;
            CANExtendedIdRunClassicFifo0AcceptAllTest(&test_result);
            passed = CANExtendedIdClassicFifo0AcceptAllTestPassed(&test_result);
            break;
        }

        case CAN_TEST_EXT_FD_BRS_FIFO0_ACCEPT_ALL:
        {
            CANExtendedIdAcceptAllTestResult test_result;
            CANExtendedIdRunFdBrsFifo0AcceptAllTest(&test_result);
            passed = CANExtendedIdFdBrsFifo0AcceptAllTestPassed(&test_result);
            break;
        }

        case CAN_TEST_TIMEOUT_ITG_SEND_IMMEDIATE:
        {
            CANTimeoutIntegrationSendImmediateTestResult test_result;
            CANTimeoutIntegrationRunSendImmediateTest(&test_result);
            passed = CANTimeoutIntegrationSendImmediateTestPassed(&test_result);
            break;
        }

        case CAN_TEST_TIMEOUT_ITG_RECEIVE_EXPIRED:
        {
            CANTimeoutIntegrationReceiveExpiredTestResult test_result;
            CANTimeoutIntegrationRunReceiveExpiredTest(&test_result);
            passed = CANTimeoutIntegrationReceiveExpiredTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_LIFECYCLE_MATRIX:
        {
            CANCoreLifecycleMatrixTestResult test_result;
            CANCoreRunLifecycleMatrixTest(&test_result);
            passed = CANCoreLifecycleMatrixTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_INVALID_BINDING:
        {
            CANCoreInvalidBindingTestResult test_result;
            CANCoreRunInvalidBindingTest(&test_result);
            passed = CANCoreInvalidBindingTestPassed(&test_result);
            break;
        }

        case CAN_TEST_TC3XX_PLATFORM_NEGATIVE:
        {
            CANTC3xxPlatformBindingNegativeTestResult test_result;
            CANTC3xxRunPlatformBindingNegativeTest(&test_result);
            passed = CANTC3xxPlatformBindingNegativeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_TC3XX_OPEN_CONFIG_NEGATIVE:
        {
            CANTC3xxOpenConfigNegativeTestResult test_result;
            CANTC3xxRunOpenConfigNegativeTest(&test_result);
            passed = CANTC3xxOpenConfigNegativeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_EXECUTOR_ALIAS:
        {
            CANExecutorAliasTestResult test_result;
            CANExecutorAliasRunTest(&test_result);
            passed = CANExecutorAliasTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_SMOKE:
        {
            CANRealPortSmokeTestResult test_result;
            CANRealPortSmokeTestRun(&test_result);
            passed =
                (test_result.init_status == CAN_STATUS_OK) &&
                (test_result.open_status == CAN_STATUS_OK) &&
                (test_result.pre_start_query_behavior_ok == true) &&
                (test_result.pre_start_behavior_ok == true) &&
                (test_result.start_status == CAN_STATUS_OK) &&
                (test_result.post_start_query_behavior_ok == true) &&
                (test_result.empty_receive_behavior_ok == true) &&
                (test_result.send_status == CAN_STATUS_OK) &&
                (test_result.stop_status == CAN_STATUS_OK) &&
                (test_result.close_status == CAN_STATUS_OK) &&
                (test_result.lifecycle_ok == true);
            break;
        }

        case CAN_TEST_CORE_OPTIONAL_OPS:
        {
            CANCoreOptionalOpsTestResult test_result;
            CANCoreRunOptionalOpsTest(&test_result);
            passed = CANCoreOptionalOpsTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_POLL:
        {
            CANCorePollTestResult test_result;
            CANCoreRunPollTest(&test_result);
            passed = CANCorePollTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_OPERATION:
        {
            CANOperationTestResult test_result;
            CANOperationRunTest(&test_result);
            passed = CANOperationTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_EXECUTOR:
        {
            CANExecutorTestResult test_result;
            CANExecutorRunTest(&test_result);
            passed = CANExecutorTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_EXECUTOR_DRAIN:
        {
            CANExecutorDrainTestResult test_result;
            CANExecutorDrainRunTest(&test_result);
            passed = CANExecutorDrainTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_EXECUTOR_SMOKE:
        {
            CANRealPortExecutorSmokeTestResult test_result;
            CANRealPortExecutorSmokeTestRun(&test_result);
            passed = CANRealPortExecutorSmokeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_CONTEXT:
        {
            CANContextTestResult test_result;
            CANContextRunTest(&test_result);
            passed = CANContextTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_CONTEXT_SMOKE:
        {
            CANRealPortContextSmokeTestResult test_result;
            CANRealPortContextSmokeTestRun(&test_result);
            passed = CANRealPortContextSmokeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_CONTEXT_HELPERS:
        {
            CANContextSubmitHelpersTestResult test_result;
            CANContextSubmitHelpersRunTest(&test_result);
            passed = CANContextSubmitHelpersTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_CONTEXT_HELPERS_SMOKE:
        {
            CANRealPortContextHelpersSmokeTestResult test_result;
            CANRealPortContextHelpersSmokeTestRun(&test_result);
            passed = CANRealPortContextHelpersSmokeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_ERROR_RECOVER_SMOKE:
        {
            CANRealPortErrorRecoverSmokeTestResult test_result;
            CANRealPortErrorRecoverSmokeTestRun(&test_result);
            passed = CANRealPortErrorRecoverSmokeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_SERVICE:
        {
            CANServiceTestResult test_result;
            CANServiceRunTest(&test_result);
            passed = CANServiceTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_SERVICE_SMOKE:
        {
            CANRealPortServiceSmokeTestResult test_result;
            CANRealPortServiceSmokeTestRun(&test_result);
            passed = CANRealPortServiceSmokeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_SOCKET:
        {
            CANSocketTestResult test_result;
            CANSocketRunTest(&test_result);
            passed = CANSocketTestPassed(&test_result);
            break;
        }

        case CAN_TEST_REAL_PORT_SOCKET_SMOKE:
        {
            CANRealPortSocketSmokeTestResult test_result;
            CANRealPortSocketSmokeTestRun(&test_result);
            passed = CANRealPortSocketSmokeTestPassed(&test_result);
            break;
        }

        case CAN_TEST_CORE_SOCKET_SUGAR:
        {
            CANSocketSugarTestResult test_result;
            CANSocketSugarRunTest(&test_result);
            passed = CANSocketSugarTestPassed(&test_result);
            break;
        }

        case CAN_TEST_NONE:
        default:
            if (result != 0)
            {
                result->executed = false;
                result->passed = false;
            }
            return false;
    }

    if (result != 0)
    {
        result->executed = true;
        result->passed = passed;
    }

    return passed;
}