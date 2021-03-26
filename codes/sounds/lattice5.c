#include "wo.h"

//#define LATTICE_WIDTH (101)
//#define LATTICE_HEIGHT (64)

#define LATTICE_WIDTH (80)
#define LATTICE_HEIGHT (80)
#define LATTICE_N (LATTICE_WIDTH * LATTICE_HEIGHT)
#define POS(x,y) ((x)+(y)*LATTICE_WIDTH)

struct lattice {
	int64_t pos[LATTICE_N];
	int64_t vel[LATTICE_N];
	//int64_t div[LATTICE_N];
	int64_t damp[LATTICE_N];
};

static struct lattice* lattice_new()
{
	struct lattice* l = calloc(1, sizeof *l);
	assert(l != NULL);
	return l;
}

static void check_pos(int x, int y)
{
	assert(x >= 0 && x < LATTICE_WIDTH && y >= 0 && y < LATTICE_HEIGHT);
}

static void lattice_impulse(struct lattice* l, int x, int y, int64_t p)
{
	check_pos(x,y);
	l->pos[POS(x,y)] = p;
}

#define ONE_SHIFT (48)

static float lattice_sample(struct lattice* l, int x, int y)
{
	check_pos(x,y);
	double sample = (double)l->pos[POS(x,y)] * ((double)1 / (double)(1L<<ONE_SHIFT));
	if (sample < -1.0f) sample = -1.0f;
	if (sample > 1.0f) sample = 1.0f;
	return (float)sample;
}

static void lattice_step(struct lattice* l)
{
	for (int y = 1; y < (LATTICE_HEIGHT-1); y++) {
		for (int x = 1; x < (LATTICE_WIDTH-1); x++) {

			const int dx = 1;
			const int dy = LATTICE_WIDTH;

			int64_t c0 = l->pos[POS(x+1,y)];
			int64_t c1 = l->pos[POS(x-1,y)];
			int64_t c2 = l->pos[POS(x,y+1)];
			int64_t c3 = l->pos[POS(x,y-1)];

			int64_t avg = (c0+c1+c2+c3) >> 2;

			int pi = POS(x,y);
			int64_t f = avg - l->pos[pi];
			l->vel[pi] = ((l->vel[pi] + f) * (l->damp[pi]-1)) / l->damp[pi];
		}
	}

	for (int i = 0; i < LATTICE_N; i++) {
		l->pos[i] += l->vel[i];
	}
}

int main(int argc, char** argv)
{
	struct lattice* lattice = lattice_new();

	int mx = LATTICE_WIDTH/2;
	int my = LATTICE_HEIGHT/2;

	for (int y = 0; y < LATTICE_HEIGHT; y++) {
		for (int x = 0; x < LATTICE_WIDTH; x++) {
			int dx = x-mx;
			int dy = y-my;

			double d = sqrt(dx*dx+dy*dy);
			double dd = d/LATTICE_WIDTH;

			int pi = POS(x,y);
			//lattice->damp[pi] = 3000 + cos(dd) * 1000;
			lattice->damp[pi] = 3000;
			//lattice->damp[pi] = 2000 + cos(dd) * 1000;
		}
	}

	lattice_impulse(lattice, LATTICE_WIDTH/3, LATTICE_HEIGHT/2, 4L<<ONE_SHIFT);

	int n = 150000;

	wo_begin("lattice5.wav");
	for (int i = 0; i < n; i++) {
		lattice_step(lattice);
		wo_push(lattice_sample(lattice, LATTICE_WIDTH/2, LATTICE_HEIGHT/2));
	}
	wo_end();

	return EXIT_SUCCESS;
}
