#include "metronome.h"
#include "TIM.h"
#include "FreeRTOS.h"
#include "task.h"
#include "play.h"
#include <math.h>

static uint32_t bpm = 0;

int metronome_init(uint32_t _bpm)
{
	if (_bpm > 240)
		return -1;

	bpm = _bpm;
	return 0;
}

void metronome_task(void *arg)
{
	float amount = 1000.0f * 60.0f / ((float) bpm);

	vTaskDelay(1000);
	while(1) {
		play_sample("tick.wav");
		vTaskDelay(round(amount));
		TIM2_delay_us((amount - round(amount)) * 1000.0);
	}
}
