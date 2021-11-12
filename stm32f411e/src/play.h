#ifndef _PLAY_H_
#define _PLAY_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void play_sample(char *name);
void play_task(void *arg);
bool play_buffer_ready(int16_t **ptr, size_t *towrite);
#endif
