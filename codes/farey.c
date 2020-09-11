#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Farey sequence generator
// stolen from: https://en.wikipedia.org/wiki/Farey_sequence#Next_term

int count = 0;
static void emit(int a, int b)
{
	count++;
	printf("%d:%d ", a, b);
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <n>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int n = strtol(argv[1], NULL, 10);
	if (errno != 0) {
		fprintf(stderr, "argument must be an integer\n");
		exit(EXIT_FAILURE);
	}

	if (n < 1) {
		fprintf(stderr, "argument must be a positive integer\n");
		exit(EXIT_FAILURE);
	}

	int a = 0;
	int b = 1;
	int c = 1;
	int d = n;
	emit(a,b);
	while (c <= n) {
		int k = (n+b) / d;
		//(a, b, c, d) = (c, d, k * c - a, k * d - b)
		int a1 = c;
		int b1 = d;
		int c1 = k*c - a;
		int d1 = k*d - b;
		a = a1;
		b = b1;
		c = c1;
		d = d1;
		emit(a,b);
	}

	printf("\nn=%d\n", count);

	return EXIT_SUCCESS;
}
