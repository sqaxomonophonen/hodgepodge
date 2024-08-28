/*
 *  very simple functions for drawing 16x16 fonts stored in a 256x256 image
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "read_png.h"
#include "SDL.h"
#include <GL/gl.h>

struct font1_vars_s {
    GLuint texture;
    float scale;
} font1_vars;


void font1_setscale(float s)
{
    font1_vars.scale = s;
}

int font1_load(const char *filename)
{
    SDL_Surface *temp;
    glGenTextures(1, &font1_vars.texture);
    
    temp = read_png(filename);
    if(temp == NULL || temp->w != 256 || temp->h != 256) return 1;
    
    glBindTexture(GL_TEXTURE_2D, font1_vars.texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, temp->w, temp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp->pixels);
    
    SDL_FreeSurface(temp);
    
    font1_setscale(1.0);
    
    return 0;
}

void font1_print_char(float x, float y, unsigned char c)
{
    unsigned int fx,fy;
    
    fx = (c % 16)*16;
    fy = (c / 16)*16;
    
    glBindTexture(GL_TEXTURE_2D, font1_vars.texture);
    glBlendFunc(GL_ONE,GL_ONE);
    
    glBegin(GL_QUADS);
         glTexCoord2f(fx/256.0,fy/256.0);
         glVertex2f(x, y);
        
         glTexCoord2f((fx+15)/256.0,fy/256.0);
         glVertex2f(x+15.0*font1_vars.scale, y);
        
         glTexCoord2f((fx+15)/256.0,(fy+15)/256.0);
         glVertex2f(x+15.0*font1_vars.scale, y+15.0*font1_vars.scale);
        
         glTexCoord2f((fx)/256.0,(fy+15)/256.0);
         glVertex2f(x, y+15.0*font1_vars.scale);
    glEnd();
}

void font1_printf(float x, float y, const char *fmt, ...)
{
    unsigned int i;
    char text[256];   
    va_list ap;
    
    va_start(ap, fmt);   
    vsprintf(text, fmt, ap); 
    va_end(ap);

    for (i=0;i<strlen(text);i++)
        font1_print_char(x+i*16.0*font1_vars.scale,y,text[i]);
}

void font1_printf_r(float x, float y, const char *fmt, ...)
{
    int i;
    int l;
    
    char text[256];   
    va_list ap;
    
    va_start(ap, fmt);   
    vsprintf(text, fmt, ap); 
    va_end(ap);
    
    l=strlen(text);
    
    for (i=0;i<l;i++)
        font1_print_char(x+(i-l)*16.0*font1_vars.scale,y,text[i]);
}

