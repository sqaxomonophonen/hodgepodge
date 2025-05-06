#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef NOW
#error "NOW is not defined"
#endif

#ifndef WIDTH
#error "WIDTH is not defined"
#endif

#ifndef HEIGHT
#error "HEIGHT is not defined"
#endif

static inline double l2f(long l, long max)
{
	return ((double)l / (double)max) * 2.0 - 1.0;
}

int main(int argc, char** argv)
{
	const double now = NOW;
	const int width = WIDTH;
	const int height = HEIGHT;
	printf("now=%f dim=%d×%d\n", now, width, height);

	for (int y=0; y<height; ++y) {
		double fy = l2f(y, height);
		for (int x=0; x<width; ++x) {
			double fx = l2f(x, width);
			const double d = sqrt(fx*fx + fy*fy) * 1;
			int inside = fmod(now-d, 1.0) < 0.5;
			putchar(inside ? '#' : '.');
		}
		putchar('\n');
	}
	return EXIT_SUCCESS;
}
