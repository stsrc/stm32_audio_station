#include <stm32f4xx.h>
#include "I2C.h"

void I2C1_init()
{
	/*
	 * GPIOB6 - SCL
	 * GPIOB9 - SDA
	 */

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	GPIOB->MODER |= GPIO_MODER_MODER6_1;
	GPIOB->MODER |= GPIO_MODER_MODER9_1;
	GPIOB->AFR[0] |= GPIO_AFRL_AFSEL6_2;
	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL9_2;
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED6_0 | GPIO_OSPEEDR_OSPEED6_1;
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED9_0 | GPIO_OSPEEDR_OSPEED9_1;
	GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT9;

	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	I2C1->CR2 = I2C_CR2_FREQ_5 | I2C_CR2_FREQ_4; //APB1 clock, CHANGE if speed changes

	I2C1->CCR = 241; //CHANGE if speed changes
	I2C1->TRISE = 49; //CHANGE if speed changes

	I2C1->CR1 |= I2C_CR1_PE;
}

void I2C1_write(uint8_t address, uint8_t *value, uint8_t size)
{
	uint8_t temp;

	I2C1->CR1 |= I2C_CR1_START;

	while(!(I2C1->SR1 & I2C_SR1_SB)); // SB - start bit

	I2C1->DR = address;

	while(!(I2C1->SR1 & I2C_SR1_ADDR)); // wait until address sent
	temp = I2C1->SR2; // clear ADDR flag
	temp = temp;
	GPIOD->ODR |= GPIO_ODR_OD15;
	while(!(I2C1->SR1 & I2C_SR1_TXE));

	for (uint8_t i = 0; i < size; i++) {
		I2C1->DR = value[i];
		while(!(I2C1->SR1 & I2C_SR1_BTF)); //BTF - byte transfer finished
		while(!(I2C1->SR1 & I2C_SR1_TXE));
	}

	I2C1->CR1 |= I2C_CR1_STOP;
}

void I2C1_read(uint8_t address, uint8_t *value)
{
	uint8_t temp;

	I2C1->CR1 |= I2C_CR1_ACK;

	I2C1->CR1 |= I2C_CR1_START;

        while(!(I2C1->SR1 & I2C_SR1_SB));

	I2C1->DR = address | 0x01;

	while(!(I2C1->SR1 & I2C_SR1_ADDR)); // wait until address sent
	temp = I2C1->SR2;
	temp = temp;

	while(!(I2C1->SR1 & I2C_SR1_RXNE));

	*value = I2C1->DR;

	I2C1->CR1 |= I2C_CR1_STOP;
	I2C1->CR1 &= ~I2C_CR1_ACK; //Disable acknowledge
}
