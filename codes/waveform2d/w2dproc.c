#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <immintrin.h>

short mm256_horizontal_sum_epi16(__m256i x)
{
	x = _mm256_hadd_epi16(x, x);
	x = _mm256_hadd_epi16(x, x);
	x = _mm256_hadd_epi16(x, x);
	__m256i y = _mm256_permute2x128_si256(x, x, 1 | (1 << 4));
	x = _mm256_add_epi16(x, y);
	return _mm256_extract_epi16(x, 0);
}

uint8_t* waveform2d(int* out_width, int* out_height, int16_t* samples, int n_samples, int downsample_log2, int size_log2)
{
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
				int sum = mm256_horizontal_sum_epi16(acc);
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
				int sum = mm256_horizontal_sum_epi16(acc);
				sum >>= (downsample_log2-8);
				if (sum > 255) sum = 255;
				*(px++) = sum;
			}
		}
	}
	#endif

	{
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
							_mm256_cmpgt_epi16(_mm256_add_epi16(y1_16, one16), y16),
							#else
							// y1=y>y0
							_mm256_cmpgt_epi16(y1_16, y16),
							#endif
							_mm256_cmpgt_epi16(y16,   y0_16)
						),
						_mm256_and_si256(
							#ifdef GTE
							// y0>=y>y1
							_mm256_cmpgt_epi16(_mm256_add_epi16(y0_16, one16), y16),
							#else
							_mm256_cmpgt_epi16(y0_16, y16),
							#endif
							_mm256_cmpgt_epi16(y16,   y1_16)
						)
					);

					acc = _mm256_sub_epi16(acc, p16);
				}
				int sum = mm256_horizontal_sum_epi16(acc);
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
				int sum = mm256_horizontal_sum_epi16(acc);
				sum *= MULT;
				sum >>= (downsample_log2-8);
				if (sum > 255) sum = 255;
				*(px++) = sum;
			}
		}
	}



	if (out_width) *out_width = width;
	if (out_height) *out_height = height;

	return pixels;
}
