#include <stm32f4xx.h>
#include <core_cm4.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tm_ili9341.h"
#include "xpt2046.h"

#include "fatfs/ff.h"

#include "cs43l22.h"

#include "RCC.h"

#include "DMA.h"

#define GPIO_setBit(PORT, PIN) (PORT->BSRR |= PIN)
#define GPIO_clearBit(PORT, PIN) (PORT->BSRR |= (PIN << 0x10))

//TODO: timeouts in fatfs
//TODO: change compilation optimatization
//TODO: fix touchscreen

static void init_blue_led() {
	//RCC clock enable
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER15_0;
}

void ledTask(void * pvParameters)
{
	const TickType_t delay = 1000 / portTICK_PERIOD_MS;
	while(1) {
		GPIO_setBit(GPIOD, 1 << 15);
		vTaskDelay(delay);
		GPIO_clearBit(GPIOD, 1 << 15);
		vTaskDelay(delay);
	}
}

void lcdTask(void *pvParameters)
{
	const TickType_t delay = 1000 / portTICK_PERIOD_MS;
	while (1) {
		TM_ILI9341_Fill(ILI9341_COLOR_BLACK);
		vTaskDelay(delay);
		for (int i = 0; i < 320; i++) {
			TM_ILI9341_DrawPixel(120, i, ILI9341_COLOR_WHITE);
		}
		vTaskDelay(delay);
	}
}

int main(void){
	rcc_init_clock();
	TM_ILI9341_Init();
	xpt2046_Init();
	init_blue_led();
	FATFS FatFs;

	if (f_mount(&FatFs, "", 1) != FR_OK) {
		while(1);
	}

	FIL fp;
	if (f_open(&fp, "A.wav", FA_READ) != FR_OK) {
		while(1);
	}

	char line[16];
	f_gets(line, sizeof(line), &fp);

	cs43l22_init();
	DMA_init();

	TaskHandle_t xHandle = NULL;
	xTaskCreate(ledTask, "LEDTask", 64, 0, tskIDLE_PRIORITY, &xHandle);
	xTaskCreate(lcdTask, "LCDTask", 64, 0, tskIDLE_PRIORITY, &xHandle);
	xTaskCreate(xpt2046_task, "XPT2046Task", 128, 0, tskIDLE_PRIORITY, &xHandle);
	xTaskCreate(cs43l22_task, "cs43l22Task", 128, 0, tskIDLE_PRIORITY + 1, &xHandle);
	vTaskStartScheduler();

	while(1){
	}

	return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	while(1);
}
