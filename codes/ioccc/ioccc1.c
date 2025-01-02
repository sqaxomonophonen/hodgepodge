#include<stdlib.h>
#include<stdio.h>
#include<math.h>

char M[1<<29],*P;
double D[1<<20],*E;
int I[1<<20],*J,C;

#define Z(s)for(char*q=s;*q;q++)*(P++)=*q-1;
#define W(w)*(P++)=((w)&255);*(P++)=(((w)>>8)&255);
#define L(l)W((l)&65535);W(((l)>>16)&65535);
#define AD(n)(E+=n,E-n)
#define AI(n)(J+=n,J-n)
#define MX(g,p,s)U[0]+=X*g*fmin(1,1-p);U[1]+=X*g*fmin(1,1+p);U[2]+=X*s;
#define S sin
#define O pow
#define H tanh

double bq(double*f,double x)
{
	double*z=AD(2),y=f[0]*x+z[0];
	z[0]=f[1]*x-f[3]*y+z[1];
	z[1]=f[2]*x-f[4]*y;
	return y;
}

void hpf(double*f,double o,double q)
{
	double c=cos(o),a=S(o)/(2*q);
	f[0]=(1+c)/2;
	f[1]=-1-c;
	f[2]=f[0];
	f[3]=-2*c;
	f[4]=1-a;
	for(int i=0;i<5;++i)f[i]/=(1+a);
}

double sq(int i,int p)
{
	return (i%p)<(p/2)?1:-1;
}

double sa(int i,int p)
{
	return (double)(i%p)/p;
}

double cf(double x,int n,double f,double d)
{
	double*b=AD(n+1),o;
	int*c=AI(1);
	o=b[*c];
	b[n]=o*(1-d)+(b[n]*d);
	b[*c]=x+(b[n]*f);
	if(++(*c)>=n)*c=0;
	return o;
}

double ap(double x,int n,double f)
{
	double*b=AD(n),u,o;
	int*c=AI(1);
	u=b[*c];
	o=u-x;
	b[*c]=x+u*f;
	if(++(*c)>=n)*c=0;
	return o;
}

int main(){
	int i,j,k;
	char*p0=M;
	P=p0;
	Z("SJGG")
	L(0)
	Z("XBWFgnu!")
	L(16)W(1)W(2)L(1<<18)L(4<<18)W(4)W(16)
	Z("ebub")
	L(0)
	char*p1=P;
	double h0[5],X,U[3],t,*u,q;
	hpf(h0,0.1,2);
	int B=17000,c0;
	for(i=0;i<(1<<23);++i){
		E=D;J=I;
		if (!(i&((1<<18)-1))) {
			if(!((C++)&15))fputs("\njust a moment",stdout);
			putchar('.');
			fflush(stdout);
		}

		U[0]=U[1]=U[2]=0;

		t=H(sa(i^((i/B)%8),B*8));
		X=H(50*S(t*141+t*(15000/(4+t*330)))*O(1-H(t),9));
		MX(0.5,0,.07)

		X=0;
		for(j=0;j<8;++j)X+=sq(i,(211+j*113)%1191);
		X=bq(h0,X);
		X=H(X*2);
		t=H(sa(i,B*(1<<(((i/(B*16)))%4))));
		X*=O(1-t,4)*O(sa(i,B*8),2);
		X=H(X);
		if(((i/(B*128))&3)==0)X=0;
		MX(1,S((double)i*1e-5),.09)

		t=1-H(sa(i+B*8,B*16));
		X=i&(1<<12)?(double)rand()/RAND_MAX-.5:.1*sq(i,1400-t*301);
		X=H(20*X*O(t,45));
		if(((i/(B*128))&3)==0)X=0;
		MX(1,0,.4)

		unsigned q0[]={
			0xab4a4efa,
			0x7cd11303,
		};

		q=(q0[1]>>(((i/(B*8))&7)<<2))&15;
		q=O(2,q/12);
		double F=1.5e-3,G=F*i*q;
		X=(((q0[0])>>((i/(B*2))&31))&1)?S(G+1.5*S(i*7e-5)+S(i*3e-7)*4*S(G*2)):0;
		t=1-H(sa(i,B*2));
		X=H(35*X*O(t,3));
		MX(0.4,0,.1)

		for(k=0;k<2;++k){
			X=0;
			for(j=0;j<8;++j)X+=cf(U[2],4001+((j*2131+k*1555)%3491),.8,.5);
			for(j=0;j<4;++j)X=ap(X,800+((j*1331+k*1441)%1303),.4);
			X=H(X/4);
			MX(.3,k?-1:1,0)
		}

		for(j=0;j<2;++j){W((unsigned)(fmin(1,fmax(-1,U[j]))*32767))}
	}
	int z=P-p1;p1=P;P=p0+4;L(z+36);P=p0+40;L(z);P=p1;
	Z("fokpz!")
	char*p2=P;
	Z("tpoh/xbw")
	*(P++)=0;
	FILE*o=fopen(p2,"wb");
	fwrite(p0,1,p1-p0,o);
	fclose(o);
	putchar('\n');
	puts(p1);
	return 0;
}
