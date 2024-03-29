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
#include "TIM.h"
#include "display.h"
#include "play.h"
#include "metronome.h"
#include "ADC.h"

#define GPIO_setBit(PORT, PIN) (PORT->BSRR |= PIN)
#define GPIO_clearBit(PORT, PIN) (PORT->BSRR |= (PIN << 0x10))

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

int main(void){
	rcc_init_clock();
	TIM2_init();
	init_blue_led();

	TM_ILI9341_Init();
	display_init();
	xpt2046_Init();

	cs43l22_init();
	DMA_init();

	metronome_init(90);

	ADC_init();
	ADC_probe();
	ADC_result();

	TaskHandle_t xHandle = NULL;
	xTaskCreate(ledTask, "LEDTask", 64, 0, tskIDLE_PRIORITY, &xHandle);
	xTaskCreate(xpt2046_task, "XPT2046Task", 128, 0, tskIDLE_PRIORITY, &xHandle);
	xTaskCreate(display_task, "displayTask", 128, 0, tskIDLE_PRIORITY, &xHandle);
	xTaskCreate(metronome_task, "metronomeTask", 512, 0, tskIDLE_PRIORITY + 1, &xHandle);
	xTaskCreate(play_task, "playTask", 512, 0, tskIDLE_PRIORITY + 2, &xHandle);
	xTaskCreate(cs43l22_task, "cs43l22Task", 512, 0, tskIDLE_PRIORITY + 3, &xHandle);

	vTaskStartScheduler();

	while(1){
	}

	return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	while(1);
}
