// c++ -std=c++17 testvn.cpp -o testvn && ./testvn
// c++ -mavx -mavx2 -DVN_AVX -std=c++17 testvn.cpp -o testvn && ./testvn
#include <stdio.h>
#include "vn.hpp"

template <int N>
static void test()
{
	printf("\nTEST<%d>\n", N);

	{
		auto a = V1<N>(1.1f);
		auto b = V1<N>(2.2f);
		#define BINOP(OP)        \
		{                        \
			v1print(a);      \
			printf(#OP);     \
			v1print(b);      \
			printf("=");     \
			v1print(a OP b); \
			printf("\n");    \
		}
		BINOP(+)
		BINOP(-)
		BINOP(*)
		BINOP(/)
		#undef BINOP
	}

	{
		#define CLAMP(X,MIN,MAX)    \
		{ \
			auto x = V1<N>(X); \
			printf("clamp("); \
			v1print(x); \
			printf("," #MIN "," #MAX ")="); \
			v1print(v1clamp(x, (float)MIN, (float)MAX)); \
			printf("\n"); \
		}
		CLAMP(2.2f, 0, 2)
		CLAMP(1.1f, 0, 2)
		CLAMP(1.1f, 0, 0.9)
	}

	{
		#define FN1(FN,X) \
		{ \
			auto x = V1<N>(X); \
			auto y = v1##FN(x); \
			printf(#FN "("); \
			v1print(x); \
			printf(")="); \
			v1print(y); \
			printf("\n"); \
		}
		FN1(abs, -1.23456f)
		FN1(abs,  2.34543f)
		FN1(neg,  4.54545f)
		FN1(neg, -3.23232f)
		FN1(sqrt,  2.34543f)
		FN1(rsqrt,  2.34543f)
		FN1(recip,  3.00000f)
		#undef FN1
	}

	{
		V2<N> a(1.0f, 2.0f);
		V2<N> b(2.0f, -3.0f);
		//printf("a="); v2print(a); printf("\n");
		//printf("b="); v2print(b); printf("\n");
		#define BINOP(OP)        \
		{                        \
			v2print(a);      \
			printf(#OP);     \
			v2print(b);      \
			printf("=");     \
			v2print(a OP b); \
			printf("\n");    \
		}
		BINOP(+)
		BINOP(-)
		BINOP(*)
		BINOP(/)
		#undef BINOP


		{
			auto c = v2dot(a,b);
			printf("dot(");
			v2print(a);
			printf(",");
			v2print(b);
			printf(")=");
			v1print(c);
			printf("\n");
		}

		{
			auto c = v2length(a);
			printf("length(");
			v2print(a);
			printf(")=");
			v1print(c);
			printf("\n");
		}
	}
}

int main(int argc, char** argv)
{
	test<1>();
	test<4>();
	#ifdef VN_AVX
	test<8>();
	#endif
	return 0;
}
