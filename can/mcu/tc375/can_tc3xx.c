#include "can_tc3xx.h"
#include "can_tc3xx_pinmap.h"
#include "board_api.h"

#include <string.h>

#define CAN_TC3XX_NODE_RAM_STRIDE               (0x0200U)
#define CAN_TC3XX_STD_FILTER_OFFSET             (0x0000U)
#define CAN_TC3XX_EXT_FILTER_OFFSET             (0x0040U)
#define CAN_TC3XX_RX_BUFFER_OFFSET              (0x0080U)
#define CAN_TC3XX_TX_BUFFER_OFFSET              (0x0100U)

#define CAN_TC3XX_TX_BUFFER_NUMBER              (0U)
#define CAN_TC3XX_RX_BUFFER_NUMBER              (0U)
#define CAN_TC3XX_FILTER_NUMBER                 (0U)

#define CAN_TC3XX_MAX_DATA_WORDS                (16U)

#define CAN_TC3XX_RX_FIFO0_OFFSET              CAN_TC3XX_RX_BUFFER_OFFSET
#define CAN_TC3XX_RX_FIFO0_ELEMENTS            (1U)

static CANTC3xxModuleContext g_can_tc3xx_modules[CAN_TC3XX_MAX_MODULES];
static bool g_can_tc3xx_node_in_use[CAN_TC3XX_MAX_MODULES][CAN_TC3XX_MAX_NODES];

static CANStatus canTC3xxValidateRxFilterConfig(const CANAcceptanceFilter *filter);
static CANStatus canTC3xxValidateOpenConfig(const BoardCANPort *board_port, const CANChannelConfig *config);
static CANStatus canTC3xxModuleIndexFromBoardInst(BoardMCUCANInst inst, uint32_t *index);
static CANStatus canTC3xxNodeIndexFromBoardNode(BoardMCUCANNode node, uint32_t *index);
static CANStatus canTC3xxReserveNode(BoardMCUCANInst inst, BoardMCUCANNode node, uint32_t *module_index_out);
static void canTC3xxReleaseNode(BoardMCUCANInst inst, BoardMCUCANNode node);
static CANStatus canTC3xxAcquireModule(BoardMCUCANInst inst, Ifx_CAN *module_sfr, uint32_t module_index, CANTC3xxModuleContext **moduleContext);
static void canTC3xxReleaseModule(uint32_t module_index);
static CANStatus canTC3xxConfigureRxFilter(CANTC3xxChannel *channel);
static CANStatus canTC3xxToFrameMode(CANMode mode, IfxCan_FrameMode *frame_mode);
static uint32_t canTC3xxResolveNominalBitrate(const BoardCANPort *board_port, const CANChannelConfig *config);
static uint32_t canTC3xxResolveDataBitrate(const BoardCANPort *board_port, const CANChannelConfig *config);
static CANStatus canTC3xxValidateRxFilterConfig(const CANAcceptanceFilter *filter);
static CANStatus canTC3xxValidateOpenConfig(const BoardCANPort *board_port, const CANChannelConfig *config);
static bool canTC3xxUseRxFifo0(const CANChannelConfig *config);

static bool canTC3xxIsValidPayloadLength(CANMode mode, uint8_t len)
{
    if (mode == CAN_MODE_CLASSIC)
    {
        return (len <= 8U);
    }

    switch (len)
    {
        case 0U:
        case 1U:
        case 2U:
        case 3U:
        case 4U:
        case 5U:
        case 6U:
        case 7U:
        case 8U:
        case 12U:
        case 16U:
        case 20U:
        case 24U:
        case 32U:
        case 48U:
        case 64U:
            return true;

        default:
            return false;
    }
}

static CANStatus canTC3xxValidateRxFilterConfig(const CANAcceptanceFilter *filter)
{
    if (filter == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (filter->enabled == false)
    {
        return CAN_STATUS_OK;
    }

    if (filter->id_type == CAN_ID_STANDARD)
    {
        if ((filter->id > 0x7FFU) || (filter->mask > 0x7FFU))
        {
            return CAN_STATUS_EINVAL;
        }

        return CAN_STATUS_OK;
    }

    if (filter->id_type == CAN_ID_EXTENDED)
    {
        if ((filter->id > 0x1FFFFFFFU) || (filter->mask > 0x1FFFFFFFU))
        {
            return CAN_STATUS_EINVAL;
        }

        return CAN_STATUS_OK;
    }

    return CAN_STATUS_EINVAL;
}

static CANStatus canTC3xxValidateOpenConfig(const BoardCANPort *board_port,
                                            const CANChannelConfig *config)
{
    CANStatus status;
    IfxCan_FrameMode dummy_frame_mode;
    uint32_t nominal_bitrate;
    uint32_t data_bitrate;

    if ((board_port == 0) || (config == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    status = canTC3xxToFrameMode(config->timing.mode, &dummy_frame_mode);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    nominal_bitrate = canTC3xxResolveNominalBitrate(board_port, config);
    if (nominal_bitrate == 0U)
    {
        return CAN_STATUS_EINVAL;
    }

    if ((config->timing.mode != CAN_MODE_CLASSIC) &&
        (board_port->fd_support != BOARD_CAN_FD_SUPPORTED))
    {
        return CAN_STATUS_EUNSUPPORTED;
    }

    if (config->timing.mode != CAN_MODE_CLASSIC)
    {
        data_bitrate = canTC3xxResolveDataBitrate(board_port, config);
        if (data_bitrate == 0U)
        {
            return CAN_STATUS_EINVAL;
        }
    }

    if (canTC3xxUseRxFifo0(config) == true)
    {
        /* FIFO0 경로:
        explicit filter 없이 standard / extended non-matching frame을
        모두 FIFO0로 수용하는 full accept-all */
        if (config->rx_filter.enabled == true)
        {
            return CAN_STATUS_EUNSUPPORTED;
        }

        return CAN_STATUS_OK;
    }

    /* BUFFER 경로 */
    if (config->rx_path == CAN_RX_PATH_BUFFER)
    {
        if (config->rx_filter.enabled == false)
        {
            return CAN_STATUS_EUNSUPPORTED;
        }

        return canTC3xxValidateRxFilterConfig(&config->rx_filter);
    }

    return CAN_STATUS_EINVAL;
}

static void canTC3xxBuildStdAcceptAllFilter(IfxCan_Filter *filter, uint8_t filterNumber)
{
    memset(filter, 0, sizeof(*filter));

    filter->number = filterNumber;
    filter->elementConfiguration = IfxCan_FilterElementConfiguration_storeInRxFifo0;
    filter->type = IfxCan_FilterType_range;
    filter->id1 = 0x000U;
    filter->id2 = 0x7FFU;
}

static CANStatus canTC3xxValidateFrame(const CANTC3xxChannel *channel, const CANFrame *frame)
{
    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (frame->id_type == CAN_ID_STANDARD)
    {
        if (frame->id > 0x7FFU)
        {
            return CAN_STATUS_EINVAL;
        }
    }
    else if (frame->id_type == CAN_ID_EXTENDED)
    {
        if (frame->id > 0x1FFFFFFFU)
        {
            return CAN_STATUS_EINVAL;
        }
    }
    else
    {
        return CAN_STATUS_EINVAL;
    }

    if (canTC3xxIsValidPayloadLength(frame->mode, frame->len) == false)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (channel->config.timing.mode)
    {
        case CAN_MODE_CLASSIC:
            if (frame->mode != CAN_MODE_CLASSIC)
            {
                return CAN_STATUS_EUNSUPPORTED;
            }
            break;

        case CAN_MODE_FD_NO_BRS:
            if (frame->mode == CAN_MODE_FD_BRS)
            {
                return CAN_STATUS_EUNSUPPORTED;
            }
            break;

        case CAN_MODE_FD_BRS:
            /* classic / fd_no_brs / fd_brs 모두 허용 */
            break;

        default:
            return CAN_STATUS_EINVAL;
    }

    return CAN_STATUS_OK;
}

static CANStatus canTC3xxNodeIndexFromBoardNode(BoardMCUCANNode node, uint32_t *index)
{
    if (index == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (node)
    {
        case BOARD_MCU_CAN_NODE_0:
            *index = 0U;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_NODE_1:
            *index = 1U;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_NODE_2:
            *index = 2U;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_NODE_3:
            *index = 3U;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_ENOTFOUND;
    }
}

static CANStatus canTC3xxReserveNode(BoardMCUCANInst inst, BoardMCUCANNode node, uint32_t *module_index_out)
{
    CANStatus status;
    uint32_t module_index;
    uint32_t node_index;

    if (module_index_out == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    status = canTC3xxModuleIndexFromBoardInst(inst, &module_index);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxNodeIndexFromBoardNode(node, &node_index);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    if (g_can_tc3xx_node_in_use[module_index][node_index] == true)
    {
        return CAN_STATUS_EBUSY;
    }

    g_can_tc3xx_node_in_use[module_index][node_index] = true;
    *module_index_out = module_index;
    return CAN_STATUS_OK;
}

static void canTC3xxReleaseNode(BoardMCUCANInst inst, BoardMCUCANNode node)
{
    uint32_t module_index;
    uint32_t node_index;

    if (canTC3xxModuleIndexFromBoardInst(inst, &module_index) != CAN_STATUS_OK)
    {
        return;
    }

    if (canTC3xxNodeIndexFromBoardNode(node, &node_index) != CAN_STATUS_OK)
    {
        return;
    }

    g_can_tc3xx_node_in_use[module_index][node_index] = false;
}

static CANStatus canTC3xxAcquireModule(BoardMCUCANInst inst, Ifx_CAN *module_sfr, uint32_t module_index, CANTC3xxModuleContext **module_context)
{
    IfxCan_Can_Config module_config;

    if ((module_sfr == 0) || (module_context == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (module_index >= CAN_TC3XX_MAX_MODULES)
    {
        return CAN_STATUS_ENOTFOUND;
    }

    if (g_can_tc3xx_modules[module_index].is_initialized == false)
    {
        IfxCan_Can_initModuleConfig(&module_config, module_sfr);
        IfxCan_Can_initModule(&g_can_tc3xx_modules[module_index].can_module, &module_config);

        g_can_tc3xx_modules[module_index].is_initialized = true;
        g_can_tc3xx_modules[module_index].ref_count = 1U;
    }
    else
    {
        g_can_tc3xx_modules[module_index].ref_count++;
    }

    *module_context = &g_can_tc3xx_modules[module_index];
    (void)inst;
    return CAN_STATUS_OK;
}

static void canTC3xxReleaseModule(uint32_t module_index)
{
    if (module_index >= CAN_TC3XX_MAX_MODULES)
    {
        return;
    }

    if (g_can_tc3xx_modules[module_index].ref_count > 0U)
    {
        g_can_tc3xx_modules[module_index].ref_count--;
    }

    if (g_can_tc3xx_modules[module_index].ref_count == 0U)
    {
        g_can_tc3xx_modules[module_index].is_initialized = false;
    }
}

static CANStatus canTC3xxModuleIndexFromBoardInst(BoardMCUCANInst inst, uint32_t *index)
{
    if (index == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (inst)
    {
        case BOARD_MCU_CAN_INST_0:
            *index = 0U;
            return CAN_STATUS_OK;

        case BOARD_MCU_CAN_INST_1:
            *index = 1U;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_ENOTFOUND;
    }
}

static uint16_t canTC3xxNodeOffset(BoardMCUCANNode node)
{
    return (uint16_t)(((uint16_t)node) * CAN_TC3XX_NODE_RAM_STRIDE);
}

static uint32_t canTC3xxResolveNominalBitrate(const BoardCANPort *board_port, const CANChannelConfig *config)
{
    if (config->timing.nominal_bitrate != 0U)
    {
        return config->timing.nominal_bitrate;
    }

    return board_port->default_nominal_bitrate;
}

static uint32_t canTC3xxResolveDataBitrate(const BoardCANPort *board_port, const CANChannelConfig *config)
{
    if (config->timing.data_bitrate != 0U)
    {
        return config->timing.data_bitrate;
    }

    return board_port->default_data_bitrate;
}

static CANStatus canTC3xxToFrameMode(CANMode mode, IfxCan_FrameMode *frame_mode)
{
    if (frame_mode == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (mode)
    {
        case CAN_MODE_CLASSIC:
            *frame_mode = IfxCan_FrameMode_standard;
            return CAN_STATUS_OK;

        case CAN_MODE_FD_NO_BRS:
            *frame_mode = IfxCan_FrameMode_fdLong;
            return CAN_STATUS_OK;

        case CAN_MODE_FD_BRS:
            *frame_mode = IfxCan_FrameMode_fdLongAndFast;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_EINVAL;
    }
}

static CANStatus canTC3xxFromFrameMode(IfxCan_FrameMode frame_mode, CANMode *mode)
{
    if (mode == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (frame_mode)
    {
        case IfxCan_FrameMode_standard:
            *mode = CAN_MODE_CLASSIC;
            return CAN_STATUS_OK;

        case IfxCan_FrameMode_fdLong:
            *mode = CAN_MODE_FD_NO_BRS;
            return CAN_STATUS_OK;

        case IfxCan_FrameMode_fdLongAndFast:
            *mode = CAN_MODE_FD_BRS;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_EUNSUPPORTED;
    }
}

static CANStatus canTC3xxToMessageIdLength(CANIdType id_type, IfxCan_MessageIdLength *message_id_length)
{
    if (message_id_length == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (id_type)
    {
        case CAN_ID_STANDARD:
            *message_id_length = IfxCan_MessageIdLength_standard;
            return CAN_STATUS_OK;

        case CAN_ID_EXTENDED:
            *message_id_length = IfxCan_MessageIdLength_extended;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_EINVAL;
    }
}

static CANStatus canTC3xxFromMessageIdLength(IfxCan_MessageIdLength message_id_length, CANIdType *id_type)
{
    if (id_type == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (message_id_length)
    {
        case IfxCan_MessageIdLength_standard:
            *id_type = CAN_ID_STANDARD;
            return CAN_STATUS_OK;

        case IfxCan_MessageIdLength_extended:
            *id_type = CAN_ID_EXTENDED;
            return CAN_STATUS_OK;

        default:
            return CAN_STATUS_EUNSUPPORTED;
    }
}

static CANStatus canTC3xxToDataFieldSize(CANMode mode, IfxCan_DataFieldSize *data_field_size)
{
    if (data_field_size == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (mode == CAN_MODE_CLASSIC)
    {
        *data_field_size = IfxCan_DataFieldSize_8;
        return CAN_STATUS_OK;
    }

    *data_field_size = IfxCan_DataFieldSize_64;
    return CAN_STATUS_OK;
}

static CANStatus canTC3xxLenToDlc(uint8_t len, IfxCan_DataLengthCode *dlc)
{
    if (dlc == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (len)
    {
        case 0U:  *dlc = IfxCan_DataLengthCode_0;  return CAN_STATUS_OK;
        case 1U:  *dlc = IfxCan_DataLengthCode_1;  return CAN_STATUS_OK;
        case 2U:  *dlc = IfxCan_DataLengthCode_2;  return CAN_STATUS_OK;
        case 3U:  *dlc = IfxCan_DataLengthCode_3;  return CAN_STATUS_OK;
        case 4U:  *dlc = IfxCan_DataLengthCode_4;  return CAN_STATUS_OK;
        case 5U:  *dlc = IfxCan_DataLengthCode_5;  return CAN_STATUS_OK;
        case 6U:  *dlc = IfxCan_DataLengthCode_6;  return CAN_STATUS_OK;
        case 7U:  *dlc = IfxCan_DataLengthCode_7;  return CAN_STATUS_OK;
        case 8U:  *dlc = IfxCan_DataLengthCode_8;  return CAN_STATUS_OK;
        case 12U: *dlc = IfxCan_DataLengthCode_12; return CAN_STATUS_OK;
        case 16U: *dlc = IfxCan_DataLengthCode_16; return CAN_STATUS_OK;
        case 20U: *dlc = IfxCan_DataLengthCode_20; return CAN_STATUS_OK;
        case 24U: *dlc = IfxCan_DataLengthCode_24; return CAN_STATUS_OK;
        case 32U: *dlc = IfxCan_DataLengthCode_32; return CAN_STATUS_OK;
        case 48U: *dlc = IfxCan_DataLengthCode_48; return CAN_STATUS_OK;
        case 64U: *dlc = IfxCan_DataLengthCode_64; return CAN_STATUS_OK;

        default:
            return CAN_STATUS_EINVAL;
    }
}

static CANStatus canTC3xxDlcToLen(IfxCan_DataLengthCode dlc, uint8_t *len)
{
    if (len == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    switch (dlc)
    {
        case IfxCan_DataLengthCode_0:  *len = 0U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_1:  *len = 1U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_2:  *len = 2U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_3:  *len = 3U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_4:  *len = 4U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_5:  *len = 5U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_6:  *len = 6U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_7:  *len = 7U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_8:  *len = 8U;  return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_12: *len = 12U; return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_16: *len = 16U; return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_20: *len = 20U; return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_24: *len = 24U; return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_32: *len = 32U; return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_48: *len = 48U; return CAN_STATUS_OK;
        case IfxCan_DataLengthCode_64: *len = 64U; return CAN_STATUS_OK;

        default:
            return CAN_STATUS_EINVAL;
    }
}

static CANStatus canTC3xxConfigureRxFilter(CANTC3xxChannel *channel)
{
    CANStatus status;
    IfxCan_Filter filter;

    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    status = canTC3xxValidateRxFilterConfig(&channel->config.rx_filter);
    if ((status != CAN_STATUS_OK) &&
        !(canTC3xxUseRxFifo0(&channel->config) == true &&
          channel->config.rx_filter.enabled == false))
    {
        return status;
    }

    memset(&filter, 0, sizeof(filter));

    if (channel->use_rx_fifo0 == true)
    {
        return CAN_STATUS_OK;
    }

    /* BUFFER 경로 */
    filter.number = channel->filter_number;
    filter.elementConfiguration = IfxCan_FilterElementConfiguration_storeInRxBuffer;
    filter.type = IfxCan_FilterType_classic;
    filter.id1 = channel->config.rx_filter.id;
    filter.id2 = channel->config.rx_filter.mask;
    filter.rxBufferOffset = (IfxCan_RxBufferId)channel->rx_buffer_number;

    if (channel->config.rx_filter.id_type == CAN_ID_STANDARD)
    {
        IfxCan_Can_setStandardFilter(&channel->can_node, &filter);
    }
    else
    {
        IfxCan_Can_setExtendedFilter(&channel->can_node, &filter);
    }

    return CAN_STATUS_OK;
}

static bool canTC3xxUseRxFifo0(const CANChannelConfig *config)
{
    if (config == 0)
    {
        return false;
    }

    return ((config->rx_path == CAN_RX_PATH_DEFAULT) ||
            (config->rx_path == CAN_RX_PATH_FIFO0));
}

void canTC3xxChannelInit(CANTC3xxChannel *channel)
{
    if (channel != 0)
    {
        memset(channel, 0, sizeof(*channel));
    }
}

CANStatus canTC3xxOpen(CANTC3xxChannel *channel, const BoardCANPort *board_port, const CANChannelConfig *config)
{
    CANStatus status;
    uint32_t module_index;
    Ifx_CAN *module_sfr;
    IfxCan_NodeId node_id;
    IfxCan_FrameMode frame_mode;
    IfxCan_DataFieldSize data_field_size;
    IfxCan_Can_NodeConfig node_config;
    IfxCan_Can_Pins pins;
    bool node_reserved;
    bool board_powered;
    bool module_acquired;
    bool channel_cleared;
    bool pins_provided;

    if ((channel == 0) || (board_port == 0) || (config == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (channel->is_open == true)
    {
        return CAN_STATUS_EBUSY;
    }

    node_reserved = false;
    board_powered = false;
    module_acquired = false;
    channel_cleared = false;
    pins_provided = false;
    module_index = 0U;
    module_sfr = 0;

    status = canTC3xxValidateOpenConfig(board_port, config);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxResolveModule(board_port->mcu_inst, &module_sfr);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxResolveNodeId(board_port->mcu_node, &node_id);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    if (board_port->phy_type != BOARD_CAN_PHY_NONE)
    {
        status = canTC3xxResolvePins(board_port, &pins);
        if (status != CAN_STATUS_OK)
        {
            return status;
        }

        pins_provided = true;
    }
    else
    {
        memset(&pins, 0, sizeof(pins));
    }

    status = canTC3xxToFrameMode(config->timing.mode, &frame_mode);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxToDataFieldSize(config->timing.mode, &data_field_size);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxReserveNode(board_port->mcu_inst, board_port->mcu_node, &module_index);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }
    node_reserved = true;

    if (boardCANPortPowerOn(board_port) != BOARD_STATUS_OK)
    {
        status = CAN_STATUS_EIO;
        goto fail;
    }
    board_powered = true;

    memset(channel, 0, sizeof(*channel));
    channel_cleared = true;

    status = canTC3xxAcquireModule(board_port->mcu_inst,
                                   module_sfr,
                                   module_index,
                                   &channel->module_context);
    if (status != CAN_STATUS_OK)
    {
        goto fail;
    }
    module_acquired = true;

    channel->board_port = board_port;
    channel->module_inst = board_port->mcu_inst;
    channel->node = board_port->mcu_node;
    channel->module_index = (uint8_t)module_index;
    channel->use_rx_fifo0 = canTC3xxUseRxFifo0(config);
    channel->config = *config;

    channel->message_ram_base_address = (uint32_t)module_sfr;
    channel->message_ram_node_offset = canTC3xxNodeOffset(board_port->mcu_node);
    channel->tx_buffer_number = CAN_TC3XX_TX_BUFFER_NUMBER;
    channel->rx_buffer_number = CAN_TC3XX_RX_BUFFER_NUMBER;
    channel->filter_number = CAN_TC3XX_FILTER_NUMBER;

    IfxCan_Can_initNodeConfig(&node_config, &channel->module_context->can_module);

    node_config.nodeId = node_id;
    node_config.clockSource = IfxCan_ClockSource_both;
    node_config.busLoopbackEnabled = config->enable_loopback ? TRUE : FALSE;

    node_config.frame.type = IfxCan_FrameType_transmitAndReceive;
    node_config.frame.mode = frame_mode;

    node_config.baudRate.baudrate = canTC3xxResolveNominalBitrate(board_port, config);

    if (config->timing.mode != CAN_MODE_CLASSIC)
    {
        node_config.fastBaudRate.baudrate = canTC3xxResolveDataBitrate(board_port, config);
    }

    node_config.txConfig.txMode = IfxCan_TxMode_dedicatedBuffers;
    node_config.txConfig.dedicatedTxBuffersNumber = 1U;
    node_config.txConfig.txBufferDataFieldSize = data_field_size;

    if (channel->use_rx_fifo0 == true)
    {
        node_config.rxConfig.rxMode = IfxCan_RxMode_fifo0;
        node_config.rxConfig.rxFifo0DataFieldSize = data_field_size;
        node_config.rxConfig.rxFifo1DataFieldSize = data_field_size;

        node_config.rxConfig.rxFifo0Size = CAN_TC3XX_RX_FIFO0_ELEMENTS;
    }
    else
    {
        node_config.rxConfig.rxMode = IfxCan_RxMode_dedicatedBuffers;
        node_config.rxConfig.rxBufferDataFieldSize = data_field_size;
        node_config.rxConfig.rxFifo0DataFieldSize = data_field_size;
        node_config.rxConfig.rxFifo1DataFieldSize = data_field_size;
    }

    if (channel->use_rx_fifo0 == true)
    {
        node_config.filterConfig.messageIdLength = IfxCan_MessageIdLength_standard;
        node_config.filterConfig.standardListSize = 0U;
        node_config.filterConfig.extendedListSize = 0U;

        node_config.filterConfig.standardFilterForNonMatchingFrames =
            IfxCan_NonMatchingFrame_acceptToRxFifo0;
        node_config.filterConfig.extendedFilterForNonMatchingFrames =
            IfxCan_NonMatchingFrame_acceptToRxFifo0;
    }
    else
    {
        if (config->rx_filter.id_type == CAN_ID_STANDARD)
        {
            node_config.filterConfig.messageIdLength = IfxCan_MessageIdLength_standard;
            node_config.filterConfig.standardListSize = 1U;
            node_config.filterConfig.extendedListSize = 0U;
        }
        else
        {
            node_config.filterConfig.messageIdLength = IfxCan_MessageIdLength_extended;
            node_config.filterConfig.standardListSize = 0U;
            node_config.filterConfig.extendedListSize = 1U;
        }
    }

    node_config.messageRAM.baseAddress = channel->message_ram_base_address;
    
    node_config.messageRAM.standardFilterListStartAddress =
        (uint16_t)(channel->message_ram_node_offset + CAN_TC3XX_STD_FILTER_OFFSET);
    node_config.messageRAM.extendedFilterListStartAddress =
        (uint16_t)(channel->message_ram_node_offset + CAN_TC3XX_EXT_FILTER_OFFSET);
    node_config.messageRAM.txBuffersStartAddress =
        (uint16_t)(channel->message_ram_node_offset + CAN_TC3XX_TX_BUFFER_OFFSET);

    if (channel->use_rx_fifo0 == true)
    {
        node_config.messageRAM.rxFifo0StartAddress =
            (uint16_t)(channel->message_ram_node_offset + CAN_TC3XX_RX_FIFO0_OFFSET);
    }
    else
    {
        node_config.messageRAM.rxBuffersStartAddress =
            (uint16_t)(channel->message_ram_node_offset + CAN_TC3XX_RX_BUFFER_OFFSET);
    }

    node_config.pins = pins_provided ? &pins : 0;

    IfxCan_Can_initNode(&channel->can_node, &node_config);

    status = canTC3xxConfigureRxFilter(channel);
    if (status != CAN_STATUS_OK)
    {
        goto fail;
    }

    channel->is_open = true;
    channel->is_started = false;
    return CAN_STATUS_OK;

fail:
    if (module_acquired == true)
    {
        canTC3xxReleaseModule(channel->module_index);
    }

    if (board_powered == true)
    {
        (void)boardCANPortPowerOff(board_port);
    }

    if (node_reserved == true)
    {
        canTC3xxReleaseNode(board_port->mcu_inst, board_port->mcu_node);
    }

    if (channel_cleared == true)
    {
        memset(channel, 0, sizeof(*channel));
    }

    return status;
}

CANStatus canTC3xxClose(CANTC3xxChannel *channel)
{
    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EINVAL;
    }

    if (channel->board_port != 0)
    {
        (void)boardCANPortPowerOff(channel->board_port);
    }

    canTC3xxReleaseModule(channel->module_index);
    canTC3xxReleaseNode(channel->module_inst, channel->node);

    memset(channel, 0, sizeof(*channel));
    return CAN_STATUS_OK;
}

CANStatus canTC3xxStart(CANTC3xxChannel *channel)
{
    BoardStatus board_status;

    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EINVAL;
    }

    board_status = boardCANPortSetStandby(channel->board_port, false);
    if ((board_status != BOARD_STATUS_OK) && (board_status != BOARD_STATUS_EUNSUPPORTED))
    {
        return CAN_STATUS_EIO;
    }

    channel->is_started = true;
    return CAN_STATUS_OK;
}

CANStatus canTC3xxStop(CANTC3xxChannel *channel)
{
    BoardStatus board_status;

    if ((channel == 0) || (channel->is_open == false))
    {
        return CAN_STATUS_EINVAL;
    }

    board_status = boardCANPortSetStandby(channel->board_port, true);
    if ((board_status != BOARD_STATUS_OK) &&
        (board_status != BOARD_STATUS_EUNSUPPORTED))
    {
        return CAN_STATUS_EIO;
    }

    channel->is_started = false;
    return CAN_STATUS_OK;
}

CANStatus canTC3xxSend(CANTC3xxChannel *channel, const CANFrame *frame)
{
    CANStatus status;
    IfxCan_Message message;
    IfxCan_MessageIdLength message_id_length;
    IfxCan_DataLengthCode dlc;
    IfxCan_FrameMode frame_mode;
    IfxCan_Status can_status;
    uint32 tx_data[16];

    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if ((channel->is_open == false) || (channel->is_started == false))
    {
        return CAN_STATUS_EBUSY;
    }

    status = canTC3xxValidateFrame(channel, frame);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxToMessageIdLength(frame->id_type, &message_id_length);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxToFrameMode(frame->mode, &frame_mode);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxLenToDlc(frame->len, &dlc);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    memset(&message, 0, sizeof(message));
    memset(tx_data, 0, sizeof(tx_data));

    IfxCan_Can_initMessage(&message);

    message.bufferNumber = channel->tx_buffer_number;
    message.messageId = frame->id;
    message.messageIdLength = message_id_length;
    message.frameMode = frame_mode;
    message.dataLengthCode = dlc;

    memcpy((void *)tx_data, (const void *)frame->data, frame->len);

    can_status = IfxCan_Can_sendMessage(&channel->can_node, &message, tx_data);
    
    if (can_status == IfxCan_Status_notSentBusy)
    {
        return CAN_STATUS_EBUSY;
    }

    if (can_status != IfxCan_Status_ok)
    {
        return CAN_STATUS_EIO;
    }

    return CAN_STATUS_OK;
}

CANStatus canTC3xxReceive(CANTC3xxChannel *channel, CANFrame *frame)
{
    CANStatus status;
    IfxCan_Message message;
    uint32 rx_data[CAN_TC3XX_MAX_DATA_WORDS];
    CANIdType id_type;
    CANMode mode;
    uint8_t len;

    if ((channel == 0) || (frame == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if ((channel->is_open == false) || (channel->is_started == false))
    {
        return CAN_STATUS_EBUSY;
    }

    memset(&message, 0, sizeof(message));
    memset(rx_data, 0, sizeof(rx_data));

    IfxCan_Can_initMessage(&message);

    if (channel->use_rx_fifo0 == true)
    {
        if (IfxCan_Node_getRxFifo0FillLevel(channel->can_node.node) == 0U)
        {
            return CAN_STATUS_ENODATA;
        }

        message.readFromRxFifo0 = TRUE;
        IfxCan_Can_readMessage(&channel->can_node, &message, rx_data);
    }
    else
    {
        if (IfxCan_Can_isNewDataReceived(&channel->can_node,
                                        (IfxCan_RxBufferId)channel->rx_buffer_number) == FALSE)
        {
            return CAN_STATUS_ENODATA;
        }

        message.bufferNumber = channel->rx_buffer_number;
        IfxCan_Can_readMessage(&channel->can_node, &message, rx_data);
    }

    status = canTC3xxFromMessageIdLength(message.messageIdLength, &id_type);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxFromFrameMode(message.frameMode, &mode);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    status = canTC3xxDlcToLen(message.dataLengthCode, &len);
    if (status != CAN_STATUS_OK)
    {
        return status;
    }

    if (canTC3xxIsValidPayloadLength(mode, len) == false)
    {
        return CAN_STATUS_EIO;
    }

    frame->id_type = id_type;
    frame->mode = mode;
    frame->id = message.messageId;
    frame->len = len;

    memset(frame->data, 0, sizeof(frame->data));
    memcpy((void *)frame->data, (const void *)rx_data, len);

    return CAN_STATUS_OK;
}

CANStatus canTC3xxGetErrorState(CANTC3xxChannel *channel, CANCoreErrorState *state)
{
    Ifx_CAN_N *node;

    if ((channel == 0) || (state == 0))
    {
        return CAN_STATUS_EINVAL;
    }

    if (channel->is_open == false)
    {
        return CAN_STATUS_EBUSY;
    }

    node = channel->can_node.node;
    if (node == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    memset(state, 0, sizeof(*state));

    state->bus_off = (IfxCan_Node_getBusOffStatus(node) != 0);
    state->error_passive = (IfxCan_Node_isErrorPassive(node) != FALSE);
    state->warning = (IfxCan_Node_getWarningStatus(node) != 0);

    /* 이번 단계는 partial-real:
       상태 플래그만 읽고 TEC/REC는 0으로 둠 */
    state->tx_error_count = 0U;
    state->rx_error_count = 0U;

    return CAN_STATUS_OK;
}

CANStatus canTC3xxRecover(CANTC3xxChannel *channel)
{
    Ifx_CAN_N *node;

    if (channel == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (channel->is_open == false)
    {
        return CAN_STATUS_EBUSY;
    }

    node = channel->can_node.node;
    if (node == 0)
    {
        return CAN_STATUS_EINVAL;
    }

    if (IfxCan_Node_getBusOffStatus(node) == 0)
    {
        return CAN_STATUS_OK;
    }

    IfxCan_Node_setInitialisation(node, TRUE);
    IfxCan_Node_setInitialisation(node, FALSE);

    return CAN_STATUS_OK;
}