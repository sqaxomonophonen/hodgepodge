#ifndef FAREY_H

// Farey sequence. Lookup by index is constant time. Lookup by integer fraction
// is a binary search, so logarithmic time.

// The Farey sequence of order 28 is interesting because its indices can be
// stored in a byte :-)

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef FAREY_TYPE
#define FAREY_TYPE unsigned char*
#endif

#ifndef FAREY_ASSERT
#define FAREY_ASSERT(x) assert(x)
#endif

#define FAREY_TYPE_SZ (sizeof(*((FAREY_TYPE)0)))

#define FAREY_TYPE_MAX (1 << (8 * FAREY_TYPE_SZ))

struct farey {
	int order;
	int n;
	FAREY_TYPE fractions;
};

static inline void farey_init(struct farey* farey, int order)
{
	FAREY_ASSERT(order > 0);
	FAREY_ASSERT((order <= FAREY_TYPE_MAX) && "(order-1) must fit in FAREY_TYPE");
	memset(farey, 0, sizeof *farey);
	farey->order = order;
	for (int pass = 0; pass < 2; pass++) {
		int n = 0;
		int a = 0;
		int b = 1;
		int c = 1;
		int d = order;
		while (c != 1 || d != 1) {
			int k = (order+b) / d;
			int a1 = c;
			int b1 = d;
			int c1 = k*c - a;
			int d1 = k*d - b;
			a = a1; b = b1; c = c1; d = d1;
			n++;
			if (pass == 1) {
				farey->fractions[n*2] = a;
				farey->fractions[n*2+1] = b-1;
			}
		}
		if (pass == 0) {
			n++;
			FAREY_ASSERT((farey->fractions = calloc(n, 2*FAREY_TYPE_SZ)) != NULL);
			farey->fractions[0] = 0;
			farey->fractions[1] = 0;
			farey->n = n;
		}
	}
}

static inline void farey_index_to_fraction(struct farey* farey, int index, int* numerator, int* denominator)
{
	FAREY_ASSERT(index >= 0 && index < farey->n);
	if (numerator != NULL) *numerator = farey->fractions[index*2];
	if (denominator != NULL) *denominator = farey->fractions[index*2+1]+1;
}

static inline int farey_index_of_fraction(struct farey* farey, int numerator, int denominator)
{
	FAREY_ASSERT(numerator >= 0);
	FAREY_ASSERT(denominator > 0);
	FAREY_ASSERT(numerator < denominator);
	int left = 0;
	int right = farey->n-1;
	while (left <= right) {
		int mid = (left+right) >> 1;
		int xnum, xdenom;
		farey_index_to_fraction(farey, mid, &xnum, &xdenom);
		int d = xnum * denominator - numerator * xdenom;
		if (d < 0) {
			left = mid+1;
		} else if (d > 0) {
			right = mid-1;
		} else {
			return mid;
		}
	}
}

#define FAREY_H
#endif

#ifdef TEST
#include <stdio.h>
int main(int argc, char** argv)
{
	for (int order = 2; order <= 28; order++) {
		printf("========\n");
		printf("ORDER %.2d\n", order);
		printf("========\n");
		struct farey f;
		farey_init(&f, order);
		for (int i = 0; i < f.n; i++) {
			int num, denom;
			farey_index_to_fraction(&f, i, &num, &denom);
			printf("  farey[%d] = %d:%d\n", i, num, denom);
			assert(farey_index_of_fraction(&f, num, denom) == i);
		}

		// assert we can find all fractions < 1, with
		// denominator<=order
		for (int denom = 1; denom <= order; denom++) {
			for (int num = 0; num < denom; num++) {
				int index = farey_index_of_fraction(&f, num, denom);
				int xnum, xdenom;
				farey_index_to_fraction(&f, index, &xnum, &xdenom);
				assert(num*xdenom == xnum*denom);
			}
		}

		int half_index = farey_index_of_fraction(&f, 1, 2);
		assert(farey_index_of_fraction(&f, 123, 246) == half_index);
	}
}
#endif
