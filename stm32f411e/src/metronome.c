#include "metronome.h"
#include "TIM.h"
#include "FreeRTOS.h"
#include "task.h"
#include "play.h"
#include "display.h"
#include <math.h>
#include <string.h>

static uint32_t bpm = 0;

int metronome_init(uint32_t _bpm)
{
	if (_bpm > 240)
		return -1;

	bpm = _bpm;
	return 0;
}

const char *ptrs[64];

void metronome_task(void *arg)
{
	float amount = 1000.0f * 60.0f / ((float) bpm) / 16.0;

	memset(ptrs, 0, sizeof(char *) * 64);

	vTaskDelay(1000);
	while(1) {
		for (int i = 0; i < 4; i++) {
			play_sample("tick.wav");
			for (int j = 0; j < 16; j++) {
				const char *sound = NULL;
				display_get_button(&sound);
				if (sound) {
					ptrs[i * 16 + j] = sound;
					play_sample((char *) sound);
				} else if (ptrs[i * 16 + j]) {
					play_sample((char *) ptrs[i * 16 + j]);
				}
				vTaskDelay(round(amount));
				TIM2_delay_us((amount - round(amount)) * 1000.0);
			}
		}
	}
}
