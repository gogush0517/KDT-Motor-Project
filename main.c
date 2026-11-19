#include "device_driver.h"
#include <stdio.h>

#define	PA0	(0)
#define PA1 (1)


#define STOP	(0)
#define CW		(1)
#define	CCW		(2)

int LED_State;
int Motor_State;
int Motor_Speed;
volatile int Key_Pressed;
volatile int Key_Released;
volatile int Uart_Pressed;
volatile unsigned char Uart_Data;

void Uart_Check(void) {
	char key;
	if ((key = Uart2_Get_Pressed()) != 0/* && (Motor_State != 0)*/) {
		switch (key) {
			case 'S': case 's':
				Motor_State = STOP;
				break;
			case 'F': case 'f':
				Motor_State = CW;
				break;
			case 'R': case 'r':
				Motor_State = CCW;
				break;
			default:
				break;
		}
	}
}

static void Key_Check(void){
	int k;
	if((k = Key_Get_Pressed()) != 0) {
		Macro_Invert_Bit(LED_State, 0);
		if(Motor_State == 1) Motor_State = 2;
		else if(Motor_State == 2) Motor_State = 1;
		Key_Wait_Key_Released();
	}
}

static void Motor_Init(void) {
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, PA0*2);
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, PA1*2);
	Macro_Clear_Bit(GPIOA->OTYPER, PA0);
	Macro_Clear_Bit(GPIOA->OTYPER, PA1);
}

#define TIM4_TICK	  		(20) 					// usec
#define TIM4_FREQ 	  		(1000000./TIM4_TICK) 	// Hz
#define TIME4_PLS_OF_1ms  	(1000./TIM4_TICK)
#define TIM4_MAX	  		(0xffffu)

static void Key_Timer_Init() {	// TIM4
	// TIM4 Clock On
		Macro_Set_Bit(RCC->APB1ENR, 2);

		TIM4->CR1 = (1<<4)|(0<<3);
		TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5)-1;
		TIM4->ARR = TIM4_MAX;
		Macro_Set_Bit(TIM4->EGR,0);

		// TIM4 Pending Clear
		// NVIC Pending Clear
		Macro_Clear_Bit(TIM4->SR, 0);
		NVIC_ClearPendingIRQ((IRQn_Type)30);

		// TIM4 Interrupt Enable
		// NVIC Interrupt Enable
		Macro_Set_Bit(TIM4->DIER, 0);
		NVIC_EnableIRQ((IRQn_Type)30);
}

#define TIM2_TICK         	(1) 					// usec
#define TIM2_FREQ 	  		(1000000./TIM2_TICK)	// Hz
#define TIME2_PLS_OF_1ms  	(1000./TIM2_TICK)
#define TIM2_MAX	  		(49) //50카운트

static void Motor_Timer_Init() {	// TIM2
	// TIM4 Clock On
		Macro_Set_Bit(RCC->APB1ENR, 0);

		TIM2->CR1 = (1<<4)|(0<<3);
		TIM2->PSC = (unsigned int)(TIMXCLK/(double)TIM2_FREQ + 0.5)-1;
		TIM2->ARR = TIM2_MAX;

		TIM2->CCMR1 = (0x3<<13)|(0x1<<11)|(0x3<<5)|(0x1<<3);

		TIM2->CCR1 =0;
		TIM2->CCR2 =0;
		
		Macro_Write_Block(TIM2->CCER,0x3,0x1,0);  //p.189 채널1(PA0) 극성과 on/off
    	Macro_Write_Block(TIM2->CCER,0x3,0x1,4);  //p.189 채널2(PA1) 극성과 on/off

		Macro_Set_Bit(TIM2->EGR,0);

		// TIM4 Pending Clear
		// NVIC Pending Clear
		Macro_Clear_Bit(TIM2->SR, 0);
		NVIC_ClearPendingIRQ((IRQn_Type)28);

		// TIM4 Interrupt Enable
		// NVIC Interrupt Enable
		Macro_Set_Bit(TIM2->DIER, 0);
		NVIC_EnableIRQ((IRQn_Type)28);
}

static void Event_Handler(void) {
	Key_Check();
	Uart_Check();
}

void Motor_Stop(){
	if(Macro_Extract_Area(GPIOA->ODR, 0x3, 0)==Motor_State) return;
    Macro_Clear_Area(GPIOA->ODR, 0x3, 0);
}

void Motor_Cw(){
	volatile int i;
    if(Macro_Extract_Area(GPIOA->ODR, 0x3, 0)==Motor_State) return;
    Motor_Stop();
    for(i=0; i<0x80000; i++);
    Macro_Write_Block(GPIOA->ODR, 0x3, 0x1, 0);
}

void Motor_CCw(){
	volatile int i;
    if(Macro_Extract_Area(GPIOA->ODR, 0x3, 0)==Motor_State) return;
    Motor_Stop();
    for(i=0; i<0x80000; i++);
    Macro_Write_Block(GPIOA->ODR, 0x3, 0x2, 0);
}

void Op_Handler(){
    LED_Display(LED_State);

    Motor_Stop();
    Motor_Cw();
    Motor_CCw();
}

static void Total_Init(void) {
	Key_Poll_Init();
	Motor_Init();
}

static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}

#if 01

void Main(void)
{
	Sys_Init(115200);
	Total_Init();

	for(;;)
	{
		Event_Handler();
		Op_Handler();
	}
}

#endif
