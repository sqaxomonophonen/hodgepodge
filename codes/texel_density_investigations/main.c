#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"

#define ARRAY_LENGTH(xs) (sizeof(xs)/sizeof(xs[0]))

static inline float frand()
{
	return (float)rand() / (float)RAND_MAX;
}

static inline float lerp(float a, float b, float t)
{
	return a + t*(b-a);
}

static inline float frandr(float min, float max)
{
	return lerp(min, max, frand());
}

static inline float random_angle()
{
	const float R = GB_MATH_TAU * (1.0 / 4.0);
	return frandr(-R, R);
}

static inline gbVec4 gb3to4(gbVec3 v)
{
	return gb_vec4(v.x, v.y, v.z, 1.0f);
}

static inline gbVec3 gb4to3(gbVec4 v)
{
	const float s = 1.0f / v.w;
	return gb_vec3(v.x*s, v.y*s, v.z*s);
}

static void rot(gbMat4* m, gbVec3 axis, float angle)
{
	gbMat4 m_tx;
	gb_mat4_rotate(&m_tx, axis, angle);
	gb_mat4_mul(m, m, &m_tx);
}

int main(int argc, char** argv)
{
	gbMat4 m_perspective;

	const int width = 1920;
	const int height = 1080;

	gb_mat4_perspective(&m_perspective, gb_to_radians(90.0f), (float)width/(float)height, 1e-1f, 1e2f);

	const int n0 = 5000;
	const int n1 = 500;

	int n_in_front = 0;
	int n_behind = 0;

	for (int i0 = 0; i0 < n0; i0++) {
		gbMat4 m_view;
		gb_mat4_identity(&m_view);
		rot(&m_view, gb_vec3(1,0,0), random_angle());
		rot(&m_view, gb_vec3(0,1,0), random_angle());
		rot(&m_view, gb_vec3(0,0,1), random_angle());

		gbMat4 m_tx;
		gb_mat4_mul(&m_tx, &m_perspective, &m_view);

		for (int i1 = 0; i1 < n1; i1++) {
			const float S = 1.0f;
			const float E = 1e-3f; // epsilon
			gbVec4 v0 = gb3to4(gb_vec3(lerp(-S,S,frand()), lerp(-S,S,frand()), -S));
			gbVec4 v1 = gb3to4(gb_vec3(v0.x+E, v0.y,   v0.z));
			gbVec4 v2 = gb3to4(gb_vec3(v0.x,   v0.y+E, v0.z));

			gbVec4 v0tn, v1tn, v2tn;
			gb_mat4_mul_vec4(&v0tn, &m_view, v0);
			gb_mat4_mul_vec4(&v1tn, &m_view, v1);
			gb_mat4_mul_vec4(&v2tn, &m_view, v2);

			gbVec3 p0 = gb4to3(v0tn);

			if (p0.z >= 0.0f) n_in_front++;
			if (p0.z < 0.0f) n_behind++;

			//if (p0.z < 0.0f) continue;
			//printf("%.4f %.4f %.4f\n", p0.x, p0.y, p0.z);

			#if 0
			gbVec4 v0s, v1s, v2s;
			gb_mat4_mul_vec4(&v0s, &m_perspective, v0tn);
			gb_mat4_mul_vec4(&v1s, &m_perspective, v1tn);
			gb_mat4_mul_vec4(&v2s, &m_perspective, v2tn);
			#endif

			//printf("%.4f %.4f %.4f %.4f\n", v0s.x, v0s.y, v0s.z, v0s.z);
		}
	}

	printf("n_in_front=%d n_behind=%d\n", n_in_front, n_behind);

	return EXIT_SUCCESS;
}
