#ifndef _RCC_H_
#define _RCC_H_

void rcc_init_clock();
void rcc_init_i2s_clock();
enum RCC_PLLI2S_clock {
	RCC_PLLI2S_48000,
	RCC_PLLI2S_44100,
	RCC_PLLI2S_32000
};

void rcc_set_PLLI2S_clock(enum RCC_PLLI2S_clock clock);

#endif
