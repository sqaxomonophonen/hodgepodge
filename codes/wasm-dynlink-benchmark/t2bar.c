__attribute__((visibility("default")))
float bar(int n, float x, float* yp)
{
	float sum = 0.0f;
	float y = *yp;
	for (int i=0; i<n; i++) sum += y*(x/(float)(1+i));
	return sum;
}
