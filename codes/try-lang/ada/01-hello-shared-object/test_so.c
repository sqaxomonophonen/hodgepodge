// cc -Wall test_so.c -o test_so && ./test_so
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>

void* must_dlsym(void* dh, const char* sym)
{
	void* p = dlsym(dh, sym);
	if (p == NULL) {
		fprintf(stderr, "%s: symbol not found\n", sym);
		exit(1);
	}
	return p;
}

int main(int argc, char** argv)
{
	const char* name = "./hello.so";
	void* dh = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
	if (dh == NULL) {
		fprintf(stderr, "%s: %s\n", name, dlerror());
		exit(1);
	}

	// NOTE: try removing adainit() to witness hello() fail with a "file
	// not open" error.
	void (*adainit)(void) = must_dlsym(dh,  "adainit");
	adainit();

	void (*hello_world)(void) = must_dlsym(dh,  "hello_world");
	hello_world();

	return 0;
}
