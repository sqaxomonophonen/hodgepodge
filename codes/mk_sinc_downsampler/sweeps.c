#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "wav.h"

#define PI (3.141592653589793)
#define PI2 (2.0*PI)


// AUTO-GENERATED with `./mk_sinc_downsampler 2 kaiser_bessel:att=60 50 c0`
static float msd_2x_downsample_table[] = {
	0.0001777809733177,
	0.0000000000000000,
	-0.0002996229974114,
	0.0000000000000000,
	0.0004616609057854,
	0.0000000000000000,
	-0.0006713520909274,
	0.0000000000000000,
	0.0009369017528564,
	0.0000000000000000,
	-0.0012673482158016,
	0.0000000000000000,
	0.0016726981112666,
	0.0000000000000000,
	-0.0021641359433232,
	0.0000000000000000,
	0.0027543435075362,
	0.0000000000000000,
	-0.0034579820189406,
	0.0000000000000000,
	0.0042924172148678,
	0.0000000000000000,
	-0.0052788140237902,
	0.0000000000000000,
	0.0064438062134075,
	0.0000000000000000,
	-0.0078220879745586,
	0.0000000000000000,
	0.0094605357690463,
	0.0000000000000000,
	-0.0114249798410278,
	0.0000000000000000,
	0.0138117957424577,
	0.0000000000000000,
	-0.0167687995636286,
	0.0000000000000000,
	0.0205354473300329,
	0.0000000000000000,
	-0.0255268840556564,
	0.0000000000000000,
	0.0325300240492834,
	0.0000000000000000,
	-0.0432356886504895,
	0.0000000000000000,
	0.0620477144912776,
	0.0000000000000000,
	-0.1051281573407923,
	0.0000000000000000,
	0.3179837371586912,
	0.5000000000000000,
	0.3179837371586912,
	0.0000000000000000,
	-0.1051281573407923,
	0.0000000000000000,
	0.0620477144912776,
	0.0000000000000000,
	-0.0432356886504895,
	0.0000000000000000,
	0.0325300240492834,
	0.0000000000000000,
	-0.0255268840556564,
	0.0000000000000000,
	0.0205354473300329,
	0.0000000000000000,
	-0.0167687995636286,
	0.0000000000000000,
	0.0138117957424577,
	0.0000000000000000,
	-0.0114249798410278,
	0.0000000000000000,
	0.0094605357690463,
	0.0000000000000000,
	-0.0078220879745586,
	0.0000000000000000,
	0.0064438062134075,
	0.0000000000000000,
	-0.0052788140237902,
	0.0000000000000000,
	0.0042924172148678,
	0.0000000000000000,
	-0.0034579820189406,
	0.0000000000000000,
	0.0027543435075362,
	0.0000000000000000,
	-0.0021641359433232,
	0.0000000000000000,
	0.0016726981112666,
	0.0000000000000000,
	-0.0012673482158016,
	0.0000000000000000,
	0.0009369017528564,
	0.0000000000000000,
	-0.0006713520909274,
	0.0000000000000000,
	0.0004616609057854,
	0.0000000000000000,
	-0.0002996229974114,
	0.0000000000000000,
	0.0001777809733177
};
#define MSD_2X_DOWNSAMPLE_FIR_SZ (99) // entire FIR table size
#define MSD_2X_DOWNSAMPLE_DELAY (49)  // delay/latency introduced by filter
#define MSD_2X_DOWNSAMPLE_OUTPUT_SZ(n,step) (((n)-MSD_2X_DOWNSAMPLE_FIR_SZ)/(step))
static void msd_2x_downsample(float* output, float* input, int n, int step)
{
	const int nm = n - MSD_2X_DOWNSAMPLE_FIR_SZ;
	float* inp = input;
	float* outp = output;
	for (int i = 0; i < nm; i+=step) {
		float acc = 0.0f;
		for (int j = 0; j < MSD_2X_DOWNSAMPLE_FIR_SZ; j++) {
			acc += inp[j] * msd_2x_downsample_table[j];
		}
		*(outp++) = acc;
		inp += step;
	}
	//assert((outp-output) == MSD_2X_DOWNSAMPLE_OUTPUT_SZ(n,step));
}


static void wav_mono(const char* filename, int sample_rate, float* samples, int n_samples)
{
	FILE* file = fopen(filename, "wb");
	assert(file != NULL);
	wav_header(file, sample_rate, 1, n_samples);
	wav_samples(file, samples, n_samples);
	fclose(file);
	printf("wrote %s\n", filename);
}

static void saw_sweep(float* output, int n_samples, int sample_rate, double fmin, double fmax, int n_harmonics)
{
	double phase = 0.0f;
	float* outp = output;
	double phase_scalar = (PI2 / (double)sample_rate);
	double nyquist = sample_rate/2;
	double taper = 0.99;
	double nyquist_taper_begin = nyquist * taper;
	for (int i = 0; i < n_samples; i++) {
		double t = (double)i / (double)n_samples;
		double f = fmin + (fmax - fmin) * t;
		double out = 0;
		for (int harmonic = 1; harmonic <= n_harmonics; harmonic++) {
			double fh = f*harmonic;
			double gain = 1.0;
			if (fh >= nyquist_taper_begin && fh < nyquist) {
				gain = 1.0 - (fh - nyquist_taper_begin) / (nyquist - nyquist_taper_begin);
			} else if (fh >= nyquist) {
				continue;
			}
			out += gain * (sin(phase * harmonic) / harmonic) * ((harmonic&1) ? -1 : 1);
		}
		out *= 0.5;
		*(outp++) = out;
		phase += phase_scalar * f;
		while (phase > PI2) phase -= PI2;
	}
}

static inline void dither(float* samples, int n, double amp)
{
	for (int i = 0; i < n; i++) {
		samples[i] += ((double)(rand()) / (double)RAND_MAX - 0.5) * amp;
	}
}

int main(int argc, char** argv)
{
	const int SR = 48000;

	const int n = SR*10;

	float* raw = calloc(n, sizeof *raw);
	assert(raw != NULL);
	saw_sweep(raw, n, SR, 1, SR/2, 8);
	//dither(raw, n, 1.0 / 8192.0);
	wav_mono("sweep_01_raw.wav", SR, raw, n);

	const int n_halfband = MSD_2X_DOWNSAMPLE_OUTPUT_SZ(n,1);
	float* halfband = calloc(n_halfband , sizeof *halfband);
	assert(halfband != NULL);
	msd_2x_downsample(halfband, raw, n, 1);
	wav_mono("sweep_02_halfband.wav", SR, halfband, n_halfband);

	const int n_halfband_decimated = MSD_2X_DOWNSAMPLE_OUTPUT_SZ(n,2);
	float* halfband_decimated = calloc(n_halfband_decimated, sizeof *halfband_decimated);
	assert(halfband_decimated != NULL);
	msd_2x_downsample(halfband_decimated, raw, n, 2);
	wav_mono("sweep_03_halfband_decimated.wav", SR/2, halfband_decimated, n_halfband_decimated);

	return EXIT_SUCCESS;
}
