#include <common.h>

const float pi=3.141592654;

void SetPalette(char *palette)
{
    int i;
    for(i=0;i<256;i++)
    {
        outp(0x3c8,i);
        outp(0x3c9,palette[3*i]);
        outp(0x3c9,palette[3*i+1]);
        outp(0x3c9,palette[3*i+2]);
    }
}

#define add 42
void shade(char *buf, int i)
{
    if(i>65000-321) i=65000-321;

    buf[i+0]+=add;
    buf[i+1]+=add;
    buf[i+320]+=add;
    buf[i+321]+=add;
}

#define add2 255
void shade2(char *buf, int i)
{
    if(i>65000-321) i=65000-321;

    buf[i+0]=add2;
    buf[i+1]=add2;
    buf[i+320]=add2;
    buf[i+321]=add2;
}

void waitVRT(void)
{
    while(!(inp(0x3da)&8));
    while((inp(0x3da)&8));
}

unsigned short *avrbuf;

void calc(char *from, char *to)
{
    int i;

    for(i=0;i<64000;i++)
    {
        to[i]=
            (from[avrbuf[i*4]]+
            from[avrbuf[i*4+1]]+
            from[avrbuf[i*4+2]]+
            from[avrbuf[i*4+3]])>>2;
    }
}



main()
{
    union REGS r;

    char *palette;
    char *screen;
    char *screenbuf1;
    char *screenbuf2;

    char *mask;

    int buffer=0;
    FILE *handle;

    screen=(char *)0xa0000;


    int i;
    int x,y;
    int x1,y1;
    float u,v;
    int ui,vi;
    unsigned short j;
    float mul;

    palette=new char[768];
    screenbuf1=new char[64000];
    screenbuf2=new char[64000];
    avrbuf=new unsigned short[64000*4];

    mask=new char[64000];

  /*
    handle=fopen("float.raw","rb");
    fread(mask,1,64000,handle);
    fclose(handle);
    */


    for(i=0;i<128;i++)
    {
        palette[i*3]=i/4;
        palette[i*3+1]=0;
        palette[i*3+2]=i/2;
    }
    for(i=128;i<256;i++)
    {
        palette[i*3]=31+(i-128)/4;
        palette[i*3+1]=(i-128)/2;
        palette[i*3+2]=63;
    }
    /*
    for(i=0;i<64;i++)
    {
        palette[i*3]=i;
        palette[i*3+1]=0;
        palette[i*3+2]=0;
    }
    for(i=64;i<128;i++)
    {
        palette[i*3]=63;
        palette[i*3+1]=i-64;
        palette[i*3+2]=0;
    }
    for(i=128;i<128+64;i++)
    {
        palette[i*3]=63;
        palette[i*3+1]=63;
        palette[i*3+2]=i-128;
    }
    for(i=128+64;i<256;i++)
    {
        palette[i*3]=63;
        palette[i*3+1]=63;
        palette[i*3+2]=63;
    }
    */


    cout << "Calculating lookup table.." << endl;

    i=0;

    for(y=0;y<200;y++)
    for(x=0;x<320;x++)
    {
        screenbuf1[i]=0;
        screenbuf2[i]=0;

        mul=1.5;
        //mul=1.3; // mindste v‘rdi f›r det begynder at ligne lort

        //if(mask[i]==0) mul=1.5; else mul=1.5;

        u=sin(45.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*2*mul;
        v=cos(45.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*2*mul;
        //cout << u << "-" << v << endl;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4]=j;

        u=sin(50.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*3*mul;
        v=cos(50.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*3*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+1]=j;

        u=sin(40.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*3*mul;
        v=cos(40.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*3*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+2]=j;

        u=sin(45.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*5*mul;
        v=cos(45.0/180.0*pi + sin(x/50.0)/5.0 + cos(y/50.0)/7.0)*5*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+3]=j;

        /*
        u=sin(atan2(x-160.0,y-100.0)+10.0/180.0*pi)*2*mul;
        v=cos(atan2(x-160.0,y-100.0)+10.0/180.0*pi)*2*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4]=j;

        u=sin(atan2(x-160.0,y-100.0)+15.0/180.0*pi)*3*mul;
        v=cos(atan2(x-160.0,y-100.0)+15.0/180.0*pi)*3*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+1]=j;

        u=sin(atan2(x-160.0,y-100.0)+5.0/180.0*pi)*3*mul;
        v=cos(atan2(x-160.0,y-100.0)+5.0/180.0*pi)*3*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+2]=j;

        u=sin(atan2(x-160.0,y-100.0)+10.0/180.0*pi)*4*mul;
        v=cos(atan2(x-160.0,y-100.0)+10.0/180.0*pi)*4*mul;
        ui=u; vi=v;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+3]=j;
        */



        i++;

        if((i&1023)==0) cout << "";
    }

    cout << endl;

    r.w.ax=0x13;
    int386(0x10,&r,&r);

    SetPalette(palette);

    i=0; j=0;
    x=0; y=0;
    while(!kbhit())
    {
        j++;

        if(buffer==0)
        {

            for(i=0;i<180;i++)
                shade(screenbuf2,rand()<<1);

            calc(screenbuf2,screenbuf1);

            waitVRT();
            memcpy(screen,screenbuf1,64000);
        }
        else
        {

            for(i=0;i<180;i++)
                shade(screenbuf1,rand()<<1);

            calc(screenbuf1,screenbuf2);

            waitVRT();
            memcpy(screen,screenbuf2,64000);
        }

        buffer++;
        buffer&=1;

    }


    getch();
    while(!kbhit())
    {
        i++;

        //u=((sin(i/100.0)*80.0)+160.0);
        //v=((cos(i/30.0)*80.0)+100.0);
        //x=rand()*320.0/32768.0;
        //y=rand()*200.0/32768.0;

        if(buffer==0)
        {
            calc(screenbuf2,screenbuf1);

            waitVRT();
            memcpy(screen,screenbuf1,64000);
        }
        else
        {
            calc(screenbuf1,screenbuf2);

            waitVRT();
            memcpy(screen,screenbuf2,64000);
        }

        buffer++;
        buffer&=1;

    }


    getch();

    r.w.ax=0x3;
    int386(0x10,&r,&r);

}

