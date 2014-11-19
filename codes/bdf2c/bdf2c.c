#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int data_cmp(const void* va, const void* vb)
{
	const int* a = va;
	const int* b = vb;
	int d0 = a[0] - b[0];
	if (d0 != 0) {
		return d0;
	} else {
		return a[1] - b[1];
	}
}

int main(int argc, char** argv)
{
	if (argc < 5) {
		fprintf(stderr, "usage: %s <width> <height> <output> <bdf> [bdf...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int bitmap_width = atoi(argv[1]);
	int bitmap_height = atoi(argv[2]);
	int n_variants = argc - 4;

	if (bitmap_width < 1 || bitmap_height < 1) {
		fprintf(stderr, "invalid dimensions: %dx%d\n", bitmap_width, bitmap_height);
		exit(EXIT_FAILURE);
	}

	FILE* output = fopen(argv[3], "w");
	if (output == NULL) {
		perror(argv[3]);
		exit(EXIT_FAILURE);
	}

	size_t bitmap_sz = bitmap_width * bitmap_height;
	unsigned char* bitmap = calloc(bitmap_sz, 1);
	if (bitmap == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(EXIT_FAILURE);
	}

	char* first_bdf = argv[4];

	char* dot = first_bdf;
	while (*dot != '.' && *dot != 0) dot++;
	char basename[256];
	memcpy(basename, first_bdf, dot - first_bdf);
	basename[dot - first_bdf] = 0;
	dot = basename;
	while (*dot != 0) {
		int valid = (*dot >= 'a' && *dot <= 'z') || (*dot >= 'A' && *dot <= 'Z') || (*dot >= '0' && *dot <= '9');
		if (!valid) *dot = '_';
		dot++;
	}

	int* meta = calloc(1<<20, sizeof(int));
	char buf[1024];

	int x0 = 0;
	int y0 = 0;
	int line_height = 0;
	int n = 0;
	int full = 0;

	int variant;
	for (variant = 0; variant < n_variants; variant++) {
		FILE* bdf = fopen(argv[4 + variant], "r");
		if (bdf == NULL) {
			perror(argv[4 + variant]);
			exit(EXIT_FAILURE);
		}

		int dy = 0;
		int glyph_width = 0;
		int glyph_height = 0;
		int encoding = 0;

		int eof = 0;

		int in_bitmap = 0;

		while (!eof && !full) {
			fscanf(bdf, "%s", buf);

			if (!in_bitmap) {
				if (strcmp("STARTCHAR", buf) == 0) {
					x0 += glyph_width;
					glyph_width = 0;
					glyph_height = 0;
				} else if (strcmp("ENCODING", buf) == 0) {
					fscanf(bdf, "%d", &encoding);
				} else if (strcmp("BBX", buf) == 0) {
					fscanf(bdf, "%d %d", &glyph_width, &glyph_height);
					if (x0 + glyph_width > bitmap_width) {
						x0 = 0;
						y0 += line_height;
						line_height = 0;
					}
					if (glyph_height > line_height) {
						line_height = glyph_height;
					}
					if (y0 + glyph_height > bitmap_height) {
						full = 1;
					}
				} else if (strcmp("BITMAP", buf) == 0) {
					meta[n*6+0] = variant;
					meta[n*6+1] = encoding;
					meta[n*6+2] = x0;
					meta[n*6+3] = y0;
					meta[n*6+4] = glyph_width;
					meta[n*6+5] = glyph_height;
					in_bitmap = 1;
					dy = 0;
				}
			} else {
				if (strcmp("ENDCHAR", buf) == 0) {
					in_bitmap = 0;
					n++;
				} else {
					unsigned int bit;
					sscanf(buf, "%x", &bit);
					int dx;
					for (dx = 0; dx < glyph_width; dx++) {
						int mask = 1 << (15-dx);
						if (bit & mask) {
							int x = x0 + dx;
							int y = y0 + dy;
							if (x < 0 || y < 0 || x >= bitmap_width || y >= bitmap_height) {
								fprintf(stderr, "BUG!!! (%d,%d) out of bounds in %dx%d bitmap", x, y, bitmap_width, bitmap_height);
								fclose(bdf);
								exit(EXIT_FAILURE);
							}
							bitmap[x + y * bitmap_width] = 255;
						}
					}
					dy++;
				}
			}

			while (1) {
				char c = fgetc(bdf);
				if (c == '\n') break;
				if (c == EOF) {
					eof = 1;
					break;
				}
			}
		}

		fclose(bdf);

		if (full) break;
	}

	qsort(meta, n, sizeof(int)*6, data_cmp);

	{
		fprintf(output, "int %s_bitmap_width = %d;\n", basename, bitmap_width);
		fprintf(output, "int %s_bitmap_height = %d;\n", basename, bitmap_height);
		fprintf(output, "int %s_n_variants = %d;\n", basename, n_variants);
		fprintf(output, "int %s_n_meta = %d;\n", basename, n);
		fprintf(output, "\n");
	}

	{
		fprintf(output, "int %s_meta[] = {\n", basename);
		int i;
		for (i = 0; i < n; i++) {
			int* m = &meta[i*6];
			fprintf(output, "%d,%d,%d,%d,%d,%d%s\n", m[0], m[1], m[2], m[3], m[4], m[5], i==n-1?"":",");
		}
		fprintf(output, "};\n\n");
	}
	{
		fprintf(output, "char %s_data[] = {\n", basename);
		int i;
		for (i = 0; i < bitmap_sz; i++) {
			fprintf(output, "%d%s", bitmap[i], i==bitmap_sz-1?"":",");
			if ((i&31)==31) fprintf(output, "\n");
		}
		fprintf(output, "};\n");
	}

	fclose(output);

	if (full) {
		fprintf(stderr, "warning: bitmap %dx%d overflowed after %d glyphs\n", bitmap_width, bitmap_height, n);
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}

