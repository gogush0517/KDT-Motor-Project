#include "device_driver.h"

#define SC16IS752_I2CADDR									0x9A
#define SC16IS752_I2CADDR_WR								(SC16IS752_I2CADDR|0x0)
#define SC16IS752_I2CADDR_RD								(SC16IS752_I2CADDR|0x1)

void I2C1_SC16IS752_Init(unsigned int freq)
{
	unsigned int r;
	volatile int i;
	
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Set_Bit(RCC->APB1ENR, 21);

	Macro_Clear_Bit(RCC->APB1RSTR, 21);
	Macro_Set_Bit(RCC->APB1RSTR, 21);
	for(i = 0; i < 1000; i++);
	Macro_Clear_Bit(RCC->APB1RSTR, 21);

	Macro_Write_Block(GPIOB->MODER, 0xf, 0xa, 12);
	Macro_Write_Block(GPIOB->AFR[0], 0xff, 0x44, 24);
	Macro_Write_Block(GPIOB->OTYPER, 0x3, 0x3, 6);
	Macro_Write_Block(GPIOB->OSPEEDR, 0xf, 0xa, 12);
	Macro_Write_Block(GPIOB->PUPDR, 0xf, 0x5, 12);

	Macro_Write_Block(I2C1->CR2, 0x3f, PCLK1 / 1000000, 0);
	Macro_Clear_Bit(I2C1->CR1, 0);
	I2C1->TRISE = (PCLK1 / 1000000) + 1;
	r = PCLK1 / (freq * 2);
	I2C1->CCR = ((r < 4) ? 4 : r);

	Macro_Clear_Bit(I2C1->CR1, 1);
	Macro_Set_Bit(I2C1->CR1, 0);
	Macro_Set_Bit(I2C1->CR1, 10);
}

void I2C1_SC16IS752_Write_Reg(unsigned int addr, unsigned int data)
{
	while(Macro_Check_Bit_Set(I2C1->SR2, 1));

	Macro_Set_Bit(I2C1->CR1, 8);
	while(Macro_Check_Bit_Clear(I2C1->SR1, 0));

	I2C1->DR = SC16IS752_I2CADDR_WR;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 1));
	(void)I2C1->SR2;

	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));
	I2C1->DR = addr << 3;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));

	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));
	I2C1->DR = data;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));

	Macro_Set_Bit(I2C1->CR1, 9);
	while(Macro_Check_Bit_Set(I2C1->CR1, 9));
}

void I2C1_SC16IS752_Config_GPIO(unsigned int config)
{
	I2C1_SC16IS752_Write_Reg(SC16IS752_IODIR, config);
}

void I2C1_SC16IS752_Write_GPIO(unsigned int data)
{
	I2C1_SC16IS752_Write_Reg(SC16IS752_IOSTATE, data);
}
