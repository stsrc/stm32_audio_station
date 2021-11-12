#ifndef _SPI_H_
#define _SPI_H_
#include <stm32f4xx.h>

void SPI_show_error(int errorCode);
int SPI_1_init(void);
void SPI_1_send(uint8_t *data);

void SPI_2_init(void);
void SPI_2_send(uint8_t data);
void SPI_2_read(uint8_t *rx, uint16_t bytes);

//SPI4 is faster on stm32f411xe
void SPI_4_init(void);
uint8_t SPI_4_send(uint8_t data);
void SPI_4_send_multi(const uint8_t *data, uint32_t bytes);
//send 0xff as dummy
void SPI_4_read(uint8_t *rx, uint32_t bytes);

void I2S_3_init(void);
void I2S_3_write(int16_t value);

enum I2S_clock {
	I2S_48000,
	I2S_44100,
	I2S_32000
};

void I2S_3_set_clock(enum I2S_clock clock);
#endif
