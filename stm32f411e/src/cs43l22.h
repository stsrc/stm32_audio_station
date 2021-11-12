#ifndef _CS43L22_H_
#define _CS43L22_H_

#include <stdint.h>

void cs43l22_init(void);
uint8_t cs43l22_beep(void);
void cs43l22_task(void *argument);
void cs43l22_sin_tone(double frequency);

enum cs43l22_clock {
	cs43l22_48000 = 0,
	cs43l22_44100,
	cs43l22_32000
};

void cs43l22_set_clock(enum cs43l22_clock clock);

#endif
