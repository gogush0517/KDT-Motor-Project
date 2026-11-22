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
extern volatile int TIM4_Expired;
extern volatile int Key_Long_Pressed;
extern volatile int Safe_Key;
extern volatile int Motor_State;
extern volatile int Motor_Speed;

extern volatile int Estop_Buzzer;
extern void Buzzer_On(int freq);

void EXTI9_5_IRQHandler(void) {
	if (Macro_Check_Bit_Set(EXTI->PR, 5)) {
		for (volatile int d = 0; d < 1000; d++);

		if (Motor_State != 0 && Macro_Check_Bit_Set(GPIOB->IDR, 5)) {
			TIM2->CCR1 = 0;
			TIM2->CCR2 = 0;
			Motor_Speed = 1;
			Motor_State = 0;
			Estop_Buzzer = 1;
		}
		EXTI->PR = (0x1 << 5);
	}
	NVIC_ClearPendingIRQ((IRQn_Type)23);
}


void EXTI15_10_IRQHandler(void)
{
    NVIC_DisableIRQ((IRQn_Type)30);
    Macro_Clear_Bit(TIM4->DIER, 0);

    Macro_Clear_Bit(TIM4->CR1, 0);

	if(Macro_Check_Bit_Clear(GPIOC->IDR, 13)){
        TIM4_Expired = 0;

        Macro_Set_Bit(TIM4->EGR, 0);

        Macro_Clear_Bit(TIM4->SR, 0);
        NVIC_ClearPendingIRQ((IRQn_Type)30);

        Macro_Set_Bit(TIM4->DIER, 0);
        NVIC_EnableIRQ((IRQn_Type)30);
        Macro_Set_Bit(TIM4->CR1, 0);

        Key_Pressed = 1;
	}
	else {
        Key_Long_Pressed = (TIM4_Expired != 0) || ((TIM4->SR & 1U) != 0);

        Macro_Clear_Bit(TIM4->SR, 0);
        NVIC_ClearPendingIRQ((IRQn_Type)30);

        Key_Released = 1;
	}

	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);
}

void USART2_IRQHandler(void)
{
	Uart_Pressed = 1;
	Uart_Data = (unsigned char)USART2->DR;
	NVIC_ClearPendingIRQ(38);
}

void TIM4_IRQHandler(void)
{
    if (TIM4->SR & 1)
    {
        Macro_Clear_Bit(TIM4->SR, 0);

        TIM4_Expired = 1;
		Buzzer_On(2000);
    }
}