#include <common.h>

const float pi=3.141592654;

unsigned char pal1[16][3] = {
{0,0,9},
{0,0,13},
{0,0,18},
{0,0,23},
{0,0,28},
{0,0,32},
{0,0,37},
{0,0,42},
{0,0,47},
{9,9,41},
{18,18,34},
{27,27,27},
{36,36,21},
{45,45,14},
{54,54,7},
{63,63,0}};

#define add 42

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

void shade(char *buf, int off)
{
    buf[off]+=200;
    buf[off-1]+=150;
    buf[off+1]+=150;
    buf[off-320]+=150;
    buf[off+320]+=150;
    buf[off-2]+=100;
    buf[off+2]+=100;
    buf[off-640]+=100;
    buf[off+640]+=100;
    buf[off-1-320]+=100;
    buf[off+1-320]+=100;
    buf[off-1+320]+=100;
    buf[off+1+320]+=100;

}

void shade2(char *buf, int off)
{
    buf[off]-=200;
    buf[off-1]-=150;
    buf[off+1]-=150;
    buf[off-320]-=150;
    buf[off+320]-=150;
    buf[off-2]-=100;
    buf[off+2]-=100;
    buf[off-640]+=100;
    buf[off+640]-=100;
    buf[off-1-320]-=100;
    buf[off+1-320]-=100;
    buf[off-1+320]-=100;
    buf[off+1+320]-=100;

}
void shade3 (char *buf, int off)
{
    buf[off]>>=1;
    buf[off-1]>>=1;
    buf[off+1]>>=1;
    buf[off-320]>>=1;
    buf[off+320]>>=1;

}



main()
{
	char *palette;
	union REGS r;
    int i,j,x,y,count;
    FILE *handle;
    float mul,u,v,f,xf,yf;
    int ui,vi;
    int currentbuffer=1;

    unsigned short *avrbuf;
    char *scrbuf1, *scrbuf2;
    char *colormap;

    palette=(char*)malloc(768);

    avrbuf=(short*)malloc(320*200 * 4 * sizeof(short));
    scrbuf1=(char*)malloc(64000);
    scrbuf2=(char*)malloc(64000);
    colormap=(char*)malloc(64000);

    handle=fopen("dat1.raw","rb");
    fread(colormap,64000,1,handle);
    fclose(handle);

    for(i=0;i<64000;i++) colormap[i]&=15;

//for(i=0;i<64000;i++)colormap[i]=0;

    cout << "Calculating lookup table.." << endl;

    i=0;

    for(y=0;y<200;y++)
    for(x=0;x<320;x++)
    {

    /*
        u=x-160; u=u*u;
        v=y-100; v=v*v;

        //scrbuf1[i]=sqrt(u+v);
        scrbuf2[i]=sqrt(u+v);

      */
        //scrbuf2[i]=((x&32)/32)*128;
      //  scrbuf2[i]=i&255;
      //  scrbuf2[i]=rand()&255;

        xf=(float)((x-160.0)/40.0);
        yf=(float)((y-100.0)/40.0);

        mul=2.0;
        //mul=1.3; // mindste v‘rdi f›r det begynder at ligne lort

        //if(mask[i]==0) mul=1.5; else mul=1.5;

/*
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
         */
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

        u=xf-yf*yf*0.5+2;
        v=0.2*xf*xf*yf-2;

        ui=cos(atan2(u, v))*mul*1.1;
        vi=sin(atan2(u, v))*mul*1.1;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4]=j;

        u=xf-yf*yf*0.5+2;
        v=0.2*xf*xf*yf-2;

        ui=cos(atan2(u, v)-0.1)*mul*1.5;
        vi=sin(atan2(u, v)-0.1)*mul*1.5;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+1]=j;

        u=xf-yf*yf*0.5+2;
        v=0.2*xf*xf*yf-2;

        ui=cos(atan2(u, v)+0.1)*mul*1.5;
        vi=sin(atan2(u, v)+0.1)*mul*1.5;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+2]=j;

        u=xf-yf*yf*0.5+2;
        v=0.2*xf*xf*yf-2;

        ui=cos(atan2(u, v))*mul*1.9;
        vi=sin(atan2(u, v))*mul*1.9;
        j=(x+ui)+(y+vi)*320;
        if(j<0) j+=64000;
        if(j>63999) j-=63999;
        avrbuf[i*4+3]=j;

        i++;

        if((i&1023)==0) cout << "";
    }




    // *********************************
    // palette fun
    // *********************************
    for(i=0;i<6;i++)
	{
        for(j=0;j<8;j++)
        {

            {
                palette[j*3+i*48]=((i*5)*(j))/8;
                palette[j*3+i*48+1]=0;
                palette[j*3+i*48+2]=0;
                if(palette[j*3+(i)*48] > 63) palette[j*3+(i)*48]=63;
                if(palette[j*3+(i)*48+1] > 63) palette[j*3+(i)*48+1]=63;
                if(palette[j*3+(i)*48+2] > 63) palette[j*3+(i)*48+2]=63;
            }

        }
        for(j=8;j<16;j++)
        {

            {
                palette[j*3+i*48]=0;
                palette[j*3+i*48+1]=(j-8)*4 + i*2;
                palette[j*3+i*48+2]=(j-8)*8;
                if(palette[j*3+(i)*48] > 63) palette[j*3+(i)*48]=63;
                if(palette[j*3+(i)*48+1] > 63) palette[j*3+(i)*48+1]=63;
                if(palette[j*3+(i)*48+2] > 63) palette[j*3+(i)*48+2]=63;
            }

        }

    }
    for(i=6;i<12;i++)
	{
        for(j=0;j<8;j++)
        {

            {
                palette[j*3+i*48]=((i*5)*(j))/8;
                palette[j*3+i*48+1]=(((i-6)*5)*(j))/8;
                palette[j*3+i*48+2]=0;

                if(palette[j*3+(i)*48] > 63) palette[j*3+(i)*48]=63;
                if(palette[j*3+(i)*48+1] > 63) palette[j*3+(i)*48+1]=63;
                if(palette[j*3+(i)*48+2] > 63) palette[j*3+(i)*48+2]=63;
            }

        }
        for(j=8;j<16;j++)
        {

            {
                palette[j*3+i*48]=0;
                palette[j*3+i*48+1]=(j-8)*4 + i*2;
                palette[j*3+i*48+2]=(j-8)*8;

                if(palette[j*3+(i)*48] > 63) palette[j*3+(i)*48]=63;
                if(palette[j*3+(i)*48+1] > 63) palette[j*3+(i)*48+1]=63;
                if(palette[j*3+(i)*48+2] > 63) palette[j*3+(i)*48+2]=63;
            }

        }


    }
    for(i=12;i<16;i++)
	{
        for(j=0;j<8;j++)
        {

            {
                palette[j*3+i*48]=(((16-i)*15)*(j))/8;
                palette[j*3+i*48+1]=(((16-i)*5)*(j))/8;
                palette[j*3+i*48+2]=0;

                if(palette[j*3+(i)*48] > 63) palette[j*3+(i)*48]=63;
                if(palette[j*3+(i)*48+1] > 63) palette[j*3+(i)*48+1]=63;
                if(palette[j*3+(i)*48+2] > 63) palette[j*3+(i)*48+2]=63;
            }

        }
        for(j=8;j<16;j++)
        {

            {
                palette[j*3+i*48]=(16-i)*7;
                palette[j*3+i*48+1]=(j-8)*4 + (16-i)*6;
                palette[j*3+i*48+2]=(j-8)*8;

                if(palette[j*3+(i)*48] > 63) palette[j*3+(i)*48]=63;
                if(palette[j*3+(i)*48+1] > 63) palette[j*3+(i)*48+1]=63;
                if(palette[j*3+(i)*48+2] > 63) palette[j*3+(i)*48+2]=63;
            }

        }

    }






    r.w.ax=0x13;
    int386(0x10,&r,&r);

    SetPalette(palette);
    /*
    i=0;
    for(x=0;x<16;x++)
    for(y=0;y<16;y++)
    {
        *(char *)(0xa0000+x+y*320)=i++;
    }
    */
    /*
    for(i=0;i<64000;i++)
        *(char *)(0xa0000+i)=scrbuf1[i]+(i/(64000/16))*16;
      */

    f=0.0;
    while(!kbhit())
    {
        f=f-0.03;



        if(currentbuffer==1)
        {


            for(i=0;i<40;i++)
            {
                x=(cos(f + (float)(i/20.0*pi) )*150);
                y=(sin(f + (float)(i/20.0*pi) )*90);
                shade(scrbuf2, x+y*320+32160);
            }
            for(i=0;i<40;i++)
            {
                x=(cos(f*1.02 + (float)(i/20.0*pi) )*150);
                y=(sin(f*1.02 + (float)(i/20.0*pi) )*90);
                shade2(scrbuf2, x+y*320+32160);
            }

            /*
            for(i=0;i<20;i++)
            {
                x=(cos(-f + (float)(i/10.0*pi) )*sin(f/2)*50);
                y=(sin(-f + (float)(i/10.0*pi) )*sin(f/2)*50);
                shade2(scrbuf2, x+y*320+32160);
            }
            */


            for(i=0;i<64000;i++)
            {

                scrbuf1[i]=
                    (scrbuf2[avrbuf[i*4]]+
                    scrbuf2[avrbuf[i*4+1]]+
                    scrbuf2[avrbuf[i*4+2]]+
                    scrbuf2[avrbuf[i*4+3]])/4-1;

                                    }

            for(i=0;i<64000;i++)
                *(char *)(0xa0000+i)=(scrbuf1[i]&240)+colormap[i];
            currentbuffer=2;
        }
        else
        {

            for(i=0;i<40;i++)
            {
                x=(cos(f + (float)(i/20.0*pi) )*150);
                y=(sin(f + (float)(i/20.0*pi) )*90);
                shade(scrbuf1, x+y*320+32160);
            }
            for(i=0;i<40;i++)
            {
                x=(cos(f*1.02 + (float)(i/20.0*pi) )*150);
                y=(sin(f*1.02 + (float)(i/20.0*pi) )*90);
                shade2(scrbuf1, x+y*320+32160);
            }


            /*
            for(i=0;i<180;i++)
            {
                j=rand()<<1;
                if(j>65000-321) j=65000-321;

                scrbuf1[j+0]+=add;
                scrbuf1[j+1]+=add;
                scrbuf1[j+320]+=add;
                scrbuf1[j+321]+=add;
            }

              */
            for(i=0;i<64000;i++)
            {

                scrbuf2[i]=
                    (scrbuf1[avrbuf[i*4]]+
                    scrbuf1[avrbuf[i*4+1]]+
                    scrbuf1[avrbuf[i*4+2]]+
                    scrbuf1[avrbuf[i*4+3]])/4-1;
                          }

            for(i=0;i<64000;i++)
                *(char *)(0xa0000+i)=(scrbuf2[i]&240)+colormap[i];
            currentbuffer=1;
        }

    }
    getch();

    for(count=0;count<80;count++)
    {
        y=(float)(count*count*count*200.0/(80*80*80));

        if(currentbuffer==1)
        {
            for(i=0;i<64000;i++)
            {

                scrbuf1[i]=
                    (scrbuf2[avrbuf[i*4]]+
                    scrbuf2[avrbuf[i*4+1]]+
                    scrbuf2[avrbuf[i*4+2]]+
                    scrbuf2[avrbuf[i*4+3]])/4;

                                    }

            for(i=0;i<y*320;i++)
                *(char *)(0xa0000+i)=0;
            for(i=y*320;i<64000;i++)
                *(char *)(0xa0000+i)=(scrbuf1[i]&240)+colormap[i-y*320];
            currentbuffer=2;
        }
        else
        {

            for(i=0;i<64000;i++)
            {

                scrbuf2[i]=
                    (scrbuf1[avrbuf[i*4]]+
                    scrbuf1[avrbuf[i*4+1]]+
                    scrbuf1[avrbuf[i*4+2]]+
                    scrbuf1[avrbuf[i*4+3]])/4;
                          }

            for(i=0;i<y*320;i++)
                *(char *)(0xa0000+i)=0;
            for(i=y*320;i<64000;i++)
                *(char *)(0xa0000+i)=(scrbuf2[i]&240)+colormap[i-y*320];
            currentbuffer=1;
        }

    }




    r.w.ax=0x3;
    int386(0x10,&r,&r);

	free(palette);
    free(avrbuf);
    free(scrbuf1); free(scrbuf2);
    free(colormap);
}

