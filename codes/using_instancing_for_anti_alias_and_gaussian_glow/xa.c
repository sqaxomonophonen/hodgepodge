/*
$ cc $(pkg-config --cflags gl sdl2) -o using_instancing_for_anti_alias_and_gaussian_glow using_instancing_for_anti_alias_and_gaussian_glow.c $(pkg-config --libs gl sdl2) -lm && ./using_instancing_for_anti_alias_and_gaussian_glow
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <SDL.h>

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"

#define SOKOL_TIME_IMPL
#include "sokol_time.h"

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))
#define MEMBER_SIZE(t,m) sizeof(((t *)0)->m)
#define MEMBER_OFFSET(t,m) (void*)((size_t)&(((t *)0)->m))

#define PI (3.141592653589793)

#define ARRAY_BUFFER_SIZE (1<<24)

void _chkgl(const char* file, const int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		fprintf(stderr, "OPENGL ERROR 0x%.4x in %s:%d\n", err, file, line);
		abort();
	}
}

#define CHKGL _chkgl(__FILE__, __LINE__);

static GLuint create_shader(GLenum type, const char* src)
{
	GLuint shader = glCreateShader(type); CHKGL;

	glShaderSource(shader, 1, &src, NULL); CHKGL;
	glCompileShader(shader); CHKGL;

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint msglen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msglen);
		GLchar* msg = (GLchar*) malloc(msglen + 1);
		assert(msg != NULL);
		glGetShaderInfoLog(shader, msglen, NULL, msg);
		const char* stype = type == GL_VERTEX_SHADER ? "VERTEX" : type == GL_FRAGMENT_SHADER ? "FRAGMENT" : "???";
		fprintf(stderr, "%s GLSL COMPILE ERROR: %s in\n\n%s\n", stype, msg, src);
		abort();
	}

	return shader;
}

static GLuint create_program(const char* vert_src, const char* frag_src)
{
	GLuint vs = create_shader(GL_VERTEX_SHADER, vert_src);
	GLuint fs = create_shader(GL_FRAGMENT_SHADER, frag_src);

	GLuint program = glCreateProgram(); CHKGL;
	glAttachShader(program, vs); CHKGL;
	glAttachShader(program, fs); CHKGL;

	#if 0
	glBindAttribLocation(program, index, name); CHKGL;
	#endif

	glLinkProgram(program); CHKGL;

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint msglen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &msglen);
		GLchar* msg = (GLchar*) malloc(msglen + 1);
		glGetProgramInfoLog(program, msglen, NULL, msg);
		fprintf(stderr, "shader link error: %s\n", msg);
		abort();
	}

	glDeleteShader(vs); CHKGL;
	glDeleteShader(fs); CHKGL;

	return program;
}

struct globals {
	SDL_Window* window;
	SDL_GLContext* gl_context;
	int true_screen_width;
	int true_screen_height;
	int screen_width;
	int screen_height;
	float pixel_ratio;
} g;

static void populate_screen_globals()
{
	int prev_width = g.true_screen_width;
	int prev_height = g.true_screen_height;
	SDL_GL_GetDrawableSize(g.window, &g.true_screen_width, &g.true_screen_height);
	int w, h;
	SDL_GetWindowSize(g.window, &w, &h);
	g.pixel_ratio = (float)g.true_screen_width / (float)w;
	g.screen_width = g.true_screen_width / g.pixel_ratio;
	g.screen_height = g.true_screen_height / g.pixel_ratio;
	if ((g.true_screen_width != prev_width || g.true_screen_height != prev_height)) {
		printf("%d×%d -> %d×%d (r=%f)\n", prev_width, prev_height, g.screen_width, g.screen_height, g.pixel_ratio);
	}
}

struct vertex {
	uint32_t a_pos[2];
	uint32_t a_color;
};

struct chunk {
	int n_vertices;
	struct vertex vertices[1<<20];
	int is_buffered;
	int buffer_idx0;
};

static struct chunk* new_chunk(void)
{
	struct chunk* chunk = calloc(1, sizeof *chunk);
	return chunk;
}

static void chunk_add_vertex(struct chunk* chunk, struct vertex v)
{
	assert((chunk->n_vertices+1) < ARRAY_LENGTH(chunk->vertices));
	chunk->vertices[chunk->n_vertices++] = v;
}

static void chunk_add_triangle(struct chunk* chunk, struct vertex v0, struct vertex v1, struct vertex v2)
{
	chunk_add_vertex(chunk, v0);
	chunk_add_vertex(chunk, v1);
	chunk_add_vertex(chunk, v2);
}

static void chunk_add_circle(struct chunk* chunk, int n, uint32_t x, uint32_t y, uint32_t r0, uint32_t r1, uint32_t color0, uint32_t color1)
{
	struct vertex prev_v0, prev_v1;

	for (int i = 0; i <= n; i++) {
		double theta = ((double)i / (double)n) * 2.0 * PI;
		double ux = sin(theta);
		double uy = cos(theta);

		uint32_t x0 = x + (uint32_t)(ux*(double)r0);
		uint32_t y0 = y + (uint32_t)(uy*(double)r0);
		struct vertex v0 = { .a_pos={x0,y0}, .a_color=color0 };

		uint32_t x1 = x + (uint32_t)(ux*(double)r1);
		uint32_t y1 = y + (uint32_t)(uy*(double)r1);
		struct vertex v1 = { .a_pos={x1,y1}, .a_color=color1 };

		if (i > 0) {
			chunk_add_triangle(chunk, prev_v0, v0, v1);
			chunk_add_triangle(chunk, prev_v0, v1, prev_v1);
		}

		prev_v0 =  v0;
		prev_v1 =  v1;
	}
}

static void chunk_add_waveform(struct chunk* chunk)
{
	uint32_t color = 0x00ffffff;
	const int n = 30000;
	struct vertex prev_v0, prev_v1;
	for (int i = 0; i <= n; i++) {
		uint32_t x = i * 16;
		double w = (double)rand() / (double)RAND_MAX;
		const int sz = 2500;
		uint32_t dy = w * sz;
		uint32_t y0 = sz - dy;
		uint32_t y1 = sz + dy;

		struct vertex v0 = { .a_pos={x,y0},  .a_color=color };
		struct vertex v1 = { .a_pos={x,y1},  .a_color=color };

		if (i > 0) {
			chunk_add_triangle(chunk, prev_v0, v0, v1);
			chunk_add_triangle(chunk, prev_v0, v1, prev_v1);
		}

		prev_v0 =  v0;
		prev_v1 =  v1;
	}
}

struct render {
	GLuint program_paint;
	GLuint array_buffer;
	int array_buffer_cursor;

	gbMat4 tx;
	gbVec2 screen_resolution;

	int n_instances;

	GLint location_u_tx;
	GLint location_u_screen_resolution;
	GLint location_u_instance;
	GLint location_u_seed;
};

#define N_VERTEX_ATTRIBUTES (2)

#define MAX_INSTANCES (128)
#define STR2(x) #x
#define STR(x) STR2(x)

static struct render* new_render(void)
{
	struct render* render = calloc(1, sizeof * render);

	render->program_paint = create_program(
		"#version 140\n"
		"in vec2  a_pos;\n"
		"in vec4  a_color;\n"
		"\n"
		"uniform mat4x4  u_tx;\n"
		"uniform vec2    u_screen_resolution;\n"
		"uniform vec3    u_instance[" STR(MAX_INSTANCES) "];\n"
		"uniform uint    u_seed;\n"
		"\n"
		"out vec4 v_color;\n"
		"\n"
		"uint hash(uint x)\n"
		"{\n"
		"	x += ( x << 10u );\n"
		"	x ^= ( x >>  6u );\n"
		"	x += ( x <<  3u );\n"
		"	x ^= ( x >> 11u );\n"
		"	x += ( x << 15u );\n"
		"	return x;\n"
		"}\n"
		"\n"
		"float rndf(uint local_seed)\n"
		"{\n"
		"	uint h = hash(hash(hash(hash(u_seed) + local_seed) + uint(gl_InstanceID)) + uint(gl_VertexID));\n"
		"	return fract(sin(h));\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec4 p = u_tx * vec4(a_pos * 0.001, 0.0, 1.0);\n"
		"	vec3 instance = u_instance[gl_InstanceID];\n"
		"	vec2 displacement = instance.xy;\n"
		"	float color_scale = instance.z;\n"
		"	vec2 d = (vec2(2,2) / u_screen_resolution) * displacement;\n"
		"	vec3 color_dither = vec3(rndf(0u)-0.5, rndf(1u)-0.5, rndf(2u)-0.5) * (2.0/256.0);\n"
		"	v_color = a_color * color_scale + vec4(color_dither, 0);\n"
		"	gl_Position = vec4(p.xyz/p.w,1) + vec4(d, 0.0, 0.0);\n"
		"}\n"
	,
		"#version 140\n"
		"in vec4 v_color;\n"
		"out vec4 frag_color;\n"
		"void main()\n"
		"{\n"
		"	frag_color = v_color;\n"
		"}\n"
	);

	#define UNIFORM(NAME) render->location_##NAME = glGetUniformLocation(render->program_paint, #NAME); CHKGL; assert(render->location_##NAME >= 0);
	UNIFORM(u_tx)
	UNIFORM(u_screen_resolution)
	UNIFORM(u_instance)
	UNIFORM(u_seed)
	#undef UNIFORM

	glGenBuffers(1, &render->array_buffer); CHKGL;
	glBindBuffer(GL_ARRAY_BUFFER, render->array_buffer); CHKGL;
	glBufferData(GL_ARRAY_BUFFER, ARRAY_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW); CHKGL;

	return render;
}

static void instances_add_anti_alias(gbVec3* instance, int* n_instances, int sublog2)
{
	const int n = 1 << sublog2;
	const int nn = n*n;
	const float color_scale = 1.0f / (float)nn;

	const float s = 1.0f / (float)n;
	const float d = -0.5f;

	const float sx = s;
	const float sy = s;
	const float dx = d;
	const float dy = d;

	for (int y = 0; y < n; y++) {
		for (int x = 0; x < n; x++) {
			instance[(*n_instances)++] = gb_vec3((float)x*sx+dx, (float)y*sy+dy, color_scale);
			assert(*n_instances <= MAX_INSTANCES);
		}
	}
}

static void instances_add_glow0(gbVec3* instance, int* n_instances)
{
	int ni0 = *n_instances;
	const double spacing = 40.0;
	const int nri = 4;
	for (int ri = 1; ri <= nri; ri++) {
		double r = (double)ri * spacing;
		double circum = 2.0*PI*r;
		int nc = (int)round(circum / spacing);
		if (nc < 1) nc = 1;
		double rr = 2.0 * ((double)ri / (double)nri);
		const double scale = exp(-(rr*rr)) * 0.1;
		for (int ci = 0; ci < nc; ci++) {
			double theta = ((double)ci / (double)nc) * 2.0 * PI;
			double dx = sin(theta)*r;
			double dy = cos(theta)*r;
			instance[(*n_instances)++] = gb_vec3(dx, dy, scale);
			if (*n_instances >= MAX_INSTANCES) {
				printf("WARNING: early exit after %d glow instances\n", *n_instances - ni0);
				return;
			}
		}
	}
}

static inline double drand()
{
	return (double)rand() / (double)RAND_MAX;
}


static void instances_add_glow1(gbVec3* instance, int* n_instances)
{
	const int n_glow = 60;
	const double radius = 150;
	for (int i = 0; i < n_glow; i++) {
		const double theta = drand() * 2.0 * PI;
		const double rn = drand();
		const double r = rn * radius;
		const double dx = sin(theta)*r;
		const double dy = cos(theta)*r;
		const double scale = exp(-(rn*rn*4.0)) * 0.04;
		//const double scale = 0.05;
		instance[(*n_instances)++] = gb_vec3(dx, dy, scale);
		assert(*n_instances <= MAX_INSTANCES);
	}
}


static void render_prep(struct render* render, int width, int height, gbVec3 campos, float pitch_radians, float yaw_radians)
{
	render->screen_resolution = gb_vec2(width, height);

	{
		gbMat4 perspective;
		gb_mat4_perspective(&perspective, 100.0, (float)width / (float)height, 1e-1, 1e3);

		gbMat4 translate;
		gb_mat4_translate(&translate, campos);

		gbMat4 look;
		gb_mat4_from_quat(&look, gb_quat_euler_angles(pitch_radians, yaw_radians, 0.0f));

		gbMat4* tx = &render->tx;
		gb_mat4_identity(tx);
		gb_mat4_mul(tx, tx, &perspective);
		gb_mat4_mul(tx, tx, &look);
		gb_mat4_mul(tx, tx, &translate);
	}

	glUseProgram(render->program_paint); CHKGL;

	glUniform1ui(render->location_u_seed, rand()); CHKGL;

	// "If transpose is GL_FALSE, each matrix is assumed to be supplied in
	// column major order. If transpose is GL_TRUE, each matrix is assumed
	// to be supplied in row major order.".
	const GLboolean transpose = GL_FALSE; // gb_math.h looks very column-major
	glUniformMatrix4fv(render->location_u_tx, 1, transpose, (GLfloat*)&render->tx); CHKGL;
	glUniform2fv(render->location_u_screen_resolution, 1, (GLfloat*)&render->screen_resolution); CHKGL;

	gbVec3 instance[MAX_INSTANCES];
	int n_instances = 0;
	instances_add_anti_alias(instance, &n_instances, 3);
	instances_add_glow0(instance, &n_instances);
	//instances_add_glow1(instance, &n_instances);
	glUniform3fv(render->location_u_instance, n_instances, (GLfloat*)&instance[0]); CHKGL;
	render->n_instances = n_instances;

	for (int i = 0; i < N_VERTEX_ATTRIBUTES; i++) {
		glEnableVertexAttribArray(i); CHKGL;
	}

	glBindBuffer(GL_ARRAY_BUFFER, render->array_buffer); CHKGL;

	const size_t stride = sizeof(struct vertex);
	glVertexAttribPointer(0, 2, GL_UNSIGNED_INT,   GL_FALSE, stride, MEMBER_OFFSET(struct vertex, a_pos)); CHKGL;
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE,  GL_TRUE,  stride, MEMBER_OFFSET(struct vertex, a_color)); CHKGL;
}

static void render_chunk(struct render* render, struct chunk* chunk)
{
	if (!chunk->is_buffered) {
		const int n = chunk->n_vertices;
		chunk->buffer_idx0 = render->array_buffer_cursor;
		const size_t vsz = sizeof(*chunk->vertices);
		glBufferSubData(GL_ARRAY_BUFFER, chunk->buffer_idx0 * vsz, chunk->n_vertices * vsz, chunk->vertices); CHKGL;
		render->array_buffer_cursor += n;
		chunk->is_buffered = 1;
	}

	glDrawArraysInstanced(
		GL_TRIANGLES,
		chunk->buffer_idx0,
		chunk->n_vertices,
		render->n_instances); CHKGL;
}

static void render_done(struct render* render)
{
	for (int i = 0; i < N_VERTEX_ATTRIBUTES; i++) {
		glDisableVertexAttribArray(i); CHKGL;
	}
}

int main(int argc, char** argv)
{
	stm_setup();

	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);

	g.window = SDL_CreateWindow(
		"Instancing Test",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1920, 1080,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	assert(g.window);

	g.gl_context = SDL_GL_CreateContext(g.window);
	assert(g.gl_context);

	#define PGL(NAME) \
		{ \
			GLint i; \
			glGetIntegerv(NAME, &i); \
			printf(#NAME " = %d\n", i); \
		}
	PGL(GL_MAX_UNIFORM_BUFFER_BINDINGS)
	PGL(GL_MAX_UNIFORM_BLOCK_SIZE)
	PGL(GL_MAX_UNIFORM_LOCATIONS)
	#undef PGL

	populate_screen_globals();

	struct render* render = new_render();

	struct chunk* chunk0 = new_chunk();
	chunk_add_circle(chunk0, 2000, 5000, 5000, 3700, 4000, 0x00ffffff, 0x00000000);

	struct chunk* chunk1 = new_chunk();
	chunk_add_waveform(chunk1);

	int exiting = 0;
	float pitch = 0.0;
	float yaw = 0.0;
	gbVec3 campos = gb_vec3(0, 0, -10);

	glEnable(GL_BLEND); CHKGL;
	glBlendFuncSeparate(
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ZERO); CHKGL;
	glDisable(GL_CULL_FACE); CHKGL;
	glDisable(GL_DEPTH_TEST); CHKGL;

	//assert(SDL_CaptureMouse(1) == 0);
	assert(SDL_SetRelativeMouseMode(1) == 0);

	while (!exiting) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) {
				exiting = 1;
			} else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
				exiting = 1;
			} else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				populate_screen_globals();
			} else if (ev.type == SDL_MOUSEMOTION) {
				int dx = ev.motion.xrel;
				int dy = ev.motion.yrel;

				const float spd = 0.0005f;
				yaw += (float)dx * -spd;
				pitch += (float)dy * -spd;
			}
		}

		glViewport(0, 0, g.true_screen_width, g.true_screen_height);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);


		uint64_t t0 = stm_now();
		render_prep(render, g.true_screen_width, g.true_screen_height, campos, pitch, yaw);
		render_chunk(render, chunk0);
		render_chunk(render, chunk1);
		render_done(render);
		glFlush();
		SDL_GL_SwapWindow(g.window);
		double dt = stm_sec(stm_diff(stm_now(), t0));
		printf("render: %fs (%f/s)\n", dt, 1.0/dt);


		//pitch += 0.001f;
		//yaw += 0.1f;
	}

	SDL_GL_DeleteContext(g.gl_context);
	SDL_DestroyWindow(g.window);

	return EXIT_SUCCESS;
}
