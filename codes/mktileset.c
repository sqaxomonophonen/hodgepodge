// cc -O3 -mavx -mavx2 mktileset.c -o mktileset -lm && ./mktileset out.pgm
// cc -O3 -mavx -mavx2 -S mktileset.c
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// resolution
#define TILE_SIZE_LOG2     (7)
#define ATLAS_COLUMNS_LOG2 (2)
#define ATLAS_ROWS_LOG2    (2)
#define SUPERSAMPLE        (16)

// style
#define LINE_WIDTH     (0.10f)
#define ROUND_RADIUS   (0.35f)
#define DOT_RADIUS     (0.25f)
#define PADDING        (0.05f)
#define SUBDOT_RADIUS  (0.50f)
#define SUBLINE_WIDTH  (0.50f)
#define SUBCENTER      (0.00f)


struct vec2 { float u,v; };
static inline struct vec2 vec2(float u, float v) { return (struct vec2) {.u=u,.v=v}; }
static inline float vec2_dot(struct vec2 a, struct vec2 b) { return a.u*b.u + a.v*b.v; }
static inline float sqr(float x) { return x*x; }

uint8_t* bitmap;

static int wire_line(struct vec2 p)
{
	return fabsf(p.u) < (LINE_WIDTH * 0.5f);
}

static int wire_corner(struct vec2 p)
{
	const float dd = (ROUND_RADIUS-LINE_WIDTH*0.5f);
	struct vec2 p1 = vec2(
		(p.u > dd ? dd : p.u) - dd,
		(p.v > dd ? dd : p.v) - dd
	);
	const float dsqr = vec2_dot(p1,p1);
	const float r0sqr = sqr(ROUND_RADIUS-LINE_WIDTH);
	const float r1sqr = sqr(ROUND_RADIUS);
	return (r0sqr < dsqr && dsqr < r1sqr);
}

static int wire_cross(struct vec2 p)
{
	int i = 0;
	i |= fabsf(p.u) < (LINE_WIDTH * 0.5f);
	i |= fabsf(p.v) < (LINE_WIDTH * 0.5f);
	i |= vec2_dot(p,p) < sqr(DOT_RADIUS);
	return i;
}

static int wire_t(struct vec2 p)
{
	int i = 0;
	i |= fabsf(p.u) < (LINE_WIDTH * 0.5f) && p.v > 0.0f;
	i |= fabsf(p.v) < (LINE_WIDTH * 0.5f);
	i |= vec2_dot(p,p) < sqr(DOT_RADIUS);
	return i;
}


static int sub_entry0(struct vec2 p)
{
	p.v += SUBCENTER;
	int i = 0;
	i |= fabsf(p.u) < (LINE_WIDTH * 0.5f) && p.v > 0.0f;
	i |= vec2_dot(p,p) < sqr(DOT_RADIUS);
	return i;
}

static int sub_pattern(struct vec2 p)
{
	p.u += 1.0f;
	p.v += 1.0f;
	const float m0 = 1.0f / 8.0f;
	const float m1 = m0 * 0.66f;
	return fmodf(vec2_dot(p, vec2(1.0f,1.0f)), m0) < m1;
}

static int sub_entry1(struct vec2 p)
{
	p.v += SUBCENTER;

	int i0 = 0, i1 = 0;
	i0 |= vec2_dot(p,p) < sqr(SUBDOT_RADIUS);
	i0 |= fabsf(p.u) < ((SUBLINE_WIDTH) * 0.5f) && p.v < 0.0f;

	i1 |= fabsf(p.u) < (LINE_WIDTH * 0.5f + PADDING) && p.v > 0.0f;
	i1 |= vec2_dot(p,p) < sqr(DOT_RADIUS + PADDING);

	return i0 && !i1 && sub_pattern(p);
}

static int sub_wire(struct vec2 p)
{
	return fabsf(p.u) < ((SUBLINE_WIDTH) * 0.5f) && sub_pattern(p);
}

__attribute__((always_inline))
static inline void render_tile(int column, int row, int(*sample)(struct vec2))
{
	const int sz = 1 << TILE_SIZE_LOG2;
	uint8_t* p = bitmap + (row << (2*TILE_SIZE_LOG2+ATLAS_COLUMNS_LOG2)) + (column << TILE_SIZE_LOG2);
	const int stride = (1 << (TILE_SIZE_LOG2+ATLAS_COLUMNS_LOG2)) - (1 << TILE_SIZE_LOG2);

	const float c0 = -1.0f; // XXX probably off by one unit? half?
	const float d0 = 2.0f / (float)sz;
	const float d1 = d0 * (1.0f / (float)SUPERSAMPLE);
	const float nscale = 1.0f / (SUPERSAMPLE*SUPERSAMPLE);

	float v0 = c0;
	for (int y = 0; y < sz; y++) {
		float u0 = c0;
		for (int x = 0; x < sz; x++) {
			int n = 0;
			float v1 = v0;
			for (int sy = 0; sy < SUPERSAMPLE; sy++) {
				float u1 = u0;
				for (int sx = 0; sx < SUPERSAMPLE; sx++) {
					n += sample(vec2(u1,v1));
					u1 += d1;
				}
				v1 += d1;
			}
			u0 += d0;
			*(p++) = (float)(255*n) * nscale;
		}
		v0 += d0;
		p += stride;
	}
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <out.pgm>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	FILE* out = fopen(argv[1], "w");
	const int atlas_width  = 1 << (TILE_SIZE_LOG2+ATLAS_COLUMNS_LOG2);
	const int atlas_height = 1 << (TILE_SIZE_LOG2+ATLAS_ROWS_LOG2);
	const int n_pixels = atlas_width*atlas_height;
	bitmap = calloc(atlas_width*atlas_height, 1);

	// tiles
	render_tile(0, 0, wire_cross);
	render_tile(1, 0, wire_line);
	render_tile(2, 0, wire_corner);
	render_tile(3, 0, wire_t);

	render_tile(0, 1, sub_entry0);
	render_tile(1, 1, sub_entry1);
	render_tile(2, 1, sub_wire);

	fprintf(out, "P2\n%d %d\n255\n", atlas_width, atlas_height);
	const int chunk = 8;
	uint8_t* p = bitmap;
	for (int i = 0; i < n_pixels; i += chunk) {
		fprintf(out, "%d %d %d %d %d %d %d %d\n",
			(int)p[0],
			(int)p[1],
			(int)p[2],
			(int)p[3],
			(int)p[4],
			(int)p[5],
			(int)p[6],
			(int)p[7]);
		p += chunk;
	}

	fclose(out);
	return EXIT_SUCCESS;
}
