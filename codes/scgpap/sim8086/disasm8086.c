#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define ARRAY_LENGTH(xs) (sizeof(xs)/sizeof(xs[0]))

static uint8_t* read_file(const char* path, size_t* out_size)
{
	FILE* f = fopen(path, "rb");
	assert(f != NULL);
	assert(fseek(f, 0, SEEK_END) == 0);
	size_t size = ftell(f);
	assert(fseek(f, 0, SEEK_SET) == 0);
	uint8_t* data = malloc(size);
	assert(fread(data, size, 1, f) == 1);
	if (out_size) *out_size = size;
	fclose(f);
	return data;
}

static inline const char* get_register_name(int index, int is_word)
{
	const char* reg_byte[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
	const char* reg_word[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
	assert(ARRAY_LENGTH(reg_byte) == ARRAY_LENGTH(reg_word));
	assert(0 <= index && index < ARRAY_LENGTH(reg_byte));
	if (is_word) {
		return reg_word[index];
	} else {
		return reg_byte[index];
	}
}

static void disasm(uint8_t* data, size_t size)
{
	uint8_t* p = data;
	uint8_t* p_end = p + size;

	while (p < p_end) {
		uint8_t op0 = *p;
		if ((op0 >> 2) == 0b100010) { // MOV
			const int W = op0 & 1;
			const int D = (op0 >> 1) & 1;

			const int is_word = W;
			const int is_source_in_reg = !D;
			const int is_destination_in_reg = !is_source_in_reg;

			uint8_t op1 = p[1];
			int RM  = (op1     ) & ((1<<3)-1);
			int REG = (op1 >> 3) & ((1<<3)-1);
			int MOD = (op1 >> 6) & ((1<<2)-1);

			if (MOD == 0b11) { // register mode
				int src, dst;
				if (is_source_in_reg) {
					src = REG;
					dst = RM;
				} else {
					src = RM;
					dst = REG;
					assert(!"UNTESTED");
				}
				printf("mov %s, %s ; [%2x %2x] W=%d D=%d R/M=%d REG=%d MOD=%d\n", 
					get_register_name(dst, is_word),
					get_register_name(src, is_word),
					op0, op1,
					W, D, RM, REG, MOD);
			} else {
				fprintf(stderr, "MOD=%d not implemented!\n", MOD);
				exit(EXIT_FAILURE);
			}

			p += 2;
		} else {
			fprintf(stderr, "Could not decode instruction byte [%2x]\n", op0);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <binary>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	size_t size;
	uint8_t* data = read_file(argv[1], &size);
	printf("; disassembly of %s, %zdb\n", argv[1], size);
	printf("bits 16\n");
	disasm(data, size);

	return EXIT_SUCCESS;
}
