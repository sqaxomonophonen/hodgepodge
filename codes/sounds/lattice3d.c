#include "wo.h"


#define LATTICE_WIDTH  (51)
#define LATTICE_HEIGHT (7)
#define LATTICE_DEPTH  (3)
#define LATTICE_N (LATTICE_WIDTH * LATTICE_HEIGHT * LATTICE_DEPTH)
#define POS(x,y,z) ((x) + ((y)*LATTICE_WIDTH) + ((z)*LATTICE_WIDTH*LATTICE_HEIGHT))

struct lattice {
	int64_t pos[LATTICE_N];
	int64_t vel[LATTICE_N];
};

static struct lattice* lattice_new()
{
	struct lattice* l = calloc(1, sizeof *l);
	assert(l != NULL);
	return l;
}

static inline void check_pos(int x, int y, int z)
{
	assert(x >= 0 && x < LATTICE_WIDTH);
	assert(y >= 0 && y < LATTICE_HEIGHT);
	assert(z >= 0 && z < LATTICE_DEPTH);
}

static void lattice_impulse(struct lattice* l, int x, int y, int z, int64_t p)
{
	check_pos(x,y,z);
	l->pos[POS(x,y,z)] = p;
}

#define ONE_SHIFT (48)

static float lattice_sample(struct lattice* l, int x, int y, int z)
{
	check_pos(x,y,z);
	double sample = (double)l->pos[POS(x,y,z)] * ((double)1 / (double)(1L<<ONE_SHIFT));
	if (sample < -1.0f) sample = -1.0f;
	if (sample > 1.0f) sample = 1.0f;
	return (float)sample;
}

static void lattice_step(struct lattice* l)
{
	{
		for (int z = 1; z < (LATTICE_DEPTH-1); z++) {
			for (int y = 1; y < (LATTICE_HEIGHT-1); y++) {
				for (int x = 1; x < (LATTICE_WIDTH-1); x++) {
					int64_t c0 = l->pos[POS(x-1,y,z)];
					int64_t c1 = l->pos[POS(x+1,y,z)];
					int64_t c2 = l->pos[POS(x,y-1,z)];
					int64_t c3 = l->pos[POS(x,y+1,z)];
					int64_t c4 = l->pos[POS(x,y,z-1)];
					int64_t c5 = l->pos[POS(x,y,z+1)];
					int64_t avg = (c0+c1+c2+c3+c4+c5)/6;
					int64_t f = avg - l->pos[POS(x,y,z)];
					f = f/21;
					l->vel[POS(x,y,z)] += f;

				}
			}
		}
	}

	for (int i = 0; i < LATTICE_N; i++) {
		l->pos[i] += l->vel[i];
	}
}

int main(int argc, char** argv)
{
	struct lattice* lattice = lattice_new();
	lattice_impulse(lattice, LATTICE_WIDTH/3, LATTICE_HEIGHT/3, LATTICE_DEPTH/3, 4L<<ONE_SHIFT);

	int n = 800000;

	wo_begin(argv[0]);
	for (int i = 0; i < n; i++) {
		lattice_step(lattice);
		wo_push(
			lattice_sample(lattice, LATTICE_WIDTH/2, LATTICE_HEIGHT/2, LATTICE_DEPTH/2),
			lattice_sample(lattice, LATTICE_WIDTH/2+10, LATTICE_HEIGHT/2, LATTICE_DEPTH/2)
		);
	}
	wo_end();

	return EXIT_SUCCESS;
}
