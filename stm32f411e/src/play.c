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
static __IO bool bufferNotRead_0 = false, bufferNotRead_1 = false;
static int16_t *data_0 = NULL, *data_1 = NULL;
static size_t read_0, read_1;

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

	if (data_0 == NULL) {
		data_0 = pvPortMalloc(buffSize);
		if (!data_0)
			while(1);
	}

	if (data_1 == NULL) {
		data_1 = pvPortMalloc(buffSize);
		if (!data_1)
			while(1);
	}

	while(1) {
		while (bufferNotRead_0);
		f_read(&fp, (void *) data_0, buffSize, &bytes_read);
		read_0 = bytes_read;
		bufferNotRead_0 = true;
		if (bytes_read != buffSize) {
			goto end;
		}

		while(bufferNotRead_1);
		f_read(&fp, (void *) data_1, buffSize, &bytes_read);
		read_1 = bytes_read;
		bufferNotRead_1 = true;
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
		while (bufferNotRead_0);
		memset((void *) data_0, 0, buffSize);
		read_0 = buffSize;
		bufferNotRead_0 = true;

		while(bufferNotRead_1);
		memset((void *) data_1, 0, buffSize);
		read_1 = buffSize;
		bufferNotRead_1 = true;

		if (newSample) {
			newSample = false;
			closeFile = true;
			goto new_sample;
		}
	}

}

bool play_buffer_ready(int16_t **ptr, size_t *towrite)
{
	if (bufferNotRead_0) {
		*ptr = data_0;
		bufferNotRead_0 = false;
		*towrite = read_0;
		return true;
	} else if (bufferNotRead_1) {
		*ptr = data_1;
		bufferNotRead_1 = false;
		*towrite = read_1;
		return true;
	} else {
		*ptr = NULL;
		*towrite = 0;
		return false;
	}
}
