/* based on http://achurch.org/SIL/ which is GPLv3, so this probably is too.
 * there's not much of value in here, compared to there, except it's self
 * contained and compiles easily  */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

const char* ROOT = "UNPKG";

struct entry {
	uint32_t hash;
	uint32_t nameofs_flags;
	uint32_t offset;
	uint32_t datalen;
	uint32_t filesize;
};

static void read_chars(char* dst, FILE* file, int n)
{
	assert(fread(dst, n, 1, file) == 1 && "could not read chars");
}

static uint16_t read_u16_le(FILE* file)
{
	uint16_t v;
	assert(fread(&v, sizeof v, 1, file) == 1 && "could not read u16");
	return v;
}

static uint16_t read_u16_be(FILE* file)
{
	uint16_t v = read_u16_le(file);
	v =
		  (((v >> 8) & 0xff) << 0)
		| (((v >> 0) & 0xff) << 8);
	return v;
}

static uint32_t read_u32_le(FILE* file)
{
	uint32_t v;
	assert(fread(&v, sizeof v, 1, file) == 1 && "could not read u32");
	return v;
}

static uint32_t read_u32_be(FILE* file)
{
	uint32_t v = read_u32_le(file);
	v =
		  (((v >> 24) & 0xff) << 0)
		| (((v >> 16) & 0xff) << 8)
		| (((v >> 8)  & 0xff) << 16)
		| (((v >> 0)  & 0xff) << 24);
	return v;
}

static void write_archive_file(const char* dst, FILE* srcfile, size_t offset, size_t size)
{
	char cwd[65536];
	assert(getcwd(cwd, sizeof(cwd)) == cwd);

	size_t n = strlen(dst);
	char buf[65536];
	assert(n < (sizeof(buf)-1));
	strcpy(buf, dst);

	printf("[%s]...\n", buf);

	char* p = buf;
	for (;;) {
		char* p0 = p;
		char* p1 = p0;
		while ((*p1 != '/') && (*p1 != 0)) p1++;
		int is_leaf = (*p1 == 0);
		int is_dir = !is_leaf;
		*p1 = 0;

		if (is_dir) {
			mkdir(p0, 0755);
			assert(chdir(p0) == 0);
		} else if (is_leaf) {
			fseek(srcfile, offset, SEEK_SET);
			assert(ftell(srcfile) == offset);
			void* buffer = malloc(size);
			assert(fread(buffer, size, 1, srcfile) == 1);
			FILE* w = fopen(p0, "wb");
			assert(fwrite(buffer, size, 1, w) == 1);
			fclose(w);
			free(buffer);
			break;
		} else {
			assert(!"UNREACHABLE");
		}

		p = p1+1;
	}

	assert(chdir(cwd) == 0);
}

int main(int argc, char** argv)
{
	const char* prg = argv[0];
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <path/to/ftl.dat>\n", prg);
		fprintf(stderr, "Throws archive into UNPKG, or complains if it already exists\n");
		exit(EXIT_FAILURE);
	}

	const char* filename = argv[1];

	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "%s: could not open\n", filename);
		exit(EXIT_FAILURE);
	}

	char magic[4];
	read_chars(magic, file, 4);
	if (strcmp(magic, "PKG\n") != 0) {
		fprintf(stderr, "%s: invalid magic\n", filename);
		exit(EXIT_FAILURE);
	}

	uint16_t header_size = read_u16_be(file);
	//printf("header size: %hu\n", header_size);
	assert((header_size == 16) && "expected header size of 16");

	uint16_t entry_size = read_u16_be(file);
	//printf("entry size: %hu\n", entry_size);
	assert((entry_size == 20) && "expected entry size of 20");
	assert(sizeof(struct entry) == entry_size);

	uint32_t entry_count = read_u32_be(file);
	//printf("entry count: %u\n", entry_count);

	uint32_t name_size = read_u32_be(file);
	//printf("name size: %u\n", name_size);

	struct entry* entries = calloc(entry_count, sizeof(*entries));
	for (int i = 0; i < entry_count; i++) {
		struct entry* e = &entries[i];
		e->hash = read_u32_be(file);
		e->nameofs_flags = read_u32_be(file);
		e->offset = read_u32_be(file);
		e->datalen = read_u32_be(file);
		e->filesize = read_u32_be(file);
	}

	char* namelut = calloc(name_size, 1);
	read_chars(namelut, file, name_size);

	if (mkdir(ROOT, 0755) == -1) {
		fprintf(stderr, "%s: %s\n", ROOT, strerror(errno));
		exit(EXIT_FAILURE);
	}
	assert(chdir(ROOT) == 0);

	for (int i = 0; i < entry_count; i++) {
		struct entry* e = &entries[i];
		int name_offset = e->nameofs_flags & 0xffffff;
		int flags = e->nameofs_flags >> 24;
		assert((flags == 0) && "flags not expected (implies unhandled deflate compression)");
		assert((e->datalen == e->filesize) && "expected same datalen and filesize");
		assert(name_offset < name_size);
		char* name = namelut + name_offset;
		//printf("#%.5d\tb:%.8x\t%s\n", i, e->filesize, name);
		write_archive_file(name, file, e->offset, e->filesize);
	}

	fclose(file);

	return EXIT_SUCCESS;
}
