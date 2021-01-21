#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "stb_image_write.h"

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

uint32_t palette[] = {
	0,
	0xaaaaaa,
	0xffffff,
	0xffaaaa,
	0x0000aa,
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

static void b3d_plot(struct b3d* b, int x, int y, int z, uint8_t c)
{
	if (!b3d_inside(b, x, y, z)) return;
	b->data[b3d_index(b, x, y, z)] = c;
}

#define NCOMP (3)

static inline void plot(uint8_t* pixels, int width, int height, int x, int y, int color_index)
{
	if (x<0 || x>=width || y<0 || y>=height) return;
	uint32_t c = get_color(color_index);
	uint8_t* pixel = &pixels[x*NCOMP + y*NCOMP*width];
	pixel[0] = (c>>16)&0xff;
	pixel[1] = (c>>8)&0xff;
	pixel[2] = (c)&0xff;
}

static void b3d_render(struct b3d* b, uint8_t* pixels, int width, int height, int xoffset, int yoffset)
{
	int sx = b->sx;
	int sy = b->sy;
	int sz = b->sz;

	const int xy_scale = 2;
	const int z_scale = 3;

	for (int y = 0; y < sy; y++) {
		for (int z = 0; z < sz; z++) {
			for (int x = 0; x < sx; x++) {
				uint8_t v = b3d_fetch(b, x, y, z);
				if (v == 0) continue;

				int dx = x*xy_scale - y*xy_scale;
				int dy = (x+y) - z*z_scale;

				for (int px = 0; px < xy_scale; px++) {
					for (int py = 0; py < z_scale; py++) {
						int tx = dx+px+xoffset;
						int ty = dy+py+yoffset;
						plot(pixels, width, height, tx, ty, v);
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
			plot(pixels, im_width, im_height, x, y, 4);
		}
	}

	b3d_render(&b, pixels, im_width, im_height, im_width/2, im_height/2);

	stbi_write_png("out.png", im_width, im_height, NCOMP, pixels, im_width*NCOMP);

	return EXIT_SUCCESS;
}
