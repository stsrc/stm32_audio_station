#include "ADC.h"
#include <stm32f4xx.h>

void ADC_init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	GPIOB->MODER |= GPIO_MODER_MODER0_1 | GPIO_MODER_MODER0_0;

	ADC1->CR2 |= ADC_CR2_ADON;

	ADC1->SMPR2 |= ADC_SMPR2_SMP8_1;
}

void ADC_probe(void)
{
	ADC1->SQR3 |= ADC_SQR3_SQ1_3;

	ADC1->CR2 |= ADC_CR2_SWSTART;
}

uint16_t ADC_result(void)
{
	while(!(ADC1->SR & ADC_SR_STRT));
	ADC1->SR &= ~ADC_SR_STRT; // flag cleared by sw
	while(!(ADC1->SR & ADC_SR_EOC));
	return (uint16_t) ADC1->DR; // clears ADC_SR_EOC also
}
