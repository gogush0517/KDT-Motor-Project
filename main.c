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
#define TIM4_PLS_OF_2s	  	(TIME4_PLS_OF_1ms*2000)

#define TIM2_TICK         	(1) 					// usec
#define TIM2_FREQ 	  		(1000000./TIM2_TICK)	// Hz
#define TIME2_PLS_OF_1ms  	(1000./TIM2_TICK)
#define TIM2_MAX	  		(49) //50카운트


int LED_State;
volatile int Motor_State;
volatile int Motor_Speed = 1;
volatile int Key_Pressed;
volatile int Key_Released;
volatile int Uart_Pressed;
volatile unsigned char Uart_Data;
volatile int TIM4_Expired = 0;      // 이번 누름에서 2초가 지났는지
volatile int Key_Long_Pressed = 0;  // 버튼을 뗀 순간 확정한 결과
volatile int Secret_Key;
char Secret_Code[5] = "sans";  // NULL 문자 포함

volatile int Safe_Key;
volatile int Safe_Key_Wait_Release = 0;
volatile int Estop_Buzzer = 0;

void Uart_Check(void) {
	if(Uart_Pressed){
		if(Uart_Data == Secret_Code[Secret_Key]) {
			printf("Secret Key %d pressed!\n", Secret_Key + 1);
			Secret_Key++;
			if(Secret_Key == 4) {
			printf("Secret Key Activated!\n");
			Secret_Key = 0;
			}
		}
		else {
			Secret_Key = Uart_Data==Secret_Code[0] ? 1 : 0;
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
void Buzzer_On(int freq) { //50% 듀티 (무극성 부저 최대 음량)
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

static void Key_Check(void){

	// 버튼 ISR이 값을 바꾸는 중간에 읽지 않도록 잠깐 차단
    NVIC_DisableIRQ(EXTI15_10_IRQn);

	if(Key_Pressed) {

		Key_Pressed = 0;
	}
	if(Key_Released) {
		Buzzer_Off();
		if(Key_Long_Pressed) {
			if(Motor_State == STOP && Macro_Check_Bit_Set(GPIOB->IDR, 5)){
				Motor_State = CW;
				Macro_Clear_Bit(EXTI->IMR, 5); // 세이프키 뗄 때 채터링으로 꺼짐 방지
				Safe_Key_Wait_Release = 1;     // 손 뗄 때까지 대기

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
static void Safe_Key_Reload(void) {
	static int cnt = 0;

	if (Safe_Key_Wait_Release) {
		if (Macro_Check_Bit_Clear(GPIOB->IDR, 5)) {
			cnt++;
			if (cnt > 300000) { // LOW 상태가 안정적으로 유지되면 다시준비
				EXTI->PR = (0x1 << 5);
				NVIC_ClearPendingIRQ((IRQn_Type)23);
				Macro_Set_Bit(EXTI->IMR, 5);
				Safe_Key_Wait_Release = 0;
				cnt = 0;
			}
		}
		else {
			cnt = 0; // 아직 누르고 있거나 떼는 중 채터링 발생 시 리셋
		}
	}
}
static void Buzzer_Check(void) {
	if (Estop_Buzzer) {
		Estop_Buzzer = 0;

		// 800Hz 낮은 경고음 3회 ("삐- 삐- 삐-")
		for (int n = 0; n < 3; n++) {
			Buzzer_Beep(800, 120);
			for (volatile int d = 0; d < 600000; d++); // 소리 사이 짧은 간격
		}
	}
}

static void Motor_Init(void) {
		Macro_Write_Block(GPIOA->MODER,0xf,0xA,0);
		Macro_Write_Block(GPIOA->AFR[0],0xff,0x11,0);
}

static void Key_Timer_Init() {	// TIM4
	// TIM4 Clock On
		Macro_Set_Bit(RCC->APB1ENR, 2);

		TIM4->CR1 = (1<<4)|(1<<3);
		TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5)-1;
		TIM4->ARR = TIM4_PLS_OF_2s;
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

static void Ext_LED_Init(void) {
	Macro_Set_Bit(RCC->AHB1ENR, 0);
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Set_Bit(RCC->AHB1ENR, 2);

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 9 * 2); // PA9 (D8, 초록)
	Macro_Clear_Bit(GPIOA->OTYPER, 9);
	Macro_Clear_Bit(GPIOA->ODR, 9);

	Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 7 * 2); // PC7 (D9, 노랑)
	Macro_Clear_Bit(GPIOC->OTYPER, 7);
	Macro_Clear_Bit(GPIOC->ODR, 7);

	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 6 * 2); // PB6 (D10, 빨강)
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


static void Safe_Key_Init(void) {
	// 1) GPIOB Clock On & PB5 Input Mode
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 5 * 2);

	// 2) SYSCFG Clock On & EXTI5에 Port B(0x1) 연결
	Macro_Set_Bit(RCC->APB2ENR, 14);
	Macro_Write_Block(SYSCFG->EXTICR[1], 0xf, 0x1, 4);

	// 3) Rising Edge(눌리는 순간)만 인터럽트 허용
	Macro_Set_Bit(EXTI->RTSR, 5);
	Macro_Clear_Bit(EXTI->FTSR, 5);

	// 4) Pending Clear 및 인터럽트 Enable (EXTI9_5 = IRQ 23)
	EXTI->PR = (0x1 << 5);
	NVIC_ClearPendingIRQ((IRQn_Type)23);
	Macro_Set_Bit(EXTI->IMR, 5);
	NVIC_EnableIRQ((IRQn_Type)23);

	// 5) 인터럽트 우선순위 설정 (0: 최고, 15: 최하)
	NVIC_SetPriority((IRQn_Type)23, 0);
}

static void Ext_LED_Display(void) {

	if (Motor_State == CW) Macro_Set_Bit(GPIOA->ODR, 9);
	else Macro_Clear_Bit(GPIOA->ODR, 9);

	if (Motor_State == CCW) Macro_Set_Bit(GPIOC->ODR, 7);
	else Macro_Clear_Bit(GPIOC->ODR, 7);
	
	if (Macro_Check_Bit_Set(GPIOB->IDR, 5)) Macro_Set_Bit(GPIOA->ODR, 6);
	else Macro_Clear_Bit(GPIOA->ODR, 6);
}
static void Buzzer_Init(void) {
	// GPIOA, TIM3 클럭 ON
	Macro_Set_Bit(RCC->AHB1ENR, 0);
	Macro_Set_Bit(RCC->APB1ENR, 1);

	// PA7을 Alternate Function(AF2: TIM3_CH2)으로 설정
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 7 * 2);
	Macro_Write_Block(GPIOA->AFR[0], 0xf, 0x2, 7 * 4);

	// 업카운팅, 연속 모드, 1틱 = 1us (1MHz)
	TIM3->CR1 = (0 << 4) | (0 << 3);
	TIM3->PSC = (unsigned int)(TIMXCLK / 1000000.0 + 0.5) - 1;
	TIM3->ARR = 999; // 기본 1kHz

	// CH2 PWM Mode 1 + Preload Enable
	TIM3->CCMR1 = (0x6 << 12) | (0x1 << 11);

	// 초기 상태 무음(듀티 0%) 및 CH2 출력 활성화
	TIM3->CCR2 = 0;
	Macro_Write_Block(TIM3->CCER, 0x3, 0x1, 4);

	// 설정 반영 및 타이머 시작
	Macro_Set_Bit(TIM3->EGR, 0);
	Macro_Set_Bit(TIM3->CR1, 0);
}

static void I2C_LED_Display(void) {
    static unsigned char prev_data = 0xFF;
    static volatile int blink_cnt = 0;
    static int blink_state = 0;
    unsigned char data;

    if (Motor_State == STOP) {
        data = 0xFF; // 정지 시 모두 OFF
        blink_cnt = 0;
    }
    else if (Motor_Speed >= 1 && Motor_Speed <= 8) {
        data = (unsigned char)(0xFF >> Motor_Speed); // 1~8단 게이지
        blink_cnt = 0;
    }
    else if (Motor_Speed == 9) {
        blink_cnt++;
        if (blink_cnt >= 300000) {
            blink_cnt = 0;
            blink_state ^= 1;
        }
        // 9단: 짝수(0xAA) <-> 홀수(0x55) 교대 점멸 (순간 전류 절감 + 8단과 구분)
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

static void Total_Init(void) {
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

static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}

static void Event_Handler(void) {
	Key_Check();
	Uart_Check();
	Safe_Key_Reload();
	Buzzer_Check();
}

void Op_Handler(){
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
