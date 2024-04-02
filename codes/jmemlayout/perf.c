#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

struct mystruct {
	float x;
	float y;
	int flags;
};

static inline float next_float(void)
{
	return (float)rand() / (float)RAND_MAX;
}

static inline int next_int(void)
{
	return rand();
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <n>\n", argv[0]);
		return 1;
	}
	const int n = atoi(argv[1]);
	struct mystruct* mem = calloc(n, sizeof mem[0]);
	for (int i = 0; i < n; i++) {
		struct mystruct* e = &mem[i];
		e->x = next_float();
		e->y = next_float();
		e->flags = next_int();
	}

	struct timespec t0;
	clock_gettime(CLOCK_MONOTONIC, &t0);

	float sum = 0.0f;
	for (int i = 0; i < n; i++) {
		struct mystruct* e = &mem[i];
		if ((e->flags & 256) == 0) continue;
		sum += e->x * e->y;
	}

	struct timespec t1;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	int64_t dtnanos = (t1.tv_sec - t0.tv_sec)*1000000000LL + (t1.tv_nsec - t0.tv_nsec);

	printf("sum=%f %f it/s\n", sum, (double)n / ((double)dtnanos * 1e-9));

	return 0;
}
