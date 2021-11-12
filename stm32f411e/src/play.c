#include "play.h"
#include "fatfs/ff.h"
#include "cs43l22.h"
#include <stdbool.h>
#include <stm32f4xx.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

static __IO char *sample = "B.wav";
static __IO bool newSample = false;

static __IO char *sample_to_mix = "D.wav";

static size_t buffSize = 4096;

struct play_buffer buffer_out_0, buffer_out_1;
struct play_buffer buffer_a, buffer_b;
const int buffer_count = 2;

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

void play_mix(struct play_buffer *a, struct play_buffer *b, struct play_buffer *out)
{
	size_t i;
	size_t a_size = a->size / sizeof(int16_t);
	size_t b_size = b->size / sizeof(int16_t);
	size_t out_size = out->size / sizeof(int16_t);
	for(i = 0;
	    i < out_size &&
	    i < a_size &&
	    i < b_size;
	    i++) {
		out->data[i] = a->data[i] / 2 + b->data[i] / 2;
	}

	for(; i < a_size; i++) {
		if (i < out_size) {
			out->data[i] = a->data[i];
		}
	}

	for(; i < b_size; i++) {
		if (i < out_size) {
			out->data[i] = b->data[i];
		}
	}
}

static void play_buffer_init(struct play_buffer *buffer)
{
	memset(buffer, 0, sizeof(struct play_buffer));
	buffer->size = buffSize;
	buffer->data = pvPortMalloc(buffer->size);
	if (!buffer->data)
		while(1);

	buffer->readHalf = true;
}

void play_task(void *arg)
{
	bool closeFile = false;
	FATFS FatFs;
	FRESULT res;
	FIL fp;
	FIL fp_to_mix;
	do {
		res = f_mount(&FatFs, "", 1);
	} while (res != FR_OK);

	play_buffer_init(&buffer_out_0);
	play_buffer_init(&buffer_out_1);
	play_buffer_init(&buffer_a);
	play_buffer_init(&buffer_b);

new_sample:
	if (closeFile) {
		f_close(&fp);
		f_close(&fp_to_mix);
		closeFile = false;
	}

	if (f_open(&fp, (char *) sample, FA_READ) != FR_OK) {
		while(1);
	}

	if (f_open(&fp_to_mix, (char *) sample_to_mix, FA_READ) != FR_OK) {
		while(1);
	}

	struct wavheader wavheader;
	UINT bytes_read;
	bool mono;

	f_read(&fp, (void *) &wavheader, sizeof(struct wavheader), &bytes_read);
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

	f_read(&fp_to_mix, (void *) &wavheader, sizeof(struct wavheader), &bytes_read);

	static int i = 0;
	struct play_buffer *buffer_out = NULL;
	bool a = true, b = true;
	while(1) {
		if (i == 0) {
			buffer_out = &buffer_out_0;
		} else {
			buffer_out = &buffer_out_1;
		}

		i = (i + 1) % buffer_count;

		while (buffer_out->notRead) {
			vTaskDelay(1);
		}

		if (a) {
			f_read(&fp, (void *) buffer_a.data, buffer_a.size, &bytes_read);
			if (bytes_read != buffer_a.size)
				a = false;
		} else {
			memset(buffer_a.data, 0, buffer_a.size);
		}

		if (b) {
			f_read(&fp_to_mix, (void *) buffer_b.data, buffer_b.size, &bytes_read);
			if (bytes_read != buffer_b.size)
				b = false;
		} else {
			memset(buffer_b.data, 0, buffer_b.size);
		}

		play_mix(&buffer_a, &buffer_b, buffer_out);

		buffer_out->notRead = true;
		buffer_out->readHalf = false;

		if (a == false && b == false) {
			goto end;
		}

		if (newSample) {
			newSample = false;
			closeFile = true;
			a = true;
			b = true;
			goto new_sample;
		}
	}

end:
	while(1) {
		if (i == 0) {
			buffer_out = &buffer_out_0;
		} else {
			buffer_out = &buffer_out_1;
		}

		i = (i + 1) % buffer_count;

		while (buffer_out->notRead) {
			vTaskDelay(1);
		}
		memset((void *) buffer_out->data, 0, buffSize);
		buffer_out->notRead = true;
		buffer_out->readHalf = false;

		if (newSample) {
			a = true;
			b = true;
			newSample = false;
			closeFile = true;
			goto new_sample;
		}
	}

}

bool play_buffer_ready(struct play_buffer **buffer)
{
	if (buffer_out_0.notRead) {
		*buffer = &buffer_out_0;
		buffer_out_0.notRead = false;
		return true;
	} else if (buffer_out_1.notRead) {
		*buffer = &buffer_out_1;
		buffer_out_1.notRead = false;
		return true;
	} else {
		*buffer = NULL;
		return false;
	}
}
