#ifndef UTF8_DECODE_H
#define UTF8_DECODE_H

int utf8_decode(char** c0z, int* n)
{
	unsigned char** c0 = (unsigned char**)c0z;
	if (*n <= 0) return -1;
	unsigned char c = **c0;
	(*n)--;
	(*c0)++;
	if ((c & 0x80) == 0) return c & 0x7f;
	int mask = 192;
	int d;
	for (d = 1; d <= 3; d++) {
		int match = mask;
		mask = (mask >> 1) | 0x80;
		if ((c & mask) == match) {
			int codepoint = (c & ~mask) << (6*d);
			while (d > 0 && *n > 0) {
				c = **c0;
				if ((c & 192) != 128) return -1;
				(*c0)++;
				(*n)--;
				d--;
				codepoint += (c & 63) << (6*d);
			}
			return d == 0 ? codepoint : -1;
		}
	}
	return -1;
}

#ifdef UNITTEST

/* $ cc -DUNITTEST utf8_decode.c -o utf8_decode && ./utf8_decode */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int err = 0;

	{
		struct test {
			char* seq;
			int expected_codepoint;
		} tests[] = {
			{"a", 97},
			{"\xc2\xa2", 0xa2},
			{"\xe2\x82\xac", 0x20ac},
			{"\xf0\xa4\xad\xa2", 0x24b62},
		};
		int n = sizeof(tests) / sizeof(struct test);
		int i;
		for (i = 0; i < n; i++) {
			struct test* test = &tests[i];
			int len = strlen(test->seq);
			int n = len;
			char* c = test->seq;
			int codepoint = utf8_decode(&c, &n);
			if (n != 0 || c != (test->seq + len) || codepoint != test->expected_codepoint) {
				fprintf(
					stderr,
					"test failed seq '%s': expected n==0, codepoint=%d; GOT n==%d, codepoint=%d\n",
					test->seq,
					test->expected_codepoint,
					n,
					codepoint);
				err = 1;
			}
		}
	}

	return err;
}
#endif

#endif/*UTF8_DECODE_H*/
