#define C(B) double \
*j=G(n+2),o; int c=(int)j[n+1];         B; if     \
              (++c>=      n)c=0; j[n+1]=c; return \
                                        o;
#define S sin
#define O pow
#define H tanh
#define A(n)for(j=0; j<n; ++j)
#define V(x)f##x

#include<stdlib.h>
#include <stdio.h>
#include  <math.h>
#include<assert.h>

char   static M[1<<29],*P;
double static D[1<<20],*E,U[4],X,t,b,
F=
  1.5e-3 //C#
;
double static* G(int n)   { E+=n; return E-n; }

int static B=
  17e3 //moderato
,J,I,N;
void static  L(short w)   { *P++=(char)w; *P++=w>>3>>5; }

unsigned
static q0[]={
  0x0ce97522,
  0x75acecaf,
                                                                         } ;
   int static q1[]     = { 0,3,7      , 3,7,14 , -2,0,0 ,  3,2,0         } ;
  char static*q2[]     = { "XBWFgnu!" , "ebub" , "SJGG" ,  "!?"          } ;//XXX
  void static  Z(char*s) { V (or) (char*q=s; *q; ++q, ++P) *P=*q-1       ; }
  void static  W(int l)  { L((short)l);  L((short)(l>>11>>5))            ; }
   int static  K(int x)         { return I/(B<<x)                        ; }
double static dp(int a, int c)  { return (double)((I+B*c)%(B<<a))/(B<<a) ; }
double static  Q(int a)         { return dp(a,0)                         ; }
double static eb(int i,int p)   { return i%p<p>>2?1:-1                   ; }
double static ap(double x,int n , double f,double d) {
C (
    o=j[c];
    j[n]=o*(1-d)+j[n]*d;
    j[c]=x+j[n]*f;
) }

void static Y(double g, double p, double r, double e)
{
    U[2] += r              * X                  ;
    U[0] += X              * g * V (min)(1,1-p) ;
    U[3] +=                  e * X              ;
    U[1] += V (min)(p+1,1) * X * g              ;
}

double static eq(double x,double a,double h,double c,double d,double e)
{
    double*z=G(2),
    y=a*x+z[0];
    z[0]=h*x-d*y+z[1];
    z[1]=c*x-e*y;
    return y;
}

double static cm(double x,int n,double f) {
C (
    double u=j[c];
    o=u-x;
    j[c]=x+u*f;
) }

int main(void)
{
    int j,k;
    char*
    px=P=M,*
    pa,*
    p0;
    FILE*
    o;
    N=B<<7;
    Z(q2[2]); W(36                       +(N   << 2) );
    Z(q2[0]); W(16); L(1); L(2); W(1<<18); W(4 << 18 ); L(4); L(16);
                                 Z(q2[1]); W(N << 2  );
    V ( or ) ( pa = P ; ; ) { E = D ; A ( 4 ) U [ j ]  = 0 ;


  //bd
    t=H(Q(3));
    X=H(50*S(t*141+t*(15e3/(4+t*330)))*O(1-H(t),9));
  Y(.5,0,.07,.1);

  //hh
    X=0;
    A(8) X+=eb(I,211+(j*113+K(2)*91)%1291);
    X=eq(X,.973,-1.946,.973,-1.942,.951);
    X=H(X*2);
    switch (K(8)) {
    case 0  : case 1: t=H(Q((K(4))%4)); break;
    case 2  : t=H(Q(2));                break;
    default : t=H(Q(1));                break;
    } if (++I>N)                        break;
    X*=O(1-t,4)*O(Q(3),2);
    X=H(X*1.4);
    if (!(K(7)&7)) X=0;
  Y(1,S(I*1e-5),.16,.2);

  //sd
    t=1-(((K(2)&63)>61)?H(Q(1))/6:H(dp(4,8)));
    X=I&(1<<12)?(double)(rand())/RAND_MAX-.5:.1*eb(I,1400-(int)(t*301));
    X=H(40*X*O(t,40));
    if (!(K(7)&3)) X=0;
  Y(1.2,.3,.3,.3);

  //bas
    k=(q0[0]>>((K(3)&7)<<2))&15;
    b=F*I*O(2,(double)k/12);
    k=K(8)%2;
    X=(((q0[1])>>(K(1)&31))&1)?S(b+1.5*S(I*7e-5)+S(I*3e-7)*(4+3*S(I*1.7e-7))*S(b*(1+k))):0;
    if (K(8)) X+=((S(b*.501)>.95)-(S(b*.498)>.8))*15*O(Q(3),2);
    t=1-H(Q(1));
    X=H(25*X*O(t,3));
  Y(0.4,0,.11,.1);

  //arp
    k=K(1)%32+3;
    k-=(k/4)*3;
    k=q1[k%3+3*(K(7)&3)]+(k/3)*12;
    X=S(F*I*O(2,(double)(14+k)/12))>S(I*5.3e-7)?1:-1;
    t=1-H(Q((K(0)&127)>4));
    X=H(5*X*O(t,9));
    if (K(6)&1 || !(K(8)&7)) X=0;
  Y(.75,S(I*7e-6),.15,.4);

  //eco
    X=ap(U[3],(int)(B*6.1),.2,.2);
  Y(1,-.6,.1,0);
    X=cm(U[3],(int)(B*7.9),.2);
  Y(.7,.6,.1,0);

  //rev
    for (k=0; k<2; ++k) {
    X=0;
    A(8) X+=ap(U[2],4001+(j*2131+k*1555)%3491,.8,.5);
    A(4) X=cm(X,800+(j*1331+k*1441)%1303,.4);
    X=H(X/4);
  Y(.3,k?-1:1,0,0);
    }


    A ( 2 ) L ( ( short ) ( V (min) ( 1 , V (max) ( -1 , U[j] ) ) * 32e3 ) ) ; if
    (!(I%(B*8)
            )
           )
          { putchar
          (
    "\njust "
  "a moment.."
     /*{*/[
      (++J)&15
          ]             )       ;
 V (flush)(   stdout    )       ;
          }             }  pa=P ;
    Z("opx!fokpz!"      ); p0=P ;
    Z("qsph/xbw"        ); assert(
    o= V (open)(p0,"wb") );
    V (write)(px,1,(size_t)(pa-px),o); V (close)(o);
    puts(pa);                        return 0;
}
