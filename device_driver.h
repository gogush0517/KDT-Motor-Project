#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

// Uart.c

extern void Uart2_Init(int baud);
extern char Uart2_Get_Char(void);
extern char Uart2_Get_Pressed(void);
extern void Uart2_RX_Interrupt_Enable(int en);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);
extern void LED_Display(int on);

// Clock.c

extern void Clock_Init(void);

// Key.c

extern void Key_Poll_Init(void);
extern int Key_Get_Pressed(void);
extern void Key_Wait_Key_Released(void);
extern void Key_Wait_Key_Pressed(void);
extern void Key_ISR_Enable(int en);

// Timer.c

extern void TIM2_Delay(int time);
extern void TIM2_Stopwatch_Start(void);
extern unsigned int TIM2_Stopwatch_Stop(void);
extern void TIM4_Repeat(int time);
extern int TIM4_Check_Timeout(void);
extern void TIM4_Stop(void);
extern void TIM4_Change_Value(int time);
extern void TIM4_Repeat_Interrupt_Enable(int en, int time);

// i2c.c

#define SC16IS752_IODIR				0x0A
#define SC16IS752_IOSTATE			0x0B

extern void I2C1_SC16IS752_Init(unsigned int freq);
extern void I2C1_SC16IS752_Write_Reg(unsigned int addr, unsigned int data);
extern void I2C1_SC16IS752_Config_GPIO(unsigned int config);
extern void I2C1_SC16IS752_Write_GPIO(unsigned int data);

// motor.c
#define	PA0	(0)
#define PA1 (1)

#define STOP	(0)
#define CW		(1)
#define	CCW		(2)

#define TIM4_TICK	  		(50)
#define TIM4_FREQ 	  		(1000000./TIM4_TICK)
#define TIME4_PLS_OF_1ms  	(1000./TIM4_TICK)
#define TIM4_PLS_OF_2s	  	(TIME4_PLS_OF_1ms*2000)

#define TIM2_TICK         	(1)
#define TIM2_FREQ 	  		(1000000./TIM2_TICK)
#define TIME2_PLS_OF_1ms  	(1000./TIM2_TICK)
#define TIM2_MAX	  		(49)

extern void Uart_Check(void);
extern void Buzzer_On(int freq);
extern void Buzzer_Off(void);
extern void Buzzer_Beep(int freq, int ms);
extern void Play_Song(void);
extern void Key_Check(void);
extern void Safe_Key_Reload(void);
extern void Buzzer_Check(void);
extern void Motor_Init(void);
extern void Key_Timer_Init(void);
extern void Motor_Timer_Init(void);
extern void Ext_LED_Init(void);
extern int Motor_Set_CCR(void);
extern void Motor_Stop(void);
extern void Motor_Cw(void);
extern void Motor_CCw(void);
extern void Safe_Key_Init(void);
extern void Ext_LED_Display(void);
extern void Buzzer_Init(void);
extern void I2C_LED_Display(void);
extern void Total_Init(void);
extern void Sys_Init(int baud);
extern void Event_Handler(void);
extern void Op_Handler(void);