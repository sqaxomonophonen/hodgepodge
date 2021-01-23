#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "stb_image_write.h"

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

uint32_t palette[] = {
	0,
	0x666666,
	0x999999,
	0x777777,
};

static inline uint32_t get_color(int index)
{
	assert(index >= 0);
	assert(index < ARRAY_LENGTH(palette));
	return palette[index];
}

struct b3d {
	int sx,sy,sz;
	uint8_t* data;
};

static void b3d_init(struct b3d* b, int sx, int sy, int sz)
{
	memset(b, 0, sizeof *b);
	b->sx = sx;
	b->sy = sy;
	b->sz = sz;
	int n = sx*sy*sz;
	assert((b->data = calloc(n, sizeof *b->data)) != NULL);
}

static inline int b3d_inside(struct b3d* b, int x, int y, int z)
{
	if (x < 0 || x >= b->sx) return 0;
	if (y < 0 || y >= b->sy) return 0;
	if (z < 0 || z >= b->sz) return 0;
	return 1;
}

static inline int b3d_index(struct b3d* b, int x, int y, int z)
{
	return x + y*b->sx + z*b->sx*b->sy;
}

static inline uint8_t b3d_fetch(struct b3d* b, int x, int y, int z)
{
	if (!b3d_inside(b, x, y, z)) return 0;
	return b->data[b3d_index(b, x, y, z)];
}

#define NORMAL_X (1<<0)
#define NORMAL_Y (1<<1)
#define NORMAL_Z (1<<2)
#define CULL     (1<<3)

static inline int b3d_shade(struct b3d* b, int x, int y, int z)
{
	if (b3d_fetch(b, x+1, y+1, z+1)) return CULL;

	return
		  (b3d_fetch(b, x+1, y, z) ? 0 : NORMAL_X)
		| (b3d_fetch(b, x, y+1, z) ? 0 : NORMAL_Y)
		| (b3d_fetch(b, x, y, z+1) ? 0 : NORMAL_Z);
}

static void b3d_plot(struct b3d* b, int x, int y, int z, uint8_t c)
{
	if (!b3d_inside(b, x, y, z)) return;
	b->data[b3d_index(b, x, y, z)] = c;
}

#define NCOMP (3)

static inline void plot(uint8_t* pixels, int width, int height, int x, int y, uint32_t color)
{
	if (x<0 || x>=width || y<0 || y>=height) return;
	uint8_t* pixel = &pixels[x*NCOMP + y*NCOMP*width];
	pixel[0] = (color>>16)&0xff;
	pixel[1] = (color>>8)&0xff;
	pixel[2] = (color)&0xff;
}

static inline int sat8add(int a, int b)
{
	int c = a + b;
	if (c < 0) return 0;
	if (c > 255) return 255;
	return c;
}

static inline uint32_t color_add(uint32_t a, uint32_t b)
{
	return sat8add(a&0xff, b&0xff)
		+ (sat8add((a>>8)&0xff, (b>>8)&0xff) << 8)
		+ (sat8add((a>>16)&0xff, (b>>16)&0xff) << 16);
}

static inline uint32_t color_shade(uint32_t color, int flags, int x)
{
	int nx = flags & NORMAL_X;
	int ny = flags & NORMAL_Y;
	int nz = flags & NORMAL_Z;

	const uint32_t colz = 0x886655;
	const uint32_t coly = 0x000820;
	const uint32_t colx = 0x443333;

	if (nz) {
		color = color_add(color, colz);
	} else if (nx && !ny) {
		color = color_add(color, colx);
	} else if (ny && !nx) {
		color = color_add(color, coly);
	} else if (nx && ny) {
		if (x == 0) {
			color = color_add(color, coly);
		} else {
			color = color_add(color, colx);
		}
	}

	return color;
}

static void b3d_render(struct b3d* b, uint8_t* pixels, int width, int height, int xoffset, int yoffset)
{
	int sx = b->sx;
	int sy = b->sy;
	int sz = b->sz;

	const int xy_scale = 2;
	const int z_scale = 2;

	for (int y = 0; y < sy; y++) {
		for (int z = 0; z < sz; z++) {
			for (int x = 0; x < sx; x++) {
				uint8_t v = b3d_fetch(b, x, y, z);
				if (v == 0) continue;

				int f = b3d_shade(b, x, y, z);
				if (f & CULL) continue;

				int dx = x*xy_scale - y*xy_scale;
				int dy = (x+y) - z*z_scale;

				for (int px = 0; px < xy_scale; px++) {
					for (int py = 0; py < z_scale; py++) {
						int tx = dx+px+xoffset;
						int ty = dy+py+yoffset;

						plot(pixels, width, height, tx, ty, color_shade(get_color(v), f, px));
					}
				}
			}
		}
	}
}

int main(int argc, char** argv)
{
	const int n = 32;
	struct b3d b;
	b3d_init(&b, n,n,n);

	for (int z = 0; z < n; z++) {
		for (int y = 0; y < n; y++) {
			for (int x = 0; x < n; x++) {
				int is_grid = (x&3)==0 || (y&3)==0 || (z&3)==0;
				int is_mid =
					((x>n/4)&&(x<n*3/4))
					|| ((y>n/4)&&(y<n*3/4))
					|| ((z>n/4)&&(z<n*3/4));
				b3d_plot(&b, x, y, z, is_grid ? 2 : (is_mid ? 3 : 1));
			}
		}
	}

	const int im_width = 1920/5;
	const int im_height = 1080/5;

	size_t im_size = im_width * im_height * NCOMP;
	uint8_t* pixels = malloc(im_size);
	assert(pixels != NULL);
	memset(pixels, 0, im_size);

	for (int y = 0; y < im_height; y++) {
		for (int x = 0; x < im_width; x++) {
			plot(pixels, im_width, im_height, x, y, 0x0000aa);
		}
	}

	b3d_render(&b, pixels, im_width, im_height, im_width/2, im_height/2);

	stbi_write_png("out.png", im_width, im_height, NCOMP, pixels, im_width*NCOMP);

	return EXIT_SUCCESS;
}
