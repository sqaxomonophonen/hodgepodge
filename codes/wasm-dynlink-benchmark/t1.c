#if 1
static float bar(float x)
{
	return x*.9999999f;
}

__attribute__((visibility("default")))
float foo(int n, float x)
{
	float sum = 0.0f;
	for (int i=0; i<n; i++) sum += bar(x/(float)(1+i));
	return sum;
}
#else
__attribute__((visibility("default")))
float foo(int n, float x)
{
	float sum = 0.0f;
	for (int i=0; i<n; i++) sum += .9999999f*(x/(float)(1+i));
	return sum;
}
#endif
