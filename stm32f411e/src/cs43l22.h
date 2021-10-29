#ifndef _CS43L22_H_
#define _CS43L22_H_

#include <stdint.h>

void cs43l22_init(void);
uint8_t cs43l22_beep(void);
void cs43l22_task(void *argument);
void cs43l22_sin_tone(double frequency);
#endif