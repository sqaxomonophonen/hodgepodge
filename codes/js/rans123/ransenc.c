// cc -Wall ransenc.c -o ransenc

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rans_byte.h"

int main(int argc, char** argv)
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <scale bits> <input symbol list> <output file>\n", argv[0]);
		fprintf(stderr, "input symbol list must contain \"<start> <freq>\\n\" pairs in *reverse* order\n");
		exit(EXIT_FAILURE);
	}
	const int scale_bits = atoi(argv[1]);
	assert(1 <= scale_bits && scale_bits <= 16);
	FILE* in = fopen(argv[2],"r");
	if (in == NULL) {
		fprintf(stderr, "%s: could not open\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	const size_t buffer_size = 1<<24;
	uint8_t* buffer = malloc(buffer_size);
	uint8_t* buffer_end = buffer+buffer_size;
	uint8_t* ptr = buffer_end;

	RansState rans;
	RansEncInit(&rans);
	int start,freq;
	int n_symbols = 0;
	while (fscanf(in, "%d %d\n", &start, &freq) == 2) {
		printf("[start=%d] [freq=%d] d=%zd\n", start, freq, ptr-buffer_end);
		RansEncSymbol esym = {0};
		RansEncSymbolInit(&esym, start, freq, scale_bits);
		RansEncPutSymbol(&rans, &ptr, &esym);
		n_symbols++;
	}
	RansEncFlush(&rans, &ptr);
	const size_t encsize = buffer_end - ptr;
	printf("%d symbols, %zd bytes\n", n_symbols, encsize);
	fclose(in);

	FILE* out = fopen(argv[3], "wb");
	if (out == NULL) {
		fprintf(stderr, "%s: could not open for writing\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	assert(fwrite(ptr, encsize, 1, out) == 1);
	fclose(out);
	printf("wrote %s\n", argv[3]);

	return EXIT_SUCCESS;
}
