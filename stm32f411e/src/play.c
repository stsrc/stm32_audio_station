#include "play.h"
#include "fatfs/ff.h"
#include "cs43l22.h"
#include <stdbool.h>
#include <stm32f4xx.h>
#include <string.h>
#include "FreeRTOS.h"

static __IO char *sample = "A.wav";
static __IO bool newSample = false;


static size_t buffSize = 8192;

struct play_buffer buffer_0, buffer_1;

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

void play_task(void *arg)
{
	bool closeFile = false;
	FATFS FatFs;
	FRESULT res;
	FIL fp;
	do {
		res = f_mount(&FatFs, "", 1);
	} while (res != FR_OK);

	buffer_0.data = pvPortMalloc(buffSize);
	if (!buffer_0.data)
		while(1);
	buffer_1.data = pvPortMalloc(buffSize);
	if (!buffer_1.data)
		while(1);

new_sample:
	if (closeFile) {
		f_close(&fp);
		closeFile = false;
	}

	if (f_open(&fp, (char *) sample, FA_READ) != FR_OK) {
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

	while(1) {
		while (buffer_0.notRead);
		f_read(&fp, (void *) buffer_0.data, buffSize, &bytes_read);
		buffer_0.count = bytes_read;
		buffer_0.notRead = true;
		if (bytes_read != buffSize) {
			goto end;
		}

		while(buffer_1.notRead);
		f_read(&fp, (void *) buffer_1.data, buffSize, &bytes_read);
		buffer_1.count = bytes_read;
		buffer_1.notRead = true;
		if (bytes_read != buffSize) {
			goto end;
		}

		if (newSample) {
			newSample = false;
			closeFile = true;
			goto new_sample;
		}
	}

end:
	while(1) {
		while (buffer_0.notRead);
		memset((void *) buffer_0.data, 0, buffSize);
		buffer_0.count = buffSize;
		buffer_0.notRead = true;

		while(buffer_1.notRead);
		memset((void *) buffer_1.data, 0, buffSize);
		buffer_1.count = buffSize;
		buffer_1.notRead = true;

		if (newSample) {
			newSample = false;
			closeFile = true;
			goto new_sample;
		}
	}

}

bool play_buffer_ready(struct play_buffer **buffer)
{
	if (buffer_0.notRead) {
		*buffer = &buffer_0;
		buffer_0.notRead = false;
		return true;
	} else if (buffer_1.notRead) {
		*buffer = &buffer_1;
		buffer_1.notRead = false;
		return true;
	} else {
		*buffer = NULL;
		return false;
	}
}
