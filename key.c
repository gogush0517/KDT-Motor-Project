#include "device_driver.h"

void Key_Poll_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 2); 
	Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);
}

#define KEY (13) 

#define N 		30000 

static int Key_Check_Input(void){
	return Macro_Check_Bit_Clear(GPIOC->IDR, KEY); 
}

int Key_Get_Pressed(void){
	unsigned int i, k;

	for(;;)	{
		k = Key_Check_Input(); 

		for(i = 0; i < N; i++){
			if(k != Key_Check_Input())	{ break; }
		}
		
		if(i == N) break;
	}

	return k;
}


void Key_Wait_Key_Pressed(void)
{
	while(!Macro_Check_Bit_Clear(GPIOC->IDR, 13));
}

void Key_Wait_Key_Released(void)
{
	while(!Macro_Check_Bit_Set(GPIOC->IDR, 13));
}

void Key_ISR_Enable(int en)
{
	if(en)
	{
		Macro_Set_Bit(RCC->AHB1ENR, 2); 
		Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);

		Macro_Set_Bit(RCC->APB2ENR, 14); 
		Macro_Write_Block(SYSCFG->EXTICR[3], 0xf, 0x2, 4);

		Macro_Set_Bit(EXTI->FTSR, 13);
		Macro_Set_Bit(EXTI->RTSR, 13);
		EXTI->PR = 0x1 << 13;
		
		NVIC_ClearPendingIRQ((IRQn_Type)40);
		Macro_Set_Bit(EXTI->IMR, 13);
		NVIC_EnableIRQ((IRQn_Type)40);

		// PB5번 Safe_key
		Macro_Set_Bit(RCC->AHB1ENR, 1); 
		Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 10);

		Macro_Write_Block(SYSCFG->EXTICR[1], 0xf, 0x1, 4); //PB5를 인터럽트로 설정
		Macro_Set_Bit(EXTI->FTSR, 5);
		Macro_Set_Bit(EXTI->RTSR, 5);

		EXTI->PR = 0x1 << 5; //펜딩클
		NVIC_ClearPendingIRQ((IRQn_Type)23);
		Macro_Set_Bit(EXTI->IMR, 5);

		NVIC_EnableIRQ((IRQn_Type)23);
	}

	else
	{
		NVIC_DisableIRQ((IRQn_Type)40);
	}
}


