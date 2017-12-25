/* renders curve fractals to SVG */

#define _POSIX_C_SOURCE 2

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

int pass;
FILE* outfile;
int svg_width = 1920;
int svg_height = 1080;
float bounds_x0, bounds_y0, bounds_x1, bounds_y1;
float first_y, last_y;
int bounds_i;

#define EMIT_STYLES \
	DST(BW_STROKE, "black/white, stroked") \
	DST(BW_FILL, "black/white, filled") \
	DST(G0, "blue/yellow gradient") \

enum style {
	#define DST(n,d) n,
	EMIT_STYLES
	#undef DST
	STYLE_MAX
} style;

char* style_names[] = {
	#define DST(n,d) #n,
	EMIT_STYLES
	#undef DST
};

char* style_descriptions[] = {
	#define DST(n,d) d,
	EMIT_STYLES
	#undef DST
};

#undef STYLES

int is_fill_style()
{
	switch (style) {
	case BW_STROKE:
		return 0;
	case BW_FILL:
	case G0:
		return 1;
	case STYLE_MAX:
		assert(!"XXX");
	}
	assert(!"XXX");
}

struct vec {
	float x,y;
};

struct vec vec_zero()
{
	return (struct vec) { .x = 0, .y = 0 };
}

struct vec vec_add(struct vec a, struct vec b)
{
	return (struct vec) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}

struct vec vec_sub(struct vec a, struct vec b)
{
	return (struct vec) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}

struct vec vec_scale(struct vec v, float scalar)
{
	return (struct vec) {
		.x = v.x * scalar,
		.y = v.y * scalar,
	};
}

struct vec vec_rot90(struct vec v)
{
	return (struct vec) {
		.x = -v.y,
		.y =  v.x,
	};
}

struct mat {
	float a,b,c,d;
};

struct mat mat_basis_from_uv(struct vec u, struct vec v)
{
	return (struct mat) {
		.a = u.x,
		.b = u.y,
		.c = v.x,
		.d = v.y,
	};
}

struct mat mat_identity()
{
	return mat_basis_from_uv(
		(struct vec) { .x = 1, .y = 0 },
		(struct vec) { .x = 0, .y = 1 }
	);
}


struct mat mat_basis_from_start_stop(struct vec start, struct vec stop)
{
	struct vec u = vec_sub(stop, start);
	struct vec v = vec_rot90(u);
	return mat_basis_from_uv(u, v);
}

struct mat mat_inverse(struct mat m)
{
	float det = 1.0f / (m.a*m.d - m.b*m.c);
	return (struct mat) {
		.a = det *  m.d,
		.b = det * -m.b,
		.c = det * -m.c,
		.d = det *  m.a,
	};
}


struct vec mat_apply(struct mat m, struct vec v)
{
	return (struct vec) {
		.x = m.a*v.x + m.c*v.y,
		.y = m.b*v.x + m.d*v.y,
	};
}

struct mat mat_scale_v(struct mat m, float scalar)
{
	m.c *= scalar;
	m.d *= scalar;
	return m;
}

struct atx {
	struct vec origin;
	struct mat basis;
};

struct atx atx_identity()
{
	return (struct atx) {
		.origin = vec_zero(),
		.basis = mat_identity(),
	};
}

struct atx atx_from_start_and_stop(struct vec start, struct vec stop)
{
	struct atx r;
	r.origin = start;
	r.basis = mat_basis_from_start_stop(start, stop);
	return r;
}

struct vec atx_apply(struct atx atx, struct vec v)
{
	return vec_add(atx.origin, mat_apply(atx.basis, v));
}


struct curve_step {
	float dx, dy;
	struct vec d;
	int reverse; // 0=forward, !0=reverse
	float v_scale;
};

struct curve {
	struct mat basis;
	int n_steps;
	struct curve_step* steps;
};


void emit_point_raw(float x, float y)
{
	fprintf(outfile, "%.3f,%.3f ", x, y);
}

void emit_point(struct vec p)
{
	float x = p.x;
	float y = -p.y;
	if (pass == 0) {
		if (bounds_i == 0) {
			bounds_x0 = bounds_x1 = x;
			bounds_y0 = bounds_y1 = y;
		} else {
			if (x < bounds_x0) bounds_x0 = x;
			if (x > bounds_x1) bounds_x1 = x;
			if (y < bounds_y0) bounds_y0 = y;
			if (y > bounds_y1) bounds_y1 = y;
		}
	} else if (pass == 1) {
		const float bounds_width = bounds_x1 - bounds_x0;
		const float bounds_height = bounds_y1 - bounds_y0;
		const int margin = 64;
		const float fit_width = svg_width - margin * 2;
		const float fit_height = svg_height - margin * 2;

		const float bounds_aspect_ratio = bounds_width / bounds_height;
		const float fit_aspect_ratio = fit_width / fit_height;

		float scale, ox, oy;
		if (bounds_aspect_ratio > fit_aspect_ratio) {
			scale = fit_width / bounds_width;
			ox = -bounds_x0 * scale + margin;
			oy = -bounds_y0 * scale + margin + (fit_height - bounds_height * scale) * 0.5f;
		} else {
			scale = fit_height / bounds_height;
			ox = -bounds_x0 * scale + margin + (fit_width - bounds_width * scale) * 0.5f;
			oy = -bounds_y0 * scale + margin;
		}

		const float px = x * scale + ox;
		const float py = y * scale + oy;

		emit_point_raw(px, py);

		if (bounds_i == 0) {
			first_y = py;
		} else {
			last_y = py;
		}
	}
	bounds_i++;
}

void render_rec(struct curve* curve, int max_depth, int depth, struct atx atx, int reverse, float v_scale)
{
	struct vec cursor = vec_zero();

	int i_start, i_stop, i_delta;
	if (reverse) {
		i_start = curve->n_steps - 1;
		i_stop = -1;
		i_delta = -1;
	} else {
		i_start = 0;
		i_stop = curve->n_steps;
		i_delta = 1;
	}

	for (int i = i_start; i != i_stop; i += i_delta) {
		struct curve_step step = curve->steps[i];

		struct vec d = step.d;
		d.y *= v_scale * (reverse ? -1 : 1);
		struct vec next = vec_add(cursor, d);
		struct vec point = atx_apply(atx, cursor);

		if (depth < max_depth) {
			render_rec(
				curve,
				max_depth,
				depth + 1,
				atx_from_start_and_stop(point, atx_apply(atx, next)),
				reverse ^ step.reverse,
				v_scale * step.v_scale
			);
		} else {
			emit_point(point);
		}

		cursor = next;
	}
}


void write_gradient(char* id, char* color0, char* color1)
{
	fprintf(outfile, "<linearGradient id=\"%s\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" x2=\"0\" y1=\"0\" y2=\"%d\">\n", id, svg_height);
	fprintf(outfile, "<stop offset=\"0%%\" stop-color=\"%s\"/>\n", color0);
	fprintf(outfile, "<stop offset=\"100%%\" stop-color=\"%s\"/>\n", color1);
	fprintf(outfile, "</linearGradient>\n");
}

void write_gradients(char* bg_color0, char* bg_color1, char* fg_color0, char* fg_color1)
{
	fprintf(outfile, "<defs>\n");
	write_gradient("bg", bg_color0, bg_color1);
	write_gradient("fg", fg_color0, fg_color1);
	fprintf(outfile, "</defs>\n");
}

void render(struct curve* curve, int iterations)
{
	/* apply curve basis (e.g. the axes of the triangle coordinate system
	 * corresponds to two sides of an equilateral unit triangle; applying
	 * the basis converts the "triangular coordinates" to the standard
	 * orthogonal coordinate system */
	for (int i = 0; i < curve->n_steps; i++) {
		curve->steps[i].d = mat_apply(curve->basis, curve->steps[i].d);
	}

	/* transform the curve so that it starts at (0,0) and stops at (1,0) */
	{
		struct vec stop = vec_zero();
		for (int i = 0; i < curve->n_steps; i++) {
			stop = vec_add(stop, curve->steps[i].d);
		}

		struct mat tx = mat_inverse(mat_basis_from_uv(stop, vec_rot90(stop)));

		for (int i = 0; i < curve->n_steps; i++) {
			curve->steps[i].d = mat_apply(tx, curve->steps[i].d);
		}
	}

	/* doing two passes; one to find fractal bounding box, and another to
	 * render the fractal as svg */
	for (pass = 0; pass < 2; pass++) {
		bounds_i = 0;

		if (pass == 1) {
			fprintf(outfile, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n", svg_width, svg_height, svg_width, svg_height);
			char* sty0 = "fill:none;";
			char* sty1 = "stroke:none;";
			switch (style) {
			case BW_STROKE:
				sty1 = "stroke:black;stroke-width:1.4;";
				break;
			case BW_FILL:
				sty0 = "fill:black;";
				break;
			case G0:
				write_gradients("#001040", "#005070", "#fff000", "#ffffff");
				sty0 = "fill:url(#fg);";
				fprintf(outfile, "<rect style=\"fill:url(#bg)\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"/>\n", svg_width, svg_height);
				break;
			case STYLE_MAX:
				assert(!"XXX");
				break;
			}
			fprintf(outfile, "<polyline style=\"%s%s\" points=\"", sty0, sty1);
		}

		struct atx atx = atx_identity();
		render_rec(curve, iterations - 1, 0, atx, 0, 1);
		emit_point(atx_apply(atx, (struct vec) { .x = 1, .y = 0}));

		if (pass == 1) {
			if (is_fill_style()) {
				emit_point_raw(svg_width, last_y);
				emit_point_raw(svg_width, svg_height);
				emit_point_raw(0, svg_height);
				emit_point_raw(0, first_y);
			}
			fprintf(outfile, "\"/>\n");
			fprintf(outfile, "</svg>\n");
		}
	}
}

void usage(char* prg)
{
	fprintf(stderr,
		"usage: %s [-o outfile.svg] [-n n_iterations] [-s style] <infile>\n"
		"n_iterations=2 by default\n"
		"available styles:\n"
		,
		prg);
	int max_len = 0;
	for (int i = 0; i < STYLE_MAX; i++) {
		int len = strlen(style_names[i]);
		if (len > max_len) max_len = len;
	}

	for (int i = 0; i < STYLE_MAX; i++) {
		fprintf(stderr, "  %s", style_names[i]);
		int len = strlen(style_names[i]);
		int padding = (max_len + 2) - len;
		for (int j = 0; j < padding; j++) fputc(' ', stderr);
		fprintf(stderr, "%s\n", style_descriptions[i]);
	}
}

int main(int argc, char** argv)
{
	int opt;

	outfile = stdout;
	int do_close_outfile = 0;

	int n_iterations = 2;

	while ((opt = getopt(argc, argv, "o:n:s:")) != -1) {
		switch (opt) {
		case 'o':
			outfile = fopen(optarg, "w");
			if (outfile == NULL) {
				perror(optarg);
				exit(EXIT_FAILURE);
			}
			do_close_outfile = 1;
			break;
		case 'n':
			n_iterations = atoi(optarg);
			break;
		case 's':
			{
				int found = 0;
				for (int i = 0; i < STYLE_MAX; i++) {
					if (strcmp(optarg, style_names[i]) == 0) {
						style = i;
						found = 1;
					}
				}
				if (!found) {
					fprintf(stderr, "invalid style\n");
					usage(argv[0]);
					exit(EXIT_FAILURE);
				}
			}
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	struct curve curve;
	memset(&curve, 0, sizeof curve);
	curve.basis = mat_identity();
	#define MAX_STEPS 512
	assert((curve.steps = calloc(MAX_STEPS, sizeof *curve.steps)) != NULL);

	/* read+parse .fractal infile */
	{
		if (optind != (argc-1)) {
			fprintf(stderr, "expected <infile>\n");
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}

		char* infilename = argv[optind];

		FILE* infile = fopen(infilename, "r");
		if (infile == NULL) {
			perror(infilename);
			exit(EXIT_FAILURE);
		}

		char line[4096];
		int lineno = 0;
		int state = 0;
		while (fgets(line, sizeof(line), infile)) {
			lineno++;

			#define PERR(s) \
				do { \
					fprintf(stderr, "ERROR in %s:%d -- %s\n", infilename, lineno, s); \
					exit(EXIT_FAILURE); \
				} while (0);

			char arg[4096];

			if (state == 0) {
				if (sscanf(line, "COORDSYS %s", arg) == 1) {
					if (strcmp(arg, "triangle") == 0) {
						curve.basis = mat_basis_from_uv(
							(struct vec) { .x = 1, .y = 0 },
							(struct vec) { .x = 0.5f, .y = 0.8660254037844386f }
						);
					} else if (strcmp(arg, "square") == 0) {
						curve.basis = mat_identity();
					} else {
						PERR("unexpected COORDSYS argument");
					}
				} else if (sscanf(line, "%s", arg) == 1) {
					if (strcmp(arg, "CURVE") == 0) {
						state = 1;
					} else {
						PERR("unexpected command");
					}
				}
			} else if (state == 1) {
				char direction;
				struct curve_step step;
				if (sscanf(line, "%f %f %c %f", &step.d.x, &step.d.y, &direction, &step.v_scale) == 4) {
					if (curve.n_steps >= MAX_STEPS) PERR("too many curve steps");
					if (direction == '>') {
						step.reverse = 0;
					} else if (direction == '<') {
						step.reverse = 1;
					} else {
						PERR("invalid direction");
					}
					curve.steps[curve.n_steps] = step;
					curve.n_steps++;
				} else if (sscanf(line, "%s", arg) == 1) {
					if (strcmp(arg, "END") == 0) {
						state = 0;
					} else {
						PERR("unexpected command (inside CURVE block)");
					}
				}
			} else {
				assert(!"invalid state");
			}
		}

		fclose(infile);

		if (curve.n_steps == 0) {
			fprintf(stderr, "ERROR %s contained no curve\n", infilename);
			exit(EXIT_FAILURE);
		}
	}

	render(&curve, n_iterations);

	if (do_close_outfile) fclose(outfile);

	return EXIT_SUCCESS;
}
