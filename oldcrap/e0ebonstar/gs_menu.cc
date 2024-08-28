#include <GL/gl.h>

#include "SDL.h"
#include "font1.h"
#include "main.h"

struct menu_vars_s {
    int new_state;
} menu_vars;

int eventfilter(const SDL_Event *event)
{
    float mx=main_mouse_x(), my=main_mouse_y();
    switch(event->type)
    {
        case SDL_MOUSEBUTTONDOWN :
            if(mx>200.0 && mx<300.0 && my>200.0 && my<224.0)
                menu_vars.new_state = GAMESTATE_TEST;

            if(mx>200.0 && mx<300.0 && my>224.0 && my<248.0)
                menu_vars.new_state = GAMESTATE_EXIT;
                    
            return 0;
        break;
        case SDL_MOUSEBUTTONUP :
            return 0;
        break;
        default :
        break;
    }  
    return 1;
}


int menu_init(void)
{
    menu_vars.new_state=0;
    SDL_SetEventFilter(&eventfilter);
    return 0;
}

void menu_shutdown(void)
{
    SDL_SetEventFilter(NULL);
}


int menu_mainloop(void)
{
    float mx=main_mouse_x(), my=main_mouse_y();
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    font1_setscale(1.5);
    if(mx>200.0 && mx<300.0 && my>200.0 && my<224.0)
        glColor4f(1.0,0.0,0.0,1.0);
    else
        glColor4f(0.5,0.5,0.5,1.0);
		
    font1_printf(200.0,200.0,"TEST");
   
    if(mx>200.0 && mx<300.0 && my>224.0 && my<248.0)
        glColor4f(1.0,0.0,0.0,1.0);
    else
        glColor4f(0.5,0.5,0.5,1.0);
		
    font1_printf(200.0,224.0,"EXIT");

    return menu_vars.new_state;
}
