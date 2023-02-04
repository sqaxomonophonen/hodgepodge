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

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))
#define MEMBER_SIZE(t,m) sizeof(((t *)0)->m)
#define MEMBER_OFFSET(t,m) (void*)((size_t)&(((t *)0)->m))

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
	uint16_t a_pos[2];
	uint32_t a_color;
};

struct chunk {
	int n_vertices;
	struct vertex vertices[1<<16];

	int n_indices;
	uint16_t indices[3<<16];

	gbVec2 origin;
	float multiplier;
	GLuint gl_buffers[2];

};

static struct chunk* new_chunk(gbVec2 origin, float multiplier)
{
	struct chunk* chunk = calloc(1, sizeof *chunk);
	chunk->origin = origin;
	chunk->multiplier = multiplier;
	glGenBuffers(2, chunk->gl_buffers); CHKGL;

	glBindBuffer(GL_ARRAY_BUFFER, chunk->gl_buffers[0]); CHKGL;
	glBufferData(GL_ARRAY_BUFFER, sizeof(chunk->vertices), NULL, GL_DYNAMIC_DRAW); CHKGL;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->gl_buffers[1]); CHKGL;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(chunk->indices), NULL, GL_DYNAMIC_DRAW); CHKGL;

	#if 0
	printf("buffer v:%zdB i:%zdB\n", sizeof(chunk->vertices), sizeof(chunk->indices));
	#endif

	return chunk;
}

static uint16_t chunk_add_vertex(struct chunk* chunk, struct vertex v)
{
	assert((chunk->n_vertices+1) < ARRAY_LENGTH(chunk->vertices));
	chunk->vertices[chunk->n_vertices++] = v;
}

static void chunk_add_triangle(struct chunk* chunk, uint16_t v0, uint16_t v1, uint16_t v2)
{
	assert((chunk->n_indices+3) < ARRAY_LENGTH(chunk->indices));
	chunk->indices[chunk->n_indices++] = v0;
	chunk->indices[chunk->n_indices++] = v1;
	chunk->indices[chunk->n_indices++] = v2;
}

struct render {
	int multisample_log2;
	GLuint program_paint;

	gbMat4 tx;
	gbVec2 dim;

	GLint u_tx;
	GLint u_dim;
	GLint u_samples;
	GLint u_multiplier;
	GLint u_origin;
};

static struct render* new_render(int multisample_log2)
{
	struct render* render = calloc(1, sizeof * render);

	render->multisample_log2 = multisample_log2;

	render->program_paint = create_program(
		"#version 140\n"
		"in vec2  a_pos;\n"
		"in vec4  a_color;\n"
		"\n"
		"uniform mat4x4  u_tx;\n"
		"uniform vec2    u_dim;\n"
		"uniform uvec2   u_samples;\n"
		"uniform float   u_multiplier;\n"
		"uniform vec2    u_origin;\n"
		"\n"
		"out vec4 v_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec4 p = u_tx * vec4(u_origin + u_multiplier*a_pos, 0.0, 1.0);\n"
		"\n"
		"	uint i = uint(gl_InstanceID);\n"
		"	vec2 ip = vec2(mod(float(i), float(u_samples.x)), float(i / u_samples.x)) / vec2(u_samples);\n"
		"	vec2 u2 = vec2(2.0, 2.0) / u_dim;\n"
		"	vec2 d = u2*ip;\n"
		"\n"
		"	v_color = a_color;\n"
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

	#define UNIFORM(NAME) render->NAME = glGetUniformLocation(render->program_paint, #NAME); CHKGL; assert(render->NAME >= 0);
	UNIFORM(u_tx)
	UNIFORM(u_dim)
	UNIFORM(u_samples)
	UNIFORM(u_multiplier)
	UNIFORM(u_origin)
	#undef UNIFORM

	return render;
}

static inline int render_get_instance_count(struct render* render)
{
	return 1 << (render->multisample_log2 << 1);
}

static void render_prep(struct render* render, int width, int height, gbVec3 campos, float pitch_radians, float yaw_radians)
{
	render->dim = gb_vec2(width, height);

	{
		gbMat4 perspective;
		gb_mat4_perspective(&perspective, 100.0, (float)width / (float)height, 1e-1, 1e3);

		gbMat4 translate;
		gb_mat4_translate(&translate, campos);

		gbMat4 look;
		gb_mat4_from_quat(&look, gb_quat_euler_angles(pitch_radians, yaw_radians, 0.0f));

		gbMat4* tx = &render->tx;
		gb_mat4_identity(tx);
		#if 1
		gb_mat4_mul(tx, tx, &perspective);
		gb_mat4_mul(tx, tx, &look);
		gb_mat4_mul(tx, tx, &translate);
		#else
		gb_mat4_mul(tx, tx, &translate);
		gb_mat4_mul(tx, tx, &look);
		gb_mat4_mul(tx, tx, &perspective);
		#endif
	}
}

static void render_chunk(struct render* render, struct chunk* chunk)
{
	glUseProgram(render->program_paint); CHKGL;

	// per render uniforms

	// "If transpose is GL_FALSE, each matrix is assumed to be supplied in
	// column major order. If transpose is GL_TRUE, each matrix is assumed
	// to be supplied in row major order.".
	const GLboolean transpose = GL_FALSE; // gb_math.h looks very column-major
	glUniformMatrix4fv(render->u_tx, 1, transpose, (GLfloat*)&render->tx); CHKGL;
	glUniform2fv(render->u_dim, 1, (GLfloat*)&render->dim); CHKGL;
	const GLuint nso = 1u << render->multisample_log2;
	glUniform2ui(render->u_samples, nso, nso); CHKGL;

	// per chunk uniforms
	glUniform1f(render->u_multiplier, chunk->multiplier); CHKGL;
	glUniform2fv(render->u_origin, 1, (GLfloat*)&chunk->origin); CHKGL;

	const int n_vertex_attribs = 2;

	glBindBuffer(GL_ARRAY_BUFFER, chunk->gl_buffers[0]); CHKGL;
	glBufferSubData(GL_ARRAY_BUFFER, 0, chunk->n_vertices * sizeof(*chunk->vertices), chunk->vertices); CHKGL;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->gl_buffers[1]); CHKGL;
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, chunk->n_indices * sizeof(*chunk->indices), chunk->indices); CHKGL;

	for (int i = 0; i < n_vertex_attribs; i++) {
		glEnableVertexAttribArray(i); CHKGL;
	}

	const size_t stride = sizeof(struct vertex);
	glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, MEMBER_OFFSET(struct vertex, a_pos)); CHKGL;
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE,  GL_TRUE, stride, MEMBER_OFFSET(struct vertex, a_color)); CHKGL;

	glDrawElementsInstanced(
		GL_TRIANGLES,
		chunk->n_indices,
		GL_UNSIGNED_SHORT,
		(void*)0,
		render_get_instance_count(render)); CHKGL;

	for (int i = 0; i < n_vertex_attribs; i++) {
		glDisableVertexAttribArray(i); CHKGL;
	}
}

int main(int argc, char** argv)
{
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

	populate_screen_globals();

	struct render* render = new_render(2);

	struct chunk* chunk = new_chunk(gb_vec2(0,0), 100.0);
	{
		const uint32_t c = 0x000f0f0f; // n=2
		//const uint32_t c = 0x00030303; // n=3
		uint16_t v0 = chunk_add_vertex(chunk, (struct vertex) { .a_pos={100,  100    }, .a_color=c });
		uint16_t v1 = chunk_add_vertex(chunk, (struct vertex) { .a_pos={1500, 0    },   .a_color=c });
		uint16_t v2 = chunk_add_vertex(chunk, (struct vertex) { .a_pos={1000, 1000 },   .a_color=c });
		uint16_t v3 = chunk_add_vertex(chunk, (struct vertex) { .a_pos={0,    1000 },   .a_color=c });
		chunk_add_triangle(chunk, v0, v1, v2);
		chunk_add_triangle(chunk, v0, v2, v3);
	}

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

	printf("instance count: %d\n", render_get_instance_count(render));

	while (!exiting) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) {
				exiting = 1;
			} else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
				exiting = 1;
			} else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				populate_screen_globals();
			}
		}

		glViewport(0, 0, g.true_screen_width, g.true_screen_height);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);

		render_prep(render, g.true_screen_width, g.true_screen_height, campos, pitch, yaw);

		render_chunk(render, chunk);

		SDL_GL_SwapWindow(g.window);


		//pitch += 0.001f;
		//yaw += 0.1f;
	}

	SDL_GL_DeleteContext(g.gl_context);
	SDL_DestroyWindow(g.window);

	return EXIT_SUCCESS;
}
