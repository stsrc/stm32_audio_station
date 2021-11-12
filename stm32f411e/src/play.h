#ifndef _PLAY_H_
#define _PLAY_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
struct play_buffer {
	volatile bool notRead;
	int16_t *data;
	size_t count;
};
void play_sample(char *name);
void play_task(void *arg);
bool play_buffer_ready(struct play_buffer **buffer);
#endif
