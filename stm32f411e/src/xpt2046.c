#include "xpt2046.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include "display.h"
#include "tm_ili9341.h"

#ifndef XPT2046_CS_PIN
#define XPT2046_CS_PORT GPIOB
#define XPT2046_CS_PIN	(1 << 12)
#endif /* XPT2046_CS_PIN */

#ifndef XPT2046_EXTI_PIN
#define XPT2046_EXTI_PORT	GPIOB
#define XPT2046_EXTI_PIN	(1 << 10)
#endif /* XPT2046_EXTI_PIN */

#ifndef XPT2046_CS_SET
#define XPT2046_CS_SET		GPIOB->BSRR |= XPT2046_CS_PIN;
#define XPT2046_CS_RESET	GPIOB->BSRR |= XPT2046_CS_PIN << 0x10;
#endif /* PT2046_CS_SET */

static __IO bool touched = false;

struct cmd_input {
	uint8_t START;
	uint8_t A2;
	uint8_t A1;
	uint8_t A0;
	uint8_t MODE;
	uint8_t SER_DFR;
	uint8_t PID1;
	uint8_t PID0;
};

void xpt2046_InterruptOn(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI10_PB;
	EXTI->FTSR |= EXTI_FTSR_TR10;
	EXTI->IMR |= EXTI_IMR_MR10;
	XPT2046_EXTI_PORT->PUPDR |= GPIO_PUPDR_PUPD10_0;

	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void xpt2046_InterruptOff(void)
{
	EXTI->IMR &= ~EXTI_IMR_MR10;
	NVIC_DisableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler()
{
	EXTI->PR |= EXTI_PR_PR10;
	touched = true;
}

static uint8_t touch_generate_command(struct cmd_input *in)
{
	uint8_t ret = 0;
	if (in->START)
		ret = 1 << 7;
	if (in->A2)
		ret |= 1 << 6;
	if (in->A1)
		ret |= 1 << 5;
	if (in->A0)
		ret |= 1 << 4;
	if (in->MODE)
		ret |= 1 << 3;
	if (in->SER_DFR)
		ret |= 1 << 2;
	if (in->PID1)
		ret |= 1 << 1;
	if (in->PID0)
		ret |= 1;
	return ret;
}

static uint16_t touch_generate_short(uint8_t rx[2])
{
	uint16_t value = 0;
	value |= (rx[0] & 0b01000000) ? 1 << 11 : 0;
	value |= (rx[0] & 0b00100000) ? 1 << 10 : 0;
	value |= (rx[0] & 0b00010000) ? 1 <<  9 : 0;
	value |= (rx[0] & 0b00001000) ? 1 <<  8 : 0;
	value |= (rx[0] & 0b00000100) ? 1 <<  7 : 0;
	value |= (rx[0] & 0b00000010) ? 1 <<  6 : 0;
	value |= (rx[0] & 0b00000001) ? 1 <<  5 : 0;
	value |= (rx[1] & 0b10000000) ? 1 <<  4 : 0;
	value |= (rx[1] & 0b01000000) ? 1 <<  3 : 0;
	value |= (rx[1] & 0b00100000) ? 1 <<  2 : 0;
	value |= (rx[1] & 0b00010000) ? 1 <<  1 : 0;
	value |= (rx[1] & 0b00001000) ? 1 <<  0 : 0;
	return value;
}

static inline void xpt2046_transmit(uint8_t *cmd, uint8_t *rx, uint16_t bytes)
{
	XPT2046_CS_RESET;
	SPI_2_send(*cmd);
	SPI_2_read(rx, bytes);
	XPT2046_CS_SET;
}

void xpt2046_Init(void)
{
	SPI_2_init();

	XPT2046_CS_PORT->MODER |= GPIO_MODER_MODER12_0;
	XPT2046_CS_PORT->OSPEEDR |= GPIO_OSPEEDR_OSPEED12_0 | GPIO_OSPEEDR_OSPEED12_1;
	XPT2046_CS_SET;

	xpt2046_InterruptOn();
}

int xpt2046_read(uint16_t *x, uint16_t *y, uint16_t *z)
{
	struct cmd_input inp = {
		.START = 1,
		.A2 = 1,
		.A1 = 0,
		.A0 = 1,
		.MODE = 0,
		.SER_DFR = 0,
		.PID1 = 0,
		.PID0 = 0
	};
	uint16_t value = 0;
	uint8_t rx[2];
	uint8_t cmd;

	xpt2046_InterruptOff();

	cmd = touch_generate_command(&inp);
	xpt2046_transmit(&cmd, rx, 2);
	value = touch_generate_short(rx);
	*x = 4095 - value;
	*x = ((float) *x) / 4095.0f * ((float) ILI9341_WIDTH);

	inp.A2 = 0;
	cmd = touch_generate_command(&inp);
	xpt2046_transmit(&cmd, rx, 2);
	value = touch_generate_short(rx);
	*y = value;
	*y = ((float) *y) / 4095.0f * ((float) ILI9341_HEIGHT);

	uint16_t z1, z2;
	inp.A1 = 1;
	cmd = touch_generate_command(&inp);
	xpt2046_transmit(&cmd, rx, 2);
	z1 = touch_generate_short(rx);
	inp.A2 = 1;
	inp.A1 = 0;
	inp.A0 = 0;
	cmd = touch_generate_command(&inp);
	xpt2046_transmit(&cmd, rx, 2);
	z2 = touch_generate_short(rx);

	*z = (uint16_t) (10000.0f * ((float) *x) / 4096.0f * // z value is not in ili9341 scale
			(((float) z2) / ((float) z1) - 1.0f) + 0.5f);

	xpt2046_InterruptOn();

	return 0;
}

void xpt2046_task(void *pvParameters)
{
	while(1) {
		if (touched) {
			uint16_t x = 0, y = 0, z = 0;
			xpt2046_read(&x, &y, &z);
			if (x > 10 && y > 10 && z < 2000) {
				display_notify(x, y, z);
			}
			touched = false;
		}
		vTaskDelay(25);
	}
}
