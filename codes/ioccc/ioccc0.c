#include<stdlib.h>
#include<stdio.h>
#include<math.h>

#define Z(s) for(char*q=s;*q;q++)*(P++)=*q-1;
#define W(w) *(P++)=((w)&255);*(P++)=(((w)>>8)&255);
#define L(l) W((l)&65535);W(((l)>>16)&65535);
#define O R+=S;R*=S;R^=S;R&=0xffff;C=(double)R/65536;
#define AD(n) (E+=n,E-n)
#define AI(n) (J+=n,J-n)
#define MX(g,p) OU[0]+=X*g*fmin(1,1-p);OU[1]+=X*g*fmin(1,1+p);
#define NH 8
#define NS 8
#define MQ(q,f) {int*_i=q;for(char*_p=f;*_p;_p++,_i++){O;*_i=((double)(*(_p)-'0')/9)>C;}}

char M[1<<29],*P;
double D[1<<16],*E,C;
int I[1<<16],*J,S,R;

double fh(double x){return x*(27+x*x)/(27+9*x*x);}
double bq(double*f,double x)
{
	double*z=AD(2),y=f[0]*x+z[0];
	z[0]=f[1]*x-f[3]*y+z[1];
	z[1]=f[2]*x-f[4]*y;
	return y;
}

void hpf(double*f,double o,double q)
{
	double c=cos(o),a=sin(o)/(2*q);
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
	return (double)(i%p)/(double)p;
}

double qs(int*q,int n,int i,int B)
{
	int j=0;
	for(;j<n;j++) if(q[(i/B+n-j)%n])break;
	return (double)((i%B)+j*B) / (double)B;
}

double lr(double a,double b,double t)
{
	return a+t*(b-a);
}

int main(int _,char**av){
	S=atoi(av[1]);
	int r=1<<18,i,j;
	char*p0=M;
	P=p0;
	Z("SJGG")
	L(0)
	Z("XBWFgnu!")
	L(16)W(1)W(2)L(r)L(r*4)W(4)W(16)
	Z("ebub")
	L(0)
	char*p1=P;
	double h0[5],X,OU[2],t,u;

	int hp[NH];

	O;int B=(20000+R/2)/4;

	double pk[6],sc;

	char* KQ[]={
		"9155812391119125",
		"9171316397111875",
		"9111911192229345",
		"1212112212234678",
		"3000622230008888",
	};
	int kq[16],Kq[16];

	char* HQ[]={
		"12233350",
		"23322332",
		"22234222",
		"03132334",
		"12341231",
		"66666143",
		"62226222",
	};
	int hi=0,st=0,sx,Zn;
	O;int Y=800000+R*10;

	for(i=0;i<(1<<23);++i){
		E=D;J=I;
		if ((i&((1<<22)-1))==0) fputs("\njust a moment",stdout);
		if ((i&((1<<18)-1))==0) {putchar('.');fflush(stdout);}

		int*ss=AI(NS*2);

		OU[0]=OU[1]=0;

		if(0==(i%(B*512))){
			O;u=C;O;hpf(h0,u*0.6,C*4);
			for(j=0;j<NH;++j){O;hp[j]=40+R%1001;}
			O;pk[0]=0.5+C*30;
			O;pk[1]=5+C*300;
			O;pk[2]=5000+C*30000;
			O;pk[3]=1+10*C;
			O;pk[4]=50+400*C;
			O;pk[5]=1+(1+C)*4;
		}

		if(0==(i%(B*128))){
			O;hi=R/7;
			MQ(kq,KQ[hi%5])
			MQ(Kq,KQ[(hi+2)%5])
			O;sx=1+(R%6);
		}

		if(0==(i%(B*64))){
			O;Zn=2<<(1+((R/55)%2));
			O;pk[1]+=C*200;
			O;pk[1]-=C*200;
			O;pk[2]+=C*3000;
			O;pk[2]-=C*3000;
		}

		if(0==(i%(B*8))){
			O;st=Y*(1+((R/44)%sx));
			O;if((R/42)&1) st=(st*7)/8;
			O;if((R/69)&1) st=(st*9)/8;
			O;if((R/111)&1) st=(st*11)/12;

			for(j=0;j<NS;++j){
				O;
				ss[j*2+1]=st+(R*((j&1)?2:-3));
			}
		}
		if(0==(i%(B*4))){
			O;st+=R;O;st-=R;
			for(j=0;j<NS;++j){
				O;
				ss[j*2+1] -= R*5;
			}
			O;
			sc=0.1+C*C*C*100;
		}
		X=0;
		for(j=0;j<NS;++j){
			double v=1-sa(i,B*Zn);
			int t0=lr(st,ss[j*2+1],pow(v,sc));
			ss[j*2]+=t0;
			t=fh(qs(kq,16,i,B*4) - qs(Kq,16,i+B*4,B*8));
			X+=(double)ss[j*2]*5e-10*v*v*v*(1-t*t*t);
		}
		X=fh(X);
		MX(1,0)

		t=fh(qs(kq,16,i,B*4));
		X=fh(pk[0]*sin(t*pk[1]+t*(pk[2]/(pk[3]+t*pk[4])))*pow(1-fh(t),pk[5]));
		MX(1,0)

		X=0;
		for(j=0;j<NH;++j)X+=sq(i,hp[j]);
		X=bq(h0,X);
		X=fh(X*2);
		X*=pow(1-sa(i,B<<(HQ[hi%7][(i/(B*8))%8]-'0')),4);
		MX(1,sin((double)i*1e-5))

		for(j=0;j<2;++j){W((unsigned)(fmin(1,fmax(-1,OU[j]))*32767))}
	}
	int z=P-p1;p1=P;P=p0+4;L(z+36);P=p0+40;L(z);P=p1;
	Z("fokpz!")
	char*p2=P;
	Z("tpoh")
	P+=sprintf(P,"%d",S);
	Z("/xbw")
	*(P++)=0;
	FILE*o=fopen(p2,"wb");
	fwrite(p0,1,p1-p0,o);
	fclose(o);
	putchar('\n');
	puts(p1);
	return 0;
}
