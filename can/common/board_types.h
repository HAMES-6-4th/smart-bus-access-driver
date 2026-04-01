#ifndef BOARD_TYPES_H
#define BOARD_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BoardStatusEnum
{
    BOARD_STATUS_OK = 0,
    BOARD_STATUS_EINVAL,
    BOARD_STATUS_ENOTFOUND,
    BOARD_STATUS_EUNSUPPORTED,
    BOARD_STATUS_EHW
} BoardStatus;

typedef enum BoardPinActiveEnum
{
    BOARD_PIN_ACTIVE_LOW = 0,
    BOARD_PIN_ACTIVE_HIGH = 1
} BoardPinActive;

typedef enum BoardCANFdSupportEnum
{
    BOARD_CAN_CLASSIC_ONLY = 0,
    BOARD_CAN_FD_SUPPORTED = 1
} BoardCANFdSupport;

typedef enum BoardCANPhyTypeEnum
{
    BOARD_CAN_PHY_NONE = 0,
    BOARD_CAN_PHY_EXTERNAL = 1
} BoardCANPhyType;

typedef enum BoardMCUCANInstEnum
{
    BOARD_MCU_CAN_INST_0 = 0,
    BOARD_MCU_CAN_INST_1 = 1
} BoardMCUCANInst;

typedef enum BoardMCUCANNodeEnum
{
    BOARD_MCU_CAN_NODE_0 = 0,
    BOARD_MCU_CAN_NODE_1 = 1,
    BOARD_MCU_CAN_NODE_2 = 2,
    BOARD_MCU_CAN_NODE_3 = 3
} BoardMCUCANNode;

/* 일반 GPIO 추상 표현 */
typedef struct BoardGPIOStruct
{
    uint16_t port;
    uint16_t pin;
} BoardGPIO;

typedef struct BoardControlPinStruct
{
    bool present;
    BoardGPIO gpio;
    BoardPinActive active_level;
} BoardControlPin;

typedef struct BoardCANPortStruct
{
    const char *name;                       /* "can0", "can1" */
    BoardMCUCANInst mcu_inst;
    BoardMCUCANNode mcu_node;

    BoardGPIO rx_pin;
    BoardGPIO tx_pin;

    BoardCANPhyType phy_type;
    BoardCANFdSupport fd_support;

    BoardControlPin transceiver_enable;
    BoardControlPin transceiver_standby;
    BoardControlPin termination_enable;

    uint32_t default_nominal_bitrate;
    uint32_t default_data_bitrate;
} BoardCANPort;

typedef struct BoardDescriptorStruct
{
    const char *board_name;
    const BoardCANPort *can_ports;
    uint32_t can_port_count;
} BoardDescriptor;

#ifdef __cplusplus
}
#endif

#endif
