#ifndef _DMA_H_
#define _DMA_H_
#include <stm32f4xx.h>
#include <core_cm4.h>
#include <stdlib.h>
#include <stdbool.h>

void DMA_init(void);
bool DMA_I2S3_write_half_word(int16_t hw);

#endif
