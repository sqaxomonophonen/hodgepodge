#include "wo.h"

#define PI (3.141592653589793)

double expimp(double x, double k)
{
	const double h = k*x;
        return h*exp(1.0 - h);
}

int main(int argc, char** argv)
{
	int n = 440000;

	wo_begin(argv[0]);
	for (int i = 0; i < n; i++) {
		double x = (double)i * ((110.0*PI*2.0) / 48000.0);
		double t = (double)i * (1.0 / 48000.0);
		//double s = cos(x+t+sin(x-t*2+cos(x-t*2.1+sin(x+t*0.1+cos(x+t*5.5+sin(x))))));
		//double s = sin(x+sin(x+t*2+sin(x-t+sin(x+sin(x+sin(x))))));

		const double tmpo = 2.2;

		double tm = fmod(t*tmpo, 1.0);
		double tm4 = fmod(t*tmpo*4, 1.0);

		double im = expimp(tm,5);
		double im2 = expimp(tm,35);
		double im3 = expimp(tm4,35);

		double left = 0;
		double right = 0;

		double kick = (sin(im2*210)*im)*2;

		left  += kick;
		right += kick;

		double duck = 1-im;

		left  += duck * sin(x+sin(x+sin(x-t+sin(x*0.6-t+sin(im3*100*tm+x*0.91-t*7+sin(x*11+t*20))))));
		right += duck * sin(x+sin(x+sin(x-t+sin(x*0.5-t+sin(im3*102*tm+x*0.92-t*7+sin(x*10+t*19))))));

		wo_push((float)left, (float)right);
	}
	wo_end();

	return EXIT_SUCCESS;
}
