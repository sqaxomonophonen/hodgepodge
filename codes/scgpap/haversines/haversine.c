// compile&run on "unixes":
// cc -O3 -std=c17 haversine.c -o haversine -lm && ./haversine

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdint.h>

static inline double radians(double degrees)
{
	return 0.017453292519943295 * degrees;
}

static inline double haversine_of_degrees(double x0, double y0, double x1, double y1, double r)
{
	const double dy = radians(y1-y0);
	const double dx = radians(x1-x0);
	const double y0r = radians(y0);
	const double y1r = radians(y1);
	const double syroot = sin(dy * 0.5);
	const double sxroot = sin(dx * 0.5);
	const double root_term = syroot*syroot + cos(y0r)*cos(y1r)*sxroot*sxroot;
	return 2.0 * r * asin(sqrt(root_term));
}

static inline double rnd(double max)
{
	return ((double)rand() / (double)RAND_MAX) * max;
}

static inline double timespec_diff(struct timespec a, struct timespec b)
{
	const double d_seconds = (int64_t)a.tv_sec - (int64_t)b.tv_sec;
	const double d_nanoseconds = (int64_t)a.tv_nsec - (int64_t)b.tv_nsec;
	return d_seconds + 1e-9*d_nanoseconds;
}

//#define WRITE_JSON
//#define WRITE_BINARY

int main()
{
	const double earth_radius_km = 6371;
	const int N = 10000000;

	#ifdef WRITE_JSON
	FILE* f_json = fopen("data_10000000_flex.json", "w");
	fprintf(f_json, "{\"pairs\":[\n"); // :[
	#endif

	#ifdef WRITE_BINARY
	FILE* f_binary = fopen("data_10000000_flex.f32", "wb");
	#endif

	double* arguments = calloc(4*N, sizeof *arguments);
	{
		double* p = arguments;
		for (int i = 0; i < N; i++) {
			p[0] = rnd(360)-180;
			p[1] = rnd(180)-90;
			p[2] = rnd(360)-180;
			p[3] = rnd(180)-90;
			const int is_last = (i == (N-1));
			#ifdef WRITE_JSON
			fprintf(f_json, "\t{\"x0\":%.6f, \"y0\":%.6f, \"x1\":%.6f, \"y1\":%.6f}%s\n", p[0], p[1], p[2], p[3], is_last?"":",");
			#endif
			#ifdef WRITE_BINARY
			float w32[4] = {p[0], p[1], p[2], p[3]};
			assert(fwrite(w32, 4, sizeof *w32, f_binary) == 4);
			#endif
			p += 4;
		}

	}
	#ifdef WRITE_JSON
	fprintf(f_json, "]}\n");
	fclose(f_json);
	#endif
	#ifdef WRITE_BINARY
	fclose(f_binary);
	#endif

	const int N_TRIALS = 10;
	double best_rate = 0.0;
	for (int i0 = 0; i0 < N_TRIALS; i0++) {
		struct timespec t0, t1;
		timespec_get(&t0, TIME_UTC);
		double sum = 0.0;
		{
			double* p = arguments;
			for (int i = 0; i < N; i++) {
				sum += haversine_of_degrees(p[0], p[1], p[2], p[3], earth_radius_km);
				p += 4;
			}
		}
		timespec_get(&t1, TIME_UTC);
		double dt = timespec_diff(t1, t0);
		double rate = (double)N/dt;
		printf("avg is %.2f. %.0f haversines/s\n", sum/(double)N, rate);
		if (rate > best_rate) best_rate = rate;
	}
	printf("best: %.0f haversines/s\n", best_rate);

	return EXIT_SUCCESS;
}
