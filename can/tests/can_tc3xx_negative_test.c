#include "can_tc3xx_negative_test.h"

#include <string.h>

#include "board_types.h"
#include "can_core.h"
#include "can_platform.h"
#include "can_tc3xx.h"

#include <string.h>

static void CANTC3xxWriteInvalidRxPath(CANChannelConfig *config)
{
    unsigned char invalid_bytes[sizeof(config->rx_path)];
    uint32_t i;

    if (config == 0)
    {
        return;
    }

    for (i = 0U; i < (uint32_t)sizeof(invalid_bytes); ++i)
    {
        invalid_bytes[i] = 0xA5U;
    }

    memcpy(&config->rx_path, invalid_bytes, sizeof(config->rx_path));
}

static void CANTC3xxWriteInvalidMode(CANChannelConfig *config)
{
    unsigned char invalid_bytes[sizeof(config->timing.mode)];
    uint32_t i;

    if (config == 0)
    {
        return;
    }

    for (i = 0U; i < (uint32_t)sizeof(invalid_bytes); ++i)
    {
        invalid_bytes[i] = 0xA5U;
    }

    memcpy(&config->timing.mode, invalid_bytes, sizeof(config->timing.mode));
}

static void CANTC3xxNegativeInitOpenParams(CANCoreOpenParams *params)
{
    if (params == 0)
    {
        return;
    }

    CANCoreInitOpenParams(params);
    params->channel_config.timing.mode = CAN_MODE_CLASSIC;
    params->channel_config.timing.nominal_bitrate = 500000U;
    params->channel_config.enable_loopback = true;
    params->channel_config.rx_path = CAN_RX_PATH_DEFAULT;
    params->channel_config.rx_filter.enabled = false;
}

static void CANTC3xxNegativeInitConfig(CANChannelConfig *config)
{
    if (config == 0)
    {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->timing.mode = CAN_MODE_CLASSIC;
    config->timing.nominal_bitrate = 500000U;
    config->timing.data_bitrate = 2000000U;
    config->enable_loopback = true;
    config->rx_path = CAN_RX_PATH_DEFAULT;
    config->rx_filter.enabled = false;
}

static void CANTC3xxNegativeInitSyntheticPort(BoardCANPort *port,
                                              const char *name,
                                              BoardMCUCANNode node)
{
    if (port == 0)
    {
        return;
    }

    memset(port, 0, sizeof(*port));
    port->name = name;
    port->mcu_inst = BOARD_MCU_CAN_INST_0;
    port->mcu_node = node;
    port->phy_type = BOARD_CAN_PHY_NONE;
    port->fd_support = BOARD_CAN_FD_SUPPORTED;
    port->default_nominal_bitrate = 500000U;
    port->default_data_bitrate = 2000000U;
}

static CANStatus CANTC3xxNegativeTryOpen(const BoardCANPort *port,
                                         const CANChannelConfig *config)
{
    CANTC3xxChannel channel;
    CANStatus status;

    canTC3xxChannelInit(&channel);
    status = canTC3xxOpen(&channel, port, config);

    if (status == CAN_STATUS_OK)
    {
        (void)canTC3xxClose(&channel);
    }

    return status;
}

static void CANTC3xxNegativeInitPlatformResult(
    CANTC3xxPlatformBindingNegativeTestResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->init_status = CAN_STATUS_EINVAL;
    result->invalid_port_status = CAN_STATUS_EINVAL;
    result->first_open_status = CAN_STATUS_EINVAL;
    result->duplicate_core_status = CAN_STATUS_EINVAL;
    result->duplicate_node_status = CAN_STATUS_EINVAL;
    result->first_close_status = CAN_STATUS_EINVAL;
}

static void CANTC3xxNegativeInitOpenConfigResult(
    CANTC3xxOpenConfigNegativeTestResult *result)
{
    if (result == 0)
    {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->init_status = CAN_STATUS_EINVAL;
    result->fifo0_with_filter_status = CAN_STATUS_EINVAL;
    result->buffer_without_filter_status = CAN_STATUS_EINVAL;
    result->invalid_rx_path_status = CAN_STATUS_EINVAL;
    result->invalid_standard_filter_status = CAN_STATUS_EINVAL;
    result->invalid_extended_filter_status = CAN_STATUS_EINVAL;
    result->invalid_mode_status = CAN_STATUS_EINVAL;
    result->fd_on_classic_only_port_status = CAN_STATUS_EINVAL;
    result->zero_nominal_bitrate_status = CAN_STATUS_EINVAL;
}

void CANTC3xxRunPlatformBindingNegativeTest(
    CANTC3xxPlatformBindingNegativeTestResult *result)
{
    CANCore core_a;
    CANCore core_b;
    CANCore core_invalid;
    CANCoreOpenParams params;

    if (result == 0)
    {
        return;
    }

    CANTC3xxNegativeInitPlatformResult(result);

    CANCoreInit(&core_a);
    CANCoreInit(&core_b);
    CANCoreInit(&core_invalid);
    CANTC3xxNegativeInitOpenParams(&params);

    result->init_status = CANPlatformInit();
    if (result->init_status != CAN_STATUS_OK)
    {
        return;
    }

    result->invalid_port_status = CANPlatformOpen(&core_invalid, "can_invalid", &params);

    result->first_open_status = CANPlatformOpen(&core_a, "can0_lb0", &params);
    if (result->first_open_status == CAN_STATUS_OK)
    {
        result->duplicate_core_status = CANPlatformOpen(&core_a, "can0_lb1", &params);
        result->duplicate_node_status = CANPlatformOpen(&core_b, "can0_lb0", &params);
        result->first_close_status = CANPlatformClose(&core_a);
    }

    CANPlatformDeinit();
}

void CANTC3xxRunOpenConfigNegativeTest(
    CANTC3xxOpenConfigNegativeTestResult *result)
{
    BoardCANPort port;
    BoardCANPort classic_only_port;
    BoardCANPort zero_bitrate_port;
    CANChannelConfig config;

    if (result == 0)
    {
        return;
    }

    CANTC3xxNegativeInitOpenConfigResult(result);

    result->init_status = CANPlatformInit();
    if (result->init_status != CAN_STATUS_OK)
    {
        return;
    }

    CANTC3xxNegativeInitSyntheticPort(&port, "neg_can0", BOARD_MCU_CAN_NODE_2);
    CANTC3xxNegativeInitSyntheticPort(&classic_only_port, "neg_classic_only", BOARD_MCU_CAN_NODE_2);
    CANTC3xxNegativeInitSyntheticPort(&zero_bitrate_port, "neg_zero_bitrate", BOARD_MCU_CAN_NODE_2);

    classic_only_port.fd_support = BOARD_CAN_CLASSIC_ONLY;
    zero_bitrate_port.default_nominal_bitrate = 0U;

    CANTC3xxNegativeInitConfig(&config);
    config.rx_path = CAN_RX_PATH_DEFAULT;
    config.rx_filter.enabled = true;
    config.rx_filter.id_type = CAN_ID_STANDARD;
    config.rx_filter.id = 0x123U;
    config.rx_filter.mask = 0x7FFU;
    result->fifo0_with_filter_status = CANTC3xxNegativeTryOpen(&port, &config);

    CANTC3xxNegativeInitConfig(&config);
    config.rx_path = CAN_RX_PATH_BUFFER;
    config.rx_filter.enabled = false;
    result->buffer_without_filter_status = CANTC3xxNegativeTryOpen(&port, &config);

    CANTC3xxNegativeInitConfig(&config);
    CANTC3xxWriteInvalidRxPath(&config);
    result->invalid_rx_path_status = CANTC3xxNegativeTryOpen(&port, &config);

    CANTC3xxNegativeInitConfig(&config);
    config.rx_path = CAN_RX_PATH_BUFFER;
    config.rx_filter.enabled = true;
    config.rx_filter.id_type = CAN_ID_STANDARD;
    config.rx_filter.id = 0x800U;
    config.rx_filter.mask = 0x7FFU;
    result->invalid_standard_filter_status = CANTC3xxNegativeTryOpen(&port, &config);

    CANTC3xxNegativeInitConfig(&config);
    config.rx_path = CAN_RX_PATH_BUFFER;
    config.rx_filter.enabled = true;
    config.rx_filter.id_type = CAN_ID_EXTENDED;
    config.rx_filter.id = 0x20000000U;
    config.rx_filter.mask = 0x1FFFFFFFU;
    result->invalid_extended_filter_status = CANTC3xxNegativeTryOpen(&port, &config);

    CANTC3xxNegativeInitConfig(&config);
    CANTC3xxWriteInvalidMode(&config);
    result->invalid_mode_status = CANTC3xxNegativeTryOpen(&port, &config);

    CANTC3xxNegativeInitConfig(&config);
    config.timing.mode = CAN_MODE_FD_BRS;
    result->fd_on_classic_only_port_status = CANTC3xxNegativeTryOpen(&classic_only_port, &config);

    CANTC3xxNegativeInitConfig(&config);
    config.timing.nominal_bitrate = 0U;
    result->zero_nominal_bitrate_status = CANTC3xxNegativeTryOpen(&zero_bitrate_port, &config);

    CANPlatformDeinit();
}

bool CANTC3xxPlatformBindingNegativeTestPassed(
    const CANTC3xxPlatformBindingNegativeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->invalid_port_status == CAN_STATUS_ENOTFOUND) &&
        (result->first_open_status == CAN_STATUS_OK) &&
        (result->duplicate_core_status == CAN_STATUS_EBUSY) &&
        (result->duplicate_node_status == CAN_STATUS_EBUSY) &&
        (result->first_close_status == CAN_STATUS_OK);
}

bool CANTC3xxOpenConfigNegativeTestPassed(
    const CANTC3xxOpenConfigNegativeTestResult *result)
{
    if (result == 0)
    {
        return false;
    }

    return
        (result->init_status == CAN_STATUS_OK) &&
        (result->fifo0_with_filter_status == CAN_STATUS_EUNSUPPORTED) &&
        (result->buffer_without_filter_status == CAN_STATUS_EUNSUPPORTED) &&
        (result->invalid_rx_path_status == CAN_STATUS_EINVAL) &&
        (result->invalid_standard_filter_status == CAN_STATUS_EINVAL) &&
        (result->invalid_extended_filter_status == CAN_STATUS_EINVAL) &&
        (result->invalid_mode_status == CAN_STATUS_EINVAL) &&
        (result->fd_on_classic_only_port_status == CAN_STATUS_EUNSUPPORTED) &&
        (result->zero_nominal_bitrate_status == CAN_STATUS_EINVAL);
}
