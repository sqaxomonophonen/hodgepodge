// $ cc -Wall -O0 -g test0.c -o test0
// $ valgrind ./test0

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

int main(int argc, char** argv)
{
	const int image_width = 1024;
	const int image_height = 1024;
	uint8_t* image = malloc(image_width * image_height);

	for (int pass=0; pass<10; ++pass) {
		uint8_t* p = image;

		for (int y=0; y<image_height; ++y) {
			for (int x=0; x<image_width; ++x) {
				*(p++) = 128;
			}
		}

		STBIR_RESIZE resize={0};

		for (int y=0; y<90; ++y) {

			const int src_w = 11 + (y%6);
			const int src_h = 10 + (y%8);
			const int dst_w = 2 + (y%9);
			const int dst_h = 2 + (y%7);

			stbir_resize_init(
				&resize,
				NULL, src_w, src_h, -1,
				NULL, dst_w, dst_h, -1,
				STBIR_1CHANNEL, STBIR_TYPE_UINT8);

			stbir_set_edgemodes(&resize, STBIR_EDGE_ZERO, STBIR_EDGE_ZERO);
			stbir_build_samplers(&resize);

			for (int x=0; x<30; ++x) {
				const int src_x = x*32;
				const int src_y = y*11;
				const int dst_x = src_x+src_w;
				const int dst_y = src_y+src_h;
				stbir_set_buffer_ptrs(
					&resize,
					image + src_x + src_y*image_width, image_width,
					image + dst_x + dst_y*image_width, image_width);

				stbir_resize_extended(&resize);
			}
		}

		char filename[] = "testX.png";
		filename[4]='0'+pass;
		stbi_write_png(filename, image_width, image_height, 1, image, image_width);
	}

	return 0;
}
