#include <math.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "SDL.h"
#include "read_png.h"

#include "main.h"

#include "gs_game.h"
#include "gs_game_i.h"

struct game_vars_s game_vars;
struct game_sprites_s game_sprite;


// maintain gamelogic frequency at 100 hz
void game_calibrate(int frames, int delay)
{
    game_vars.gradient = (int)((float(delay) / float(frames)) * 6553.6);
}

void game_add_hit(struct game_hit_data_s **hit_data, int x, int y, int r, float scale)
{
    struct game_hit_data_s *temphit1;
        
    temphit1 = new struct game_hit_data_s;
    temphit1->x = x;
    temphit1->y = y;
    temphit1->r = r;
    temphit1->scale = scale;
    temphit1->next=*hit_data;
    *hit_data=temphit1; 
}

void game_delete_hit(struct game_hit_data_s *hit_data)
{
    struct game_hit_data_s *temphit1, *temphit2;
        
    temphit1=hit_data;
    
    while(temphit1!=NULL)
    {
        temphit2=temphit1->next;
        delete temphit1;
        temphit1=temphit2;
    }
}

int game_init()
{
    SDL_Surface *temp;
    glGenTextures(GAME_TEXTURE_COUNT, &game_vars.texture[0]);
    float t;
    int i;
    int time;
    
    for(i=0;i<GAME_TEXTURE_COUNT;i++)
    {
        switch(i)
        {
            case 0 : temp = read_png("gfx/grid.png"); break;
            case 1 : temp = read_png("gfx/shade.png"); break;
            case 2 : temp = read_png("gfx/hole.png"); break;
            case 3 : temp = read_png("gfx/station.png"); break;
            case 4 : temp = read_png("gfx/ship.png"); break;
            case 5 : temp = read_png("gfx/particle.png"); break;
            case 6 : temp = read_png("gfx/title.png"); break;
            default : temp = NULL; break;
        }
            
        if(temp!=NULL)
        {
            glBindTexture(GL_TEXTURE_2D, game_vars.texture[i]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, 4, temp->w, temp->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp->pixels);
            SDL_FreeSurface(temp);
        }
    }
    
    // HIT ZONES
    
    game_sprite.station_hit=NULL;
    game_sprite.ship_hit=NULL;
    game_sprite.ship_shot_hit=NULL;
    
    // STATION SPRITES
    game_sprite.station.texture=game_vars.texture[3];
    game_sprite.station_light.texture=game_vars.texture[3];
    game_sprite.station_glow.texture=game_vars.texture[3];
    game_sprite.station_parabola.texture=game_vars.texture[3];
    game_sprite.station_hook.texture=game_vars.texture[3];
    
    game_sprite.station.tx1=0.0f; game_sprite.station.ty1=0.0f;
    game_sprite.station.tx2=0.5f; game_sprite.station.ty2=0.5f;
    game_sprite.station.ox=0.0f; game_sprite.station.oy=0.0f;

    game_sprite.station_light.tx1=0.5f; game_sprite.station_light.ty1=0.0f;
    game_sprite.station_light.tx2=1.0f; game_sprite.station_light.ty2=0.5f;
    game_sprite.station_light.ox=0.0f; game_sprite.station_light.oy=0.0f;
    
    game_sprite.station_glow.tx1=0.54f; game_sprite.station_glow.ty1=0.54f;
    game_sprite.station_glow.tx2=0.96f; game_sprite.station_glow.ty2=0.96f;
    game_sprite.station_glow.ox=0.0f; game_sprite.station_glow.oy=0.0f;
    
    game_sprite.station_parabola.tx1=0.0f; game_sprite.station_parabola.ty1=0.5f;
    game_sprite.station_parabola.tx2=0.25f; game_sprite.station_parabola.ty2=1.0f;
    game_sprite.station_parabola.ox=-1.6f; game_sprite.station_parabola.oy=0.0f;
    
    game_sprite.station_hook.tx1=0.25f; game_sprite.station_hook.ty1=0.5f;
    game_sprite.station_hook.tx2=0.5f; game_sprite.station_hook.ty2=1.0f;
    game_sprite.station_hook.ox=0.0f; game_sprite.station_hook.oy=1.0f;

    t=128.0;
    game_sprite.station.scale=t;
    game_sprite.station_light.scale=t;
    game_sprite.station_glow.scale=t;
    game_sprite.station_parabola.scale=t;
    game_sprite.station_hook.scale=96.0f;
    
    // station hit zones
    game_add_hit(&game_sprite.station_hit,0,0,3,t);

    
    // SHIP SPRITES
    game_sprite.ship_white.texture=game_vars.texture[4];
    game_sprite.ship_colour.texture=game_vars.texture[4];
    game_sprite.ship_thrust.texture=game_vars.texture[4];
    game_sprite.ship_shot.texture=game_vars.texture[4];
    
    game_sprite.ship_white.tx1=0.0f; game_sprite.ship_white.ty1=0.0f;
    game_sprite.ship_white.tx2=0.5f; game_sprite.ship_white.ty2=0.5f;
    game_sprite.ship_white.ox=0.0f; game_sprite.ship_white.oy=0.0f;
    
    game_sprite.ship_colour.tx1=0.5f; game_sprite.ship_colour.ty1=0.0f;
    game_sprite.ship_colour.tx2=1.0f; game_sprite.ship_colour.ty2=0.5f;
    game_sprite.ship_colour.ox=0.0f; game_sprite.ship_colour.oy=0.0f;

    game_sprite.ship_thrust.tx1=0.04f; game_sprite.ship_thrust.ty1=0.54f;
    game_sprite.ship_thrust.tx2=0.46f; game_sprite.ship_thrust.ty2=0.96f;
    game_sprite.ship_thrust.ox=0.0f; game_sprite.ship_thrust.oy=0.0f;
    
    game_sprite.ship_shot.tx1=0.54f; game_sprite.ship_shot.ty1=0.54f;
    game_sprite.ship_shot.tx2=0.96f; game_sprite.ship_shot.ty2=0.96f;
    game_sprite.ship_shot.ox=0.0f; game_sprite.ship_shot.oy=0.0f;
    
    //t=45.0;
    t=52.0;
    game_sprite.ship_white.scale=t;
    game_sprite.ship_colour.scale=t;
    game_sprite.ship_thrust.scale=t;
    game_sprite.ship_shot.scale=t;
    // ship hit zones
    game_add_hit(&game_sprite.ship_hit,0,0,2,t);
    game_add_hit(&game_sprite.ship_hit,-2,2,1,t);
    game_add_hit(&game_sprite.ship_hit,0,2,1,t);
    game_add_hit(&game_sprite.ship_hit,2,2,1,t);
    game_add_hit(&game_sprite.ship_hit,0,0,4,t);
    
    // ship shot hit zones
    game_add_hit(&game_sprite.ship_shot_hit,0,0,1,t);
    game_add_hit(&game_sprite.ship_shot_hit,0,-2,1,t);
    game_add_hit(&game_sprite.ship_shot_hit,0,2,1,t);
    game_add_hit(&game_sprite.ship_shot_hit,0,0,3,t);
    
    //fprintf(stderr, "%d\n", game_sprite.ship_shot_hit->x);

    game_vars.i=0;
    
    
    // BLOB
    game_vars.blobs=1;
    game_vars.blob=new struct blob_s[2];
    game_vars.blob[0].z=0.0;
    
    
    // PLAYER VARS
    game_reset_player(0,1);
    game_reset_player(1,1);
    game_reset_player(2,1);
    game_reset_player(3,1);
    game_vars.player[4].status=0;
    game_vars.player[5].status=0;
    game_vars.player[6].status=0;
    game_vars.player[7].status=0;
    
    game_vars.particle=NULL;
    
    time=SDL_GetTicks();
    
    for(i=0;i<50;i++)
        SDL_GL_SwapBuffers();
    
    game_calibrate(50, SDL_GetTicks()-time);
    
    game_vars.lasttime=SDL_GetTicks();
    game_vars.wait=128;
    
    return 0;
}

void game_shutdown()
{
    glDeleteTextures(GAME_TEXTURE_COUNT, &game_vars.texture[0]);
    game_delete_hit(game_sprite.station_hit);
    game_delete_hit(game_sprite.ship_hit);
    game_delete_hit(game_sprite.ship_shot_hit);
}



int game_mainloop()
{
    float mx, my;
    float z;
    
    // keep the game logic called at 100 hz
    game_vars.counter += game_vars.gradient;
    
    while(game_vars.counter>=65536)
    {
        game_logic();
        game_vars.counter -= 65536;
    }
    
    // calibrate gradient once in a while
    if(game_vars.wait==0)
    {
        game_calibrate(128,SDL_GetTicks() - game_vars.lasttime);
        game_vars.wait=128;
        game_vars.lasttime=SDL_GetTicks();
        
    }
    game_vars.wait--;
    
    mx=main_mouse_x(); my=main_mouse_y();
    game_vars.blob[0].x=mx;
    game_vars.blob[0].y=my;
    //game_vars.blob[0].z=1.0;
    game_vars.blob[0].r=140;
    
    game_vars.blob[1].x=200;
    game_vars.blob[1].y=200;
    game_vars.blob[1].z=-0.8;
    game_vars.blob[1].r=40;
    
    game_drawgrid();
    
    game_drawparticles();
    game_drawsprites();
    
    z=-game_vars.blob[0].z;
    
    // flash when hole explodes
    if(z>0.0)
    {
        glColor4f(1.0f,1.0f,1.0f,1.0f*z*z*z*z*z*z*z*z);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
            glVertex2f(0.0f,0.0f);
            glVertex2f(800.0f,0.0f);
            glVertex2f(800.0f,600.0f);
            glVertex2f(0.0f,600.0f);
        glEnd();
    }
    
    
    //game_vars.title_intensity=1.0;
    //game_drawtitle();
    
    
    game_drawstatus(1.0f);
    
    if(main_keys()[SDLK_ESCAPE])
        return GAMESTATE_EXIT;
    else
        return 0;
}


