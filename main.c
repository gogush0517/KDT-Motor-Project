#include "device_driver.h"
#include <stdio.h>

#define	PA0	(0)
#define PA1 (1)


#define STOP	(0)
#define CW		(1)
#define	CCW		(2)

int LED_State;
int Motor_State;

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
