#ifndef WO_H

#ifndef WO_NORMALIZE
#define WO_NORMALIZE 1
#endif

#ifndef WO_SAMPLE_RATE
#define WO_SAMPLE_RATE (48000)
#endif

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "stretchy_buffer.h"

float* wo_samples;
char wo_filename[1<<12];

static inline void wo_begin(const char* prg)
{
	wo_samples = NULL;
	snprintf(wo_filename, sizeof wo_filename, "%s.wav", prg);
}

static inline void wo_push(float left, float right)
{
	int i = sb_count(wo_samples);
	if ((i % (2*WO_SAMPLE_RATE)) == 0) {
		printf("sample %d...\n", i/2);
	}
	sb_push(wo_samples, left);
	sb_push(wo_samples, right);
}

static inline void wo_end()
{
	int n = sb_count(wo_samples);

	#if WO_NORMALIZE
	{
		float max_sample = 0;
		for (int i = 0; i < n; i++) {
			float abs_sample = fabsf(wo_samples[i]);
			if (abs_sample > max_sample) max_sample = abs_sample;
		}
		float scale = 1.0f / max_sample;
		for (int i = 0; i < n; i++) {
			wo_samples[i] *= scale;
		}
	}
	#endif

	drwav_int16* samples16 = calloc(n, sizeof *samples16);
	assert(samples16 != NULL);
	drwav_f32_to_s16(samples16, wo_samples, n);

	drwav wav;
	drwav_data_format format;
	format.container = drwav_container_riff;
	format.format = DR_WAVE_FORMAT_PCM;
	format.channels = 2;
	format.sampleRate = WO_SAMPLE_RATE;
	format.bitsPerSample = 16;
	drwav_init_file_write(&wav, wo_filename, &format, NULL);
	drwav_write_pcm_frames(&wav, n/2, samples16);
	drwav_uninit(&wav);
}

#define WO_H
#endif
