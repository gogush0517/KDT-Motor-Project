#include "device_driver.h"
#include <stdio.h>

#define	PA0	(0)
#define PA1 (1)

#define STOP	(0)
#define CW		(1)
#define	CCW		(2)

#define TIM4_TICK	  		(50) 					// usec
#define TIM4_FREQ 	  		(1000000./TIM4_TICK) 	// Hz
#define TIME4_PLS_OF_1ms  	(1000./TIM4_TICK)
#define TIM4_MAX	  		(39999)

#define TIM2_TICK         	(1) 					// usec
#define TIM2_FREQ 	  		(1000000./TIM2_TICK)	// Hz
#define TIME2_PLS_OF_1ms  	(1000./TIM2_TICK)
#define TIM2_MAX	  		(49) //50카운트


int LED_State;
int Motor_State;
int Motor_Speed;
volatile int Key_Pressed;
volatile int Key_Released;
volatile int Uart_Pressed;
volatile unsigned char Uart_Data;
volatile int TIM4_Expired = 0;      // 이번 누름에서 2초가 지났는지
volatile int Key_Long_Pressed = 0;  // 버튼을 뗀 순간 확정한 결과
void Uart_Check(void) {
	if(Uart_Pressed) {
		if(Uart_Data >= '1' && Uart_Data <= '9') { Motor_Speed = Uart_Data - '0'; }
		else if(Uart_Data == 'S' || Uart_Data == 's'){ Motor_State = STOP; }
		else if(Uart_Data == 'F' || Uart_Data == 'f'){ Motor_State = CW; }
		else if(Uart_Data == 'R' || Uart_Data == 'r'){ Motor_State = CCW; }
	}
}

static void Key_Check(void){

	// 버튼 ISR이 값을 바꾸는 중간에 읽지 않도록 잠깐 차단
    NVIC_DisableIRQ(EXTI15_10_IRQn);

	if(Key_Pressed) {

		Key_Pressed = 0;
	}
	if(Key_Released) {

		if(Key_Long_Pressed) {
			if(Motor_State == STOP) Motor_State = CW;
			else Motor_State = STOP;
		}
		else {
			Macro_Invert_Bit(LED_State, 0);
			if(Motor_State == CW) Motor_State = CCW;
			else if(Motor_State == CCW) Motor_State = CW;
		}
		Key_Released = 0;
	}
	NVIC_EnableIRQ(EXTI15_10_IRQn); 
}

static void Motor_Init(void) {
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, PA0*2);
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, PA1*2);
	Macro_Clear_Bit(GPIOA->OTYPER, PA0);
	Macro_Clear_Bit(GPIOA->OTYPER, PA1);
}



static void Key_Timer_Init() {	// TIM4
	// TIM4 Clock On
		Macro_Set_Bit(RCC->APB1ENR, 2);

		TIM4->CR1 = (1<<4)|(1<<3);
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

static void Motor_Timer_Init() {	// TIM2
	// TIM2 Clock On
		Macro_Set_Bit(RCC->APB1ENR, 0);

		Macro_Write_Block(GPIOA->MODER,0xf,0xA,0);
		Macro_Write_Block(GPIOA->AFR[0],0xff,0x11,0);

		TIM2->CR1 = (1<<4)|(0<<3);
		TIM2->PSC = (unsigned int)(TIMXCLK/(double)TIM2_FREQ + 0.5)-1;
		TIM2->ARR = TIM2_MAX;

		TIM2->CCMR1 = (0x3<<13)|(0x1<<11)|(0x3<<5)|(0x1<<3);

		TIM2->CCR1 =0;
		TIM2->CCR2 =0;
		
		Macro_Write_Block(TIM2->CCER,0x3,0x1,0);
    	Macro_Write_Block(TIM2->CCER,0x3,0x1,4);

		Macro_Set_Bit(TIM2->EGR,0);

		Macro_Set_Bit(TIM2->CR1, 0);
}

static void Event_Handler(void) {
	Key_Check();
	Uart_Check();
}

int Motor_Set_CCR(void){
    return (24 + 25*(Motor_Speed -1)/8);
}

void Motor_Stop(){
	if(Macro_Extract_Area(GPIOA->ODR, 0x3, 0)==Motor_State) return;
    TIM2->CCR1 = 0;
	TIM2->CCR2 = 0;
}

void Motor_Cw(){
	volatile int i;
    if(Macro_Extract_Area(GPIOA->ODR, 0x3, 0)==Motor_State) return;
    Motor_Stop();
    for(i=0; i<0x80000; i++);
    TIM2->CCR1 = Motor_Set_CCR();
	TIM2->CCR2 = 0;
}

void Motor_CCw(){
	volatile int i;
    if(Macro_Extract_Area(GPIOA->ODR, 0x3, 0)==Motor_State) return;
    Motor_Stop();
    for(i=0; i<0x80000; i++);
    TIM2->CCR1 = 0;
	TIM2->CCR2 = Motor_Set_CCR();
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
	Key_Timer_Init();
	Motor_Timer_Init();
	Key_ISR_Enable(1);
	Uart2_RX_Interrupt_Enable(1);
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
