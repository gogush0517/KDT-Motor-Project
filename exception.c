#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}

extern volatile int Key_Pressed;
extern volatile int Key_Released;
extern volatile int Uart_Pressed;
extern volatile unsigned char Uart_Data;

void EXTI15_10_IRQHandler(void)
{
	if(Macro_Check_Bit_Clear(GPIOC->IDR, 13)){
		Key_Pressed = 1;
	}
	else {
		Key_Released = 1;
	}

	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);
}

void USART2_IRQHandler(void)
{
	int PA0 = Macro_Extract_Area(GPIOD->OTYPER, 0x1, 0); 
	int PA1 = Macro_Extract_Area(GPIOD->OTYPER, 0x1, 1); 

	if(PA0 ^= PA1) {
		Uart_Data = (unsigned char)USART2->DR;
		Uart_Pressed = 1;
	}
	
	NVIC_ClearPendingIRQ(38);
}