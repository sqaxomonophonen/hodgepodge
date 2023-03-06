#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include <immintrin.h>

static inline double timespec_diff_seconds(struct timespec a, struct timespec b)
{
	const double d_seconds = (int64_t)a.tv_sec - (int64_t)b.tv_sec;
	const double d_nanoseconds = (int64_t)a.tv_nsec - (int64_t)b.tv_nsec;
	return d_seconds + 1e-9*d_nanoseconds;
}


static inline short m256_horizontal_sum_epi16(__m256i x)
{
	x = _mm256_hadd_epi16(x, x);
	x = _mm256_hadd_epi16(x, x);
	x = _mm256_hadd_epi16(x, x);
	__m256i y = _mm256_permute2x128_si256(x, x, 1 | (1 << 4));
	x = _mm256_add_epi16(x, y);
	return _mm256_extract_epi16(x, 0);
}

static inline void m256_epi16_sort2(__m256i* x, __m256i* y)
{
	__m256i min = _mm256_min_epi16(*x, *y);
	__m256i max = _mm256_max_epi16(*x, *y);
	*x = min;
	*y = max;
}

static inline __m256i calc_step(int size_log2, __m256i x0, __m256i x1)
{
	short x0s[16];
	short x1s[16];
	_mm256_storeu_si256((__m256i*)x0s, x0);
	_mm256_storeu_si256((__m256i*)x1s, x1);
	short r[16];
	const double m = 1.0 / (double)(1 << (16 - size_log2));
	for (int i = 0; i < 16; i++) {
		int x0i = x0s[i];
		int x1i = x1s[i];
		int d = x1i - x0i;
		double ss = 2047.0 / (1.0 + (double)d * m);
		if (ss > 2047.0) ss = 2047.0;
		r[i] = ss;
	}
	return _mm256_loadu_si256((__m256i*)r);
}

uint8_t* waveform2d(int* out_width, int* out_height, int16_t* samples, int n_samples, int downsample_log2, int size_log2)
{
	struct timespec t0;
	timespec_get(&t0, TIME_UTC);

	const int size = 1 << size_log2;
	const int n_slices = (n_samples + (1 << downsample_log2) - 1) >> downsample_log2;
	const int width = size;
	const int height = n_slices;

	const int n_pixels = width * height;
	uint8_t* pixels = calloc(n_pixels, sizeof *pixels);

	#if 0
	{ // AVX2 method 1 (for large downsample?) // XXX not good

		uint8_t* px = pixels;
		const int downsample = 1 << downsample_log2;
		const int half_size = size >> 1;
		for (int i0 = 0; i0 < (n_slices-1); i0++) {
			const int16_t* ps0 = &samples[i0 << downsample_log2];
			for (int i1 = 0; i1 < half_size; i1++) {
				const int16_t* ps = ps0;
				__m256i acc = _mm256_setzero_si256();
				const __m256i z16 = _mm256_setzero_si256();
				const short threshold = -32768 + (i1 << (15 - (size_log2-1)));
				const __m256i threshold16 = _mm256_set1_epi16(threshold);
				for (int i2 = 0; i2 < downsample; i2 += 16) {
					__m256i x16 = _mm256_loadu_si256((__m256i*)&ps[i2]);
					__m256i p16 = _mm256_cmpgt_epi16(threshold16, x16);
					acc = _mm256_sub_epi16(acc, p16);
				}
				int sum = m256_horizontal_sum_epi16(acc);
				sum >>= (downsample_log2-8);
				if (sum > 255) sum = 255;
				*(px++) = sum;
			}
			for (int i1 = 0; i1 < half_size; i1++) {
				const int16_t* ps = ps0;
				__m256i acc = _mm256_setzero_si256();
				const short threshold = (i1 << (15 - (size_log2-1)))-1;
				const __m256i threshold16 = _mm256_set1_epi16(threshold);
				for (int i2 = 0; i2 < downsample; i2 += 16) {
					__m256i x16 = _mm256_loadu_si256((__m256i*)&ps[i2]);
					__m256i p16 = _mm256_cmpgt_epi16(x16, threshold16);
					acc = _mm256_sub_epi16(acc, p16);
				}
				int sum = m256_horizontal_sum_epi16(acc);
				sum >>= (downsample_log2-8);
				if (sum > 255) sum = 255;
				*(px++) = sum;
			}
		}
	}
	#endif

	#if 0
	{ // AVX2 method 2 (better but dead end?)
		uint8_t* px = pixels;
		const int downsample = 1 << downsample_log2;
		const int half_size = size >> 1;

		//#define GTE hard to tell any difference
		#ifdef GTE
		const __m256i one16 = _mm256_set1_epi16(1);
		#endif

		for (int i0 = 0; i0 < (n_slices-1); i0++) {
			const int16_t* ps0 = &samples[i0 << downsample_log2];
			const int MULT = 10;
			for (int i1 = 0; i1 < half_size; i1++) {
				const int16_t* ps = ps0;
				__m256i acc = _mm256_setzero_si256();
				const __m256i z16 = _mm256_setzero_si256();
				const short y = -32768 + (i1 << (15 - (size_log2-1)));
				const __m256i y16 = _mm256_set1_epi16(y);
				for (int i2 = 0; i2 < downsample; i2 += 16) {
					__m256i y0_16 = _mm256_loadu_si256((__m256i*)&ps[i2]);
					__m256i y1_16 = _mm256_loadu_si256((__m256i*)&ps[i2+1]);

					__m256i p16 = _mm256_or_si256(
						_mm256_and_si256(
							#ifdef GTE
							// y1>=y>y0
							_mm256_cmpgt_epi16(_mm256_add_epi16(y1_16, one16), _mm256_sub_epi16(y16, one16)),
							#else
							// y1=y>y0
							_mm256_cmpgt_epi16(y1_16, y16),
							#endif
							_mm256_cmpgt_epi16(y16,   y0_16)
						),
						_mm256_and_si256(
							#ifdef GTE
							// y0>=y>y1
							_mm256_cmpgt_epi16(_mm256_add_epi16(y0_16, one16), _mm256_sub_epi16(y16, one16)),
							#else
							_mm256_cmpgt_epi16(y0_16, y16),
							#endif
							_mm256_cmpgt_epi16(y16,   y1_16)
						)
					);

					acc = _mm256_sub_epi16(acc, p16);
				}
				int sum = m256_horizontal_sum_epi16(acc);
				sum *= MULT;
				sum >>= (downsample_log2-8);
				if (sum > 255) sum = 255;
				*(px++) = sum;
			}
			for (int i1 = 0; i1 < half_size; i1++) {
				const int16_t* ps = ps0;
				__m256i acc = _mm256_setzero_si256();
				const short y = (i1 << (15 - (size_log2-1)))-1;
				const __m256i y16 = _mm256_set1_epi16(y);
				for (int i2 = 0; i2 < downsample; i2 += 16) {
					__m256i y0_16 = _mm256_loadu_si256((__m256i*)&ps[i2]);
					__m256i y1_16 = _mm256_loadu_si256((__m256i*)&ps[i2+1]);

					__m256i p16 = _mm256_or_si256(
						_mm256_and_si256(
							#ifdef GTE
							// y1>=y>y0
							_mm256_cmpgt_epi16(_mm256_add_epi16(y1_16, one16), y16),
							#else
							// y1>y>y0
							_mm256_cmpgt_epi16(y1_16, y16),
							#endif
							_mm256_cmpgt_epi16(y16,   y0_16)
						),
						_mm256_and_si256(
							#ifdef GTE
							_mm256_cmpgt_epi16(_mm256_add_epi16(y0_16, one16), y16),
							#else
							// y0>y>y1
							_mm256_cmpgt_epi16(y0_16, y16),
							#endif
							_mm256_cmpgt_epi16(y16,   y1_16)
						)
					);
					acc = _mm256_sub_epi16(acc, p16);
				}
				int sum = m256_horizontal_sum_epi16(acc);
				sum *= MULT;
				sum >>= (downsample_log2-8);
				if (sum > 255) sum = 255;
				*(px++) = sum;
			}
		}
	}
	#endif

	#if 1
	{ // AVX2 method 3
		uint8_t* px = pixels;
		const int downsample = 1 << downsample_log2;

		const short scan_step = 1 << (16 - size_log2);
		const __m256i scan_step_16 = _mm256_set1_epi16(scan_step);
		const __m256i scan0 = _mm256_set1_epi16(-32768);

		int column[1<<12];

		for (int i0 = 0; i0 < (n_slices-1); i0++) {
			const int16_t* ps = &samples[i0 << downsample_log2];
			const int sample_step = 2*16;
			assert(sample_step <= downsample);

			memset(column, 0, size * sizeof(column[0]));

			for (int i1 = 0; i1 < downsample; i1 += sample_step) {
				__m256i min0 = _mm256_loadu_si256((__m256i*)&ps[0*16+i1]);
				__m256i max0 = _mm256_loadu_si256((__m256i*)&ps[0*16+i1+1]);
				m256_epi16_sort2(&min0, &max0);
				__m256i step0 = calc_step(size_log2, min0, max0);

				__m256i min1 = _mm256_loadu_si256((__m256i*)&ps[1*16+i1]);
				__m256i max1 = _mm256_loadu_si256((__m256i*)&ps[1*16+i1+1]);
				m256_epi16_sort2(&min1, &max1);
				__m256i step1 = calc_step(size_log2, min1, max1);

				__m256i scan_position = scan0;

				for (int i2 = 0; i2 < size; i2++) {
					const __m256i next_scan_position = _mm256_add_epi16(scan_position, scan_step_16);

					const int s0 = m256_horizontal_sum_epi16(
						_mm256_and_si256(
							_mm256_and_si256(
								_mm256_cmpgt_epi16(max0, scan_position),
								_mm256_cmpgt_epi16(next_scan_position, min0)
							),
							step0
						)
					);

					const int s1 = m256_horizontal_sum_epi16(
						_mm256_and_si256(
							_mm256_and_si256(
								_mm256_cmpgt_epi16(max1, scan_position),
								_mm256_cmpgt_epi16(next_scan_position, min1)
							),
							step1
						)
					);

					column[i2] += (s0+s1);

					scan_position = next_scan_position;
				}
			}

			for (int i3 = 0; i3 < size; i3++) {
				const int I = 40;
				int v = (column[i3]*I) / (downsample*(2048/256));
				uint8_t p = v > 255 ? 255 : v;
				*(px++) = p;
			}
		}
	}
	#endif

	if (out_width) *out_width = width;
	if (out_height) *out_height = height;

	struct timespec t1;
	timespec_get(&t1, TIME_UTC);
	double dt = timespec_diff_seconds(t1, t0);
	printf("took %f seconds ; %f samples per second; %f pixels per second\n", dt, (double)n_samples/dt, (double)n_pixels/dt);

	return pixels;
}
