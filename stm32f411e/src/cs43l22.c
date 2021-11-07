#include <stddef.h>
#include <stm32f4xx.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "fatfs/ff.h"
#include "cs43l22.h"
#include "I2C.h"
#include "SPI.h"
#include "DMA.h"
#include "RCC.h"

#define ADDRESS 0x94

static enum cs43l22_clock currentClock;
static double clock[] = { 47991.0714285714, 44108.0729166667, 32001.2019230769 };

static uint8_t cs43l22_read_reg(uint8_t reg)
{
	uint8_t value;
	I2C1_write(ADDRESS, &reg, sizeof(reg));
	I2C1_read(ADDRESS, &value);
	return value;
}

static void cs43l22_write_reg(uint8_t reg, uint8_t data)
{
	uint8_t pair[2] = {reg, data};
	I2C1_write(ADDRESS, pair, sizeof(pair));
}

void cs43l22_init(void)
{
	I2C1_init();

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER4_0;
	GPIOD->ODR |= GPIO_ODR_OD4;

	rcc_init_i2s_clock();
	I2S_3_init();
	currentClock = cs43l22_48000;

	cs43l22_write_reg(0x04, 0xaf);
	cs43l22_write_reg(0x06, 0x07);

	//power on
	cs43l22_write_reg(0x02, 0x9E);

	//set volume to sane level
	cs43l22_write_reg(0x22, 0b11011100);
	cs43l22_write_reg(0x23, 0b11011100);
}

uint8_t cs43l22_beep(void)
{
	uint8_t freq = (0b00000111 < 4); //1kHz
	uint8_t time = 0b00001111;
	cs43l22_write_reg(0x1c, freq | time);

	uint8_t vol = (0b00110);
	cs43l22_write_reg(0x1d, vol);

	uint8_t occurance = 0b11 << 6;
	uint8_t mix = 0b1 << 5;
	cs43l22_write_reg(0x1e, occurance | mix);

	uint8_t status = cs43l22_read_reg(0x2e);
	return status;
}

void cs43l22_sin_tone(double frequency)
{
	double S_rate = clock[currentClock];

	if (frequency > 22050.0) {
		while(1);
	}

	const size_t LUT_SIZE = 1024;
	int16_t *LUT = pvPortMalloc(LUT_SIZE * sizeof(int16_t));
	if (!LUT)
		while(1);

	for (size_t i = 0; i < LUT_SIZE; i++) {
		LUT[i] = (int16_t) round(SHRT_MAX * sin(2.0 * M_PI * (double) i / LUT_SIZE));
	}

	const int BUFF_SIZE = 4096;
	int16_t *audio_data = pvPortMalloc(2 * BUFF_SIZE * sizeof(int16_t));
	if (!audio_data)
		while(1);

	const double delta_phi = frequency / S_rate * LUT_SIZE;

	double phase = 0.0f;

	for (int i = 0; i < 2 * BUFF_SIZE; i += 2) {
		int phase_i = (int) phase;
		audio_data[i] = LUT[phase_i];
		audio_data[i + 1] = audio_data[i];
		phase += delta_phi;
		if (phase >= (double)LUT_SIZE)
			phase -= (double)LUT_SIZE;
	}

	for (;;) {
		bool ret;
		do {
			ret = DMA_I2S3_write_half_words(audio_data, 2 * BUFF_SIZE);
		} while(!ret);
		vTaskDelay(1);
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

static size_t buffSize = 8192;
static __IO bool bufferNotRead_0 = false, bufferNotRead_1 = false;
static int16_t *data_0 = NULL, *data_1 = NULL;
static size_t read_0, read_1;

static bool cs43l22_buffer_ready(int16_t **ptr, size_t *towrite)
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


static void cs43l22_play_content()
{
	bool mono = false;

	int16_t *data = NULL;
	size_t towrite = 0;

	if (!mono) {
		while(1) {
	                bool ret;
			do {
				vTaskDelay(1);
				ret = cs43l22_buffer_ready(&data, &towrite);
			} while(!ret);

	                do {
	                        ret = DMA_I2S3_write_half_words(data, towrite / sizeof(int16_t));
	                } while(!ret);
		}
	} else {
		while(1);
	}
}

void cs43l22_task(void *pvParameters)
{
//	cs43l22_sin_tone(1234.5);
	cs43l22_play_content();
}

void cs43l22_task_file_read(void *argument)
{
	FATFS FatFs;
	FRESULT res;
	do {
		res = f_mount(&FatFs, "", 1);
	} while (res != FR_OK);

	FIL fp;

	if (f_open(&fp, "A.wav", FA_READ) != FR_OK) {
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

	data_0 = pvPortMalloc(buffSize);
	if (!data_0)
		while(1);

	data_1 = pvPortMalloc(buffSize);
	if (!data_1)
		while(1);

	while(1) {
		while (bufferNotRead_0);
		f_read(&fp, (void *) data_0, buffSize, &bytes_read);
		read_0 = bytes_read;
		if (bytes_read != buffSize) {
			f_lseek(&fp, 44);
		}
		bufferNotRead_0 = true;

		while(bufferNotRead_1);
		f_read(&fp, (void *) data_1, buffSize, &bytes_read);
		read_1 = bytes_read;
		if (bytes_read != buffSize) {
			f_lseek(&fp, 44);
		}
		bufferNotRead_1 = true;

	}
}

void cs43l22_set_clock(enum cs43l22_clock clock)
{
	switch (clock) {
	case cs43l22_48000:
		rcc_set_PLLI2S_clock(RCC_PLLI2S_48000);
		I2S_3_set_clock(I2S_48000);
		break;
	case cs43l22_44100:
		rcc_set_PLLI2S_clock(RCC_PLLI2S_44100);
		I2S_3_set_clock(I2S_44100);
		break;
	case cs43l22_32000:
		rcc_set_PLLI2S_clock(RCC_PLLI2S_32000);
		I2S_3_set_clock(I2S_32000);
		break;
	default:
		while(1);
	}
	currentClock = clock;
}
