// cc -Wall img2moi.c -o img2moi -lm
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char** argv)
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <path/to/image> <mass-in-grams> <radius-in-millimeters>\n", argv[0]);
		fprintf(stderr, "Calculates moment of inertia like so:\n");
		fprintf(stderr, " - Image is converted to grayscale, and each pixel has a mass from 0 (black) to 255 (white).\n");
		fprintf(stderr, " - The sum of all pixels corresponds to the <mass-in-grams> argument.\n");
		fprintf(stderr, " - Center of image is axis of rotation.\n");
		fprintf(stderr, " - <radius-in-millimeters> corresponds width/2 or height/2 whichever is shorter.\n");
		fprintf(stderr, " - Output is converted to kg*m^2\n");
		exit(EXIT_FAILURE);
	}

	int mass_in_grams = atoi(argv[2]);
	if (mass_in_grams <= 0) {
		fprintf(stderr, "bad mass\n");
		exit(EXIT_FAILURE);
	}

	int radius_in_millimeters = atoi(argv[3]);
	if (radius_in_millimeters <= 0) {
		fprintf(stderr, "bad radius\n");
		exit(EXIT_FAILURE);
	}

	int width, height;
	unsigned char* data = stbi_load(argv[1], &width, &height, NULL, 1);


	uint64_t mass_sum = 0;
	uint64_t I_sum = 0;
	unsigned char* p = data;
	const int wh = width/2;
	const int hh = height/2;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			const int dx = (x - wh);
			const int dy = (y - hh);
			const int r2 = dx*dx + dy*dy;
			const int mass = *(p++);
			mass_sum += mass;
			I_sum += mass * r2;
		}
	}

	const double length_conversion_ratio = (1e-3 * (double)radius_in_millimeters) / (double)((width < height ? width : height) / 2);
	const double mass_conversion_ratio = (1e-3 * (double)mass_in_grams) / (double)mass_sum;
	const double conversion_ratio = mass_conversion_ratio * length_conversion_ratio * length_conversion_ratio;
	const double I = (double)I_sum * conversion_ratio;
	printf("I = %f kg*m^2\n", I);

	return EXIT_SUCCESS;
}
