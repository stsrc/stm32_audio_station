#include "DMA.h"
#include <stdbool.h>
#include "cs43l22.h"

#include "FreeRTOS.h"
#include "task.h"

volatile __IO bool do_new = true;
static TaskHandle_t xTaskToNotify = NULL;

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

void DMA2_Stream0_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint32_t lisr = DMA2->LISR;
	if (lisr & (1 << 5)) {
		DMA2->LIFCR |= DMA_LIFCR_CTCIF0 |
			       DMA_LIFCR_CHTIF0 |
			       DMA_LIFCR_CTEIF0 |
			       DMA_LIFCR_CDMEIF0|
			       DMA_LIFCR_CFEIF0;
		if (xTaskToNotify) {
			vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
		}
	} else if((lisr & (1 << 0)) || (lisr & (1 << 4))) {
		DMA2->LIFCR |= DMA_LIFCR_CTCIF0 |
			       DMA_LIFCR_CHTIF0 |
			       DMA_LIFCR_CTEIF0 |
			       DMA_LIFCR_CDMEIF0|
			       DMA_LIFCR_CFEIF0;
	} else {
		while(1);
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void DMA2_Stream1_IRQHandler(void)
{
	DMA2->LIFCR |= DMA_LIFCR_CTCIF1 |
		       DMA_LIFCR_CHTIF1 |
		       DMA_LIFCR_CTEIF1 |
		       DMA_LIFCR_CDMEIF1|
		       DMA_LIFCR_CFEIF1;
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

static inline void DMA2_turn_off(void)
{
	DMA2_Stream0->CR &= ~DMA_SxCR_EN;
	while(DMA2_Stream0->CR & DMA_SxCR_EN);
	DMA2_Stream1->CR &= ~DMA_SxCR_EN;
	while(DMA2_Stream1->CR & DMA_SxCR_EN);
}

static inline void DMA_turn_on(void)
{
	DMA1_Stream5->CR |= DMA_SxCR_EN; // DMA Stream Enabled
	while(!(DMA1_Stream5->CR & DMA_SxCR_EN));
}

static inline void DMA2_turn_on(void)
{
	DMA2_Stream0->CR |= DMA_SxCR_EN;
	while(!(DMA2_Stream0->CR & DMA_SxCR_EN));
	DMA2_Stream1->CR |= DMA_SxCR_EN;
	while(!(DMA2_Stream1->CR & DMA_SxCR_EN));
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
	//TODO BURST?

	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	DMA2_turn_off();

	NVIC_SetPriority(DMA2_Stream0_IRQn, 9);
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);
	DMA2_Stream0->CR |= DMA_SxCR_CHSEL_2;
	DMA2_Stream0->CR |= DMA_SxCR_TCIE;
	DMA2_Stream0->CR |= DMA_SxCR_HTIE;
	DMA2_Stream0->CR |= DMA_SxCR_TEIE;
	DMA2_Stream0->CR |= DMA_SxCR_DMEIE;
	DMA2_Stream0->FCR |= DMA_SxFCR_FEIE;

	NVIC_SetPriority(DMA2_Stream1_IRQn, 10);
	NVIC_EnableIRQ(DMA2_Stream1_IRQn);
	DMA2_Stream1->CR |= DMA_SxCR_DIR_0;
	DMA2_Stream1->CR |= DMA_SxCR_CHSEL_2;
	DMA2_Stream1->CR |= DMA_SxCR_TCIE;
	DMA2_Stream1->CR |= DMA_SxCR_HTIE;
	DMA2_Stream1->CR |= DMA_SxCR_TEIE;
	DMA2_Stream1->CR |= DMA_SxCR_DMEIE;
	DMA2_Stream1->FCR |= DMA_SxFCR_FEIE;
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

bool DMA_SPI4_read_bytes(uint8_t *buffer, size_t count)
{
	const uint8_t tmp = 0xff;
	xTaskToNotify = xTaskGetCurrentTaskHandle();

	DMA2_Stream0->PAR = (uint32_t) &(SPI4->DR);
	DMA2_Stream0->M0AR = (uint32_t) buffer;
	DMA2_Stream0->NDTR = count;
	DMA2_Stream0->CR |= DMA_SxCR_MINC;

	DMA2_Stream1->PAR = (uint32_t) &(SPI4->DR);
	DMA2_Stream1->M0AR = (uint32_t) &tmp;
	DMA2_Stream1->NDTR = count;

	DMA2_turn_on();

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	if (DMA2_Stream0->CR & DMA_SxCR_EN) {
		DMA2_turn_off();
	}

	return true;
}
