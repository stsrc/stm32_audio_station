#ifndef _METRONOME_H_
#define _METRONOME_H_
#include <stdint.h>

int metronome_init(uint32_t _bpm);
void metronome_task(void *arg);

//tick.wav
#endif
