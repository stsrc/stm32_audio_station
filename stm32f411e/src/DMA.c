#include "DMA.h"
#include <stdbool.h>

volatile __IO bool do_new = true;
void DMA1_Stream5_IRQHandler(void)
{
	uint32_t hisr = DMA1->HISR;

	if (hisr & (1 << 11)) {
		do_new = true;
		DMA1->HIFCR |= DMA_HIFCR_CTCIF5 |
			       DMA_HIFCR_CHTIF5 |
			       DMA_HIFCR_CTEIF5 |
			       DMA_HIFCR_CDMEIF5|
			       DMA_HIFCR_CFEIF5;
	} else {
		while(1);
	}


}

void DMA_init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	DMA1_Stream5->CR &= ~DMA_SxCR_EN; // DMA Stream Disabled
	while(DMA1_Stream5->CR & DMA_SxCR_EN);

	NVIC_SetPriority(DMA1_Stream5_IRQn, 0x00);
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
		DMA1_Stream5->CR &= ~DMA_SxCR_EN; // DMA Stream Disabled
		while(DMA1_Stream5->CR & DMA_SxCR_EN);
	}

	DMA1_Stream5->PAR = (uint32_t) &(SPI3->DR);
	DMA1_Stream5->M0AR = (uint32_t) &buffer;
	DMA1_Stream5->NDTR = 1; //1 - number of data items to transfer
//	DMA1_Stream5->CR |= DMA_SxCR_CHSEL_1; // Stream 5, channel 2, DMA 1 (I hope)

	DMA1_Stream5->CR |= DMA_SxCR_EN; // DMA Stream Enabled
	while(!(DMA1_Stream5->CR & DMA_SxCR_EN));

	return true;
}
