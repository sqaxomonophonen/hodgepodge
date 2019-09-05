#ifndef WAV_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

static inline void wav__u8(FILE* file, uint8_t v)
{
	assert(fputc(v, file) == v);
}

static inline void wav__u16(FILE* file, uint16_t v)
{
	for (int shift = 0; shift < 16; shift += 8) wav__u8(file, (v >> shift) & 0xff);
}

static inline void wav__i16(FILE* file, int16_t v)
{
	wav__u16(file, v);
}

static inline void wav__u32(FILE* file, uint32_t v)
{
	for (int shift = 0; shift < 32; shift += 16) wav__u16(file, (v >> shift) & 0xffff);
}

static inline void wav__str(FILE* file, const char* str)
{
	size_t n = strlen(str);
	assert(fwrite(str, sizeof *str, n, file) == n);
}

static inline void wav_header(FILE* file, int sample_rate, int n_channels, int n_frames)
{
	wav__str(file, "RIFF");
	const int data_length = 2 * n_channels * n_frames;
	wav__u32(file, data_length + 36);
	wav__str(file, "WAVEfmt ");
	wav__u32(file, 16);
	wav__u16(file, 1);
	wav__u16(file, n_channels);
	wav__u32(file, sample_rate);
	wav__u32(file, sample_rate * n_channels * 2);
	wav__u16(file, n_channels * 2);
	wav__u16(file, 16); // bits per sample
	wav__str(file, "data");
	wav__u32(file, data_length);

}

/* emit a single sample (NOTE: 1 mono frame contains 1 sample; 1 stereo frame
 * contains 2 samples) */
static inline void wav_sample(FILE* file, float sample)
{
	wav__i16(file, (int16_t)(fminf(1.0f, fmaxf(-1.0f, sample)) * 32767.0f));
}

static inline void wav_samples(FILE* file, float* samples, int n_samples)
{
	for (int i = 0; i < n_samples; i++) wav_sample(file, samples[i]);
}

#define WAV_H
#endif
