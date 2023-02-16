#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"

#define ARRAY_LENGTH(xs) (sizeof(xs)/sizeof(xs[0]))



/* your random number god */

struct rng {
	uint32_t z;
	uint32_t w;
};

static inline uint32_t rng_uint32(struct rng* rng)
{
	/*
	   The MWC generator concatenates two 16-bit multiply-
	   with-carry generators, x(n)=36969x(n-1)+carry,
	   y(n)=18000y(n-1)+carry mod 2^16, has period about
	   2^60 and seems to pass all tests of randomness. A
	   favorite stand-alone generator---faster than KISS,
	   which contains it.
	 */
	rng->z = 36969 * (rng->z & 65535) + (rng->z>>16);
	rng->w = 18000 * (rng->w & 65535) + (rng->w>>16);
	return (rng->z<<16) + rng->w;
}

static inline float rng_float(struct rng* rng)
{
	union {
		uint32_t i;
		float f;
	} magick;
	uint32_t r = rng_uint32(rng);
	magick.i = (r & 0x007fffff) | (127 << 23);
	return magick.f - 1;
}

static inline void rng_seed(struct rng* rng, uint32_t seed)
{
	rng->z = 654654 + seed;
	rng->w = 7653234 + seed * 69069;
}

struct rng global_rng;

static inline float frand()
{
	//return (float)rand() / (float)RAND_MAX;
	return rng_float(&global_rng);
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

static inline gbVec2 gb3to2screen(gbVec3 v, float width, float height)
{
	return gb_vec2(
		lerp(0, width, (v.x+1.0f)*0.5f),
		lerp(0, height, (v.y+1.0f)*0.5f)
	);
}

int main(int argc, char** argv)
{
	rng_seed(&global_rng, 1);

	gbMat4 m_perspective;

	const int width = 1920;
	const int height = 1080;

	const float z_near = 1e-1f;
	const float z_far = 1e2f;
	const float fov_x_degrees = 90.0f;
	const float fov_y_radians = atanf(tanf(gb_to_radians(fov_x_degrees)*0.5f) * ((float)height / (float)width))*2.0f;

	gb_mat4_perspective(&m_perspective, fov_y_radians, (float)width/(float)height, z_near, z_far);

	const int n0 = 15000;
	const int n1 = 5000;

	int n_in_view = 0;
	int n_not_in_view = 0;

	const int n_qsides = 7;

	for (int tt = 0; tt < n_qsides; tt++) {
		for (int ss = 0; ss < n_qsides; ss++) {
			int minmax_first = 1;
			float du_min = 0.0f;
			float du_max = 0.0f;
			float dv_min = 0.0f;
			float dv_max = 0.0f;

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
					const float E = 1e-5f; // epsilon

					const float u0 = (float)ss / (float)n_qsides;
					const float u1 = (float)(ss+1) / (float)n_qsides;
					const float v0 = (float)tt / (float)n_qsides;
					const float v1 = (float)(tt+1) / (float)n_qsides;
					gbVec4 vt0 = gb3to4(gb_vec3(lerp(-S,S,lerp(u0,u1,frand())), lerp(-S,S,lerp(v0,v1,frand())), -S));
					gbVec4 vt1 = gb3to4(gb_vec3(vt0.x+E, vt0.y,   vt0.z));
					gbVec4 vt2 = gb3to4(gb_vec3(vt0.x,   vt0.y+E, vt0.z));
					const float dtex = (float)width * (E * 0.5f);

					gbVec4 v0s, v1s, v2s;
					gb_mat4_mul_vec4(&v0s, &m_tx, vt0);
					gb_mat4_mul_vec4(&v1s, &m_tx, vt1);
					gb_mat4_mul_vec4(&v2s, &m_tx, vt2);

					gbVec3 p0 = gb4to3(v0s);
					gbVec3 p1 = gb4to3(v1s);
					gbVec3 p2 = gb4to3(v2s);

					const int in_view = (-1.0f < p0.x && p0.x < 1.0f) && (-1.0 < p0.y && p0.y < 1.0f) && (z_near < p0.z && p0.z < z_far);

					if (in_view) {
						n_in_view++;
						gbVec2 px0 = gb3to2screen(p0, width, height);
						gbVec2 px1 = gb3to2screen(p1, width, height);
						gbVec2 px2 = gb3to2screen(p2, width, height);

						gbVec2 px_du, px_dv;
						gb_vec2_sub(&px_du, px1, px0);
						gb_vec2_sub(&px_dv, px2, px0);

						float du = dtex / gb_vec2_mag(px_du);
						float dv = dtex / gb_vec2_mag(px_dv);

						if (minmax_first) {
							du_min = du_max = du;
							dv_min = dv_max = dv;
							minmax_first = 0;
						} else {
							if (du < du_min) du_min = du;
							if (du > du_max) du_max = du;
							if (dv < dv_min) dv_min = dv;
							if (dv > dv_max) dv_max = dv;
						}
					} else {
						n_not_in_view++;
					}
				}
			}
			printf("[%d,%d] du=[%.4f;%.4f] dv=[%.4f;%.4f]\n", ss, tt, du_min, du_max, dv_min, dv_max);
		}
	}

	printf("in view: %d\nnot in view: %d\n", n_in_view, n_not_in_view);

	return EXIT_SUCCESS;
}
