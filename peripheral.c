#include "device_driver.h"
#include <stdio.h>

#define SECRETCODE 	("nintendo")

int LED_State;
volatile int Motor_State;
volatile int Motor_Speed = 1;
volatile int Key_Pressed;
volatile int Key_Released;
volatile int Uart_Pressed;
volatile unsigned char Uart_Data;
volatile int TIM4_Expired = 0;
volatile int Key_Long_Pressed = 0;
volatile int Secret_Key;

volatile int Safe_Key;
volatile int Safe_Key_Wait_Release = 0;
volatile int Estop_Buzzer = 0;

void Uart_Check(void) {
	if(Uart_Pressed){
		if(Uart_Data == SECRETCODE[Secret_Key]) {
			printf("Secret Key %d pressed!\n", Secret_Key + 1);
			Secret_Key++;
			if(Secret_Key == sizeof(SECRETCODE) - 1) {
				printf("Secret Key Activated!\n");
				Play_Song();
				Secret_Key = 0;
			}
		}
		else {
			Secret_Key = Uart_Data==SECRETCODE[0] ? 1 : 0;
		}
	}
	if(Uart_Pressed && (TIM2->CCR1 ^ TIM2->CCR2)) {
		if(Uart_Data >= '1' && Uart_Data <= '9') { Motor_Speed = Uart_Data - '0'; }
		else if(Uart_Data == 'S' || Uart_Data == 's'){ Motor_State = STOP; Motor_Speed = 1; }
		else if(Uart_Data == 'F' || Uart_Data == 'f'){ Motor_State = CW; }
		else if(Uart_Data == 'R' || Uart_Data == 'r'){ Motor_State = CCW; }
	}

	Uart_Pressed = 0;
}
void Buzzer_On(int freq) {
	if (freq <= 0) {
		TIM3->CCR2 = 0;
		return;
	}
	unsigned int period = 1000000 / freq;
	TIM3->ARR = period - 1;
	TIM3->CCR2 = period / 2; 
	Macro_Set_Bit(TIM3->EGR, 0);
}

void Buzzer_Off(void) {
	TIM3->CCR2 = 0;
}

void Buzzer_Beep(int freq, int ms) {
	volatile int i;
	Buzzer_On(freq);
	for (i = 0; i < ms * 6000; i++); 
	Buzzer_Off();
}
#define BASE  (250)
void Buzzer_Beep2(unsigned char tone, int duration) {
	const static unsigned short tone_value[] = {261,277,293,311,329,349,369,391,415,440,466,493,523,554,587,622,659,698,739,783,830,880,932,987};

	TIM3_Out_Freq_Generation(tone_value[tone]);
	TIM2_Delay(duration);
	TIM3_Out_Stop();
}

void Play_Song() {
	int i;
	enum key{C1, C1_, D1, D1_, E1, F1, F1_, G1, G1_, A1, A1_, B1, C2, C2_, D2, D2_, E2, F2, F2_, G2, G2_, A2, A2_, B2};
	enum note{N16=BASE/4, N8=BASE/2, N4=BASE, N2=BASE*2, N1=BASE*4};

	const int song[][2] = {{E2,N8},{E2,N4},{E2,N4},{C2,N8},{E2,N4},{G2,N2},{G1,N2}};
	const char * note_name[] = {"C1", "C1#", "D1", "D1#", "E1", "F1", "F1#", "G1", "G1#", "A1", "A1#", "B1", "C2", "C2#", "D2", "D2#", "E2", "F2", "F2#", "G2", "G2#", "A2", "A2#", "B2"};

	for(i=0; i<(sizeof(song)/sizeof(song[0])); i++)
	{
		printf("%s ", note_name[song[i][0]]);
		Buzzer_Beep(song[i][0], song[i][1]);
	}
}

void Key_Check(void){
    NVIC_DisableIRQ(EXTI15_10_IRQn);

	if(Key_Pressed) {

		Key_Pressed = 0;
	}
	if(Key_Released) {
		Buzzer_Off();
		if(Key_Long_Pressed) {
			if(Motor_State == STOP && Macro_Check_Bit_Set(GPIOB->IDR, 5)){
				Motor_State = CW;
				Macro_Clear_Bit(EXTI->IMR, 5);
				Safe_Key_Wait_Release = 1;

			} 
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
void Safe_Key_Reload(void) {
	static int cnt = 0;

	if (Safe_Key_Wait_Release) {
		if (Macro_Check_Bit_Clear(GPIOB->IDR, 5)) {
			cnt++;
			if (cnt > 300000) {
				EXTI->PR = (0x1 << 5);
				NVIC_ClearPendingIRQ((IRQn_Type)23);
				Macro_Set_Bit(EXTI->IMR, 5);
				Safe_Key_Wait_Release = 0;
				cnt = 0;
			}
		}
		else {
			cnt = 0;
		}
	}
}
void Buzzer_Check(void) {
	if (Estop_Buzzer) {
		Estop_Buzzer = 0;

		for (int n = 0; n < 3; n++) {
			Buzzer_Beep(800, 120);
			for (volatile int d = 0; d < 600000; d++);
		}
	}
}

void Motor_Init(void) {
		Macro_Write_Block(GPIOA->MODER,0xf,0xA,0);
		Macro_Write_Block(GPIOA->AFR[0],0xff,0x11,0);
}

void Key_Timer_Init(void) {
		Macro_Set_Bit(RCC->APB1ENR, 2);

		TIM4->CR1 = (1<<4)|(1<<3);
		TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5)-1;
		TIM4->ARR = TIM4_PLS_OF_2s;
		Macro_Set_Bit(TIM4->EGR,0);

		Macro_Clear_Bit(TIM4->SR, 0);
		NVIC_ClearPendingIRQ((IRQn_Type)30);

		Macro_Set_Bit(TIM4->DIER, 0);
		NVIC_EnableIRQ((IRQn_Type)30);
}

void Motor_Timer_Init(void) {
		Macro_Set_Bit(RCC->APB1ENR, 0);

		TIM2->CR1 = (0<<4)|(0<<3);
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

void Ext_LED_Init(void) {
	Macro_Set_Bit(RCC->AHB1ENR, 0);
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Set_Bit(RCC->AHB1ENR, 2);

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 9 * 2);
	Macro_Clear_Bit(GPIOA->OTYPER, 9);
	Macro_Clear_Bit(GPIOA->ODR, 9);

	Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 7 * 2);
	Macro_Clear_Bit(GPIOC->OTYPER, 7);
	Macro_Clear_Bit(GPIOC->ODR, 7);

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 6 * 2);
	Macro_Clear_Bit(GPIOA->OTYPER, 6);
	Macro_Clear_Bit(GPIOA->ODR, 6);
}

int Motor_Set_CCR(void){
    return (25 + 25*(Motor_Speed - 1)/8);
}

void Motor_Stop(){
	TIM2->CCR1 = 0;
	TIM2->CCR2 = 0;
}

void Motor_Cw(){
	TIM2->CCR1 = Motor_Set_CCR();
	TIM2->CCR2 = 0;
}

void Motor_CCw(){
	TIM2->CCR1 = 0;
	TIM2->CCR2 = Motor_Set_CCR();
}


void Safe_Key_Init(void) {
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 5 * 2);

	Macro_Set_Bit(RCC->APB2ENR, 14);
	Macro_Write_Block(SYSCFG->EXTICR[1], 0xf, 0x1, 4);

	Macro_Set_Bit(EXTI->RTSR, 5);
	Macro_Clear_Bit(EXTI->FTSR, 5);

	EXTI->PR = (0x1 << 5);
	NVIC_ClearPendingIRQ((IRQn_Type)23);
	Macro_Set_Bit(EXTI->IMR, 5);
	NVIC_EnableIRQ((IRQn_Type)23);

	NVIC_SetPriority((IRQn_Type)23, 0);
}

void Ext_LED_Display(void) {

	if (Motor_State == CW) Macro_Set_Bit(GPIOA->ODR, 9);
	else Macro_Clear_Bit(GPIOA->ODR, 9);

	if (Motor_State == CCW) Macro_Set_Bit(GPIOC->ODR, 7);
	else Macro_Clear_Bit(GPIOC->ODR, 7);
	
	if (Macro_Check_Bit_Set(GPIOB->IDR, 5)) Macro_Set_Bit(GPIOA->ODR, 6);
	else Macro_Clear_Bit(GPIOA->ODR, 6);
}
void Buzzer_Init(void) {
	Macro_Set_Bit(RCC->AHB1ENR, 0);
	Macro_Set_Bit(RCC->APB1ENR, 1);

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 7 * 2);
	Macro_Write_Block(GPIOA->AFR[0], 0xf, 0x2, 7 * 4);

	TIM3->CR1 = (0 << 4) | (0 << 3);
	TIM3->PSC = (unsigned int)(TIMXCLK / 1000000.0 + 0.5) - 1;
	TIM3->ARR = 999;

	TIM3->CCMR1 = (0x6 << 12) | (0x1 << 11);

	TIM3->CCR2 = 0;
	Macro_Write_Block(TIM3->CCER, 0x3, 0x1, 4);

	Macro_Set_Bit(TIM3->EGR, 0);
	Macro_Set_Bit(TIM3->CR1, 0);
}

void I2C_LED_Display(void) {
    static unsigned char prev_data = 0xFF;
    static volatile int blink_cnt = 0;
    static int blink_state = 0;
    unsigned char data;

    if (Motor_State == STOP) {
        data = 0xFF;
        blink_cnt = 0;
    }
    else if (Motor_Speed >= 1 && Motor_Speed <= 8) {
        data = (unsigned char)(0xFF >> Motor_Speed);
        blink_cnt = 0;
    }
    else if (Motor_Speed == 9) {
        blink_cnt++;
        if (blink_cnt >= 300000) {
            blink_cnt = 0;
            blink_state ^= 1;
        }
        data = blink_state ? 0xAA : 0x55;
    }
    else {
        data = 0xFF;
    }

    if (data != prev_data) {
        I2C1_SC16IS752_Write_GPIO(data);
        prev_data = data;
    }
}

void Total_Init(void) {
	Motor_Init();
	Key_Timer_Init();
	Motor_Timer_Init();
	Key_ISR_Enable(1);
	Uart2_RX_Interrupt_Enable(1);
	Safe_Key_Init();
	Ext_LED_Init();
	Buzzer_Init();
	I2C1_SC16IS752_Init(100000);
    I2C1_SC16IS752_Config_GPIO(0xFF);
    I2C1_SC16IS752_Write_GPIO(0xFF);
}

void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}

void Event_Handler(void) {
	Key_Check();
	Uart_Check();
	Safe_Key_Reload();
	Buzzer_Check();
}

void Op_Handler(void){
	volatile int i;
    LED_Display(LED_State);
	Ext_LED_Display();
	I2C_LED_Display();

	if (Motor_State == 0) {
		Motor_Stop();
	}
	else if (Motor_State == 1) {
		if (TIM2->CCR2 != 0) {
			Motor_Stop();
			for(i=0; i<0x80000; i++) {}
		}
		Motor_Cw();
	}
	else if (Motor_State == 2) {
		if (TIM2->CCR1 != 0) {
			Motor_Stop();
			for(i=0; i<0x80000; i++) {}
		}
		Motor_CCw();
	}
}