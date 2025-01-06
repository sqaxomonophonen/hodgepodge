float bar(float);
__attribute__((visibility("default")))
float foo(int n, float x)
{
	float sum = 0.0f;
	for (int i=0; i<n; i++) sum += bar(x/(float)(1+i));
	return sum;
}
