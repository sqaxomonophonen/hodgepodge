#include "wo.h"

#define PI (3.141592653589793)

int main(int argc, char** argv)
{
	int n = 440000;

	wo_begin(argv[0]);
	for (int i = 0; i < n; i++) {
		double x = (double)i * ((110.0*PI*2.0) / 48000.0);
		double t = (double)i * (1.0 / 48000.0);
		//double s = cos(x+t+sin(x-t*2+cos(x-t*2.1+sin(x+t*0.1+cos(x+t*5.5+sin(x))))));
		//double s = sin(x+sin(x+t*2+sin(x-t+sin(x+sin(x+sin(x))))));
		double s = sin(x+sin(x*(20-t*2)+t*2+sin(x-t+sin(x+sin(x*3-t*7+sin(x+t*20))))));
		wo_push((float)s, (float)s);
	}
	wo_end();

	return EXIT_SUCCESS;
}

