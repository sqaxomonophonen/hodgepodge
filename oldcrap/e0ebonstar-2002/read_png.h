#ifndef READ_PNG_H

/*
"restoration" of read_png(); see trash/png1.*; was easier to port to
stb_image.h than fixing the libpng crap
*/

#include <stdio.h>
#include <assert.h>
#include "SDL.h"
#include "stb_image.h"

static inline SDL_Surface *read_png(const char* path)
{
	int width, height, n_comp;
	stbi_uc* image = stbi_load(path, &width, &height, &n_comp, 0);
	printf("%s: %dx%dc%d\n", path, width, height, n_comp);

	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	const Uint32 rmask = 0xff000000;
	const Uint32 gmask = 0x00ff0000;
	const Uint32 bmask = 0x0000ff00;
	const Uint32 amask = 0x000000ff;
	#else
	const Uint32 rmask = 0x000000ff;
	const Uint32 gmask = 0x0000ff00;
	const Uint32 bmask = 0x00ff0000;
	const Uint32 amask = 0xff000000;
	#endif

	SDL_Surface* surface;
	switch (n_comp) {
	case 3: {
		surface = SDL_CreateRGBSurface(
			SDL_SWSURFACE,
			width,
			height,
			24,
			rmask,gmask,bmask,amask);
	}	break;
	case 4: {
		surface = SDL_CreateRGBSurface(
			SDL_SWSURFACE,
			width,
			height,
			32,
			rmask,gmask,bmask,amask);
	}	break;
	default: assert(!"unhandled case");
	}

	stbi_uc* ip = image;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			for (int i = 0; i < n_comp; i++) {
				*(((stbi_uc*)surface->pixels) + x*n_comp + y*surface->pitch + i) = *(ip++);
			}
			//*(((stbi_uc*)surface->pixels) + x*n_comp + (height-y-1)*surface->pitch) = *(ip++);
			//*(((stbi_uc*)surface->pixels) + x*n_comp + (height-y-1)*surface->pitch) = 255;
		}
		//row_pointers[row] = pixels + pitch*row;
	}

	free(image);

	return surface;
}

#define READ_PNG_H
#endif
