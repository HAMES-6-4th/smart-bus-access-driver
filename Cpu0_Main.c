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

IFX_ALIGN(4) IfxCpu_syncEvent cpuSyncEvent = 0;

/******** CAN 부분 ********/
#ifndef BOARD_TC375_LITE
#define BOARD_TC375_LITE
#endif
static DriverECUCAN g_driver_can;
static uint32_t g_ticks_per_ms = 0U;

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

/*********************************************/

// 하차벨 상태
volatile StopBtnState g_stopBtnState = STATE_STOP_BTN_OFF;
// 장애인 하차벨 상태
volatile DisabledStopBtnState g_disabledStopBtnState = STATE_DISABLED_STOP_BTN_OFF;

// 문 상태
volatile DoorState_t doorState = STATE_DOOR_CLOSE;   // 실제 상태
volatile DoorState_t doorRequest = STATE_DOOR_NONE;  // 요청

volatile SlopeState slopeState = STATE_SLOPE_CLOSE;
volatile SlopeState slopeRequest = STATE_SLOPE_NONE;


volatile bool stopBtnPressed = false;
volatile bool disabledStopBtnPressed = false;

volatile bool buzzerOn = false;
uint64 buzzerStart = 0;

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

    // 1초 주기를 위한 설정 (500ms)
    uint32 ticksFor5Sec = IfxStm_getTicksFromMilliseconds(&MODULE_STM0, 500);
    uint32 lastTick = IfxStm_get(&MODULE_STM0);

    while (1)
    {
        // 하차벨 초기화 버튼
        if (IfxPort_getPinState(STOP_BTN_OFF.port, STOP_BTN_OFF.pinIndex) == 0) // 안 누르면
        {
            offStopButtonLED();
        }

        bool updated = FALSE;
        if(doorState == STATE_DOOR_OPEN)
        {
            monitorFlags |= 0x80;
        }
        else if(doorState == STATE_DOOR_CLOSE)
        {
            monitorFlags &= 0x7F;
        }

        // 슬로프 상태 확인
        if(slopeState == STATE_SLOPE_OPEN)
        {
            monitorFlags |= 0x40;
        }
        else if(slopeState == STATE_SLOPE_CLOSE)
        {
            monitorFlags &= 0xBF;
        }

        updated = false;

        uint32 now_ms = DriverEcuGetNowMs();
        (void)driverECUCANPollStatus(&g_driver_can, now_ms, &updated);
        if (updated)
        {
            const DriverECUCANStatusSnapshot *st = driverECUCANGetStatus(&g_driver_can);

            if(st->pinch_detected == true)
            {
                monitorFlags |= 0x20;
            }
            else
            {
                monitorFlags &= 0xDF;
            }
            (void)st;
        }



        // 하차벨 눌림
        if(stopBtnPressed)
        {
            stopBtnPressed = false;
            onStopButtonLED();
        }

        if(disabledStopBtnPressed)
        {
            disabledStopBtnPressed = false;
            if(g_disabledStopBtnState == STATE_DISABLED_STOP_BTN_OFF)
            {
                g_disabledStopBtnState = STATE_DISABLED_STOP_BTN_ON;
                if(g_stopBtnState == STATE_STOP_BTN_OFF)
                {
                    onStopButtonLED();
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

        if (doorRequest == STATE_DOOR_OPEN)
        {
            if(g_disabledStopBtnState == STATE_DISABLED_STOP_BTN_ON) // 장애인 하차벨이 들어와 있으면
            {
                (void)driverECUCANSendCommand(&g_driver_can, DOOR_CMD_OPEN, RAMP_CMD_DEPLOY, true, false, false); // 문과 슬로프 둘 다 열어
                slopeState = STATE_SLOPE_OPEN;
            }
            else
                (void)driverECUCANSendCommand(&g_driver_can, DOOR_CMD_OPEN, RAMP_CMD_NOP, true, false, false); // 문만 열어
            offStopButtonLED();

            playDoorOpenSound();
            doorState = STATE_DOOR_OPEN;
            doorRequest = STATE_DOOR_NONE;
        }
        else if (doorRequest == STATE_DOOR_CLOSE)
        {
            // 문 닫는 명렁 ( 슬로프가 열려 있는 경우이는 전동문에서 알아서 슬로프도 접을 것임 (slop_manual은 false로 주었기 때문)
            (void)driverECUCANSendCommand(&g_driver_can, DOOR_CMD_CLOSE, RAMP_CMD_NOP, false, false, false);  // CAN 통신으로 전동문 ECU에 명령 날리기 (문 닫아)

            if(slopeState == STATE_SLOPE_OPEN) slopeState = STATE_SLOPE_CLOSE;

            playDoorCloseSound();
            doorState = STATE_DOOR_CLOSE;
            doorRequest = STATE_DOOR_NONE;
        }

        if (slopeRequest == STATE_SLOPE_OPEN)
        {
            (void)driverECUCANSendCommand(&g_driver_can, DOOR_CMD_NOP, RAMP_CMD_DEPLOY, true, false, false);  // CAN 통신으로 전동문 ECU에 명령 날리기 (슬로프 열어)
            playSlopeOpenSound();
            slopeState = STATE_SLOPE_OPEN;
            slopeRequest = STATE_SLOPE_NONE;
        }
        else if (slopeRequest == STATE_SLOPE_CLOSE)
        {
            (void)driverECUCANSendCommand(&g_driver_can, DOOR_CMD_NOP, RAMP_CMD_STOW, true, false, false);  // CAN 통신으로 전동문 ECU에 명령 날리기 (슬로프 닫아)
            playSlopeCloseSound();
            slopeState = STATE_SLOPE_CLOSE;
            slopeRequest = STATE_SLOPE_NONE;
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

void doorControlBtnISR(void)
{
    IfxPort_setPinHigh(LED_2.port, LED_2.pinIndex); // LED OFF


    if (doorState == STATE_DOOR_CLOSE)
    {
        doorRequest = STATE_DOOR_OPEN;
    }
    else
    {
        doorRequest = STATE_DOOR_CLOSE;
    }
}

void slopeControlBtnISR(void)
{
    IfxPort_setPinLow(LED_2.port, LED_2.pinIndex); // 내장 LED2 킴
    if(slopeState == STATE_SLOPE_CLOSE)
    {
        slopeRequest = STATE_SLOPE_OPEN;
    }
    else
    {
        slopeRequest = STATE_SLOPE_CLOSE;
    }
}
