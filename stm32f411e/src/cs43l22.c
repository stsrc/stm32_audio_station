#include <stddef.h>
#include <stm32f4xx.h>
#include <math.h>
#include <FreeRTOS.h>

#include "cs43l22.h"
#include "I2C.h"
#include "SPI.h"

#define ADDRESS 0x94

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

	I2S_3_init();

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
/*	if (frequency > 22050.0) {
		return;
	}

	double S_rate = 22031.0;
	double T = 1.0 / S_rate;
	double t = 1.0 / frequency;
	size_t sampleCount = t / T;

	uint16_t *values = pvPortMalloc(sampleCount * sizeof(uint16_t));
	if (values == NULL) {
		while(1);
	}

	double freq = frequency;
	double omega = 2.0 * M_PI * freq;

	for (size_t i = 0; i < sampleCount; i++) {
		values[i] = (uint16_t) (1000.0 + 900.0 * sin(omega * i * T));
	}
*/
#define BUFFER_SIZE 2200
	int16_t audio_data[2*BUFFER_SIZE];
	for (int i = 0; i < BUFFER_SIZE; i++) {
		int16_t value = (int16_t)(32000.0 * sin(2.0 * M_PI * i / 22.0));
	        audio_data[i * 2] = value;
		audio_data[i * 2 + 1] = value;
	}
	static size_t it = 0;
	while(1) {
		I2S_3_write(audio_data[it]);
		it = (it + 1) % (2 * BUFFER_SIZE);
	}
}

void cs43l22_task(void *pvParameters)
{
	cs43l22_sin_tone(1000.0);
}
