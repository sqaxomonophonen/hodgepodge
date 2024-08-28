#include <stdio.h>
#include <stdarg.h>

#include <GL/gl.h>

#include "SDL.h"
#include "SDL_image.h"



#include "gs_game.h"
#include "gs_menu.h"
#include "font1.h"
#include "fileop.h"
#include "console.h"
#include "audio.h"

#include "main.h"

// viewport will always be 800x600 regardless of screen resolution
#define VIEWPORT_WIDTH 800.0
#define VIEWPORT_HEIGHT 600.0


SDL_Surface *primary_surface;


struct configuration_s {
    int fullscreen;
    int width;
    int bpp;
    int showfps;
    int console_enable;
} configuration;

struct main_control_s
{
    int type;
    
    SDLKey key[4];
};

struct main_vars_s {
    float mouse_x, mouse_y;
    int lmb, mmb, rmb; // left, middle and right mouse button
    
    struct main_control_s control[4];
    
    Uint8 *keys;
} main_vars;



float main_mouse_x(void)
{
    return main_vars.mouse_x;
}

float main_mouse_y(void)
{
    return main_vars.mouse_y;
}

Uint8 *main_keys(void)
{
    return main_vars.keys;
}

int main_control(int player, int control)
{
    switch(main_vars.control[player].type)
    {
        case 1 : //keyboard
            return main_vars.keys[main_vars.control[player].key[control]];
        break;        
        default :
            return 0;
        break;
    }
    return 0;
}

void load_configuration()
{
    configuration.fullscreen=0;
    configuration.width=800;
    configuration.bpp=32;
    configuration.showfps=1;
    configuration.console_enable=1;   
    
    main_vars.control[0].type=1;
    main_vars.control[0].key[CONTROL_LEFT]=SDLK_LEFT;
    main_vars.control[0].key[CONTROL_RIGHT]=SDLK_RIGHT;
    main_vars.control[0].key[CONTROL_THRUST]=SDLK_UP;
    main_vars.control[0].key[CONTROL_FIRE]=SDLK_SPACE;
    
    main_vars.control[1].type=1;
    main_vars.control[1].key[CONTROL_LEFT]=SDLK_a;
    main_vars.control[1].key[CONTROL_RIGHT]=SDLK_d;
    main_vars.control[1].key[CONTROL_THRUST]=SDLK_w;
    main_vars.control[1].key[CONTROL_FIRE]=SDLK_f;
    
    main_vars.control[2].type=0;
    main_vars.control[3].type=0;

}

/*
 *  restart_video
 * starts or restarts the video mode automaticly
 * returns 1 if it didn't succeed (but didn't change the current mode)
 * returns 2 if it didn't succeed and no primary_surface is available (program cannot continue)
 * returns 3 if ZORG¤&¤%%
 */


int restart_video(void)
{
    int width, height, bpp, flags, temp;
    
    // fetch configuration
    width=configuration.width;
    if(((width*3) % 4) != 0) return 3;  // height is hopefully not a fractional value
    height=((width*3)/4);   // 4:3 aspect ratio
    bpp=configuration.bpp;
    flags = SDL_OPENGL | SDL_RESIZABLE;
    if(configuration.fullscreen == 1)
        flags |= SDL_FULLSCREEN;

    // setup SDL video
    temp = SDL_VideoModeOK(width, height, bpp, flags);
    if(temp==0) return 1;

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    primary_surface = SDL_SetVideoMode(width, height, temp, flags);
    if(primary_surface == NULL) return 2;

    // gl specific
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // setup the screen as 800x600, even if the actual resolution is different
    glOrtho( 	0.0,     			//left x
                VIEWPORT_WIDTH,   	        //right x
                VIEWPORT_HEIGHT,   	        //bottom y
                0.0,     			//top y
                0.0,     			//near z
                1.0);    			//far z

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glShadeModel(GL_SMOOTH);

    return 0;
}

void show_fps(void)
{
    int i;
    float fps;
    int static samples[50];

    for(i=48;i>=0;i--)
        samples[i+1]=samples[i];

    samples[0]=SDL_GetTicks();

    fps=0.0;
    for(i=0;i<25;i++)
    fps += 25000.0/(samples[0+i]-samples[25+i]);   
        
    fps /= 25.0;
    
    glColor4f(0.5,0.5,0.5,1.0);
    font1_setscale(0.7);
    font1_printf(5,580,"fps: %.1f\n", fps);
}


int main(int argc, char *argv[])
{
    int temp;
    int new_gamestate, current_gamestate, exiting=0;
    SDL_Event event;

    fileop_init();

    load_configuration();

    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "SDL_Init(): Couldn't initialize SDL: '%s'\n", SDL_GetError());
        return 1;
    }
    
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(300, 30);

    SDL_WM_SetCaption("X", "X");
/*
    if( (temp = audio_init()) != 0)
    {
        fprintf(stderr, "audio_init(): Warning - sound not initialized\n");
    }
    
    audio_set_volume(255,100,100);
    audio_change_music("audio/mus/bliss.ogg");
  */  
    if( (temp = restart_video()) != 0)
    {
        fprintf(stderr, "restart_video(): Couldn't set video mode (%d): '%s'\n", temp, SDL_GetError());
        return 1;
    }

    if(font1_load("./gfx/font1.png") != 0)
    {
        fprintf(stderr, "font1_load(): blghh..\n");
        return 1;
    }
    
    if(console_init() != 0)
    {
        fprintf(stderr, "console_init(): blghh..\n");
        return 1;
    }
    
    new_gamestate = GAMESTATE_TEST;
    current_gamestate = GAMESTATE_NONE;

    while(exiting == 0)
    {
        // event handling
        while(SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT :
                    new_gamestate = GAMESTATE_EXIT;
                break;
                case SDL_MOUSEBUTTONDOWN :
                break;
                case SDL_MOUSEBUTTONUP :
                break;
                case SDL_MOUSEMOTION :
                    main_vars.mouse_x = (event.motion.x * VIEWPORT_WIDTH)/configuration.width;
                    main_vars.mouse_y = (event.motion.y * VIEWPORT_HEIGHT)/((configuration.width*3)/4);
                break; 
                case SDL_KEYDOWN :
                    console_process_key(&event.key.keysym);
                
                    if(event.key.keysym.sym == SDLK_BACKQUOTE)
                        console_toggle();
                break;
                default:
                break;
            }
        }

        main_vars.keys = SDL_GetKeyState(NULL);

        // temporary exit
        //     if(main_vars.keys[SDLK_ESCAPE]) new_gamestate = GAMESTATE_EXIT;
        

        // handling change of gamestate
        if(new_gamestate != GAMESTATE_NONE)
        {
            // shut down previous gamestate
            switch(current_gamestate)
            {
                case GAMESTATE_TEST :
                    game_shutdown();
                break;
                case GAMESTATE_MENU :
                    menu_shutdown();
                break;
                default :
                break;
           }

           // initialize new gamestate
           switch(new_gamestate)
           {
                case GAMESTATE_MENU :
                    menu_init();
                break;
                case GAMESTATE_TEST :
                    game_init();
                break;
                case GAMESTATE_EXIT :
                    exiting = 1;
                break;
                default :
                break;
            }

           current_gamestate = new_gamestate;
           new_gamestate = GAMESTATE_NONE;
        }

        // handle main loop of current gamestate
        switch(current_gamestate)
        {
            case GAMESTATE_TEST :
                if((temp = game_mainloop()) > 0) new_gamestate=temp;
            break;
            case GAMESTATE_GAME :
            break;
            case GAMESTATE_MENU :
                if((temp = menu_mainloop()) > 0) new_gamestate=temp;
            break;
            default :
                new_gamestate = GAMESTATE_NONE;
            break;
        }

        if(configuration.showfps==1) show_fps();
            
        console_mainloop();

        SDL_GL_SwapBuffers();

    }

    console_shutdown();
    audio_shutdown();
    SDL_Quit();

    return 0;
}
