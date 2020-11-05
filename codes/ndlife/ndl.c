#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include <SDL.h>

#define L_LOG2 (6)

#if 0
//interesting.. blue chaos, eventually filled with reddish patterns...
#define X1_LOG2 (3)
#define X2_LOG2 (2)
#define X3_LOG2 (1)
#define SURVIVE_LOWER (20)
#define SURVIVE_UPPER (55)
#define REPRODUCE_LOWER (44)
#define REPRODUCE_UPPER (50)
#define RAND_PCT (10)
#endif

#if 0
//cool patterns.. eventually dies out (black)
#define X1_LOG2 (2)
#define X2_LOG2 (2)
#define X3_LOG2 (2)
#define SURVIVE_LOWER (27)
#define SURVIVE_UPPER (59)
#define REPRODUCE_LOWER (42)
#define REPRODUCE_UPPER (52)
#define RAND_PCT (15)
#endif

#if 0
// wow.. very game-of-life like.. eventually dies out but only after more than
// an hour of iterations, with population going up and down. didn't see any
// periodic/cyclic structures more interesting than blinking dots though (and a
// fair number of stable structures), but some pretty symmetric "explosions"
// occasionally. also some seemingly stable structures blow up after a few
// iterations.
#define X1_LOG2 (2)
#define X2_LOG2 (2)
#define X3_LOG2 (2)
#define SURVIVE_LOWER (25)
#define SURVIVE_UPPER (60)
#define REPRODUCE_LOWER (42)
#define REPRODUCE_UPPER (51)
#define RAND_PCT (15)
#endif

#if 0
// also life-like.. but some all-white not-too-complex patterns expand
// indefinitely, filling space with new chaos, so it's probably a die hard rule
// set
#define X1_LOG2 (2)
#define X2_LOG2 (2)
#define X3_LOG2 (2)
#define SURVIVE_LOWER (28)
#define SURVIVE_UPPER (62)
#define REPRODUCE_LOWER (43)
#define REPRODUCE_UPPER (53)
#define RAND_PCT (25)
#endif

#if 1
#define X1_LOG2 (2)
#define X2_LOG2 (2)
#define X3_LOG2 (2)
#define SURVIVE_LOWER (36)
#define SURVIVE_UPPER (63)
#define REPRODUCE_LOWER (41)
#define REPRODUCE_UPPER (51)
#define RAND_PCT (15)
#endif

#define L (1<<L_LOG2)
#define X1 (1<<X1_LOG2)
#define X2 (1<<X2_LOG2)
#define X3 (1<<X3_LOG2)

#define N (L*L*X1*X2*X3)
#define NL (N>>6)

static inline int index_of(int x, int y, int d1, int d2, int d3)
{
	return
		(x << (L_LOG2 + X1_LOG2 + X2_LOG2 + X3_LOG2)) + 
		(y << (X1_LOG2 + X2_LOG2 + X3_LOG2)) + 
		(d1 << (X2_LOG2 + X3_LOG2)) + 
		(d2 << (X3_LOG2)) + 
		(d3);
}

static inline int lookup_wrap(uint64_t* plane, int x, int y, int d1, int d2, int d3)
{
	int index = index_of(x & (L-1), y & (L-1), d1 & (X1-1), d2 & (X2-1), d3 & (X3-1));
	int i64 = index >> 6;
	int ib = index & 63;
	return (plane[i64] & (1L << ib)) != 0;
}


static inline int lookup(uint64_t* plane, int x, int y, int d1, int d2, int d3)
{
	int index = index_of(x, y, d1, d2, d3);
	int i64 = index >> 6;
	int ib = index & 63;
	return (plane[i64] & (1L << ib)) != 0;
}

static void step(uint64_t* dst, uint64_t* src)
{
	int n_set = 0;
	int n_clear = 0;
	memset(dst, 0, N/8);
	for (int x = 0; x < L; x++) {
		for (int y = 0; y < L; y++) {
			for (int d1 = 0; d1 < X1; d1++) {
				for (int d2 = 0; d2 < X2; d2++) {
					for (int d3 = 0; d3 < X3; d3++) {

						int n = 0;
						for (int dx = -1; dx <= 1; dx++) {
							for (int dy = -1; dy <= 1; dy++) {
								for (int dd1 = -1; dd1 <= 1; dd1++) {
									for (int dd2 = -1; dd2 <= 1; dd2++) {
										for (int dd3 = -1; dd3 <= 1; dd3++) {
											n += lookup_wrap(
												src,
												x + dx,
												y + dy,
												d1 + dd1,
												d2 + dd2,
												d3 + dd3
											);
										}
									}
								}
							}
						}

						int live = lookup(src, x, y, d1, d2, d3);
						int set = 0;
						n -= live;
						if (live) {
							set = n > SURVIVE_LOWER && n < SURVIVE_UPPER;
						} else {
							set = n > REPRODUCE_LOWER && n < REPRODUCE_UPPER;
						}

						if (set) {
							int index = index_of(x, y, d1, d2, d3);
							int i64 = index >> 6;
							int ib = index & 63;
							dst[i64] |= (1L << ib);
							n_set++;
						} else {
							n_clear++;
						}
					}
				}
			}
		}
	}
}

static void swap_planes(uint64_t** a, uint64_t** b)
{
	uint64_t* tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

static void init_random(uint64_t* plane)
{
	srand(90);

	for (int i = 0; i < NL; i++) {
		uint64_t value = 0;
		for (int j = 0; j < 64; j++) {
			int d = rand();
			if (d > (((RAND_MAX)/100)*(100-RAND_PCT))) value |= (1L<<j);
		}
		plane[i] = value;
	}
}


int main(int argc, char** argv)
{
	uint64_t* plane0;
	uint64_t* plane1;
	uint8_t* bitmap;

	printf("NL=%d\n", NL);

	assert((plane0 = calloc(NL, sizeof *plane0)) != NULL);
	assert((plane1 = calloc(NL, sizeof *plane1)) != NULL);

	assert((bitmap = calloc(L*L*4, sizeof *bitmap)) != NULL);

	init_random(plane0);

	SDL_Window* window = SDL_CreateWindow(
		"NDLIFE",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		L, L,
		SDL_WINDOW_RESIZABLE);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, L, L);

	int exiting = 0;
	while (!exiting) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				exiting = 1;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				}
				break;
			}
		}

		int i = 0;
		for (int x = 0; x < L; x++) {
			for (int y = 0; y < L; y++) {
				int r = 0;
				for (int d1 = 0; d1 < X1; d1++) {
					for (int d2 = 0; d2 < X2; d2++) {
						for (int d3 = 0; d3 < X3; d3++) {
							if (lookup(plane0, x, y, d1, d2, d3)) {
								r += 255;
								break;
							}
						}
					}
				}
				r /= (X1*X2);

				int g = 0;
				for (int d1 = 0; d1 < X1; d1++) {
					for (int d3 = 0; d3 < X3; d3++) {
						for (int d2 = 0; d2 < X2; d2++) {
							if (lookup(plane0, x, y, d1, d2, d3)) {
								g += 255;
								break;
							}
						}
					}
				}
				g /= (X1*X3);

				int b = 0;
				for (int d2 = 0; d2 < X2; d2++) {
					for (int d3 = 0; d3 < X3; d3++) {
						for (int d1 = 0; d1 < X1; d1++) {
							if (lookup(plane0, x, y, d1, d2, d3)) {
								b += 255;
								break;
							}
						}
					}
				}
				b /= (X2*X3);

				bitmap[i++] = r;
				bitmap[i++] = g;
				bitmap[i++] = b;
				i++;
			}
		}

		SDL_UpdateTexture(texture, NULL, bitmap, L * 4);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		int r_width, r_height;
		SDL_GetRendererOutputSize(renderer, &r_width, &r_height);
		SDL_Rect dst_rect;
		float aspect_ratio = (float)L / (float)L;
		float r_aspect_ratio = (float)r_width / (float)r_height;
		if (r_aspect_ratio > aspect_ratio) {
			dst_rect.w = (int)roundf((float)r_height * aspect_ratio);
			dst_rect.h = r_height;
			dst_rect.x = (r_width - dst_rect.w) / 2;
			dst_rect.y = 0;
		} else {
			dst_rect.w = r_width;
			dst_rect.h = (int)roundf((float)r_width / aspect_ratio);
			dst_rect.x = 0;
			dst_rect.y = (r_height - dst_rect.h) / 2;
		}
		SDL_RenderCopy(renderer, texture, NULL, &dst_rect);

		SDL_RenderPresent(renderer);

		step(plane1, plane0);
		swap_planes(&plane0, &plane1);

	}


	SDL_DestroyRenderer(renderer);
	SDL_Quit();

	
	return EXIT_SUCCESS;
}
