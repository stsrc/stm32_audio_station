#include "DMA.h"
#include <stdbool.h>
#include "cs43l22.h"

volatile __IO bool do_new = true;

void DMA1_Stream5_IRQHandler(void)
{
	uint32_t hisr = DMA1->HISR;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (hisr & (1 << 11)) {
		do_new = true;
		DMA1->HIFCR |= DMA_HIFCR_CTCIF5 |
			       DMA_HIFCR_CHTIF5 |
			       DMA_HIFCR_CTEIF5 |
			       DMA_HIFCR_CDMEIF5|
			       DMA_HIFCR_CFEIF5;
		xHigherPriorityTaskWoken = cs43l22_dma_callback();
	} else if((hisr & (1 << 6)) || (hisr & (1 << 10))) {
		DMA1->HIFCR |= DMA_HIFCR_CTCIF5 |
			       DMA_HIFCR_CHTIF5 |
			       DMA_HIFCR_CTEIF5 |
			       DMA_HIFCR_CDMEIF5|
			       DMA_HIFCR_CFEIF5;
		cs43l22_dma_half_callback();
	} else {
		while(1);
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

bool DMA_I2S3_ok(void)
{
	return do_new;
}

static inline void DMA_turn_off(void)
{
	DMA1_Stream5->CR &= ~DMA_SxCR_EN; // DMA Stream Disabled
	while(DMA1_Stream5->CR & DMA_SxCR_EN);
}

static inline void DMA_turn_on(void)
{
	DMA1_Stream5->CR |= DMA_SxCR_EN; // DMA Stream Enabled
	while(!(DMA1_Stream5->CR & DMA_SxCR_EN));
}

void DMA_init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	DMA_turn_off();

	NVIC_SetPriority(DMA1_Stream5_IRQn, 8);
	NVIC_EnableIRQ(DMA1_Stream5_IRQn);

	DMA1_Stream5->CR |= DMA_SxCR_MSIZE_0; // Memory data size = halfword
	DMA1_Stream5->CR |= DMA_SxCR_PSIZE_0; // Peripheral data size = halfword
	DMA1_Stream5->CR |= DMA_SxCR_DIR_0;


	DMA1_Stream5->CR |= DMA_SxCR_TCIE;
	DMA1_Stream5->CR |= DMA_SxCR_HTIE;
	DMA1_Stream5->CR |= DMA_SxCR_TEIE;
	DMA1_Stream5->CR |= DMA_SxCR_DMEIE;
	DMA1_Stream5->FCR |= DMA_SxFCR_FEIE;

//	DMA1_Stream5->CR |= DMA_SxCR_PFCTRL;

}

bool DMA_I2S3_write_half_word(int16_t hw)
{
	if (!do_new)
		return false;

	static int16_t buffer = 0;
	do_new = false;
	buffer = hw;

	if (DMA1_Stream5->CR & DMA_SxCR_EN) {
		DMA_turn_off();
	}

	DMA1_Stream5->PAR = (uint32_t) &(SPI3->DR);
	DMA1_Stream5->M0AR = (uint32_t) &buffer;
	DMA1_Stream5->NDTR = 1; //1 - number of data items to transfer
//	DMA1_Stream5->CR |= DMA_SxCR_CHSEL_1; // Stream 5, channel 2, DMA 1 (I hope)
	DMA1_Stream5->CR &= ~DMA_SxCR_MINC;
	DMA_turn_on();

	return true;
}

bool DMA_I2S3_write_half_words(int16_t *buffer, size_t count)
{
	if (!do_new)
		return false;

	do_new = false;
	if (DMA1_Stream5->CR & DMA_SxCR_EN) {
		DMA_turn_off();
	}
	DMA1_Stream5->PAR = (uint32_t) &(SPI3->DR);
	DMA1_Stream5->M0AR = (uint32_t) buffer;
	DMA1_Stream5->NDTR = count;
	DMA1_Stream5->CR |= DMA_SxCR_MINC;
	DMA_turn_on();

	return true;
}
