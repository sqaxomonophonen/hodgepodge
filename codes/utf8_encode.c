char* utf8_encode(char* p, unsigned codepoint)
{
	if (codepoint < (1<<7)) {
		// 1-byte/7-bit ascii: 0b0xxxxxxx
		p[0] = (char)codepoint;
		p += 1;
	} else if (codepoint < (1<<11)) {
		// 2-byte/11-bit utf8 codepoint: 0b110xxxxx 0b10xxxxxx
		p[0] = (char)(0xc0 | (char)((codepoint >> 6) & 0x1f));
		p[1] = (char)(0x80 | (char)(codepoint & 0x3f));
		p += 2;
	} else if (codepoint < (1<<16)) {
		// 3-byte/16-bit utf8 codepoint: 0b1110xxxx 0b10xxxxxx 0b10xxxxxx
		p[0] = (char)(0xe0 | (char)((codepoint >> 12) & 0x0f));
		p[1] = (char)(0x80 | (char)((codepoint >> 6) & 0x3f));
		p[2] = (char)(0x80 | (char)(codepoint & 0x3f));
		p += 3;
	} else if (codepoint < (1<<21)) {
		// 4-byte/21-bit utf8 codepoint: 0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx
		p[0] = (char)(0xf0 | (char)((codepoint >> 18) & 0x07));
		p[1] = (char)(0x80 | (char)((codepoint >> 12) & 0x3f));
		p[2] = (char)(0x80 | (char)((codepoint >> 6) & 0x3f));
		p[3] = (char)(0x80 | (char)(codepoint & 0x3f));
		p += 4;
	}
	return p;
}

#ifdef UNIT_TEST

// cc -DUNIT_TEST utf8_encode.c -o utf8_encode && ./utf8_encode

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char** argv)
{
	char buf[256];
	char* p = buf;
	p = utf8_encode(p, 230);
	p = utf8_encode(p, 248);
	p = utf8_encode(p, 229);
	p = utf8_encode(p, '!');
	*p = 0;
	assert((p-buf) == strlen(buf));
	const char* expected =  "æøå!";
	if (strcmp(buf, expected) != 0) {
		fprintf(stderr, "expected \"%s\" got \"%s\"\n", expected, buf);
		return EXIT_FAILURE;
	} else {
		printf("OK!\n");
		return EXIT_SUCCESS;
	}
}
#endif
