// make w2d && ./w2d music_cubase_opiate2312312%202.wav 12 8 foo.png

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "w2dproc.h"

int main(int argc, char** argv)
{
	if (argc != 5) {
		fprintf(stderr, "usage: %s <in.wav> <downsample log2> <size log2> <out.png>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int downsample_log2 = atoi(argv[2]);
	if (downsample_log2 <= 0) {
		fprintf(stderr, "invalid downsample log2 value\n");
		exit(EXIT_FAILURE);
	}

	int size_log2 = atoi(argv[3]);
	if (size_log2 <= 0) {
		fprintf(stderr, "invalid size log2 value\n");
		exit(EXIT_FAILURE);
	}

	unsigned int n_channels, sample_rate;
	drwav_uint64 n_frames;
	drwav_int16* frames = drwav_open_file_and_read_pcm_frames_s16(argv[1], &n_channels, &sample_rate, &n_frames, NULL);

	printf("channels:    %d\n", n_channels);
	printf("frames:      %lld\n", n_frames);
	printf("sample rate: %d\n", sample_rate);
	printf("duration:    %.1fs / %.2fm\n", (float)n_frames / (float)sample_rate, ((float)n_frames / (float)sample_rate) / 60.0f);

	drwav_int16* samples = calloc(n_frames, sizeof *samples);
	for (int i = 0; i < n_frames; i++) samples[i] = frames[i*2];
	#if 0
	for (int i = 0; i < n_frames; i++) samples[i] = sin((double)i * 0.0001) * 20000;
	//for (int i = 0; i < n_frames; i++) samples[i] = sin((double)i * 0.00001) * 30000;
	#endif

	int width, height;
	uint8_t* pixels = waveform2d(&width, &height, samples, n_frames, downsample_log2, size_log2);

	printf("dim %d×%d\n", width, height);

	stbi_write_png(argv[4], width, height, 1, pixels, width);

	return EXIT_SUCCESS;
}
