#ifndef VN_HPP

#include <stdio.h>
#include <math.h>

#include <xmmintrin.h>
#ifdef VN_AVX
#include <immintrin.h>
#endif

template <int N> struct V1;

template <> struct V1<1> {
	float v;

	V1<1>(float v0) : v(v0) {}
	V1<1> operator+(const V1<1>& other) const { return V1<1>(this->v + other.v); }
	V1<1> operator-(const V1<1>& other) const { return V1<1>(this->v - other.v); }
	V1<1> operator*(const V1<1>& other) const { return V1<1>(this->v * other.v); }
	V1<1> operator/(const V1<1>& other) const { return V1<1>(this->v / other.v); }
};
inline V1<1> v1min(const V1<1>& a, const V1<1>& b) { return V1<1>(a.v<b.v?a.v:b.v); }
inline V1<1> v1max(const V1<1>& a, const V1<1>& b) { return V1<1>(a.v>b.v?a.v:b.v); }
inline V1<1> v1abs(const V1<1>& a) { return V1<1>(fabsf(a.v)); }
inline V1<1> v1rsqrt(const V1<1>& a) { return V1<1>(1.0f/sqrtf(a.v)); }
inline V1<1> v1sqrt(const V1<1>& a) { return V1<1>(sqrtf(a.v)); }
inline V1<1> v1neg(const V1<1>& a) { return V1<1>(-a.v); }
inline V1<1> v1recip(const V1<1>& a) { return V1<1>(1.0f/a.v); }
inline int v1print(V1<1> a) { return printf("%f", a.v); }
inline int v1fmt(char* dst, size_t size, V1<1> a) { return snprintf(dst, size, "%f", a.v); }


template <> struct V1<4> {
	__m128 v;
	V1<4>(float v0) : v(_mm_set_ps1(v0)) {}
	V1<4>(__m128 v0) : v(v0) {}
	V1<4> operator+(const V1<4>& other) const { return V1<4>(_mm_add_ps(this->v, other.v)); }
	V1<4> operator-(const V1<4>& other) const { return V1<4>(_mm_sub_ps(this->v, other.v)); }
	V1<4> operator*(const V1<4>& other) const { return V1<4>(_mm_mul_ps(this->v, other.v)); }
	V1<4> operator/(const V1<4>& other) const { return V1<4>(_mm_div_ps(this->v, other.v)); }
};
inline V1<4> v1min(const V1<4>& a, const V1<4>& b) { return V1<4>(_mm_min_ps(a.v, b.v)); }
inline V1<4> v1max(const V1<4>& a, const V1<4>& b) { return V1<4>(_mm_max_ps(a.v, b.v)); }
inline V1<4> v1abs(const V1<4>& a) {
	__m128i maski = _mm_set1_epi32(-1);
	__m128 mask = _mm_castsi128_ps(_mm_srli_epi32(maski, 1));
	return V1<4>(_mm_and_ps(mask, a.v));
}
inline V1<4> v1rsqrt(const V1<4>& a) { return V1<4>(_mm_rsqrt_ps(a.v)); }
inline V1<4> v1sqrt(const V1<4>& a) { return V1<4>(_mm_sqrt_ps(a.v)); }
inline V1<4> v1neg(const V1<4>& a) {
	__m128 mask = _mm_castsi128_ps(_mm_slli_epi32(_mm_set1_epi32(1), 31));
	return V1<4>(_mm_xor_ps(mask, a.v));
}
inline V1<4> v1recip(const V1<4>& a) { return V1<4>(_mm_rcp_ps(a.v)); }
inline int v1print(V1<4> a) {
	alignas(16) float x[4];
	_mm_store_ps(x, a.v);
	return printf("{%f,%f,%f,%f}",x[0],x[1],x[2],x[3]);
}

#ifdef VN_AVX
template <> struct V1<8> {
	__m256 v;
	V1<8>(float v0) : v(_mm256_set1_ps(v0)) {}
	V1<8>(__m256 v0) : v(v0) {}
	V1<8> operator+(const V1<8>& other) const { return V1<8>(_mm256_add_ps(this->v, other.v)); }
	V1<8> operator-(const V1<8>& other) const { return V1<8>(_mm256_sub_ps(this->v, other.v)); }
	V1<8> operator*(const V1<8>& other) const { return V1<8>(_mm256_mul_ps(this->v, other.v)); }
	V1<8> operator/(const V1<8>& other) const { return V1<8>(_mm256_div_ps(this->v, other.v)); }
};
inline V1<8> v1min(const V1<8>& a, const V1<8>& b) { return V1<8>(_mm256_min_ps(a.v, b.v)); }
inline V1<8> v1max(const V1<8>& a, const V1<8>& b) { return V1<8>(_mm256_max_ps(a.v, b.v)); }
inline V1<8> v1abs(const V1<8>& a) {
	__m256i maski = _mm256_set1_epi32(-1);
	__m256 mask = _mm256_castsi256_ps(_mm256_srli_epi32(maski, 1));
	return V1<8>(_mm256_and_ps(mask, a.v));
}
inline V1<8> v1rsqrt(const V1<8>& a) { return V1<8>(_mm256_rsqrt_ps(a.v)); }
inline V1<8> v1sqrt(const V1<8>& a) { return V1<8>(_mm256_sqrt_ps(a.v)); }
inline V1<8> v1neg(const V1<8>& a) {
	__m256 mask = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_set1_epi32(1), 31));
	return V1<8>(_mm256_xor_ps(mask, a.v));
}
inline V1<8> v1recip(const V1<8>& a) { return V1<8>(_mm256_rcp_ps(a.v)); }
inline int v1print(V1<8> a) {
	alignas(32) float x[8];
	_mm256_store_ps(x, a.v);
	return printf("{%f,%f,%f,%f,%f,%f,%f,%f}",x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7]);
}
#endif

template <int N>
inline V1<N> v1clamp(const V1<N>& x, float min, float max) {
	return v1min(V1<N>(max), v1max(V1<N>(min), x));
}

template <int N>
struct V2 {
	V1<N> x,y;
	V2(V1<N> _x, V1<N> _y) : x(_x), y(_y) {}
	V2 operator+(const V2& other) const { return V2(this->x+other.x, this->y+other.y); }
	V2 operator-(const V2& other) const { return V2(this->x-other.x, this->y-other.y); }
	V2 operator*(const V2& other) const { return V2(this->x*other.x, this->y*other.y); }
	V2 operator/(const V2& other) const { return V2(this->x/other.x, this->y/other.y); }
};

template <int N>
inline V1<N> v2dot(const V2<N>& a, const V2<N>& b) {
	return a.x*b.x + a.y*b.y;
}

template <int N>
inline V1<N> v2length(const V2<N>& a) {
	return v1sqrt(v2dot(a,a));
}

template <int N>
inline int v2print(V2<N> a) {
	int r = 0;
	r += printf("(");
	r += v1print(a.x);
	r += printf(",");
	r += v1print(a.y);
	r += printf(")");
	return r;
}

template <int N>
struct V4 {
	V1<N> x,y,z,w;
	V4(V1<N> _x, V1<N> _y, V1<N> _z, V1<N> _w) : x(_x), y(_y), z(_z), w(_z) {}
	V4 operator+(const V4& other) const { return V4(this->x+other.x, this->y+other.y); }
	V4 operator-(const V4& other) const { return V4(this->x-other.x, this->y-other.y); }
	V4 operator*(const V4& other) const { return V4(this->x*other.x, this->y*other.y); }
	V4 operator/(const V4& other) const { return V4(this->x/other.x, this->y/other.y); }
};

#define VN_HPP
#endif
