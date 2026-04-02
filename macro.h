
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
#define STOP_LED_1               IfxPort_P02_4  // 하차벨 LED
#define STOP_BTN_ON              IfxPort_P02_1  // 하차벨 버튼
#define STOP_BTN_ON_ISR_PRIORITY 10             // 하차벨 버튼 인터럽트 우선순위

#define DISABLED_STOP_BTN_ON_ISR_PRIORITY 11    // 장애인 하차벨 버튼 인터럽트 우선순위

#define DOOR_CONTROL_BTN_ISR_PRIORITY 12
#define SLOPE_CONTROL_BTN_ISR_PRIORITY 13

#define STOP_BTN_OFF             IfxPort_P00_7  // 하차벨 초기화 버튼
#define BUZZER                   IfxPort_P10_5


#define LED_1               IfxPort_P00_5  // 내장 LED1
#define LED_2               IfxPort_P00_6  // 내장 LED2
/*********************************************************************************************************************/
