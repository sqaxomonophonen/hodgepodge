
//double X=i&1024?0.5:-0.5;
//double X=(double)(i&4095) * (1.0/1024.0) - 0.5;
//X=fh(X*10);
//oo(&X,0.01,0);
//X=ms(0,X,0.01,0.19)*2;
//X=hp(X,1.5);
//X*=5555;
#if 0
oo(double*x,double o,double s)
{
	int i;
	double*z=AD(7),t=z[3]*0.360891+z[4]*0.417290+z[5]*0.177896+z[6]*0.0439725;
	for(i=0;i<3;++i)z[6-i]=z[5-i];
	z[0]+=(fh(*x-s*t)-fh(z[0]))*o;
	for(i=1;i<4;++i)z[i]+=(fh(z[i-1])-fh(z[i]))*o;
	*x=t;
}
#endif
#if 0
double ms(double l,double b,double o,double s)
{
	double*z=AD(2),r=s*(2-o),q=z[1]*r;
	z[0]+=o*(fh(l+b-q)-fh(z[0]));
	z[1]+=o*(fh(z[0]-b+q)-fh(z[1]));
	return z[1];
}
#endif

