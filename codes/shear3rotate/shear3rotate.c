// cc $(pkg-config --cflags sdl2) shear3rotate.c -o shear3rotate $(pkg-config --libs sdl2)
// rotation with 3 shear transforms
// no pixels ever lost or duplicated
// inspiration from here: https://cohost.org/tomforsyth/post/891823-rotation-with-three

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <SDL.h>

#define PI (3.141592653589793)

struct img {
	int w;
	int h;
	uint8_t* p;
};

static int img_get_n_pixels(struct img* img)
{
	return img->w * img->h;
}

static size_t img_get_size(struct img* img)
{
	return 3*img_get_n_pixels(img);
}

static void img_init(struct img* img, int width, int height, uint8_t* pixels)
{
	memset(img, 0, sizeof *img);
	img->w = width;
	img->h = height;
	img->p = pixels;
}

static void img_init_for_x_shear(struct img* img, struct img* src, uint8_t* pixels)
{
	img_init(img, src->w + src->h, src->h, pixels);
}

static void img_init_for_y_shear(struct img* img, struct img* src, uint8_t* pixels)
{
	img_init(img, src->w, src->w + src->h, pixels);
}

struct s3r {
	int original_width;
	int original_height;
	uint8_t* original_pixels;
	int scratch_width;
	int scratch_height;
	size_t scratch_sz;
	uint8_t* scratch0;
	uint8_t* scratch1;
};

struct s3r* s3r_new(int width, int height, uint8_t* pixels_rgb)
{
	// for shearing in [-1;1] range...
	//   X-shearing a W×H image requires (W+H)×H space
	//   Y-shearing a W×H image requires W×(W+H) space
	int scratch_width  = 2*width + 3*height;
	int scratch_height = 1*width + 2*height;
	struct s3r* o = calloc(1, sizeof *o);
	o->original_width = width;
	o->original_height = height;
	o->original_pixels = pixels_rgb; // XXX do a copy?
	o->scratch_width = scratch_width;
	o->scratch_height = scratch_height;
	o->scratch_sz = 3*(scratch_width*scratch_height);
	o->scratch0 = malloc(o->scratch_sz);
	o->scratch1 = malloc(o->scratch_sz);
	return o;
}

static inline double f64clamp(double v, double min, double max)
{
	return v < min ? min : v > max ? max : v;
}

static void s3r__x_shear(struct s3r* s3r, struct img* dst, struct img* src, double t)
{
	assert(!"TODO");
}

static void s3r__y_shear(struct s3r* s3r, struct img* dst, struct img* src, double t)
{
	assert(!"TODO");
}

static void s3r_rotate(struct s3r* s3r, double radians)
{
	const double x_shear = f64clamp(-tan(radians*0.5), -1.0, 1.0);
	const double y_shear = f64clamp( sin(radians),     -1.0, 1.0);
	struct img step0, step1, step2, step3;

	img_init(&step0, s3r->original_width, s3r->original_height, s3r->original_pixels);
	img_init_for_x_shear(&step1, &step0, s3r->scratch0);
	assert(img_get_size(&step1) < s3r->scratch_sz);
	s3r__x_shear(s3r, &step1, &step0, x_shear);

	img_init_for_y_shear(&step2, &step1, s3r->scratch1);
	assert(img_get_size(&step2) < s3r->scratch_sz);
	s3r__y_shear(s3r, &step2, &step1, y_shear);

	img_init_for_x_shear(&step3, &step2, s3r->scratch0);
	assert(img_get_size(&step3) == s3r->scratch_sz);
	s3r__x_shear(s3r, &step3, &step2, x_shear);
}

static void s3r_blit_to_rgba(struct s3r* s3r, int dst_width, int dst_height, uint32_t* dst_pixels_rgba)
{
	assert(!"TODO");
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <path/to/image>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int img_width, img_height;
	uint8_t* img_pixels = stbi_load(argv[1], &img_width, &img_height, NULL, 3);

	if (img_pixels == NULL) {
		fprintf(stderr, "%s: not an image\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	struct s3r* s3r = s3r_new(img_width, img_height, img_pixels);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_CreateWindowAndRenderer(1920, 1080, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI, &window, &renderer);
	SDL_SetWindowTitle(window, argv[1]);

	int canvas_width = -1;
	int canvas_height = -1;
	uint32_t* canvas_pixels = NULL;
	size_t canvas_sz;
	SDL_Texture* canvas_texture = NULL;

	int frame = 0;
	int exiting = 0;
	while (!exiting) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				exiting = 1;
			} else if (e.type == SDL_KEYDOWN) {
				int sym = e.key.keysym.sym;
				if (sym == SDLK_ESCAPE) {
					exiting = 1;
				}
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

		s3r_rotate(s3r, (double)frame * 0.01); // XXX use time?
		s3r_blit_to_rgba(s3r, canvas_width, canvas_height, canvas_pixels);

		SDL_UpdateTexture(canvas_texture, NULL, canvas_pixels, canvas_width * sizeof(*canvas_pixels));
		SDL_RenderCopy(renderer, canvas_texture, NULL, NULL);

		SDL_RenderPresent(renderer);

		frame++;
	}

	SDL_DestroyWindow(window);

	return EXIT_SUCCESS;
}
