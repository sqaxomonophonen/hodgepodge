float bar(int,float,float*);
static float M=.9999999f;
__attribute__((visibility("default")))
float foo(int n, float x)
{
	return bar(n,x,&M);
}

