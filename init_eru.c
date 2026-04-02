/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
#include "init_eru.h"
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
void initERU()
{
    uint16 password = IfxScuWdt_getSafetyWatchdogPasswordInline();
    IfxScuWdt_clearSafetyEndinitInline(password);

    /*일반 하차벨 버튼 인터럽트 설정 */
    //Port 02.1을 Pull-Down Input으로 Set
    MODULE_P02.IOCR0.B.PC1 = 0x01;

    /* EICR.EXIS 레지스터 설정 : ERS2, 1번 신호 */
    MODULE_SCU.EICR[1].B.EXIS0 = 1;

    /* rising, falling edge 트리거 설정 */
    MODULE_SCU.EICR[1].B.REN0 = 1;
    MODULE_SCU.EICR[1].B.FEN0 = 0;

    // Channel 2의 Trigger Event 활성화
    MODULE_SCU.EICR[1].B.EIEN0 = 1;

    // OGU 0으로 ETL2(TR) Trigger Event 보낼거임
    MODULE_SCU.EICR[1].B.INP0 = 0;

    // OGU0은 IGCR[0]/IGP0임 Pattern 고려 없이
    MODULE_SCU.IGCR[0].B.IGP0 = 1;


    //Interrupt를 위해 SCU의 SRC설정
    volatile Ifx_SRC_SRCR *src;
    src = (volatile Ifx_SRC_SRCR*) (&MODULE_SRC.SCU.SCUERU[0]);
    src->B.SRPN = STOP_BTN_ON_ISR_PRIORITY;
    src->B.TOS = 0;
    src->B.CLRR = 1;
    src->B.SRE = 1;


    /*장애인 하차벨 버튼 인터럽트 설정 */
    //Port 02.0을 Pull-Down Input으로 Set
    MODULE_P02.IOCR0.B.PC0 = 0x01;

    /* EICR.EXIS 레지스터 설정 */
    MODULE_SCU.EICR[1].B.EXIS1 = 2;   //

    /* rising, falling edge 트리거 설정 */
    MODULE_SCU.EICR[1].B.REN1 = 1;    //
    MODULE_SCU.EICR[1].B.FEN1 = 0;    //

    // Channel 2의 Trigger Event 활성화
    MODULE_SCU.EICR[1].B.EIEN1 = 1;   //

    // OGU 1으로 보내기
    MODULE_SCU.EICR[1].B.INP1 = 1;    //

    // OGU1 활성화
    MODULE_SCU.IGCR[0].B.IGP1 = 1;    //


    //Interrupt를 위해 SCU의 SRC설정 (기존 변수 유지)
    volatile Ifx_SRC_SRCR *src1;
    src1 = (volatile Ifx_SRC_SRCR*) (&MODULE_SRC.SCU.SCUERU[1]);
    src1->B.SRPN = DISABLED_STOP_BTN_ON_ISR_PRIORITY;
    src1->B.TOS = 0;
    src1->B.CLRR = 1;
    src1->B.SRE = 1;

    /* 문열기 버튼 인터럽트 */

    // Port 15.4를 Pull-Down Input으로 Set
    MODULE_P15.IOCR4.B.PC4 = 0x01;

    /* EICR0.EXIS0 : ERS0, In00 */
    MODULE_SCU.EICR[0].B.EXIS0 = 0;

    /* rising edge 트리거 */
    MODULE_SCU.EICR[0].B.REN0 = 1;
    MODULE_SCU.EICR[0].B.FEN0 = 0;

    // Trigger Event 활성화
    MODULE_SCU.EICR[0].B.EIEN0 = 1;

    // OGU 2으로 전달
    MODULE_SCU.EICR[0].B.INP0 = 2;

    // OGU2 활성화
    MODULE_SCU.IGCR[1].B.IGP0 = 1;

    // SRC 설정
    volatile Ifx_SRC_SRCR *src2;
    src2 = (volatile Ifx_SRC_SRCR*) (&MODULE_SRC.SCU.SCUERU[2]);
    src2->B.SRPN = DOOR_CONTROL_BTN_ISR_PRIORITY;
    src2->B.TOS = 0;
    src2->B.CLRR = 1;
    src2->B.SRE = 1;


    /* 문 닫기 버튼 인터럽트 */

    // Port 15.5을 Pull-Down Input으로 Set
    MODULE_P15.IOCR4.B.PC5 = 0x01;

    /* EICR2.EXIS0 : ERS4, In40 */
    MODULE_SCU.EICR[2].B.EXIS0 = 3;

    /* rising edge 트리거 */
    MODULE_SCU.EICR[2].B.REN0 = 1;
    MODULE_SCU.EICR[2].B.FEN0 = 0;

    // Trigger Event 활성화
    MODULE_SCU.EICR[2].B.EIEN0 = 1;

    // OGU 3으로 전달
    MODULE_SCU.EICR[2].B.INP0 = 3;

    // OGU3 활성화
    MODULE_SCU.IGCR[1].B.IGP1 = 1;

    // SRC 설정
    volatile Ifx_SRC_SRCR *src3;
    src3 = (volatile Ifx_SRC_SRCR*) (&MODULE_SRC.SCU.SCUERU[3]);
    src3->B.SRPN = SLOPE_CONTROL_BTN_ISR_PRIORITY;
    src3->B.TOS = 0;
    src3->B.CLRR = 1;
    src3->B.SRE = 1;

    IfxScuWdt_setSafetyEndinitInline(password);
}

/*********************************************************************************************************************/
