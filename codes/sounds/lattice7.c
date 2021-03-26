#include "wo.h"

//#define LATTICE_WIDTH (101)
//#define LATTICE_HEIGHT (64)

#define LATTICE_WIDTH (80)
#define LATTICE_HEIGHT (7)
#define LATTICE_N (LATTICE_WIDTH * LATTICE_HEIGHT)
#define POS(x,y) ((x)+(y)*LATTICE_WIDTH)

struct lattice {
	float pos[LATTICE_N];
	float vel[LATTICE_N];
	float damp[LATTICE_N];
	float bounce[LATTICE_N];
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

static void lattice_impulse(struct lattice* l, int x, int y, float p)
{
	check_pos(x,y);
	l->pos[POS(x,y)] = p;
}

static float lattice_sample(struct lattice* l, int x, int y)
{
	check_pos(x,y);
	float sample = l->pos[POS(x,y)];
	if (sample < -1.0f) sample = -1.0f;
	if (sample > 1.0f) sample = 1.0f;
	return sample;
}

static void lattice_step(struct lattice* l)
{
	int pi = POS(1,1);
	const int dx = 1;
	const int dy = LATTICE_WIDTH;

	for (int y = 1; y < (LATTICE_HEIGHT-1); y++) {
		for (int x = 1; x < (LATTICE_WIDTH-1); x++) {

			float c0 = l->pos[pi-dx];
			float c1 = l->pos[pi+dx];
			float c2 = l->pos[pi-dy];
			float c3 = l->pos[pi+dy];

			float avg = (c0+c1+c2+c3) * 0.25f;

			float f = avg - l->pos[pi];
			l->vel[pi] = (l->vel[pi] + f) * l->damp[pi];

			if (l->bounce[pi] > 0.0f && l->pos[pi] > l->bounce[pi] && l->vel[pi] > 0.0f) {
				l->vel[pi] = -(l->vel[pi]) * 0.9f;
			}

			pi++;
		}
		pi+=2;
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
			lattice->damp[pi] = 0.9998f;
			lattice->bounce[pi] = x < LATTICE_WIDTH/3 ? 0.001f : 0.0f;
		}
	}

	lattice_impulse(lattice, LATTICE_WIDTH/3, LATTICE_HEIGHT/2, 1.0f);

	int n = 150000;

	wo_begin(argv[0]);
	for (int i = 0; i < n; i++) {
		lattice_step(lattice);
		wo_push(
			lattice_sample(lattice, LATTICE_WIDTH/2, LATTICE_HEIGHT/2),
			lattice_sample(lattice, LATTICE_WIDTH/2+10, LATTICE_HEIGHT/2+1)
		);
	}
	wo_end();

	return EXIT_SUCCESS;
}
