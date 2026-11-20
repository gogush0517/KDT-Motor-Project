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

void EXTI9_5_IRQHandler(void){
    if(Motor_State==0){
        printf("누름\n");
        Safe_Key = 1;   
    }
    else {
        printf("뗌\n");
        Safe_Key = 0;
    }
    EXTI->PR = 0x1 << 5; //펜딩클
	NVIC_ClearPendingIRQ((IRQn_Type)23);
}


void EXTI15_10_IRQHandler(void)
{
    // 타이머 설정 또는 판정 중에는 TIM4 인터럽트 차단
    NVIC_DisableIRQ((IRQn_Type)30);
    Macro_Clear_Bit(TIM4->DIER, 0);

	// 타이머 정지
    Macro_Clear_Bit(TIM4->CR1, 0);

	// printf("interrupted\n");
	if(Macro_Check_Bit_Clear(GPIOC->IDR, 13)){
        // 새 측정을 시작하므로 이전 만료 기록 초기화
        TIM4_Expired = 0;

        // 카운터와 프리스케일러 초기화
        Macro_Set_Bit(TIM4->EGR, 0);

        // 초기화로 발생한 UIF와 pending 제거
        Macro_Clear_Bit(TIM4->SR, 0);
        NVIC_ClearPendingIRQ((IRQn_Type)30);

        // 인터럽트 허용 후 2초 측정 시작
        Macro_Set_Bit(TIM4->DIER, 0);
        NVIC_EnableIRQ((IRQn_Type)30);
        Macro_Set_Bit(TIM4->CR1, 0);

        Key_Pressed = 1;
	}
	else {
		// ISR이 실행됐거나, 실행 직전 UIF가 켜졌으면 긴 누름
        Key_Long_Pressed = (TIM4_Expired != 0) || ((TIM4->SR & 1U) != 0);

        // 판정이 끝났으므로 타이머 플래그 정리
        Macro_Clear_Bit(TIM4->SR, 0);
        NVIC_ClearPendingIRQ((IRQn_Type)30);

        // 결과를 저장한 다음 메인에 알림
        Key_Released = 1;
	}

	EXTI->PR = 0x1 << 13;
	NVIC_ClearPendingIRQ(40);
}

void USART2_IRQHandler(void)
{
	// int PA0 = Macro_Extract_Area(GPIOD->OTYPER, 0x1, 0); 
	// int PA1 = Macro_Extract_Area(GPIOD->OTYPER, 0x1, 1); 

	Uart_Pressed = 1;
	Uart_Data = (unsigned char)USART2->DR;
	NVIC_ClearPendingIRQ(38);
}

void TIM4_IRQHandler(void)
{
    if (TIM4->SR & 1)
    {
        // 타이머 인터럽트 발생 원인 해제
        Macro_Clear_Bit(TIM4->SR, 0);

        // 2초가 지났다는 사실 기록
        TIM4_Expired = 1;
    }
}