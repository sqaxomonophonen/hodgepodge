#include "wo.h"

#define PI (3.141592653589793)

int main(int argc, char** argv)
{
	int n = 440000;

	wo_begin(argv[0]);
	for (int i = 0; i < n; i++) {
		double x = (double)i * ((110.0*PI*2.0) / 48000.0);
		double t = (double)i * (1.0 / 48000.0);
		double left = sin(x+cos(x*1.01+sin(x-t+sin(x*0.5-t+sin(x*0.91-t*7+cos(x*10+t*20))))));
		double right = sin(x+sin(x+sin(x*1.012-t+cos(x*0.5-t+sin((x-3)*0.92-t*7+sin(x*10+t*20))))));
		wo_push((float)left, (float)right);
	}
	wo_end();

	return EXIT_SUCCESS;
}

