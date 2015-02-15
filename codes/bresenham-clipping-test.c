#include <stdio.h>
#include <stdlib.h>

static int solve_iteratively_for_x(int y1, int dx, int dy)
{
	int det = dy*2 - dx;
	int x = 0;
	int y = 0;
	while (y != y1) {
		if (det > 0) {
			det -= dx*2;
			y++;
		}
		det += dy*2;
		x++;
		// "plot x,y"
	}
	if (y != y1) exit(1);
	return x;
}

static int solve_algebraically_for_x(int y1, int dx, int dy)
{
	int x1 = ((y1-1)*dx)/dy;
	int det = -2*dx*y1 + 2*dy*x1 + 2*dy + dx;
	x1 += (-det / (dy*2)) + 2 - (det>0);
	return x1;
}

static int solve_iteratively_for_y(int x1, int dx, int dy)
{
	int det = dy*2 - dx;
	int x = 0;
	int y = 0;
	while (x != x1) {
		if (det > 0) {
			det -= dx*2;
			y++;
		}
		det += dy*2;
		x++;
		// "plot x,y"
	}
	if (x != x1) exit(1);
	return y;
}

static int solve_algebraically_for_y(int x1, int dx, int dy)
{
	// source: http://stackoverflow.com/questions/25488264/bresenham-integer-equation-not-just-the-algorithm
	return (x1 * dy*2 + dx - 1) / (dx*2);
}


static void paranoid_verify(int x1, int y1, int dx, int dy)
{
	int det = dy*2 - dx;
	int x = 0;
	int y = 0;
	while (x <= dx || x <= x1) {
		if (det > 0) {
			det -= dx*2;
			y++;
		}
		det += dy*2;
		x++;
		if (x == x1 && y == y1) return;
	}
	printf("failed verification for x1:%d y1:%d dx:%d dy:%d\n", x1, y1, dx, dy);
	exit(1);
}

int main(int argc, char** argv)
{
	int N = 100000;
	{
		int correct = 0;
		int i;
		for (i = 0; i < N; i++) {
			int dx = (rand() % 1500) + 1;
			int dy = (rand() % 1500) + 1;
			if (dy > dx) {
				int tmp = dx;
				dx = dy;
				dy = tmp;
			}
			int y1 = (rand() % 100) + 1;

			int x1_it = solve_iteratively_for_x(y1, dx, dy);
			int x1_al = solve_algebraically_for_x(y1, dx, dy);
			if (x1_it == x1_al) correct++;
			paranoid_verify(x1_al, y1, dx, dy);
		}
		printf("solve for y: %d/%d\n", correct, N);
	}

	{
		int correct = 0;
		int i;
		for (i = 0; i < N; i++) {
			int dx = (rand() % 1500) + 1;
			int dy = (rand() % 1500) + 1;
			if (dy > dx) {
				int tmp = dx;
				dx = dy;
				dy = tmp;
			}
			int x1 = (rand() % 100) + 1;

			int y1_it = solve_iteratively_for_y(x1, dx, dy);
			int y1_al = solve_algebraically_for_y(x1, dx, dy);
			if (y1_it == y1_al) correct++;
			paranoid_verify(x1, y1_al, dx, dy);
		}
		printf("solve for x: %d/%d\n", correct, N);
	}


	return 0;
}

