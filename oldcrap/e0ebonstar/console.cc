// tilde stands for console! yeah it does


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "SDL.h"
#include <GL/gl.h>

#include "font1.h"
#include "read_png.h"

#include "console.h"

#define CONSOLE_TOGGLE_SPEED    50
#define CONSOLE_ROWS            25   
#define CONSOLE_BLINK_SPEED     8
#define CONSOLE_LINES           300

struct console_vars_s {
    int active;
    int position;   
    int i;

    int cursor_pos, view_pos, prompt_len, scroll_pos;
    unsigned char prompt[257];
    
    GLuint texture;
    
    unsigned char *scrbuf;
} console_vars;


int console_init(void)
{
    int i;
    
    // reset variables
    console_vars.active=0;
    console_vars.position=0;
    
    console_vars.cursor_pos=0;
    console_vars.view_pos=0;
    console_vars.prompt_len=0;
    
    console_vars.i=0;
    
    console_vars.scrbuf = new unsigned char[50*CONSOLE_LINES];
    for(i=0;i<(50*CONSOLE_ROWS);i++)
        console_vars.scrbuf[i] = 0x20;
    
    for(i=0;i<256;i++)
        console_vars.prompt[i]=' ';
    
    // setup texture
    SDL_Surface *temp;
    glGenTextures(1, &console_vars.texture);
    
    temp = read_png("gfx/con1.png");
    if(temp == NULL || temp->w != 256 || temp->h != 256) return 1;
    
    glBindTexture(GL_TEXTURE_2D, console_vars.texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, temp->w, temp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp->pixels);
    
    SDL_FreeSurface(temp);
    
    return 0;
}

void console_shutdown(void)
{
    glDeleteTextures(2, &console_vars.texture);
}

void console_toggle(void)
{
    if(console_vars.active==0)
        console_vars.active=1;
    else
        console_vars.active=0;
}

int console_delete_char(int pos)
{
    int i;
    
    if(console_vars.prompt_len == pos)
        return 1;
    
    if(pos>=0 && pos<=255)
    {    
        for(i=pos;i<255;i++)
            console_vars.prompt[i] = console_vars.prompt[i+1];
        console_vars.prompt[255] = ' ';
    }
    else
        return 1;
  
    console_vars.prompt_len--;
    
    return 0;
}

int console_insert_char(int pos, unsigned char c)
{
    int i;
    if(console_vars.prompt[255] != ' ' || pos >= 255 || pos < 0 || console_vars.prompt_len==255)
        return 1;
    
    for(i=console_vars.prompt_len;i>pos;i--)
        console_vars.prompt[i] = console_vars.prompt[i-1];
    
    console_vars.prompt[pos] = c;   

    console_vars.prompt_len++;
    
    return 0;
}

void console_inc_cursor_pos(void)
{
    console_vars.cursor_pos++;
    if(console_vars.cursor_pos>(console_vars.view_pos+40))
        console_vars.view_pos=console_vars.cursor_pos-41;
    
    if(console_vars.view_pos>(255-50))
        console_vars.view_pos=255-50;
}

void console_dec_cursor_pos(void)
{
    console_vars.cursor_pos--;
    if(console_vars.cursor_pos<(console_vars.view_pos+5))
        console_vars.view_pos=console_vars.cursor_pos-4;

    if(console_vars.view_pos<0)
        console_vars.view_pos=0;
}

void console_auto_finish(void)
{
    // TODO:
}

void console_process_key(SDL_keysym *keysym)
{
    int i;
    char c;
    if(console_vars.active==1)
    {
        if(keysym->sym == SDLK_LEFT && console_vars.cursor_pos>0)
            console_dec_cursor_pos();
        
        if(keysym->sym == SDLK_RIGHT && console_vars.cursor_pos!=console_vars.prompt_len)
            console_inc_cursor_pos();
        
        if(keysym->sym == SDLK_BACKSPACE && console_vars.cursor_pos>0)
            if(console_delete_char(console_vars.cursor_pos-1)==0)
                console_dec_cursor_pos();
            
        if(keysym->sym == SDLK_HOME)
        {
            console_vars.cursor_pos=0;
            console_vars.view_pos=0;
        }
        
        if(keysym->sym == SDLK_END)
        {
            console_vars.cursor_pos=console_vars.prompt_len;
            console_dec_cursor_pos();
            console_inc_cursor_pos();
        }

        if(keysym->sym == SDLK_DELETE && console_vars.prompt_len>0 && console_vars.cursor_pos!=console_vars.prompt_len)
            console_delete_char(console_vars.cursor_pos);
        
        if(keysym->sym == SDLK_TAB && console_vars.prompt_len>0)
            console_auto_finish();
        
        if(keysym->sym == SDLK_RETURN)
        {
            console_vars.prompt[console_vars.prompt_len]='\0';
            
            console_parse_string((const char *)console_vars.prompt);
            
            for(i=0;i<256;i++)
                console_vars.prompt[i]=' ';
            console_vars.prompt_len=0;
            console_vars.cursor_pos=0;
            console_vars.view_pos=0;
        }
            
        if((keysym->unicode & 0xFF80) == 0)
        {
            c=keysym->unicode & 0x7F;

            if(c>31)
                if(console_insert_char(console_vars.cursor_pos,c)==0)
                    console_inc_cursor_pos();
        }
    }
}

void console_mainloop(void)
{
    int i,j;
    unsigned char c;
    float x1,y1,y2,y3,y4;
    
    if(console_vars.active==1)
    {
        console_vars.position+=CONSOLE_TOGGLE_SPEED;
        if(console_vars.position>1000) console_vars.position=1000;
    }
    else
    {
        console_vars.position-=CONSOLE_TOGGLE_SPEED;
        if(console_vars.position<0) console_vars.position=0;
    }
    
    // display
    if(console_vars.position != 0)
    {
        i=console_vars.i;
        glBindTexture(GL_TEXTURE_2D, console_vars.texture);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f,0.1f,0.2f,0.5f);
        x1=i / 1000.0;
        y1=-i / 5000.0;
        y4=(3.0*((CONSOLE_ROWS*16.0)/600.0));
        y2=(1000-console_vars.position)/(1000.0/y4);
        y3=CONSOLE_ROWS*16.0*(console_vars.position/1000.0);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0+x1, 0.0+y1+y2); glVertex2f(0.0, 0.0);
            glTexCoord2f(4.0+x1, 0.0+y1+y2); glVertex2f(800.0, 0.0);
            glTexCoord2f(4.0+x1, y4+y1); glVertex2f(800.0, y3);
            glTexCoord2f(0.0+x1, y4+y1); glVertex2f(0.0, y3);
        glEnd();
        
        glBlendFunc(GL_ONE, GL_ONE);
        glColor4f(0.2f,0.0f,0.0f,1.0f);
        x1=i / 1200.0;
        y1=-i / 5200.0;
        glBegin(GL_QUADS);
            glTexCoord2f(0.0+x1, 0.0+y1+y2); glVertex2f(0.0, 0.0);
            glTexCoord2f(4.0+x1, 0.0+y1+y2); glVertex2f(800.0, 0.0);
            glTexCoord2f(4.0+x1, y4+y1); glVertex2f(800.0, y3);
            glTexCoord2f(0.0+x1, y4+y1); glVertex2f(0.0, y3);
        glEnd();
        
        glColor4f(0.6f,0.6f,0.6f,1.0f);
        glBlendFunc(GL_ONE, GL_ONE);
        font1_setscale(1.0);

        for(i=0;i<(CONSOLE_ROWS-1);i++)
            for(j=0;j<50;j++)
                font1_print_char(j*16.0,y3-16.0*(i+2),console_vars.scrbuf[j+(i+console_vars.scroll_pos)*50]);
            
        glColor4f(0.6f,1.0f,1.0f,1.0f);
        glBlendFunc(GL_ONE, GL_ONE);
        for(j=0;j<50;j++)
        {
            i=j+console_vars.view_pos;
            if(i==0)
                c='>';
            else if(i>1)
                c=console_vars.prompt[i-2];
            else
                c=' ';
            font1_print_char(j*16.0,y3-16.0,c);
        }
        
        i=console_vars.cursor_pos-console_vars.view_pos+2;
        if((SDL_GetTicks() & ((1 << CONSOLE_BLINK_SPEED))) && i>=0 && i<50)
            font1_print_char(i*16.0,(CONSOLE_ROWS-1)*16.0+y3-16.0*CONSOLE_ROWS,'_');
    }
    
    // control
    if(console_vars.active==1)
    {
    }
    
    console_vars.i++;

}

void console_print(const char *s)
{
    int i,j,k;
    int lines=(strlen(s)/50)+1;
    int len=strlen(s);
    
    for(k=0;k<lines;k++)
        for(i=(CONSOLE_LINES-1);i>0;i--)
            for(j=0;j<50;j++)
                console_vars.scrbuf[i*50+j]=
                    console_vars.scrbuf[(i-1)*50+j];
        
    for(j=0;j<(50*lines);j++)
        console_vars.scrbuf[j]=' ';
    
    for(i=0;i<lines;i++)
        for(j=0;j<50;j++)
           if(i*50+j < len)
                console_vars.scrbuf[(lines-i-1)*50+j]=s[i*50+j];
}

void console_printf(const char *fmt, ...)
{
    char text[256];   
    va_list ap;
    
    va_start(ap, fmt);   
    vsprintf(text, fmt, ap); 
    va_end(ap);
    
    console_print(text);
}

void console_parse_string(const char *s)
{
    console_printf("%s",s);
}

// TODO: finish these functions

// register a keyword to a group of keywords (groups are for house keeping, not visible to user)
// link a function to the keyword
void console_register_keyword(const char *keyword, const char *group, void(*func)(void))
{
}

// delete all keywords which belongs to a group
void console_unregister_group(const char *group)
{
}

// set a variable to value
void console_set_var(const char *var, const char *value)
{
}

// return the value of a variable, or null if it dosn't exist
const char *console_get_var(const char *var)
{
    return NULL;
}

// get the next argument from an invoked keyword
// return null if there are no more arguments
const char *console_next_arg(void)
{
    return NULL;
}


