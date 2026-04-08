#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "Ifx_Cfg_Ssw.h"
#include "IfxPort.h"
#include "IfxPort_PinMap.h"
#include "IfxScuEru.h"
#include "IfxSrc.h"
#include "IfxStm.h"
#include "Bsp.h"
#include <stdbool.h>
#include "stop_bell.h"
#include "state.h"
#include "macro.h"
#include "audio_module.h"
#include "monitor_module.h"
#include "driver_ecu_can.h"
#include "fire_detect_module.h"
#include "ble_uart.h"
#include "ble_uart_tc375_lite.h"

IFX_ALIGN(4) IfxCpu_syncEvent cpuSyncEvent = 0;

/******** CAN 부분 ********/
#ifndef BOARD_TC375_LITE
#define BOARD_TC375_LITE
#endif
static DriverECUCAN g_driver_can;

void AppInit(void)
{
    DriverECUCANConfig cfg;

    driverECUCANInit(&g_driver_can);
    driverECUCANInitConfig(&cfg);
    cfg.port_name = "can0";
    cfg.nominal_bitrate = 500000U;

    if (driverECUCANOpen(&g_driver_can, &cfg) != CAN_STATUS_OK)
    {
        while (1)
        {
        }
    }
}


static uint32_t DriverEcuGetNowMs(void)
{
    uint64 ticks;
    uint64 freq_hz;

    ticks = IfxStm_get(&MODULE_STM0);
    freq_hz = (uint64)IfxStm_getFrequency(&MODULE_STM0);

    if (freq_hz == 0u)
    {
        return 0u;
    }

    return (uint32_t)((ticks * 1000u) / freq_hz);
}
//set
/*********************************************/

// 하차벨 상태
volatile StopBtnState g_stopBtnState = STATE_STOP_BTN_OFF;
// 장애인 하차벨 상태
volatile DisabledStopBtnState g_disabledStopBtnState = STATE_DISABLED_STOP_BTN_OFF;

// 문 상태
volatile DoorState doorState = DOOR_STATE_CLOSED;

volatile RampState slopeState = RAMP_STATE_STOWED;

volatile bool doorOpenReq = false;
volatile bool doorCloseReq = false;

volatile bool slopeOpenReq = false;
volatile bool slopeCloseReq = false;


volatile bool stopBtnPressed = false;
volatile bool disabledStopBtnPressed = false;

volatile bool buzzerOn = false;
uint64 buzzerStart = 0;

volatile bool obstacleDetected = false;

volatile bool fireAlarmActive = false;

void initERU();

IFX_INTERRUPT(onStopBtnISR, 0, STOP_BTN_ON_ISR_PRIORITY);
IFX_INTERRUPT(onDisabledStopBtnISR, 0, DISABLED_STOP_BTN_ON_ISR_PRIORITY);

IFX_INTERRUPT(doorControlBtnISR, 0, DOOR_CONTROL_BTN_ISR_PRIORITY);
IFX_INTERRUPT(slopeControlBtnISR, 0, SLOPE_CONTROL_BTN_ISR_PRIORITY);

void onStopBtnISR(void);
void onDisabledStopBtnISR(void);
void doorControlBtnISR(void);
void slopeControlBtnISR(void);

void core0_main(void)
{
    IfxCpu_enableInterrupts();

    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    initGPIO();
    initERU();

    /* CAN 초기화 */
    AppInit();

    initAudioModule(&IfxAsclin1_TX_P15_0_OUT,
                    &IfxAsclin1_RXA_P15_1_IN,
                    &MODULE_ASCLIN1);

    // 기본 LED 모두 끔
    IfxPort_setPinLow(STOP_LED_1.port, STOP_LED_1.pinIndex);
    IfxPort_setPinHigh(LED_1.port, LED_1.pinIndex);
    IfxPort_setPinHigh(LED_2.port, LED_2.pinIndex);

    // 초기 설정
    monitorFlags = 0x00;

    //ble 설정
    bleUartTc375Init();

    uint8_t cmd;


    uint8 gTemp = 0;
    while (1)
    {
        // 하차벨 초기화 버튼
        if (IfxPort_getPinState(STOP_BTN_OFF.port, STOP_BTN_OFF.pinIndex) == 0) // 안 누르면
        {
            offStopButton();
        }

        if(doorState == DOOR_STATE_OPENED)
        {
            monitorFlags |= 0x80;
        }
        else if(doorState == DOOR_STATE_CLOSED)
        {
            monitorFlags &= 0x7F;
        }

        // 슬로프 상태 확인
        if(slopeState == RAMP_STATE_DEPLOYED)
        {
            monitorFlags |= 0x40;
        }
        else if(slopeState == RAMP_STATE_STOWED)
        {
            monitorFlags &= 0xBF;
        }

        /*************** CAN 통신 수신 ***************/
        bool updated = false;
        uint32 now_ms = DriverEcuGetNowMs();
        (void)driverECUCANPollStatus(&g_driver_can, now_ms, &updated);
        if (updated)
        {
            const DriverECUCANStatusSnapshot *st = driverECUCANGetStatus(&g_driver_can);

            if(st->pinch_detected == true)
            {
                monitorFlags |= 0x20;
                obstacleDetected = true;
            }
            else
            {
                monitorFlags &= 0xDF;
                obstacleDetected = false;
            }


            doorState  = (DoorState)st->door_state;
            slopeState = (RampState)st->ramp_state;


            (void)st;
        }

        /*************** CAN 통신 송신 ***************/
        if(doorOpenReq)
        {
            doorOpenReq = false;

            if(g_disabledStopBtnState == STATE_DISABLED_STOP_BTN_ON)
            {
                // 장애인 하차벨이 ON인 경우에는 전동문과 슬로프 둘 다 같이 열어야 됨
                (void)driverECUCANSendCommand(&g_driver_can,
                                             DOOR_CMD_OPEN,
                                             RAMP_CMD_DEPLOY,
                                             true, false, false);
            }
            else
            {
            (void)driverECUCANSendCommand(&g_driver_can,
                                         DOOR_CMD_OPEN,
                                         RAMP_CMD_NOP,
                                         false, false, false);
            }

            offStopButton(); // 하차벨 끄기 (LED / 상태 OFF + 모니터링 마스킹)
            playDoorOpenSound();
        }

        if(doorCloseReq)
        {
            doorCloseReq = false;

            (void)driverECUCANSendCommand(&g_driver_can,
                                         DOOR_CMD_CLOSE,
                                         RAMP_CMD_NOP,
                                         false, false, false);

            playDoorCloseSound();
        }

        if(slopeOpenReq)
        {
            slopeOpenReq = false;

            (void)driverECUCANSendCommand(&g_driver_can,
                                         DOOR_CMD_NOP,
                                         RAMP_CMD_DEPLOY,
                                         true, false, false);

            playSlopeOpenSound();
        }

        if(slopeCloseReq)
        {
            slopeCloseReq = false;

            (void)driverECUCANSendCommand(&g_driver_can,
                                         DOOR_CMD_NOP,
                                         RAMP_CMD_STOW,
                                         true, false, false);

            playSlopeCloseSound();
        }
        /****************************************/

        // 하차벨 눌림
        if(stopBtnPressed)
        {
            stopBtnPressed = false;
            onStopButton();
        }

        if(disabledStopBtnPressed)
        {
            disabledStopBtnPressed = false;
            if(g_disabledStopBtnState == STATE_DISABLED_STOP_BTN_OFF)
            {
                g_disabledStopBtnState = STATE_DISABLED_STOP_BTN_ON;
                if(g_stopBtnState == STATE_STOP_BTN_OFF)
                {
                    onStopButton();
                }else
                {
                    playBuzzer();
                }
            }
        }

        // 부저 끄기
        if (buzzerOn)
        {
            if (IfxStm_get(&MODULE_STM0) - buzzerStart > 70000000)
            {
                IfxPort_setPinLow(BUZZER.port, BUZZER.pinIndex);
                buzzerOn = false;
            }
        }

        /*****************  화재 감지  ***********************/
        if(isFireDetected() == true)
        {
            if(fireAlarmActive == false)
            {
                playFireAlarmSound();
                fireAlarmActive = true;

                (void)driverECUCANSendCommand(&g_driver_can, DOOR_CMD_OPEN, RAMP_CMD_NOP, true, false, false); // 문 열어
                monitorFlags |= 0x80; // 문 열림 모니터링
            }
        }
        else
        {
            fireAlarmActive = false;
        }


        /***************** BLE 하차벨 감지 ********************/
        bleUartTc375Poll();
        if (bleUartTc375ConsumeCmd(&cmd) != 0u)
        {
            uint8_t doorReq  = bleUartCmdGetDoor(cmd);
            uint8_t slopeReq = bleUartCmdGetSlope(cmd);

            if(doorReq == BLE_UART_REQ_OPEN)
            {
                stopBtnPressed = true;
            }
            if(slopeReq == BLE_UART_REQ_OPEN)
            {
                disabledStopBtnPressed = true;
            }
        }
    }
}


void onStopBtnISR()
{
    stopBtnPressed = true;
}

void onDisabledStopBtnISR(void)
{
    disabledStopBtnPressed = true;
}

// 수동 - false / 자동 - true
void doorControlBtnISR(void)
{
    IfxPort_setPinHigh(LED_2.port, LED_2.pinIndex);

    if (doorState == DOOR_STATE_CLOSED)
    {
        doorOpenReq = true;
    }
    else if (doorState == DOOR_STATE_OPENED)
    {
        if(obstacleDetected) return; // 장애물 감지 중에는 문 닫기 금지
        doorCloseReq = true;
    }
}

void slopeControlBtnISR(void)
{
    IfxPort_setPinLow(LED_2.port, LED_2.pinIndex);

    if(slopeState == RAMP_STATE_STOWED)
    {
        slopeOpenReq = true;
    }
    else if(slopeState == RAMP_STATE_DEPLOYED)
    {
        if(obstacleDetected) return; // 슬로프 감지 중에는 슬로프 닫기 금지
        slopeCloseReq = true;
    }
}
