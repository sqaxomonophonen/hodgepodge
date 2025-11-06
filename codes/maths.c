// cc maths.c -o maths -lm && ./maths
#include <math.h>
#include <stdio.h>
#include <stdint.h>

union x32 {
	float f;
	uint32_t i;
};

#define F32_BIAS (0x7f)
#define F32_EXP_SHIFT (23)
#define F32_EXP_BITS (8)
#define F32_EXP_MASK (((1<<F32_EXP_BITS)-1) << F32_EXP_SHIFT)

static inline float bad1_exp2f(float x)
{
	const float xfl = floorf(x);
	const int exp = (int)xfl;
	union x32 xx0;
	xx0.i = (F32_BIAS+exp) << F32_EXP_SHIFT;
	float m = (x-xfl);
	m += 1.0f;
	return xx0.f * m;
}

static inline float bad2_exp2f(float x)
{
	const float xfl = floorf(x);
	const int exp = (int)xfl;
	union x32 xx0;
	xx0.i = (F32_BIAS+exp) << F32_EXP_SHIFT;
	const float ln2 = 0.6931471805599453f;
	float x0 = (x-xfl);
	float mm = 1.0f + ln2*x0 + (ln2*ln2)*(1.0f/2.0f)*x0*x0 + (ln2*ln2*ln2)*(1.0f/6.0f)*x0*x0*x0;
	return xx0.f * mm;
}

// F = lambda x,n: sum([(math.log(2)**(i+1)) * (1/math.factorial(i+1)) * (x**(i+1)) for i in range(n)])

static inline float bad3_exp2f(float x)
{
	const float xfl = floorf(x);
	union x32 xx0;
	xx0.i = (F32_BIAS+(int)xfl) << F32_EXP_SHIFT;
	const float ln2 = 0.6931471805599453f; // ln(2)
	const float x0 = (x-xfl);
	const float x0x0 = x0*x0;
	float mm =
		1.0f +
		( (ln2*x0
		+ (ln2*ln2)*(1.0f/2.0f)*x0x0
		+ (ln2*ln2*ln2)*(1.0f/6.0f)*x0x0*x0
		+ (ln2*ln2*ln2*ln2)*(1.0f/24.0f)*x0x0*x0x0
		+ (ln2*ln2*ln2*ln2*ln2)*(1.0f/120.0f)*x0x0*x0x0*x0
		) * 1.0001707480437783f);
	return xx0.f * mm;
}

static inline float bad1_log2f(float x)
{
	union x32 xx0;
	xx0.f = x;
	const float exp = (((xx0.i >> F32_EXP_SHIFT) & 0xff) - F32_BIAS);
	union x32 xx1 = xx0;
	xx1.i &= ~F32_EXP_MASK;
	xx1.i |= (F32_BIAS << F32_EXP_SHIFT);
	const float x12 = xx1.f; // [1;2]
	// now the problem has been reduced to return exp + log2(x12) which is
	// easier because x12 is between 1 and 2, making the output between 0 and
	// 1.
	// find taylor series for log2(x) around x=1
	// taylor series as function of x is:
	//   f(a) + (f'(a)/1!)*((x-a)**1) + (f''(a)/2!)*((x-a)**2) + (f'''(a)/3!)*((x-a)**3) + ...
	// a is "anchor point" 1
	// the derivative of log2(x) is 1/(ln(2)*x)   (ln(2) is the natural logarithm)
	const float ln_of_2 = 0.6931471805599453;
	const float fm = 1.0f/ln_of_2;
	// the next derivative is -1/(ln(2)*(x**2))
	const float fmm = -1.0f/ln_of_2;
	// the next derivative is 2/(ln(2)*(x**3))
	const float fmmm = 2.0f/ln_of_2;
	// the next derivative is -6/(ln(2)*(x**4))
	const float fmmmm = -6.0f/ln_of_2;
	// the next derivative is 24/(ln(2)*(x**5))
	const float fmmmmm = 24.0f/ln_of_2;

	const float xa = x12 - 1.0f;
	const float xaxa = xa*xa;

	float mm =
		  0.0f
		+ (fm)*xa
		+ (fmm/2.0)*xaxa
		+ (fmmm/6.0)*xaxa*xa
		+ (fmmmm/24.0)*xaxa*xaxa
		+ (fmmmmm/120.0)*xaxa*xaxa*xa;
	
	mm *= 0.8848688314687672f;

	// hmmm taylor series are really bad... oscillating?

	//return exp + mm;
	return exp + mm;
}

//http://www.machinedlearnings.com/2011/06/fast-approximate-logarithm-exponential.html
// hvis jeg da bare forstod det her bras...
static inline float bad2_log2f(float x)
{
	union { float f; uint32_t i; } vx = { x };
	union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | (0x7e << 23) };
	float y = vx.i;
	y *= 1.0 / (1 << 23);

	return
		y - 124.22544637f - 1.498030302f * mx.f - 1.72587999f / (0.3520887068f + mx.f);
}

static inline float bad3_log2f(float x)
{
	union x32 xx;
	xx.f = x;
	float y = xx.i * (1.0f / (float)(1<<23)) - 127.0f;

	#if 0
	xx.i &= ~F32_EXP_MASK;
	xx.i |= (F32_BIAS << F32_EXP_SHIFT);
	const float x12 = xx.f; // [1;2]
	#endif

	// what to do from here? the log2f(x)-bad3_log2f(x) error looks a bit like
	// a sine curve...

	return y;
}


int main(int argc, char** argv)
{

	for (int i=0; i<32; ++i) {
		const float x = (float)i * 0.25f;
		const float y0 = exp2f(x);
		const float y1 = bad1_exp2f(x);
		const float y2 = bad2_exp2f(x);
		const float y3 = bad3_exp2f(x);
		printf("exp2f(%f) = %f = %f = %f = %f\n", x, y0, y1, y2, y3);
	}

	printf("=====\n");

	for (int i=1; i<=1024; i+=1) {
		const float x = i;
		const float y0 = log2f(x);
		const float y1 = bad1_log2f(x);
		const float y2 = bad2_log2f(x);
		const float y3 = bad3_log2f(x);
		printf("log2f(%f) = %f = %f = %f = %f\n", x, y0, y1, y2, y3);
		#if 0
		const float err = fabs(y3-y0);
		for (float f=0; f<err; f+=1e-3) printf("*");
		printf("\n");
		#endif
		//printf("err=%f\n",err);
	}

	return 0;
}
