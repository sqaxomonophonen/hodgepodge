// functions for reading a png image to a SDL surface and writing surfaces to png

#include <malloc.h>

#include "SDL.h"
#include <png.h>

#include "fileop.h"

#include "png1.h"

struct png_vars_struct {
    fileop_handle *fp;
} png_vars;

static void pngtest_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    png_size_t check;
    check = (png_size_t)fileop_read(data, length, png_vars.fp);
    if (check != length)
    {
        png_error(png_ptr, "Read Error!");
    }
}

// custom memory handling functions
png_voidp png_custom_malloc(png_structp png_ptr, png_size_t size)
{
    return malloc(size);
}

void png_custom_free(png_structp png_ptr, png_voidp ptr)
{
    free(ptr);
}


#define png_HEADER_READ_BYTES   4

SDL_Surface *read_png(const char *filename)
{
    png_byte buf[png_HEADER_READ_BYTES];

    // open file
    if ((png_vars.fp = fileop_open(filename, "rb")) == NULL)
    {
        fprintf(stderr, "read_png() - file could not be opened\n");
        return NULL;
    }

   
    // check header
    if (fileop_read(buf, png_HEADER_READ_BYTES, png_vars.fp) != png_HEADER_READ_BYTES)
    {
        fprintf(stderr, "read_png() - error reading file\n");
        fileop_close(png_vars.fp);
        return NULL;
    }
    
    if (png_sig_cmp(buf, (png_size_t)0, png_HEADER_READ_BYTES) != 0)
    {
        fprintf(stderr, "read_png() - file is not png\n");
        fileop_close(png_vars.fp);
        return NULL;
    }   
    
    // create png read structure
    
    png_structp png_ptr;
    
    png_ptr = png_create_read_struct_2(
            PNG_LIBPNG_VER_STRING, 
            png_voidp_NULL,
            png_error_ptr_NULL, 
            png_error_ptr_NULL, 
            png_voidp_NULL, 
            png_custom_malloc, 
            png_custom_free); 
    

    
    if(!png_ptr)
    {
        fprintf(stderr, "read_png() - internal error 1\n");
        fileop_close(png_vars.fp);
        return NULL;
    }    
    
    // create png info structure
    png_infop info_ptr;
    
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        fprintf(stderr, "read_png() - internal error 2\n");
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        fileop_close(png_vars.fp);
        return NULL;
    }
    
    // create .. uh.. something
    png_infop end_info;
    
    end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        fprintf(stderr, "read_png() - internal error 3\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        fileop_close(png_vars.fp);
        return NULL;
    }
    
    // no fucking clue&¤%"
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fprintf(stderr, "read_png() - internal error 4\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fileop_close(png_vars.fp);
        return NULL;
    }
        
    // set up custom data read function
    png_set_read_fn(png_ptr, NULL, pngtest_read_data);
    
    // notify png functions of the current file position
    png_set_sig_bytes(png_ptr, png_HEADER_READ_BYTES);
    
    // read image information
    png_read_info(png_ptr, info_ptr);
    
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    
    // fetch image information
    png_get_IHDR(   png_ptr, 
                    info_ptr, 
                    &width, 
                    &height, 
                    &bit_depth, 
                    &color_type,
                    &interlace_type, 
                    int_p_NULL, 
                    int_p_NULL);
    
    // some types unsupported. return error 
    if(bit_depth != 8)
    {
        fprintf(stderr, "read_png() - file type not supported (1)\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fileop_close(png_vars.fp);
        return NULL;
    }
    
    
    SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;
    
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    
    
    // create SDL surface according to png information
    if(color_type == PNG_COLOR_TYPE_RGB)
    {    
        surface = SDL_CreateRGBSurface(
                            SDL_SWSURFACE,
                            width,
                            height,
                            24,
                            rmask,gmask,bmask,amask);
    }
    else if(color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    {
        surface = SDL_CreateRGBSurface(
                            SDL_SWSURFACE,
                            width,
                            height,
                            32,
                            rmask,gmask,bmask,amask);

    }
    else
    {
        fprintf(stderr, "read_png() - file type not supported (2)\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fileop_close(png_vars.fp);
        return NULL;
    }
    
    // read data into surface
    png_bytep row_pointers[height];
    png_uint_32 row;
    png_uint_32 pitch;
    png_bytep pixels;
    
    pitch = (png_uint_32)surface->pitch;
    pixels = (png_bytep)surface->pixels;
    
    for (row=0; row<height; row++)
        row_pointers[row] = pixels + pitch*row;
    
    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, end_info);
    
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    fileop_close(png_vars.fp);

    // TEMP!
    return surface;
}

int write_png(const char *filename, SDL_Surface *surface)
{
    // TODO: isn't it obvious?
    return 0;
}




