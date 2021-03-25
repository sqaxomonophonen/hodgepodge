#include "wo.h"

//#define LATTICE_WIDTH (101)
//#define LATTICE_HEIGHT (64)

#define LATTICE_WIDTH (2134)
#define LATTICE_HEIGHT (5)

struct lattice {
	int64_t pos[LATTICE_WIDTH * LATTICE_HEIGHT];
	int64_t vel[LATTICE_WIDTH * LATTICE_HEIGHT];
};

static struct lattice* lattice_new()
{
	struct lattice* l = calloc(1, sizeof *l);
	assert(l != NULL);
	return l;
}

static void lattice_impulse(struct lattice* l, int x, int y, int64_t p)
{
	assert(x >= 0 && x < LATTICE_WIDTH && y >= 0 && y < LATTICE_HEIGHT);
	l->pos[x + y*LATTICE_WIDTH] = p;
}

#define ONE_SHIFT (48)

static float lattice_sample(struct lattice* l, int x, int y)
{
	assert(x >= 0 && x < LATTICE_WIDTH && y >= 0 && y < LATTICE_HEIGHT);
	double sample = (double)l->pos[x + y*LATTICE_WIDTH] * ((double)1 / (double)(1L<<ONE_SHIFT));
	if (sample < -1.0f) sample = -1.0f;
	if (sample > 1.0f) sample = 1.0f;
	return (float)sample;
}

static void lattice_step(struct lattice* l)
{
	{
		int64_t* pos = l->pos;
		int64_t* vel = l->vel;
		pos += 1 + LATTICE_WIDTH;
		vel += 1 + LATTICE_WIDTH;
		for (int y = 1; y < (LATTICE_HEIGHT-1); y++) {
			for (int x = 1; x < (LATTICE_WIDTH-1); x++) {

				const int dx = 1;
				const int dy = LATTICE_WIDTH;

				int64_t c0 = *(pos-dx);
				int64_t c1 = *(pos+dx);
				int64_t c2 = *(pos-dy);
				int64_t c3 = *(pos+dy);

				#if 0
				int64_t avg = (c0+c1+c2+c3) >> 2;
				#else
				int64_t c4 = *(pos-dx-dy);
				int64_t c5 = *(pos-dx+dy);
				int64_t c6 = *(pos+dx-dy);
				int64_t c7 = *(pos+dx+dy);
				int64_t avg = (c0+c1+c2+c3+c4+c5+c6+c7) >> 3;
				#endif

				int64_t f = avg - (*pos);
				//f = (f * 999)/1000;
				//f = (f * 99)/100;
				//f = (f * 9)/10;
				//f = f/11;
				f = f/20;
				(*vel) += f;

				pos++;
				vel++;
			}
			pos += 2;
			vel += 2;
		}
	}

	{
		int64_t* pos = l->pos;
		int64_t* vel = l->vel;
		pos += 1 + LATTICE_WIDTH;
		vel += 1 + LATTICE_WIDTH;
		for (int y = 1; y < (LATTICE_HEIGHT-1); y++) {
			for (int x = 1; x < (LATTICE_WIDTH-1); x++) {
				*pos = (*pos) + (*vel);
				pos++;
				vel++;
			}
			pos += 2;
			vel += 2;
		}
	}
}

int main(int argc, char** argv)
{
	struct lattice* lattice = lattice_new();
	lattice_impulse(lattice, LATTICE_WIDTH/3, LATTICE_HEIGHT/3, 4L<<ONE_SHIFT);

	int n = 800000;

	wo_begin("lattice.wav");
	for (int i = 0; i < n; i++) {
		lattice_step(lattice);
		wo_push(lattice_sample(lattice, LATTICE_WIDTH/2, LATTICE_HEIGHT/2));
	}
	wo_end();

	return EXIT_SUCCESS;
}
