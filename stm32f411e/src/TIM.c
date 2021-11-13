#include "TIM.h"
#include <stm32f4xx.h>
#include <stdbool.h>

static __IO bool triggered = false;
static bool on = false;

void TIM2_IRQHandler(void)
{
	triggered = true;
	TIM2->SR = 0;
}

void TIM2_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	NVIC_SetPriority(TIM2_IRQn, 0x00);
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM2->DIER |= TIM_DIER_UIE; // Update interrupt enable
	TIM2->EGR |= TIM_EGR_UG;
	TIM2->ARR = 48000;
	//CEN is cleared automatically in one-pulse mode, when an update event occurs
	TIM2->CR1 |= TIM_CR1_CEN; //enable timer

	on = true;
}

void TIM2_delay_us(uint32_t us)
{
	while(on && us) {
		if (triggered) {
			if (us < 1000)
				us = 0;
			else
				us -= 1000;
			triggered = false;
		}
	}
}

void TIM2_deinit(void)
{
	on = false;
	TIM2->CR1 &= ~TIM_CR1_CEN;
//	NVIC_DisableIRQ(TIM2_IRQn); //TODO: Why not?
}
