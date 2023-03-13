#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <SDL.h>

#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#define ARRAY_LENGTH(xs) (sizeof(xs)/sizeof((xs)[0]))
static inline int is_numeric(char ch) { return '0' <= ch && ch <= '9'; }

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

		// attempt to parse "0:<linenumber>" in error message
		int line_number = 0;
		if (strlen(msg) >= 3 && msg[0] == '0' && msg[1] == ':' && is_numeric(msg[2])) {
			const char* p0 = msg+2;
			const char* p1 = p0+1;
			while (is_numeric(*p1)) p1++;
			char buf[32];
			const int n = p1-p0;
			if (n < ARRAY_LENGTH(buf)) {
				memcpy(buf, p0, n);
				buf[n] = 0;
				line_number = atoi(buf);
			}
		}

		fprintf(stderr, "%s GLSL COMPILE ERROR: %s in:\n", stype, msg);
		if (line_number > 0) {
			const char* p = src;
			int remaining_line_breaks_to_find = line_number;
			while (remaining_line_breaks_to_find > 0) {
				for (;;) {
					char ch = *p;
					if (ch == 0) {
						remaining_line_breaks_to_find = 0;
						break;
					} else if (ch == '\n') {
						remaining_line_breaks_to_find--;
						p++;
						break;
					}
					p++;
				}
			}
			fwrite(src, p-src, 1, stderr);
			printf("~^~^~^~ ERROR ~^~^~^~\n");
			printf("%s\n", p);
		} else {
			fprintf(stderr, "%s\n", src);
		}

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


int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);

	SDL_Window* window = SDL_CreateWindow(
		"fakemip",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1920, 1080,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

	SDL_GLContext* glctx = SDL_GL_CreateContext(window);

	int has_aniso = SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic");
	printf("has_aniso=%d\n", has_aniso);

	float max_aniso_level = -1.0f;
	float aniso_level = 1.0f;
	if (has_aniso) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso_level);
		printf("max aniso: %f\n", max_aniso_level);
	}

	int exiting = 0;

	float mlook_rotx = 0.0f;
	float mlook_roty = 0.0f;
	const float mlook_sensitivity = 0.002f;

	int grab = 1;
	SDL_SetRelativeMouseMode(grab);

	const float fovy = 70.0f;

	GLuint vtxbuf_quad;
	{
		const static uint8_t vertices[] = {0,0, 1,0, 1,1, 0,1};
		glGenBuffers(1, &vtxbuf_quad); CHKGL;
		glBindBuffer(GL_ARRAY_BUFFER, vtxbuf_quad); CHKGL;
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); CHKGL;
		glBindBuffer(GL_ARRAY_BUFFER, 0); CHKGL;
	}

	GLuint idxbuf_quad;
	{
		const static uint8_t indices[] = {0,1,2, 0,2,3};
		glGenBuffers(1, &idxbuf_quad); CHKGL;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf_quad); CHKGL;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); CHKGL;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHKGL;
	}

	GLuint prg = create_program(
		"#version 140\n"
		"in vec2 a_coord;\n"
		"uniform mat4x4 u_transform;\n"
		"out vec2 v_uv;\n"
		"void main()\n"
		"{\n"
		"	float n = 100;\n"
		"	v_uv = a_coord * n * vec2(0.5,1);\n"
		"	gl_Position = u_transform * vec4(n*100*(a_coord*2-vec2(1,1)), 0, 1);\n"
		"}\n"
	,
		"#version 140\n"
		"in vec2 v_uv;\n"
		"uniform sampler2D u_texture;\n"
		"out vec4 frag_color;\n"
		"void main()\n"
		"{\n"
		"	frag_color = texture(u_texture, v_uv);\n"
		"}\n"
	);

	GLint u_transform = glGetUniformLocation(prg, "u_transform"); CHKGL; assert(u_transform >= 0);
	GLint u_texture = glGetUniformLocation(prg, "u_texture"); CHKGL; assert(u_texture >= 0);

	GLuint tex;
	{
		glGenTextures(1, &tex); CHKGL;
		glBindTexture(GL_TEXTURE_2D, tex); CHKGL;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CHKGL;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CHKGL;
		const int n_levels = 8;
		for (int level = 0; level <= n_levels; level++) {
			const int size = 1 << (n_levels - level);
			uint32_t* bitmap = calloc(size*size, sizeof *bitmap);
			uint32_t* p = bitmap;
			for (int y = 0; y < size; y++) {
				for (int x = 0; x < size; x++) {
					int hx = x >= (size>>1) ? 1 : 0;
					int hy = y >= (size>>1) ? 1 : 0;
					int h = hx^hy;
					if (h) {
						int k = level%6;
						if (k == 0) {
							*p = 0xff0000ff;
						} else if (k == 1) {
							*p = 0xff00ffff;
						} else if (k == 2) {
							*p = 0xff00ff00;
						} else if (k == 3) {
							*p = 0xffffff00;
						} else if (k == 4) {
							*p = 0xffff0000;
						} else if (k == 5) {
							*p = 0xffff00ff;
						}
					} else {
						int k = level%2;
						if (k) {
							*p = 0xffffffff;
						} else {
							*p = 0xff000000;
						}
					}
					p++;
				}
			}
			glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap); CHKGL;
			free(bitmap);
		}
		glBindTexture(GL_TEXTURE_2D, 0); CHKGL;
	}

	glEnable(GL_BLEND); CHKGL;
	glBlendFuncSeparate(
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
		GL_ONE, GL_ZERO); CHKGL;
	glDisable(GL_CULL_FACE); CHKGL;
	glDisable(GL_DEPTH_TEST); CHKGL;

	while (!exiting) {
		SDL_Event e;
		float set_aniso_level = -1.0f;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT: {
				exiting = 1;
			} break;

			case SDL_KEYDOWN: {
				const int sym = e.key.keysym.sym;
				if (sym == SDLK_ESCAPE) {
					exiting = 1;
				} else if (sym == ' ') {
					grab = !grab;
					SDL_SetRelativeMouseMode(grab);
				} else if (has_aniso && sym == '[') {
					set_aniso_level = aniso_level - 1.0f;
				} else if (has_aniso && sym == ']') {
					set_aniso_level = aniso_level + 1.0f;
				}
			} break;


			case SDL_MOUSEMOTION: {
				mlook_rotx += (float)e.motion.xrel * mlook_sensitivity;
				mlook_roty += (float)e.motion.yrel * mlook_sensitivity;
			} break;
			}
		}

		if (1.0f <= set_aniso_level && set_aniso_level <= max_aniso_level) {
			aniso_level = set_aniso_level;
			glBindTexture(GL_TEXTURE_2D, tex); CHKGL;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso_level); CHKGL;
			printf("new anisotropic filtering level %f\n", aniso_level);
		}


		int screen_width;
		int screen_height;
		SDL_GL_GetDrawableSize(window, &screen_width, &screen_height);

		gbVec3 campos = gb_vec3(0,0,-900);
		gbMat4 tx;
		{
			const float aspect_ratio = (float)screen_width / (float)screen_height;
			gbMat4 mp,my,mx,mt;
			gb_mat4_perspective(&mp, fovy, aspect_ratio, 1e-4, 1e4);
			gb_mat4_rotate(&my, gb_vec3(1,0,0), mlook_roty);
			gb_mat4_rotate(&mx, gb_vec3(0,1,0), mlook_rotx);
			gb_mat4_translate(&mt, campos);

			gb_mat4_identity(&tx);
			gb_mat4_mul(&tx, &tx, &mp);
			gb_mat4_mul(&tx, &tx, &my);
			gb_mat4_mul(&tx, &tx, &mx);
			gb_mat4_mul(&tx, &tx, &mt);
		}

		glViewport(0, 0, screen_width, screen_height); CHKGL;
		glClearColor(0,0,0.5,0); CHKGL;
		glClear(GL_COLOR_BUFFER_BIT); CHKGL;

		glUseProgram(prg); CHKGL;
		glUniformMatrix4fv(u_transform, 1, 0, (GLfloat*)&tx); CHKGL;

		glEnableVertexAttribArray(0); CHKGL;
		glBindBuffer(GL_ARRAY_BUFFER, vtxbuf_quad); CHKGL;
		glVertexAttribPointer(0, 2, GL_BYTE, GL_FALSE, 2, (const void*)0); CHKGL;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf_quad); CHKGL;

		glBindTexture(GL_TEXTURE_2D, tex); CHKGL;
		glUniform1i(u_texture, 0); CHKGL;

		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_BYTE,
			(const void*)0); CHKGL;

		glUseProgram(0); CHKGL;
		glBindTexture(GL_TEXTURE_2D, 0); CHKGL;
		glDisableVertexAttribArray(0); CHKGL;
		glBindBuffer(GL_ARRAY_BUFFER, 0); CHKGL;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHKGL;

		SDL_GL_SwapWindow(window);
	}

	return EXIT_SUCCESS;

}
