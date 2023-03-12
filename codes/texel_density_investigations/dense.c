#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <SDL.h>

#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"

static int calc_st(gbVec3 H, gbVec3 I, gbVec3 J, gbVec3 xy, float* s, float* t)
{
	const float a = gb_vec3_dot(H, xy);
	const float b = gb_vec3_dot(I, xy);
	const float c = gb_vec3_dot(J, xy);
	if (s) *s = a/c;
	if (t) *t = b/c;
	return c > 0.0f;
}

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_CreateWindowAndRenderer(1920, 1080, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI, &window, &renderer);
	SDL_SetWindowTitle(window, "dense");

	SDL_Texture* canvas_texture = NULL;
	int canvas_width = -1;
	int canvas_height = -1;
	uint32_t* canvas_pixels = NULL;
	size_t canvas_sz;

	int exiting = 0;

	float mlook_rotx = 0.0f;
	float mlook_roty = 0.0f;
	const float mlook_sensitivity = 0.002f;

	int grab = 1;
	SDL_SetRelativeMouseMode(grab);

	const float fovy = 70.0f;

	while (!exiting) {
		SDL_Event e;
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
				}
			} break;


			case SDL_MOUSEMOTION: {
				mlook_rotx += (float)e.motion.xrel * mlook_sensitivity;
				mlook_roty += (float)e.motion.yrel * mlook_sensitivity;
			} break;
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		int screen_width;
		int screen_height;
		SDL_GL_GetDrawableSize(window, &screen_width, &screen_height);

		if (canvas_texture == NULL || canvas_width != screen_width || canvas_height != screen_height) {
			if (canvas_texture != NULL) SDL_DestroyTexture(canvas_texture);
			if (canvas_pixels != NULL) free(canvas_pixels);
			canvas_width = screen_width;
			canvas_height = screen_height;
			canvas_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, canvas_width, canvas_height);
			canvas_sz = sizeof(*canvas_pixels) * canvas_width * canvas_height;
			canvas_pixels = malloc(canvas_sz);
		}

		memset(canvas_pixels, 0, canvas_sz);

		{
			gbVec3 campos = gb_vec3(0,0,-500);

			const float aspect_ratio = (float)canvas_width / (float)canvas_height;
			const float fovx = 2.0f * atanf(tanf(fovy * 0.5f) * aspect_ratio);
			float tan_fovx = tanf(fovx * 0.5f);
			float tan_fovy = tanf(fovy * 0.5f);

			gbMat4 tx;
			{
				gbMat4 mp,my,mx,mt;
				gb_mat4_perspective(&mp, fovy, aspect_ratio, 1e-4, 1e4);
				gb_mat4_rotate(&my, gb_vec3(1,0,0), -mlook_roty);
				gb_mat4_rotate(&mx, gb_vec3(0,1,0), mlook_rotx);
				gb_mat4_translate(&mt, campos);

				gb_mat4_identity(&tx);
				gb_mat4_mul(&tx, &tx, &mp);
				gb_mat4_mul(&tx, &tx, &my);
				gb_mat4_mul(&tx, &tx, &mx);
				gb_mat4_mul(&tx, &tx, &mt);
			}

			// http://nothings.org/gamedev/ray_plane.html
			gbVec4 mP = gb_vec4(0,0,0,1);
			gbVec4 mS = gb_vec4(1,0,0,0);
			gbVec4 mT = gb_vec4(0,1,0,0);

			gbVec4 P,S,T;

			gb_mat4_mul_vec4(&P, &tx, mP);
			gb_mat4_mul_vec4(&S, &tx, mS);
			gb_mat4_mul_vec4(&T, &tx, mT);

			gbVec3 tP = gb_vec3(P.x, P.y, P.w);
			gbVec3 tS = gb_vec3(S.x, S.y, S.w);
			gbVec3 tT = gb_vec3(T.x, T.y, T.w);

			gbVec3 H,I,J;
			gb_vec3_cross(&H, tT, tP);
			gb_vec3_cross(&I, tP, tS);
			gb_vec3_cross(&J, tS, tT);

			uint32_t* px = canvas_pixels;
			const float step_nx = 2.0f / (float)canvas_width;
			const float step_ny = 2.0f / (float)canvas_height;
			float ny = -1.0f;
			for (int y = 0; y < canvas_height; y++) {
				float nx = -1.0f;
				for (int x = 0; x < canvas_width; x++) {
					uint32_t p = 0;
					gbVec3 xy = gb_vec3(nx, ny, 1.0f);
					const float epsilon = 1e-1;
					gbVec3 x_epsilon = gb_vec3(epsilon/(float)canvas_width,0,0);
					gbVec3 y_epsilon = gb_vec3(0,epsilon/(float)canvas_height,0);
					float s,t;
					if (calc_st(H,I,J,xy,&s,&t)) {
						gbVec3 rgb = {0};
						{
							const float ambient = 0.2f;
							gb_vec3_add(&rgb, rgb, gb_vec3(ambient,ambient,ambient));
						}

						float s0,t0;
						float s1,t1;
						gbVec3 xy0, xy1;
						gb_vec3_add(&xy0, xy, x_epsilon);
						gb_vec3_add(&xy1, xy, y_epsilon);
						calc_st(H,I,J,xy0,&s0,&t0);
						calc_st(H,I,J,xy1,&s1,&t1);

						const gbVec2 st = gb_vec2(s,t);
						const gbVec2 st0 = gb_vec2(s0,t0);
						const gbVec2 st1 = gb_vec2(s1,t1);
						gbVec2 dst0,dst1;
						gb_vec2_sub(&dst0, st0, st);
						gb_vec2_sub(&dst1, st1, st);

						const float rdst0 = gb_vec2_mag(dst0) / epsilon;
						const float rdst1 = gb_vec2_mag(dst1) / epsilon;

						const float threshold = 1.0f;
						if (rdst0 > threshold) gb_vec3_add(&rgb, rgb, gb_vec3(0.5,0,0));
						if (rdst1 > threshold) gb_vec3_add(&rgb, rgb, gb_vec3(0,0.5,0));

						const float grid_scale = 1.0f / 256.0f;
						const int gu = (int)floorf(s*grid_scale);
						const int gv = (int)floorf(t*grid_scale);
						const int g = (gu^gv)&1;

						if (g) {
							if (s < 0.0f && t < 0.0f) {
								gb_vec3_add(&rgb, rgb, gb_vec3(1,0,0));
							} else if (s < 0.0f && t > 0.0f) {
								gb_vec3_add(&rgb, rgb, gb_vec3(0,1,0));
							} else if (s > 0.0f && t < 0.0f) {
								gb_vec3_add(&rgb, rgb, gb_vec3(0,0,1));
							} else if (s > 0.0f && t > 0.0f) {
								gb_vec3_add(&rgb, rgb, gb_vec3(1,1,1));
							}
						}

						gbVec3 point_on_plane = gb_vec3(mP.x, mP.y, mP.z);
						gb_vec3_add(&point_on_plane, point_on_plane, gb_vec3(s*mS.x, s*mS.y, s*mS.z));
						gb_vec3_add(&point_on_plane, point_on_plane, gb_vec3(t*mT.x, t*mT.y, t*mT.z));

						float near_distance = gb_vec3_mag(gb_vec3(
							tan_fovx * nx,
							tan_fovy * ny,
							1.0f));

						gbVec3 rr = campos;
						gb_vec3_sub(&rr, rr, point_on_plane);
						float distance = gb_vec3_mag(rr) / near_distance;
						//float distance = gb_vec3_mag(rr);

						{
							const float spacing = 700.f;
							const float width = 0.02f;
							const gbVec3 col = gb_vec3(0.6,0.6,0.6);
							if (fmodf(distance, spacing) < spacing*width) {
								gb_vec3_add(&rgb, rgb, col);
							}
						}

						p =
							  (((int)fmaxf(0.0f, fminf(255.0f, rgb.r * 255.0f))) << 24)
							| (((int)fmaxf(0.0f, fminf(255.0f, rgb.g * 255.0f))) << 16)
							| (((int)fmaxf(0.0f, fminf(255.0f, rgb.b * 255.0f))) << 8)
							| (0xff << 0);
					}
					*(px++) = p;
					nx += step_nx;
				}
				ny += step_ny;
			}
		}

		SDL_UpdateTexture(canvas_texture, NULL, canvas_pixels, canvas_width * sizeof(*canvas_pixels));
		SDL_RenderCopy(renderer, canvas_texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	return EXIT_SUCCESS;

}
