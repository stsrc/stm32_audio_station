#include <stddef.h>
#include <stm32f4xx.h>
#include <math.h>
#include <limits.h>

#include "FreeRTOS.h"
#include "task.h"

#include "cs43l22.h"
#include "I2C.h"
#include "SPI.h"
#include "DMA.h"
#include "RCC.h"

#define ADDRESS 0x94

static enum cs43l22_clock currentClock;
static double clock[] = { 47991.0714285714, 44108.0729166667, 32001.2019230769 };

static uint8_t cs43l22_read_reg(uint8_t reg)
{
	uint8_t value;
	I2C1_write(ADDRESS, &reg, sizeof(reg));
	I2C1_read(ADDRESS, &value);
	return value;
}

static void cs43l22_write_reg(uint8_t reg, uint8_t data)
{
	uint8_t pair[2] = {reg, data};
	I2C1_write(ADDRESS, pair, sizeof(pair));
}

void cs43l22_init(void)
{
	I2C1_init();

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER4_0;
	GPIOD->ODR |= GPIO_ODR_OD4;

	rcc_init_i2s_clock();
	I2S_3_init();
	currentClock = cs43l22_48000;

	cs43l22_write_reg(0x04, 0xaf);
	cs43l22_write_reg(0x06, 0x07);

	//power on
	cs43l22_write_reg(0x02, 0x9E);

	//set volume to sane level
	cs43l22_write_reg(0x22, 0b10001000);
	cs43l22_write_reg(0x23, 0b10001000);
}

uint8_t cs43l22_beep(void)
{
	uint8_t freq = (0b00000111 < 4); //1kHz
	uint8_t time = 0b00001111;
	cs43l22_write_reg(0x1c, freq | time);

	uint8_t vol = (0b00110);
	cs43l22_write_reg(0x1d, vol);

	uint8_t occurance = 0b11 << 6;
	uint8_t mix = 0b1 << 5;
	cs43l22_write_reg(0x1e, occurance | mix);

	uint8_t status = cs43l22_read_reg(0x2e);
	return status;
}

void cs43l22_sin_tone(double frequency)
{
	double S_rate = clock[currentClock];

	if (frequency > 22050.0) {
		while(1);
	}

	const size_t LUT_SIZE = round(S_rate / 750);
	int16_t *LUT = pvPortMalloc(LUT_SIZE * sizeof(int16_t));
	if (!LUT)
		while(1);

	for (size_t i = 0; i < LUT_SIZE; i++) {
		LUT[i] = (int16_t) round(SHRT_MAX * sin(2.0 * M_PI * (double) i / LUT_SIZE));
	}

	const int BUFF_SIZE = LUT_SIZE;
	int16_t *audio_data = pvPortMalloc(2 * BUFF_SIZE * sizeof(int16_t));
	if (!audio_data)
		while(1);

	const double delta_phi = frequency / S_rate * LUT_SIZE;

	double phase = 0.0f;

	for (int i = 0; i < 2 * BUFF_SIZE; i += 2) {
		int phase_i = (int) phase;
		audio_data[i] = LUT[phase_i];
		audio_data[i + 1] = audio_data[i];
		phase += delta_phi;
		if (phase >= (double)LUT_SIZE)
			phase -= (double)LUT_SIZE;
	}

//	for (int i = 0; i < 5000; i++) {
	for (;;) {
		bool ret;
		do {
			ret = DMA_I2S3_write_half_words(audio_data, 2 * BUFF_SIZE);
		} while(!ret);
		vTaskDelay(1);
	}

	vPortFree(audio_data);
}

void cs43l22_task(void *pvParameters)
{
	cs43l22_sin_tone(1500.0);
}

void cs43l22_set_clock(enum cs43l22_clock clock)
{
	switch (clock) {
	case cs43l22_48000:
		rcc_set_PLLI2S_clock(RCC_PLLI2S_48000);
		I2S_3_set_clock(I2S_48000);
		break;
	case cs43l22_44100:
		rcc_set_PLLI2S_clock(RCC_PLLI2S_44100);
		I2S_3_set_clock(I2S_44100);
		break;
	case cs43l22_32000:
		rcc_set_PLLI2S_clock(RCC_PLLI2S_32000);
		I2S_3_set_clock(I2S_32000);
		break;
	default:
		while(1);
	}
	currentClock = clock;
}
