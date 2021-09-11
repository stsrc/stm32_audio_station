#include "SPI.h"
#include <assert.h>

void SPI_show_error(int errorCode)
{
}

int SPI_1_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	/*
	 * GPIOA4 - NSS
	 * GPIOA5 - SCK
	 * GPIOA6 - MISO
	 * GPIOA7 - MOSI
	 */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

//	GPIOA->MODER |= GPIO_MODER_MODER4_1;
	GPIOA->MODER |= GPIO_MODER_MODER5_1;
	GPIOA->MODER |= GPIO_MODER_MODER6_1;
	GPIOA->MODER |= GPIO_MODER_MODER7_1;
//	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL4_0 | GPIO_AFRL_AFSEL4_2;
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0 | GPIO_AFRL_AFSEL5_2;
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL6_0 | GPIO_AFRL_AFSEL6_2;
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL7_0 | GPIO_AFRL_AFSEL7_2;

	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED4_0 | GPIO_OSPEEDR_OSPEED4_1;
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED5_0 | GPIO_OSPEEDR_OSPEED5_1;
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED6_0 | GPIO_OSPEEDR_OSPEED6_1;
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED7_0 | GPIO_OSPEEDR_OSPEED7_1;
/*
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD4_0;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD5_0;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD6_0;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD7_0;
*/
//	SPI1->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2;
	SPI1->CR1 |= SPI_CR1_SSM;
	SPI1->CR1 |= SPI_CR1_SSI;
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR1 |= SPI_CR1_SPE;

	return 0;
}

void SPI_1_send(uint8_t *data)
{
	while(SPI1->SR & SPI_SR_BSY);
	SPI1->DR = *data;
	while(SPI1->SR & SPI_SR_BSY);
	(void)SPI1->DR;
}

void SPI_1_DMA_send(uint8_t *data, uint16_t size)
{
	for (uint16_t i = 0; i < size; i++)
		SPI_1_send(data);
}

void SPI_2_init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	/*
	 * GPIOB12 - NSS
	 * GPIOB13 - SCK
	 * GPIOB14 - MISO
	 * GPIOB15 - MOSI
	 */

	GPIOB->MODER |= GPIO_MODER_MODER13_1;
	GPIOB->MODER |= GPIO_MODER_MODER14_1;
	GPIOB->MODER |= GPIO_MODER_MODER15_1;

	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL13_0 | GPIO_AFRH_AFSEL13_2;
	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL14_0 | GPIO_AFRH_AFSEL14_2;
	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL15_0 | GPIO_AFRH_AFSEL15_2;

	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED13_0 | GPIO_OSPEEDR_OSPEED13_1;
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED14_0 | GPIO_OSPEEDR_OSPEED14_1;
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED15_0 | GPIO_OSPEEDR_OSPEED15_1;

	SPI2->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2;

	SPI2->CR1 |= SPI_CR1_SSM;
	SPI2->CR1 |= SPI_CR1_SSI;
	SPI2->CR1 |= SPI_CR1_MSTR;
	SPI2->CR1 |= SPI_CR1_SPE;
}

void SPI_2_send(uint8_t data)
{
	while(SPI2->SR & SPI_SR_BSY);
	SPI2->DR = data;
	while(SPI2->SR & SPI_SR_BSY);
	while(!(SPI2->SR & SPI_SR_RXNE));
	(void)SPI2->DR;
}

void SPI_2_read(uint8_t *rx, uint16_t bytes)
{
	for (uint16_t i = 0; i < bytes; i++) {
		while(SPI2->SR & SPI_SR_BSY);
		SPI2->DR = 0x00;
		while(SPI2->SR & SPI_SR_BSY);
		while(!(SPI2->SR & SPI_SR_RXNE));
		rx[i] = SPI2->DR;
	}
}

void SPI_4_init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
	RCC->APB2ENR |= RCC_APB2ENR_SPI4EN;

	/*
	 * GPIOE11 - NSS
	 * GPIOE12 - SCK
	 * GPIOE13 - MISO
	 * GPIOE14 - MOSI
	 */

	GPIOE->MODER |= GPIO_MODER_MODER12_1;
	GPIOE->MODER |= GPIO_MODER_MODER13_1;
	GPIOE->MODER |= GPIO_MODER_MODER14_1;

	GPIOE->AFR[1] |= GPIO_AFRH_AFSEL12_0 | GPIO_AFRH_AFSEL12_2;
	GPIOE->AFR[1] |= GPIO_AFRH_AFSEL13_0 | GPIO_AFRH_AFSEL13_2;
	GPIOE->AFR[1] |= GPIO_AFRH_AFSEL14_0 | GPIO_AFRH_AFSEL14_2;

	GPIOE->OSPEEDR |= GPIO_OSPEEDR_OSPEED12_0 | GPIO_OSPEEDR_OSPEED12_1;
	GPIOE->OSPEEDR |= GPIO_OSPEEDR_OSPEED13_0 | GPIO_OSPEEDR_OSPEED13_1;
	GPIOE->OSPEEDR |= GPIO_OSPEEDR_OSPEED14_0 | GPIO_OSPEEDR_OSPEED14_1;

	GPIOE->PUPDR |= GPIO_PUPDR_PUPD12_0;
	GPIOE->PUPDR |= GPIO_PUPDR_PUPD13_0;
	GPIOE->PUPDR |= GPIO_PUPDR_PUPD14_0;

//        SPI4->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2;

	SPI4->CR1 |= SPI_CR1_SSM;
	SPI4->CR1 |= SPI_CR1_SSI;
	SPI4->CR1 |= SPI_CR1_MSTR;
	SPI4->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI_4_send(uint8_t data)
{
	while(SPI4->SR & SPI_SR_BSY);
	SPI4->DR = data;
	while(SPI4->SR & SPI_SR_BSY);
	while(!(SPI4->SR & SPI_SR_RXNE));
	return (uint8_t) SPI4->DR;
}

void SPI_4_send_multi(const uint8_t *data, uint32_t bytes)
{
	for (uint32_t i = 0; i < bytes; i++)
		SPI_4_send(data[i]);
}

void SPI_4_read(uint8_t *rx, uint32_t bytes)
{
	for (uint32_t i = 0; i < bytes; i++) {
		while(SPI4->SR & SPI_SR_BSY);
		SPI4->DR = 0xff;
		while(SPI4->SR & SPI_SR_BSY);
		while(!(SPI4->SR & SPI_SR_RXNE));
		rx[i] = SPI4->DR;
	}
}
