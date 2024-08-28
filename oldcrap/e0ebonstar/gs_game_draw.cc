#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "font1.h"

#include "gs_game.h"
#include "gs_game_i.h"

void game_circle(float hx, float hy, float r)
{
    glColor4f(0.1f,0.1f,0.1f,1.0f);
    glBindTexture(GL_TEXTURE_2D, game_vars.texture[5]);
    glBlendFunc(GL_ONE,GL_ONE);
        
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f,0.0f); glVertex2f(hx-r,hy-r);
        glTexCoord2f(1.0f,0.0f); glVertex2f(hx+r,hy-r);
        glTexCoord2f(1.0f,1.0f); glVertex2f(hx+r,hy+r);
        glTexCoord2f(0.0f,1.0f); glVertex2f(hx-r,hy+r);
    glEnd();
}

void game_drawsprite(struct game_sprite_s *sprite, float cx, float cy, int r, float scale)
{
    float x=(sprite->tx1+sprite->tx2)/2.0+sprite->ox*((sprite->tx2-sprite->tx1)/2.0);
    float y=(sprite->ty1+sprite->ty2)/2.0+sprite->oy*((sprite->ty2-sprite->ty1)/2.0);
    float d=(r/65536.0)*6.283;
    float s=sprite->scale*scale;
    
    glBindTexture(GL_TEXTURE_2D, sprite->texture);

    glBegin(GL_QUADS);
        glTexCoord2f(sprite->tx1,sprite->ty1);
        glVertex2f(
            (cos(d)*(sprite->tx1-x)-sin(d)*(sprite->ty1-y))*s+cx,
            (sin(d)*(sprite->tx1-x)+cos(d)*(sprite->ty1-y))*s+cy);
        glTexCoord2f(sprite->tx2,sprite->ty1);
        glVertex2f(
            (cos(d)*(sprite->tx2-x)-sin(d)*(sprite->ty1-y))*s+cx,
            (sin(d)*(sprite->tx2-x)+cos(d)*(sprite->ty1-y))*s+cy);
        glTexCoord2f(sprite->tx2,sprite->ty2);
        glVertex2f(
            (cos(d)*(sprite->tx2-x)-sin(d)*(sprite->ty2-y))*s+cx,
            (sin(d)*(sprite->tx2-x)+cos(d)*(sprite->ty2-y))*s+cy);
        glTexCoord2f(sprite->tx1,sprite->ty2);
        glVertex2f(
            (cos(d)*(sprite->tx1-x)-sin(d)*(sprite->ty2-y))*s+cx,
            (sin(d)*(sprite->tx1-x)+cos(d)*(sprite->ty2-y))*s+cy);
    glEnd();

}

void game_drawgrid(void)
{
    int x,y,j,k,i;
    float fx,fy,rd,d,r,z,dz,s,a,zl;
    int isset;
    float hx,hy;
    float vx,vy;
    float dx,dy;
    int odx[4]={0,1,1,0}, ody[4]={0,0,1,1};
    struct game_grid_s *grid;

    // draw blue bg
    glColor4f(0.0f,0.0f,0.1f,1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glVertex2f(0.0f,0.0f);
        glVertex2f(800.0f,0.0f);
        glVertex2f(800.0f,600.0f);
        glVertex2f(0.0f,600.0f);
    glEnd();

    // draw hole shade

    for(k=0;k<game_vars.blobs;k++)
    {
        r=game_vars.blob[k].r;
        hx=game_vars.blob[k].x;
        hy=game_vars.blob[k].y;
        z=game_vars.blob[k].z;
        if(z>=0.0)
        {
            glBindTexture(GL_TEXTURE_2D, game_vars.texture[1]);
            glColor4f(0.0f,0.0f,0.0f,z);
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f,0.0f); glVertex2f(hx-r,hy-r);
                glTexCoord2f(1.0f,0.0f); glVertex2f(hx+r,hy-r);
                glTexCoord2f(1.0f,1.0f); glVertex2f(hx+r,hy+r);
                glTexCoord2f(0.0f,1.0f); glVertex2f(hx-r,hy+r);
            glEnd();
        }
        else
        {
            
        }
    }
    
    // calculate border grid
    for(x=0;x<GAME_BOARD_WIDTH;x++)
    {
        i=x+0*GAME_BOARD_WIDTH;
        game_vars.grid[i].dx=0.0f;
        game_vars.grid[i].dy=0.0f;
        game_vars.grid[i].r=0.0f;
        game_vars.grid[i].g=0.0f;
        game_vars.grid[i].b=0.0f;
        i=x+(GAME_BOARD_HEIGHT-1)*GAME_BOARD_WIDTH;
        game_vars.grid[i].dx=0.0f;
        game_vars.grid[i].dy=0.0f;
        game_vars.grid[i].r=0.0f;
        game_vars.grid[i].g=0.0f;
        game_vars.grid[i].b=0.0f;
    }
    
    for(y=0;y<GAME_BOARD_HEIGHT;y++)
    {
        i=0+y*GAME_BOARD_WIDTH;
        game_vars.grid[i].dx=0.0f;
        game_vars.grid[i].dy=0.0f;
        game_vars.grid[i].r=0.0f;
        game_vars.grid[i].g=0.0f;
        game_vars.grid[i].b=0.0f;
        i=(GAME_BOARD_WIDTH-1)+y*GAME_BOARD_WIDTH;
        game_vars.grid[i].dx=0.0f;
        game_vars.grid[i].dy=0.0f;
        game_vars.grid[i].r=0.0f;
        game_vars.grid[i].g=0.0f;
        game_vars.grid[i].b=0.0f;
    }
    
    // calculate main grid
    for(y=1;y<(GAME_BOARD_HEIGHT-1);y++)
    for(x=1;x<(GAME_BOARD_WIDTH-1);x++)
    {
        i=x+y*GAME_BOARD_WIDTH;
        fx=x*10.0;
        fy=y*10.0;
        
        isset=false;
        for(k=0;k<game_vars.blobs;k++)
        {
            r=game_vars.blob[k].r;
            z=game_vars.blob[k].z;
            hx=game_vars.blob[k].x;
            hy=game_vars.blob[k].y;
            
            dx=fx-hx;     
            dy=fy-hy;
            d=sqrt(dx*dx+dy*dy);
            rd=r-d;
            
            if(z>=0.0)
                dz=(rd/2.5)*z;
            else
                dz=((rd/2.5)*z)*(d/(r/2.0));

            a=atan2(dy,dx);
            vx=cos(a);
            vy=sin(a);

            zl=z*(50.0*(r/160.0));
            
            if(d<r && d>(zl))
            {
                game_vars.grid[i].dx=-vx*dz;
                game_vars.grid[i].dy=-vy*dz;
                if(z>=0.0)
                {
                    game_vars.grid[i].r=((rd/r)*1.5f)*z;
                    game_vars.grid[i].g=0.1f;
                    game_vars.grid[i].b=0.6f-((rd/r)*z);
                }
                else
                {
                    game_vars.grid[i].r=0.0f;
                    game_vars.grid[i].g=0.1f-((rd/r)*z*0.1f);
                    game_vars.grid[i].b=0.6f-((rd/r)*z);
                }
                isset=true;
            } else if(d<=(zl)) {            
                game_vars.grid[i].dx=0.0f;
                game_vars.grid[i].dy=0.0f;
                game_vars.grid[i].r=0.0f;
                game_vars.grid[i].g=0.0f;
                game_vars.grid[i].b=0.0f;
                isset=true;
            }
        }
        
        if(!isset)
        {
            game_vars.grid[i].dx=0.0f;
            game_vars.grid[i].dy=0.0f;
            game_vars.grid[i].r=0.0f;
            game_vars.grid[i].g=0.1f;
            game_vars.grid[i].b=0.6f;
        }
    }

    // draw grid
    float lr,lg,lb;
    grid=&game_vars.grid[0+odx[0]+(0+ody[0])*GAME_BOARD_WIDTH];
    glColor4f(grid->r, grid->g, grid->b, 1.0f);
    lr=grid->r;
    lg=grid->g;
    lb=grid->b;
    
    glBindTexture(GL_TEXTURE_2D, game_vars.texture[0]);
    glBlendFunc(GL_ONE, GL_ONE);
    
    glBegin(GL_QUADS);
        for(y=0;y<(GAME_BOARD_HEIGHT-1);y++)
        for(x=0;x<(GAME_BOARD_WIDTH-1);x++)
        {
            for(j=0;j<4;j++)
            {
                grid=&game_vars.grid[x+odx[j]+(y+ody[j])*GAME_BOARD_WIDTH];
                
                if(lr != grid->r || lg != grid->g || lb != grid->b)
                {
                    glColor4f(grid->r, grid->g, grid->b, 1.0f);
                    lr=grid->r;
                    lg=grid->g;
                    lb=grid->b;
                }
                glTexCoord2f(((x&1)+odx[j])/2.0,((y&1)+ody[j])/2.0);
                glVertex2f((x+odx[j])*10.0+grid->dx,(y+ody[j])*10.0+grid->dy);             
            }
        }
    glEnd();
    
    // draw hole
    glColor4f(0.0f,0.0f,0.0f,1.0f);
    glBindTexture(GL_TEXTURE_2D, game_vars.texture[2]);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
    for(k=0;k<game_vars.blobs;k++)
    {
        z=game_vars.blob[k].z;
        if(z>0.0)
        {
            r=game_vars.blob[k].r;
            
            hx=game_vars.blob[k].x;
            hy=game_vars.blob[k].y;
    
            s=(1.0f/4.5f)*z*(r/140.0);
    
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f,0.0f); glVertex2f(hx-r*s,hy-r*s);
                glTexCoord2f(1.0f,0.0f); glVertex2f(hx+r*s,hy-r*s);
                glTexCoord2f(1.0f,1.0f); glVertex2f(hx+r*s,hy+r*s);
                glTexCoord2f(0.0f,1.0f); glVertex2f(hx-r*s,hy+r*s);
            glEnd();
        }
    }
}

void game_drawtitle(void)
{
    float i = game_vars.title_intensity;
    
    glColor4f(1.0f,1.0f,1.0f,1.0f*i);
    glBindTexture(GL_TEXTURE_2D, game_vars.texture[6]);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f,0.0f); glVertex2f(20.0f,60.0f);
        glTexCoord2f(1.0f,0.0f); glVertex2f(780.0f,60.0f);
        glTexCoord2f(1.0f,1.0f); glVertex2f(780.0f,240.0f);
        glTexCoord2f(0.0f,1.0f); glVertex2f(20.0f,240.0f);
    glEnd();
}

void game_drawship(float x, float y, int r, int colour, float p)
{   
    glColor4f(0.6f,0.6f,0.6f,1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    game_drawsprite(&game_sprite.ship_white,x,y,r,p);
    
    switch(colour)
    {
        case 0 :
            glColor4f(0.7f,0.2f,0.2f,1.0f); // red
        break;
        case 1 :
            glColor4f(0.7f,0.7f,0.2f,1.0f); // yellow
        break;
        case 2 :
            glColor4f(0.2f,0.7f,0.2f,1.0f); // green
        break;
        case 3 :
            glColor4f(0.2f,0.2f,1.0f,1.0f); // blue
        break;
        default :
            glColor4f(1.0f,1.0f,1.0f,1.0f); // whut?
        break;
    }
    game_drawsprite(&game_sprite.ship_colour,x,y,r,p);
}

void game_drawsprites(void)
{
    float x,y;
    float vx,vy;
    int i;
    float t,p;
    
    int r=game_vars.i*100;
    
    vx=cos(r*6.283/65536.0)*140.0;
    vy=sin(r*6.283/65536.0)*140.0;
    x=game_vars.blob[0].x+vx;
    y=game_vars.blob[0].y+vy;
    
    
    t=rand()/(float)RAND_MAX;
    
    //draw beam
    glColor4f(0.2f*t,0.2f*t,0.2f*t,1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBlendFunc(GL_ONE, GL_ONE);
    glBegin(GL_TRIANGLES);
        glVertex2f(game_vars.blob[0].x,game_vars.blob[0].y);
        glVertex2f(x+vy*0.1f,y-vx*0.1f);
        glVertex2f(x-vy*0.1f,y+vx*0.1f);
    glEnd();
    
    // draw station
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    game_drawsprite(&game_sprite.station_hook,x,y,-r,1.0);
    game_drawsprite(&game_sprite.station_hook,x,y,int(65536.0*0.333-r),1.0);
    game_drawsprite(&game_sprite.station_hook,x,y,int(65536.0*0.666-r),1.0);
    
    game_drawsprite(&game_sprite.station,x,y,0,1.0);
    
    glColor4f(0.8f,0.3f,0.0f,1.0f);
    game_drawsprite(&game_sprite.station_light,x,y,r,1.0);
    glBlendFunc(GL_ONE, GL_ONE);
    
    glColor4f(0.8f*t,0.3f*t,0.0f,1.0f);

    game_drawsprite(&game_sprite.station_glow,x,y,r,1.0);
    
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    game_drawsprite(&game_sprite.station_parabola,x,y,65536/2+r,1.0);

    // draw all ships

    for(i=0;i<GAME_SHIPS_MAX;i++)
    {
        if(game_vars.player[i].status > 0)
        {
            p=1.0-game_vars.player[i].how_dead;
            x=game_vars.player[i].ship_x;
            y=game_vars.player[i].ship_y;
            r=game_vars.player[i].ship_dir;

            game_drawship(x,y,r,i,p);
            
            // draw shot
            if(game_vars.player[i].shot_ttl>0)
            {
                glBlendFunc(GL_ONE, GL_ONE);
                game_drawsprite(&game_sprite.ship_shot,
                game_vars.player[i].shot_x,
                game_vars.player[i].shot_y,
                game_vars.player[i].shot_dir,p=1.0-game_vars.player[i].shot_how_dead);
            }
            
            // draw thrust
            if(game_vars.player[i].thrust==1)
            {
                glBlendFunc(GL_ONE, GL_ONE);
                glColor4f(1.0f*t,0.3f*t,0.0f,1.0f);
                game_drawsprite(&game_sprite.ship_thrust,x,y,r,p);
            }
        }
    }
    
}

void game_drawstatus(float t)
{
    int i;
    float x,y;
    
    glColor4f(0.6f,0.6f,0.6f,1.0f);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBegin(GL_QUADS);
        glColor4f(0.0f,0.0f,0.0f,0.0f); glVertex2f(0.0f,520.0f);
        glColor4f(0.0f,0.0f,0.0f,0.0f); glVertex2f(800.0f,520.0f);
        glColor4f(0.0f,0.0f,0.0f,1.0f*t); glVertex2f(800.0f,540.0f);
        glColor4f(0.0f,0.0f,0.0f,1.0f*t); glVertex2f(0.0f,540.0f);
    
        glVertex2f(0.0f, 540.0f);
        glVertex2f(800.0f, 540.0f);
        glVertex2f(800.0f, 600.0f);
        glVertex2f(0.0f, 600.0f);
    glEnd();
    
    font1_setscale(1.0);
    
    for(i=0;i<4;i++)
    {
        if(game_vars.player[i].status!=0)
        {
            switch(i)
            {
                case 0 :
                    x=780.0f;
                    y=550.0f;
                break;
                case 1 :
                    x=780.0f;
                    y=570.0f;
                break;
                case 2 :
                    x=580.0f;
                    y=550.0f;
                break;
                case 3 :
                    x=580.0f;
                    y=570.0f;
                break;
                default :
                    x=y=0.0;
                break;
            }
            
            game_drawship(x, y, 0, i, 1.0);
            font1_printf(x-30.0f, y-5.0f, "%d", game_vars.player[i].lives);
            
            font1_printf_r(x-70.0f, y-5.0f, "%d", game_vars.player[i].score);
        }
    }
}

void game_drawparticles(void)
{
    float t;
    float a;
    float x,y,r;
    game_particle_s *prevpar=NULL, *nextpar, *temppar=game_vars.particle;
    
    //glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, game_vars.texture[5]);
    glBlendFunc(GL_ONE, GL_ONE);
    
    while(temppar!=NULL)
    {
        if(temppar->ttl==0)
        {
            if(prevpar!=NULL)
            {
                prevpar->next=temppar->next;
            }
            else
            {
                game_vars.particle=temppar->next;
            }
            nextpar = temppar->next;
            delete temppar;
            temppar=nextpar;
        }
        else
        {
            game_advance(
                &temppar->x,
                &temppar->y,
                temppar->dir,
                temppar->speed);
        
            temppar->ttl--;
        
            a=temppar->ttl/64.0;
            t=float(rand()) / float(RAND_MAX);

            switch(temppar->colour&3)
            {
                case 0:
                    glColor4f(1.0f*(t+0.7)*a,0.5f*t*a,0.0f,1.0f);
                break;
                case 1:
                    glColor4f(0.0f,0.0f,1.0f*t,1.0f);
                break;
                case 2:
                    glColor4f(1.0f*a,1.0f*a,1.0f*a,1.0f);
                break;
                case 3:
                    glColor4f(1.0f*a,0.3f*a,0.0f,1.0f);
                break;
            }
        
            x=temppar->x;
            y=temppar->y;
            r=2.0;

            glBegin(GL_QUADS);
                glTexCoord2f(0.0f,0.0f); glVertex2f(x-r,y-r);
                glTexCoord2f(1.0f,0.0f); glVertex2f(x+r,y-r);
                glTexCoord2f(1.0f,1.0f); glVertex2f(x+r,y+r);
                glTexCoord2f(0.0f,1.0f); glVertex2f(x-r,y+r);
            glEnd();
            prevpar=temppar;
            temppar=temppar->next;
        }
    }
}



