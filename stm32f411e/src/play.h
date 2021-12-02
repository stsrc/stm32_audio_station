#ifndef _PLAY_H_
#define _PLAY_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "fatfs/ff.h"

struct play_buffer {
	FIL fp;
	char *fileName;
	volatile bool notRead;
	volatile bool readHalf;
	volatile bool readAll;
	bool opened;
	int16_t *data;
	size_t size;
	struct play_buffer *next;
	struct play_buffer *prev;
};
void play_sample(char *name);
void play_task(void *arg);
bool play_buffer_ready(struct play_buffer **buffer);
#endif
