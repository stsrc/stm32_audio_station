#include "play.h"
#include "fatfs/ff.h"
#include "cs43l22.h"
#include <stdbool.h>
#include <stm32f4xx.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

static __IO char *sample = "B.wav";
static __IO bool newSample = false;

static const char *sample_to_mix = "D.wav";

static size_t buffSize = 4096;

struct play_buffer buffer_out_0, buffer_out_1;
struct play_buffer buffer_a, buffer_b;
const int buffer_count = 2;

static bool buffer_0 = true;

void play_sample(char *name)
{
	if (name) {
		sample = name;
		newSample = true;
	}
}

struct wavheader
{
    /* RIFF Chunk Descriptor */
    uint8_t         RIFF[4];        // RIFF Header Magic header
    uint32_t        ChunkSize;      // RIFF Chunk Size
    uint8_t         WAVE[4];        // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t         fmt[4];         // FMT header
    uint32_t        Subchunk1Size;  // Size of the fmt chunk
    uint16_t        AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t        NumOfChan;      // Number of channels 1=Mono 2=Sterio
    uint32_t        SamplesPerSec;  // Sampling Frequency in Hz
    uint32_t        bytesPerSec;    // bytes per second
    uint16_t        blockAlign;     // 2=16-bit mono, 4=16-bit stereo
    uint16_t        bitsPerSample;  // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t         Subchunk2ID[4]; // "data"  string
    uint32_t        Subchunk2Size;  // Sampled data length
};

void play_mix(struct play_buffer *a, struct play_buffer *out, bool first)
{
	size_t i;
	size_t a_size = a->size / sizeof(int16_t);
	size_t out_size = out->size / sizeof(int16_t);
	for(i = 0;
	    i < out_size &&
	    i < a_size;
	    i++) {
		if (first) {
			memcpy(out->data, a->data, a->size);
			break;
		} else {
			out->data[i] = a->data[i] / 2 + out->data[i] / 2;
		}
	}
}

static void play_buffer_init(struct play_buffer *buffer, const char *path)
{
	memset(buffer, 0, sizeof(struct play_buffer));
	buffer->size = buffSize;
	buffer->data = pvPortMalloc(buffer->size);
	if (!buffer->data)
		while(1);

	buffer->notRead = false;
	buffer->readHalf = true;
	buffer->readAll = true;

	if (!path)
		return;

	buffer->fileName = strdup(path);

	if (f_open(&(buffer->fp), buffer->fileName, FA_READ) != FR_OK)
		while(1);

	struct wavheader wavheader;
	UINT bytes_read;
	bool mono;

	f_read(&buffer->fp, (void *) &wavheader, sizeof(struct wavheader), &bytes_read);
	if (bytes_read != sizeof(struct wavheader))
		while(1);

	if (wavheader.AudioFormat != 1)
		while(1);

	if (wavheader.bytesPerSec / wavheader.SamplesPerSec != 2)
		mono = false;
	else
		mono = true;

	if (mono)
		while(1);

	enum cs43l22_clock clock;

	switch(wavheader.SamplesPerSec) {
	case 48000:
		clock = cs43l22_48000;
		break;
	case 44100:
		clock = cs43l22_44100;
		break;
	default:
		while(1);
	}

	cs43l22_set_clock(clock);
}

/*
static void play_buffer_deinit(struct play_buffer *buffer)
{
	if (buffer->data)
		vPortFree(buffer->data);

	if (buffer->fileName) {
		f_close(&buffer->fp);
		free(buffer->fileName);
	}
}
*/

void play_task(void *arg)
{
	UINT bytes_read;
	FATFS FatFs;
	FRESULT res;
	do {
		res = f_mount(&FatFs, "", 1);
	} while (res != FR_OK);


	buffer_0 = true;
	play_buffer_init(&buffer_out_0, NULL);
	play_buffer_init(&buffer_out_1, NULL);
	play_buffer_init(&buffer_a, (const char *) sample);
	play_buffer_init(&buffer_b, sample_to_mix);

	int i = 0;
	struct play_buffer *buffer_out = NULL;
	bool loop_a = true, loop_b = true;
new_sample:
	if (strcmp(buffer_a.fileName, (const char *) sample)) {
		f_close(&buffer_a.fp);
		free(buffer_a.fileName);
		buffer_a.fileName = strdup((const char *) sample);
		if (f_open(&buffer_a.fp, buffer_a.fileName, FA_READ) != FR_OK)
			while(1);
		f_lseek(&buffer_a.fp, sizeof(struct wavheader));
	} else {
		f_lseek(&buffer_a.fp, sizeof(struct wavheader));
	}

	f_lseek(&buffer_b.fp, sizeof(struct wavheader));

	loop_a = true;
	loop_b = true;

	while(loop_a || loop_b) {
		if (i == 0) {
			buffer_out = &buffer_out_0;
		} else {
			buffer_out = &buffer_out_1;
		}

		i = (i + 1) % buffer_count;

		while (buffer_out->notRead || !buffer_out->readAll) {
			vTaskDelay(1);
		}

		for (int i = 0; i < 2; i++) {
			struct play_buffer *buffer;
			bool firstLoop = true;
			if (i == 0) {
				firstLoop = true;
				buffer = &buffer_a;
			} else {
				firstLoop = false;
				buffer = &buffer_b;
			}

			if (((i == 0) && (loop_a == true)) || ((i == 1) && (loop_b == true))) {
				f_read(&buffer->fp, (void *) buffer->data, buffer->size, &bytes_read);
			} else {
				memset(buffer->data, 0, buffer->size);
				bytes_read = buffer->size;
			}

			if (bytes_read != buffer->size) {
				if (i == 0) {
					loop_a = false;
				} else {
					loop_b = false;
				}
			}

			play_mix(buffer, buffer_out, firstLoop);
		}

		buffer_out->notRead = true;
		buffer_out->readHalf = false;
		buffer_out->readAll = false;

		if (newSample) {
			newSample = false;
			goto new_sample;
		}
	}

	while(1) {
		if (i == 0) {
			buffer_out = &buffer_out_0;
		} else {
			buffer_out = &buffer_out_1;
		}

		i = (i + 1) % buffer_count;

		while (buffer_out->notRead || !buffer_out->readAll) {
			vTaskDelay(1);
		}

		memset((void *) buffer_out->data, 0, buffSize);
		buffer_out->notRead = true;
		buffer_out->readHalf = false;
		buffer_out->readAll = false;

		if (newSample) {
			newSample = false;
			goto new_sample;
		}
	}

}

bool play_buffer_ready(struct play_buffer **buffer)
{
	if (buffer_0) {
		if (buffer_out_0.notRead) {
			*buffer = &buffer_out_0;
			buffer_out_0.notRead = false;
			buffer_0 = false;
			return true;
		}
	} else {
		if (buffer_out_1.notRead) {
			*buffer = &buffer_out_1;
			buffer_out_1.notRead = false;
			buffer_0 = true;
			return true;
		}
	}
	*buffer = NULL;
	return false;
}
