#include "device_driver.h"

void LED_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 0); 

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10);
	Macro_Clear_Bit(GPIOA->OTYPER, 5);
	Macro_Clear_Bit(GPIOA->ODR, 5); 
}

void LED_Display(int on)
{
	Macro_Write_Block(GPIOA->ODR, 0x1, on & 0x1, 5);
}
