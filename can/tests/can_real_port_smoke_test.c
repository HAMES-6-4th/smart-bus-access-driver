#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "can_platform.h"
#include "can_real_port_smoke_test.h"

void CANRealPortSmokeTestRun(CANRealPortSmokeTestResult *result)
{
    CANCore core;
    CANCoreOpenParams open_params;
    CANFrame tx_frame;
    CANFrame rx_frame;
    CANStatus status;

    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->init_status = CAN_STATUS_EINVAL;
    result->open_status = CAN_STATUS_EINVAL;
    result->pre_start_query_status = CAN_STATUS_EINVAL;
    result->pre_start_send_status = CAN_STATUS_EINVAL;
    result->pre_start_receive_status = CAN_STATUS_EINVAL;
    result->start_status = CAN_STATUS_EINVAL;
    result->post_start_query_status = CAN_STATUS_EINVAL;
    result->receive_empty_status = CAN_STATUS_EINVAL;
    result->send_status = CAN_STATUS_EINVAL;
    result->stop_status = CAN_STATUS_EINVAL;
    result->close_status = CAN_STATUS_EINVAL;
    result->pre_start_event_mask = 0U;
    result->post_start_event_mask = 0U;

    CANCoreInit(&core);

    status = CANPlatformInit();
    result->init_status = status;
    if (status != CAN_STATUS_OK)
    {
        return;
    }

    CANCoreInitOpenParams(&open_params);
    open_params.channel_config.timing.mode = CAN_MODE_CLASSIC;
    open_params.channel_config.timing.nominal_bitrate = 500000U;
    open_params.channel_config.enable_loopback = false;
    open_params.channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    open_params.channel_config.rx_filter.enabled = false;

    status = CANPlatformOpen(&core, "can0", &open_params);
    result->open_status = status;
    if (status != CAN_STATUS_OK)
    {
        CANPlatformDeinit();
        return;
    }

    result->pre_start_event_mask = 0xFFFFFFFFUL;
    result->pre_start_query_status =
        CANCoreQueryEvents(&core, &result->pre_start_event_mask);

    result->pre_start_query_behavior_ok =
        (result->pre_start_query_status == CAN_STATUS_OK) &&
        (result->pre_start_event_mask == 0U);

    memset(&tx_frame, 0, sizeof(tx_frame));
    tx_frame.id_type = CAN_ID_STANDARD;
    tx_frame.mode = CAN_MODE_CLASSIC;
    tx_frame.id = 0x321U;
    tx_frame.len = 4U;
    tx_frame.data[0] = 0xAAU;
    tx_frame.data[1] = 0xBBU;
    tx_frame.data[2] = 0xCCU;
    tx_frame.data[3] = 0xDDU;

    memset(&rx_frame, 0, sizeof(rx_frame));

    result->pre_start_send_status = CANCoreSend(&core, &tx_frame);
    result->pre_start_receive_status = CANCoreReceive(&core, &rx_frame);

    result->pre_start_behavior_ok =
        (result->pre_start_send_status == CAN_STATUS_EBUSY) &&
        (result->pre_start_receive_status == CAN_STATUS_EBUSY);

    status = CANCoreStart(&core);
    result->start_status = status;
    if (status != CAN_STATUS_OK)
    {
        result->close_status = CANPlatformClose(&core);
        CANPlatformDeinit();
        return;
    }

    result->poll_tx_ready_mask = 0U;
    result->poll_tx_ready_status =
        CANCorePoll(&core,
                    (uint32_t)CAN_CORE_EVENT_TX_READY,
                    0U,
                    &result->poll_tx_ready_mask);

    result->poll_tx_ready_behavior_ok =
        (result->poll_tx_ready_status == CAN_STATUS_OK) &&
        ((result->poll_tx_ready_mask & (uint32_t)CAN_CORE_EVENT_TX_READY) != 0U);

    result->post_start_event_mask = 0U;
    result->post_start_query_status =
        CANCoreQueryEvents(&core, &result->post_start_event_mask);

    result->post_start_query_behavior_ok =
        (result->post_start_query_status == CAN_STATUS_OK) &&
        ((result->post_start_event_mask & CAN_CORE_EVENT_TX_READY) != 0U);

    memset(&rx_frame, 0, sizeof(rx_frame));
    result->receive_empty_status = CANCoreReceive(&core, &rx_frame);
    result->empty_receive_behavior_ok =
        (result->receive_empty_status == CAN_STATUS_ENODATA) ||
        (result->receive_empty_status == CAN_STATUS_OK);

    result->send_status = CANCoreSend(&core, &tx_frame);

    result->stop_status = CANCoreStop(&core);
    result->close_status = CANPlatformClose(&core);

    result->lifecycle_ok =
        (result->init_status == CAN_STATUS_OK) &&
        (result->open_status == CAN_STATUS_OK) &&
        (result->pre_start_query_behavior_ok == true) &&
        (result->start_status == CAN_STATUS_OK) &&
        (result->poll_tx_ready_behavior_ok == true) &&
        (result->post_start_query_behavior_ok == true) &&
        (result->stop_status == CAN_STATUS_OK) &&
        (result->close_status == CAN_STATUS_OK);

    CANPlatformDeinit();
}