#include <math.h>
#include <stdlib.h>
#include <GL/gl.h>
#include "SDL.h"

#include "main.h"

#include "gs_game.h"
#include "gs_game_i.h"

void game_advance(float *x, float *y, int dir, int distance)
{
    float r=(dir/65536.0f)*6.283f-(6.283f/4.0f);
    (*x) += cos(r)*(distance/(25.0f*256.0f));
    (*y) += sin(r)*(distance/(25.0f*256.0f));
}

// bounces something if it hits a border
int game_bounce(float x, float y, int *dir)
{
    *dir&=65535;
    int ret=0;
    
    if(
        (*dir>=0 && *dir<32768 && x>GAME_BOARD_RIGHT) ||
        (*dir>=32768 && *dir<65536 && x<GAME_BOARD_LEFT))
    {
        *dir=-*dir;
        ret=1;
    }
    if(
        (*dir>=16384 && *dir<49152 && y>GAME_BOARD_BOTTOM) ||
        ((*dir>=49152 || *dir<16386) && y<GAME_BOARD_TOP))
    {
        *dir=32768-*dir;
        ret=1;
    }
    
    *dir&=65535;
    return ret;
}

void game_turn_towards(int *startangle, int a, int strength)
{
    int angle;
    int str,aa;
    
    angle = *startangle & 65535;
    aa=abs(angle-a);
    if(angle>a)
        if(aa>32768)
            str=(-strength*(32768-(65536-aa)))/32768;
        else
            str=(strength*(32768-aa))/32768;
    else 
        if(aa>32768)
            str=(strength*(32768-(65536-aa)))/32768;
        else
            str=(-strength*(32768-aa))/32768;
        
    angle += str;
    angle &= 65535;
        
    *startangle = angle;    
}

void game_SUCK(int *startangle, float x, float y, float r)
{
    int a=(int((65536.0/6.283)*atan2(x,-y)))&65535;
    int strength=GAME_HOLE_SUCK_POWER1+(int)((1.0-r)*GAME_HOLE_SUCK_POWER2);
    
    game_turn_towards(startangle, a, strength);
}

void game_reset_player(int i, int first)
{
    switch(i)
    {
        case 0 :
            game_vars.player[i].ship_x=GAME_BOARD_RIGHT-10.0;
            game_vars.player[i].ship_y=GAME_BOARD_BOTTOM-10.0;
        break;
        case 1 :
            game_vars.player[i].ship_x=GAME_BOARD_LEFT+10.0;
            game_vars.player[i].ship_y=GAME_BOARD_TOP+10.0;
        break;
        case 2 :
            game_vars.player[i].ship_x=GAME_BOARD_LEFT+10.0;
            game_vars.player[i].ship_y=GAME_BOARD_BOTTOM-10.0;
        break;
        case 3 :
            game_vars.player[i].ship_x=GAME_BOARD_RIGHT-10.0;
            game_vars.player[i].ship_y=GAME_BOARD_TOP+10.0;
        break;
    }
    game_vars.player[i].ship_dir=65536-8192;
    if(i&1)
        game_vars.player[i].ship_dir += 32768;
    if(i&2)
        game_vars.player[i].ship_dir += 16384;
    
    game_vars.player[i].ship_dir &= 65535;
    
    game_vars.player[i].ship_speed=0;
    game_vars.player[i].thrust=0;
    game_vars.player[i].shot_ttl=0;
    game_vars.player[i].firehold=0;
    
    game_vars.player[i].how_dead=0.0;
    game_vars.player[i].shot_how_dead=0.0;    
    game_vars.player[i].status=1;
    
    game_vars.player[i].hit_speed=0;
    game_vars.player[i].hit_owner=-1;
    
    game_vars.player[i].control_owner = 0;
    
    if(first==1)
    {
        game_vars.player[i].lives=5;
        game_vars.player[i].score=0;
    }
}

void game_add_particle(float x, float y, int dir, int speed)
{
    game_particle_s *temppar1, *temppar2;

    temppar1=new struct game_particle_s;
    temppar1->x=x;
    temppar1->y=y;
    temppar1->dir=dir;
    temppar1->speed=speed;
    temppar1->ttl=rand()&127;
    temppar1->colour=rand();
    
    temppar2=game_vars.particle;
    temppar1->next=temppar2;
    game_vars.particle=temppar1;
}

// add n particles spraying out from (x,y) in dir direction
void game_add_spray_particles(float x, float y, int dir, int n)
{
    int i;
    
    for(i=0;i<n;i++)
    {
    	float a = ((float) rand()) / ((float) RAND_MAX);
    	float b = ((float) rand()) / ((float) RAND_MAX) - .5f;
    	
        game_add_particle(
            x,y,
	    (int) (dir + a * 4000.0f),
	    (int) (b * 4000.0f));
            //dir+(rand()-(RAND_MAX/2))/5, 
            //rand()/5);
    }
}

void game_add_explosion_particles(float x, float y, int n)
{
    int i;
    
    for(i=0;i<n;i++)
    {
    	float a = ((float) rand()) / ((float) RAND_MAX);
    	float b = ((float) rand()) / ((float) RAND_MAX) - .5f;
        game_add_particle(
            x,y,
	    (int) (a * 65536.0f),
	    (int) (b * 4000.0f));
            /*(rand()*65536)/RAND_MAX, 
            rand()/5);*/
    }
}

int game_control(int i, int c)
{
    if(game_vars.player[i].control_owner == 0)
        return main_control(i,c);
        
    return 0;
    
}

int game_hit_detect(
    struct game_hit_data_s *h1, float x1, float y1, int dir1, float st1,
    struct game_hit_data_s *h2, float x2, float y2, int dir2, float st2)
{
    struct game_hit_data_s *temp1, *temp2;
    int first;
    float cx1,cy1,cx2,cy2,dx,dy,d,r,s;
    float s1=st1/24.0, s2=st2/24.0;
    
    temp1=h1; 
    first=0;
    while(temp1 != NULL)
    {
        temp2=h2;
        while(temp2 != NULL)
        {
            r=(dir1*6.283)/65536.0;
            s=s1*temp1->scale;
            cx1=x1 + cos(r)*(float)temp1->x*s - sin(r)*(float)temp1->y*s; 
            cy1=y1 + sin(r)*(float)temp1->x*s + cos(r)*(float)temp1->y*s;
            
            r=(dir2*6.283)/65536.0;
            s=s2*temp2->scale;
            cx2=x2 + cos(r)*(float)temp2->x*s - sin(r)*(float)temp2->y*s; 
            cy2=y2 + sin(r)*(float)temp2->x*s + cos(r)*(float)temp2->y*s;

            dx=cx1-cx2;
            dy=cy1-cy2;
            d=sqrt(dx*dx+dy*dy);
            
            if(d < (temp1->r*s1*temp1->scale+temp2->r*s2*temp2->scale))
            {
                if(first != 0)
                {
                    return 1;
                }
                else
                {
                    temp2 = temp2->next;
                    temp1 = temp1->next;
                    first=1;
                }
            }
            else
            {
                if(first == 0)
                    return 0;
            }
            if(first != 0) temp2 = temp2->next;
        }
        if(first !=0) temp1 = temp1->next;
    }
    
    return 0;
}

void game_logic(void)
{
    Uint8 *keys=main_keys();
    
    float r,x,y;
    int i,j,temp;
    
    // temp
    if(keys[SDLK_DOWN] && game_vars.blob[0].z>-1.0f)
        game_vars.blob[0].z -= 0.01;
    else if(game_vars.blob[0].z<1.0f)
        game_vars.blob[0].z += 0.01f;
    
    
    // for each player...
    for(i=0;i<GAME_SHIPS_MAX;i++)
    {
        if(game_vars.player[i].status > 0)
        {
            if(game_control(i,CONTROL_LEFT))
                game_vars.player[i].ship_dir -= GAME_PLAYER_TURN_SPEED;
            if(game_control(i,CONTROL_RIGHT))
                game_vars.player[i].ship_dir += GAME_PLAYER_TURN_SPEED;
            
            if(game_vars.player[i].ship_dir > 65535) game_vars.player[i].ship_dir-=65536;
            if(game_vars.player[i].ship_dir < 0) game_vars.player[i].ship_dir+=65536;
            
            if(!game_control(i,CONTROL_THRUST))
                game_vars.player[i].thrust=0;
            
            if(game_control(i,CONTROL_THRUST) && game_vars.player[i].ship_speed<GAME_PLAYER_TOP_SPEED)
            {
                game_vars.player[i].ship_speed+=GAME_PLAYER_ACCEL;
                game_vars.player[i].thrust=1;
            }
            else if(game_vars.player[i].ship_speed>0)
            {
                game_vars.player[i].ship_speed-=GAME_PLAYER_DEACCEL;
            }
            
            if(game_control(i,CONTROL_FIRE) && game_vars.player[i].firehold!=1)
            {
                game_vars.player[i].shot_x=game_vars.player[i].ship_x;
                game_vars.player[i].shot_y=game_vars.player[i].ship_y;
                game_vars.player[i].shot_dir=game_vars.player[i].ship_dir;
                game_vars.player[i].shot_ttl=GAME_PLAYER_SHOT_TTL;
                game_vars.player[i].shot_bounce=0;
                game_vars.player[i].firehold=1;
                game_vars.player[i].shot_how_dead=0.0;
            }
            
            if(!game_control(i,CONTROL_FIRE))
            {
                game_vars.player[i].firehold=0;
            }
            
            for(j=0;j<game_vars.blobs;j++)
            {
                x=game_vars.player[i].ship_x-game_vars.blob[j].x;
                y=game_vars.player[i].ship_y-game_vars.blob[j].y;
                r=sqrt(x*x+y*y)/game_vars.blob[j].r;
                if(r<1.0)
                {
                    game_vars.player[i].how_dead=0.0;
                    if(game_vars.blob[j].z>0.0)
                    {
                        game_vars.player[i].how_dead=1.0-r;
                        game_SUCK(&game_vars.player[i].ship_dir,x,y,r);
                        game_vars.player[i].ship_speed+=GAME_PLAYER_ACCEL*2;
                        if(r<0.2)
                        {
                            // PLAYER DIES
                            game_vars.player[i].status=-GAME_PLAYER_DEATH_TIME;
                            game_vars.player[i].lives--;
                            if(game_vars.player[i].hit_owner != -1)
                                game_vars.player[game_vars.player[i].hit_owner].score+=GAME_SCORE_KILL_NORMAL;
                        }
                    }
                    else
                    {
                    }
                }
                else
                {
                    game_vars.player[i].how_dead=0.0;
                }
                    
                
                x=game_vars.player[i].shot_x-game_vars.blob[j].x;
                y=game_vars.player[i].shot_y-game_vars.blob[j].y;
                r=sqrt(x*x+y*y)/game_vars.blob[j].r;
                if(r<1.0)
                {
                    game_vars.player[i].shot_how_dead=0.0;
                    if(game_vars.blob[j].z>0.0)
                    {
                        game_vars.player[i].shot_how_dead=1.0-r;
                        game_SUCK(&game_vars.player[i].shot_dir,x,y,r);
                        if(r<0.2)
                        {
                            game_vars.player[i].shot_ttl=0;
                        }
                    }
                    else
                    {
                    }
                }
                else
                {
                    game_vars.player[i].shot_how_dead=0.0;
                }
            }
            
            game_advance(
                &game_vars.player[i].ship_x, 
                &game_vars.player[i].ship_y,
                game_vars.player[i].ship_dir,
                game_vars.player[i].ship_speed);
            
            // handle physics while being shot at
            if(game_vars.player[i].hit_speed>0)
            {
                // move in hit direction
                game_advance(
                    &game_vars.player[i].ship_x, 
                    &game_vars.player[i].ship_y,
                    game_vars.player[i].hit_dir,
                    game_vars.player[i].hit_speed);
                game_vars.player[i].hit_speed-=GAME_PLAYER_DEACCEL;
                
                // turn the ship towards hit direction
                game_turn_towards(
                    &game_vars.player[i].ship_dir,
                    (game_vars.player[i].hit_dir+32768)&65535,
                    1000);
                
                // bounce if needed
                if(game_bounce(
                    game_vars.player[i].ship_x,
                    game_vars.player[i].ship_y,
                    &game_vars.player[i].hit_dir)!=0)
                {}
                    
                // fireworks
                game_add_spray_particles(
                    game_vars.player[i].ship_x,
                    game_vars.player[i].ship_y,
                    game_vars.player[i].hit_dir,
                    game_vars.player[i].hit_speed/32768);
                game_add_explosion_particles(
                    game_vars.player[i].ship_x,
                    game_vars.player[i].ship_y,
                    game_vars.player[i].hit_speed/16384);

            }
            
            // bounce ship if there's a need for bouncing
            if(game_vars.player[i].ship_speed>0)
                if(game_bounce(
                    game_vars.player[i].ship_x,
                    game_vars.player[i].ship_y,
                    &game_vars.player[i].ship_dir)!=0)
                {}
            

            // handle shot
            if(game_vars.player[i].shot_ttl>0)
            {
                game_advance(
                    &game_vars.player[i].shot_x, 
                    &game_vars.player[i].shot_y,
                    game_vars.player[i].shot_dir,
                    GAME_PLAYER_SHOT_SPEED);
                game_add_spray_particles(
                        game_vars.player[i].shot_x,
                        game_vars.player[i].shot_y,
                        (game_vars.player[i].shot_dir+32768)&65535,
                        1);
                
                temp=(game_vars.player[i].shot_dir+32768)&65535;
                
                if(
                    game_bounce(game_vars.player[i].shot_x,
                    game_vars.player[i].shot_y,
                    &game_vars.player[i].shot_dir)==1)
                {
                    
                    // fireworks when hitting border
                    game_add_spray_particles(
                        game_vars.player[i].shot_x,
                        game_vars.player[i].shot_y,
                        game_vars.player[i].shot_dir,
                        50);
                    game_add_spray_particles(
                        game_vars.player[i].shot_x,
                        game_vars.player[i].shot_y,
                        temp,
                        5);
                    game_add_explosion_particles(
                        game_vars.player[i].shot_x,
                        game_vars.player[i].shot_y,
                        5);
                    game_vars.player[i].shot_bounce++;
                }
                // decrement time to live
                game_vars.player[i].shot_ttl--;
            }
        }
        // wait while player is dead
        else if (game_vars.player[i].status < 0)
        {
            game_vars.player[i].status++;
        }
        
        // reset player after delay
        if(game_vars.player[i].status == -1 && game_vars.player[i].lives>0)
        {
            game_reset_player(i,0);
        }
    }
    
    // SHIP <-> SHOT hit detection
    for(i=0;i<GAME_SHIPS_MAX;i++)
    for(j=0;j<GAME_SHIPS_MAX;j++)
    {
        if(game_vars.player[j].shot_ttl>0 && i!=j && game_vars.player[i].status>0)
            if(
            game_hit_detect(
                game_sprite.ship_hit,
                game_vars.player[i].ship_x,
                game_vars.player[i].ship_y,
                game_vars.player[i].ship_dir,
                1.0,
        
                game_sprite.ship_shot_hit,
                game_vars.player[j].shot_x,
                game_vars.player[j].shot_y,
                game_vars.player[j].shot_dir,
                1.0)==1)        
               
        {
            game_vars.player[j].score += GAME_SCORE_HIT_NORMAL; // credit shooter
            game_vars.player[j].shot_ttl=0; // remove shot
            
            game_vars.player[i].hit_owner=j; // 
            game_vars.player[i].hit_speed=GAME_PLAYER_HIT_SPEED; // push player
            game_vars.player[i].hit_dir=game_vars.player[j].shot_dir;
            game_add_spray_particles( // add fireworks
                                game_vars.player[j].shot_x,
                                game_vars.player[j].shot_y,
                                (game_vars.player[j].shot_dir+32768)&65535,
                                50);

        }
    }
       
    game_vars.i++;
}


