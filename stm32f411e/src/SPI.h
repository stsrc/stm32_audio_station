#ifndef _SPI_H_
#define _SPI_H_
#include <stm32f4xx.h>

void SPI_show_error(int errorCode);
int SPI_1_init(void);
void SPI_1_send(uint8_t *data);
void SPI_1_DMA_send(uint8_t *data, uint16_t size);

#endif
