#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// FFTPACK port-to-C stolen from monty/xiph - public domain - thanks! see:
//    http://www.netlib.org/fftpack/fft.c
// The code here does forward real-FFT only; using it to make a spectrogram
//////////////////////////////////////////////////////////////////////////////

static void pp__drfti1(int n, float *wsave, int *ifac)
{
	static int ntryh[4] = { 4,2,3,5 };
	static float tpi = 6.28318530717958647692528676655900577;
	float arg,argh,argld,fi;
	int ntry=0,i,j=-1;
	int k1, l1, l2, ib;
	int ld, ii, ip, is, nq, nr;
	int ido, ipm, nfm1;
	int nl=n;
	int nf=0;

L101:
	j++;
	if (j < 4)
		ntry=ntryh[j];
	else
		ntry+=2;

L104:
	nq=nl/ntry;
	nr=nl-ntry*nq;
	if (nr!=0) goto L101;

	nf++;
	ifac[nf+1]=ntry;
	nl=nq;
	if(ntry!=2)goto L107;
	if(nf==1)goto L107;

	for (i=1;i<nf;i++){
		ib=nf-i+1;
		ifac[ib+1]=ifac[ib];
	}
	ifac[2] = 2;

L107:
	if(nl!=1)goto L104;
	ifac[0]=n;
	ifac[1]=nf;
	argh=tpi/n;
	is=0;
	nfm1=nf-1;
	l1=1;

	if(nfm1==0)return;

	for (k1=0;k1<nfm1;k1++){
		ip=ifac[k1+2];
		ld=0;
		l2=l1*ip;
		ido=n/l2;
		ipm=ip-1;

		for (j=0;j<ipm;j++){
			ld+=l1;
			i=is;
			argld=(float)ld*argh;
			fi=0.;
			for (ii=2;ii<ido;ii+=2){
				fi+=1.;
				arg=fi*argld;
				wsave[i++]=cosf(arg);
				wsave[i++]=sinf(arg);
			}
			is+=ido;
		}
		l1=l2;
	}
}

static void pp__dradf4(int ido,int l1,float *cc,float *ch,float *wa1, float *wa2,float *wa3)
{
	static float hsqt2 = .70710678118654752440084436210485;
	int i,k,t0,t1,t2,t3,t4,t5,t6;
	float ci2,ci3,ci4,cr2,cr3,cr4,ti1,ti2,ti3,ti4,tr1,tr2,tr3,tr4;
	t0=l1*ido;

	t1=t0;
	t4=t1<<1;
	t2=t1+(t1<<1);
	t3=0;

	for(k=0;k<l1;k++){
		tr1=cc[t1]+cc[t2];
		tr2=cc[t3]+cc[t4];
		ch[t5=t3<<2]=tr1+tr2;
		ch[(ido<<2)+t5-1]=tr2-tr1;
		ch[(t5+=(ido<<1))-1]=cc[t3]-cc[t4];
		ch[t5]=cc[t2]-cc[t1];

		t1+=ido;
		t2+=ido;
		t3+=ido;
		t4+=ido;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	for(k=0;k<l1;k++){
		t2=t1;
		t4=t1<<2;
		t5=(t6=ido<<1)+t4;
		for(i=2;i<ido;i+=2){
			t3=(t2+=2);
			t4+=2;
			t5-=2;

			t3+=t0;
			cr2=wa1[i-2]*cc[t3-1]+wa1[i-1]*cc[t3];
			ci2=wa1[i-2]*cc[t3]-wa1[i-1]*cc[t3-1];
			t3+=t0;
			cr3=wa2[i-2]*cc[t3-1]+wa2[i-1]*cc[t3];
			ci3=wa2[i-2]*cc[t3]-wa2[i-1]*cc[t3-1];
			t3+=t0;
			cr4=wa3[i-2]*cc[t3-1]+wa3[i-1]*cc[t3];
			ci4=wa3[i-2]*cc[t3]-wa3[i-1]*cc[t3-1];

			tr1=cr2+cr4;
			tr4=cr4-cr2;
			ti1=ci2+ci4;
			ti4=ci2-ci4;
			ti2=cc[t2]+ci3;
			ti3=cc[t2]-ci3;
			tr2=cc[t2-1]+cr3;
			tr3=cc[t2-1]-cr3;


			ch[t4-1]=tr1+tr2;
			ch[t4]=ti1+ti2;

			ch[t5-1]=tr3-ti4;
			ch[t5]=tr4-ti3;

			ch[t4+t6-1]=ti4+tr3;
			ch[t4+t6]=tr4+ti3;

			ch[t5+t6-1]=tr2-tr1;
			ch[t5+t6]=ti1-ti2;
		}
		t1+=ido;
	}
	if(ido%2==1)return;

L105:

	t2=(t1=t0+ido-1)+(t0<<1);
	t3=ido<<2;
	t4=ido;
	t5=ido<<1;
	t6=ido;

	for(k=0;k<l1;k++){
		ti1=-hsqt2*(cc[t1]+cc[t2]);
		tr1=hsqt2*(cc[t1]-cc[t2]);
		ch[t4-1]=tr1+cc[t6-1];
		ch[t4+t5-1]=cc[t6-1]-tr1;
		ch[t4]=ti1-cc[t1+t0];
		ch[t4+t5]=ti1+cc[t1+t0];
		t1+=ido;
		t2+=ido;
		t4+=t3;
		t6+=ido;
	}
}

static void pp__dradf2(int ido,int l1,float *cc,float *ch,float *wa1)
{
	int i,k;
	float ti2,tr2;
	int t0,t1,t2,t3,t4,t5,t6;

	t1=0;
	t0=(t2=l1*ido);
	t3=ido<<1;
	for(k=0;k<l1;k++){
		ch[t1<<1]=cc[t1]+cc[t2];
		ch[(t1<<1)+t3-1]=cc[t1]-cc[t2];
		t1+=ido;
		t2+=ido;
	}

	if(ido<2)return;
	if(ido==2)goto L105;

	t1=0;
	t2=t0;
	for(k=0;k<l1;k++){
		t3=t2;
		t4=(t1<<1)+(ido<<1);
		t5=t1;
		t6=t1+t1;
		for(i=2;i<ido;i+=2){
			t3+=2;
			t4-=2;
			t5+=2;
			t6+=2;
			tr2=wa1[i-2]*cc[t3-1]+wa1[i-1]*cc[t3];
			ti2=wa1[i-2]*cc[t3]-wa1[i-1]*cc[t3-1];
			ch[t6]=cc[t5]+ti2;
			ch[t4]=ti2-cc[t5];
			ch[t6-1]=cc[t5-1]+tr2;
			ch[t4-1]=cc[t5-1]-tr2;
		}
		t1+=ido;
		t2+=ido;
	}

	if(ido%2==1)return;

L105:
	t3=(t2=(t1=ido)-1);
	t2+=t0;
	for(k=0;k<l1;k++){
		ch[t1]=-cc[t2];
		ch[t1-1]=cc[t3];
		t1+=ido<<1;
		t2+=ido;
		t3+=ido;
	}
}

static void pp__dradfg(int ido,int ip,int l1,int idl1,float *cc,float *c1, float *c2,float *ch,float *ch2,float *wa)
{
	static float tpi=6.28318530717958647692528676655900577;
	int idij,ipph,i,j,k,l,ic,ik,is;
	int t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;
	float dc2,ai1,ai2,ar1,ar2,ds2;
	int nbd;
	float dcp,arg,dsp,ar1h,ar2h;
	int idp2,ipp2;

	arg=tpi/(float)ip;
	dcp=cosf(arg);
	dsp=sinf(arg);
	ipph=(ip+1)>>1;
	ipp2=ip;
	idp2=ido;
	nbd=(ido-1)>>1;
	t0=l1*ido;
	t10=ip*ido;

	if(ido==1)goto L119;
	for(ik=0;ik<idl1;ik++)ch2[ik]=c2[ik];

	t1=0;
	for(j=1;j<ip;j++){
		t1+=t0;
		t2=t1;
		for(k=0;k<l1;k++){
			ch[t2]=c1[t2];
			t2+=ido;
		}
	}

	is=-ido;
	t1=0;
	if(nbd>l1){
		for(j=1;j<ip;j++){
			t1+=t0;
			is+=ido;
			t2= -ido+t1;
			for(k=0;k<l1;k++){
				idij=is-1;
				t2+=ido;
				t3=t2;
				for(i=2;i<ido;i+=2){
					idij+=2;
					t3+=2;
					ch[t3-1]=wa[idij-1]*c1[t3-1]+wa[idij]*c1[t3];
					ch[t3]=wa[idij-1]*c1[t3]-wa[idij]*c1[t3-1];
				}
			}
		}
	}else{

		for(j=1;j<ip;j++){
			is+=ido;
			idij=is-1;
			t1+=t0;
			t2=t1;
			for(i=2;i<ido;i+=2){
				idij+=2;
				t2+=2;
				t3=t2;
				for(k=0;k<l1;k++){
					ch[t3-1]=wa[idij-1]*c1[t3-1]+wa[idij]*c1[t3];
					ch[t3]=wa[idij-1]*c1[t3]-wa[idij]*c1[t3-1];
					t3+=ido;
				}
			}
		}
	}

	t1=0;
	t2=ipp2*t0;
	if(nbd<l1){
		for(j=1;j<ipph;j++){
			t1+=t0;
			t2-=t0;
			t3=t1;
			t4=t2;
			for(i=2;i<ido;i+=2){
				t3+=2;
				t4+=2;
				t5=t3-ido;
				t6=t4-ido;
				for(k=0;k<l1;k++){
					t5+=ido;
					t6+=ido;
					c1[t5-1]=ch[t5-1]+ch[t6-1];
					c1[t6-1]=ch[t5]-ch[t6];
					c1[t5]=ch[t5]+ch[t6];
					c1[t6]=ch[t6-1]-ch[t5-1];
				}
			}
		}
	}else{
		for(j=1;j<ipph;j++){
			t1+=t0;
			t2-=t0;
			t3=t1;
			t4=t2;
			for(k=0;k<l1;k++){
				t5=t3;
				t6=t4;
				for(i=2;i<ido;i+=2){
					t5+=2;
					t6+=2;
					c1[t5-1]=ch[t5-1]+ch[t6-1];
					c1[t6-1]=ch[t5]-ch[t6];
					c1[t5]=ch[t5]+ch[t6];
					c1[t6]=ch[t6-1]-ch[t5-1];
				}
				t3+=ido;
				t4+=ido;
			}
		}
	}

L119:
	for(ik=0;ik<idl1;ik++)c2[ik]=ch2[ik];

	t1=0;
	t2=ipp2*idl1;
	for(j=1;j<ipph;j++){
		t1+=t0;
		t2-=t0;
		t3=t1-ido;
		t4=t2-ido;
		for(k=0;k<l1;k++){
			t3+=ido;
			t4+=ido;
			c1[t3]=ch[t3]+ch[t4];
			c1[t4]=ch[t4]-ch[t3];
		}
	}

	ar1=1.;
	ai1=0.;
	t1=0;
	t2=ipp2*idl1;
	t3=(ip-1)*idl1;
	for(l=1;l<ipph;l++){
		t1+=idl1;
		t2-=idl1;
		ar1h=dcp*ar1-dsp*ai1;
		ai1=dcp*ai1+dsp*ar1;
		ar1=ar1h;
		t4=t1;
		t5=t2;
		t6=t3;
		t7=idl1;

		for(ik=0;ik<idl1;ik++){
			ch2[t4++]=c2[ik]+ar1*c2[t7++];
			ch2[t5++]=ai1*c2[t6++];
		}

		dc2=ar1;
		ds2=ai1;
		ar2=ar1;
		ai2=ai1;

		t4=idl1;
		t5=(ipp2-1)*idl1;
		for(j=2;j<ipph;j++){
			t4+=idl1;
			t5-=idl1;

			ar2h=dc2*ar2-ds2*ai2;
			ai2=dc2*ai2+ds2*ar2;
			ar2=ar2h;

			t6=t1;
			t7=t2;
			t8=t4;
			t9=t5;
			for(ik=0;ik<idl1;ik++){
				ch2[t6++]+=ar2*c2[t8++];
				ch2[t7++]+=ai2*c2[t9++];
			}
		}
	}

	t1=0;
	for(j=1;j<ipph;j++){
		t1+=idl1;
		t2=t1;
		for(ik=0;ik<idl1;ik++)ch2[ik]+=c2[t2++];
	}

	if(ido<l1)goto L132;

	t1=0;
	t2=0;
	for(k=0;k<l1;k++){
		t3=t1;
		t4=t2;
		for(i=0;i<ido;i++)cc[t4++]=ch[t3++];
		t1+=ido;
		t2+=t10;
	}

	goto L135;

L132:
	for(i=0;i<ido;i++){
		t1=i;
		t2=i;
		for(k=0;k<l1;k++){
			cc[t2]=ch[t1];
			t1+=ido;
			t2+=t10;
		}
	}

L135:
	t1=0;
	t2=ido<<1;
	t3=0;
	t4=ipp2*t0;
	for(j=1;j<ipph;j++){

		t1+=t2;
		t3+=t0;
		t4-=t0;

		t5=t1;
		t6=t3;
		t7=t4;

		for(k=0;k<l1;k++){
			cc[t5-1]=ch[t6];
			cc[t5]=ch[t7];
			t5+=t10;
			t6+=ido;
			t7+=ido;
		}
	}

	if(ido==1)return;
	if(nbd<l1)goto L141;

	t1=-ido;
	t3=0;
	t4=0;
	t5=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t2;
		t3+=t2;
		t4+=t0;
		t5-=t0;
		t6=t1;
		t7=t3;
		t8=t4;
		t9=t5;
		for(k=0;k<l1;k++){
			for(i=2;i<ido;i+=2){
				ic=idp2-i;
				cc[i+t7-1]=ch[i+t8-1]+ch[i+t9-1];
				cc[ic+t6-1]=ch[i+t8-1]-ch[i+t9-1];
				cc[i+t7]=ch[i+t8]+ch[i+t9];
				cc[ic+t6]=ch[i+t9]-ch[i+t8];
			}
			t6+=t10;
			t7+=t10;
			t8+=ido;
			t9+=ido;
		}
	}
	return;

L141:

	t1=-ido;
	t3=0;
	t4=0;
	t5=ipp2*t0;
	for(j=1;j<ipph;j++){
		t1+=t2;
		t3+=t2;
		t4+=t0;
		t5-=t0;
		for(i=2;i<ido;i+=2){
			t6=idp2+t1-i;
			t7=i+t3;
			t8=i+t4;
			t9=i+t5;
			for(k=0;k<l1;k++){
				cc[t7-1]=ch[t8-1]+ch[t9-1];
				cc[t6-1]=ch[t8-1]-ch[t9-1];
				cc[t7]=ch[t8]+ch[t9];
				cc[t6]=ch[t9]-ch[t8];
				t6+=t10;
				t7+=t10;
				t8+=ido;
				t9+=ido;
			}
		}
	}
}

static void pp__drftf1(int n,float *c,float *ch,float *wa,int *ifac)
{
	int i,k1,l1,l2;
	int na,kh,nf;
	int ip,iw,ido,idl1,ix2,ix3;

	nf=ifac[1];
	na=1;
	l2=n;
	iw=n;

	for (k1=0;k1<nf;k1++) {
		kh=nf-k1;
		ip=ifac[kh+1];
		l1=l2/ip;
		ido=n/l2;
		idl1=ido*l1;
		iw-=(ip-1)*ido;
		na=1-na;

		if(ip!=4)goto L102;

		ix2=iw+ido;
		ix3=ix2+ido;
		if(na!=0)
			pp__dradf4(ido,l1,ch,c,wa+iw-1,wa+ix2-1,wa+ix3-1);
		else
			pp__dradf4(ido,l1,c,ch,wa+iw-1,wa+ix2-1,wa+ix3-1);
		goto L110;

L102:
		if(ip!=2)goto L104;
		if(na!=0)goto L103;

		pp__dradf2(ido,l1,c,ch,wa+iw-1);
		goto L110;

L103:
		pp__dradf2(ido,l1,ch,c,wa+iw-1);
		goto L110;

L104:
		if(ido==1)na=1-na;
		if(na!=0)goto L109;

		pp__dradfg(ido,ip,l1,idl1,c,c,c,ch,ch,wa+iw-1);
		na=1;
		goto L110;

L109:
		pp__dradfg(ido,ip,l1,idl1,ch,ch,ch,c,c,wa+iw-1);
		na=0;

L110:
		l2=l1;
	}

	if(na==1)return;

	for(i=0;i<n;i++)c[i]=ch[i];
}

//////////////////////////////////////////////////////////////////////////////
// END OF FFTPACK CODE
//////////////////////////////////////////////////////////////////////////////

struct PP__FFT {
	int n;
	float* wsave;
	int* ifac;
};

static void pp__fft_init(struct PP__FFT* fft, int n)
{
	assert(n > 1);
	memset(fft, 0, sizeof *fft);
	fft->n = n;
	assert((fft->wsave = calloc(3*n, sizeof *fft->wsave)) != NULL);
	assert((fft->ifac = calloc(32, sizeof *fft->ifac)) != NULL);
	pp__drfti1(fft->n, fft->wsave+fft->n, fft->ifac);
}

static float* pp__fft_alloc(struct PP__FFT* fft)
{
	float* r = calloc(fft->n, sizeof *r);
	assert(r != NULL);
	return r;
}

static void pp__fft_forward(struct PP__FFT* fft, float* data)
{
	pp__drftf1(fft->n, data, fft->wsave, fft->wsave + fft->n, fft->ifac);
}

static inline double nsinc(int downsample_factor, int index)
{
	assert(downsample_factor > 0);
	double scalar = 1.0 / (double)downsample_factor;
	if (index < 0) index = -index;
	if (index == 0) return scalar;
	if ((index % downsample_factor) == 0) return 0.0;
	const double pi = 3.141592653589793;
	double x = ((double)index / (double)downsample_factor) * pi;
	return scalar * (sin(x)/x);
}

static inline double bessel_I0(double x)
{
	double d = 0.0;
	double ds = 1.0;
	double s = 1.0;
	do {
		d += 2.0;
		ds *= (x*x) / (d*d);
		s += ds;
	} while (ds > s*1e-7);
	return s;
}

static inline double kaiser_bessel_attenuation_to_alpha(double attenuation)
{
	if (attenuation > 50.0f) {
		return 0.1102f * (attenuation - 8.7f);
	} else if (attenuation > 20.0f) {
		return 0.5842f * pow(attenuation - 21.0f, 0.4f) + 0.07886f * (attenuation - 21.0f);
	} else {
		return 0;
	}
}

static inline double kaiser_bessel(double alpha, double x)
{
	return bessel_I0(alpha * sqrt(1.0f - x*x)) / bessel_I0(alpha);
}

struct {
	int    n;
	int    downsample_factor;
	int    window_none_use;
	int    window_kaiser_bessel_use;
	double window_kaiser_bessel_alpha;
} cfg;

static inline double eval_window(int index)
{
	int n = cfg.n;
	if (index < 0) index = -index;
	assert(index < n);
	double ws = 0.0;
	if (cfg.window_none_use) {
		ws = 1.0;
	} else if (cfg.window_kaiser_bessel_use) {
		double wt = (double)index / (double)n;
		ws = kaiser_bessel(cfg.window_kaiser_bessel_alpha, wt);
	}
	return ws;
}

static inline double eval_fir(int index)
{
	return nsinc(cfg.downsample_factor, index) * eval_window(index);
}

int main(int argc, char** argv)
{
	if (argc != 5) {
		fprintf(stderr, "Usage: %s <downsample factor> <window function> <n> <mode>\n", argv[0]);
		fprintf(stderr, "\n");
		fprintf(stderr, "   <downsample factor>  Integer downsample factor\n");
		fprintf(stderr, "   <window function>    See below\n");
		fprintf(stderr, "   <n>                  Length of FIR filter (actual length is 2n-1)\n");
		fprintf(stderr, "   <mode>               See below\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Available window functions:\n");
		fprintf(stderr, "   none                          No window function; simply truncate at edge\n");
		fprintf(stderr, "   kaiser_bessel:alpha=<alpha>   Kaiser-Bessel window with specified alpha value\n");
		fprintf(stderr, "   kaiser_bessel:att=<att>       Kaiser-Bessel window with alpha value derived from attenuation\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Modes:\n");
		fprintf(stderr, "    c0    C output (normal)\n");
		fprintf(stderr, "    c1    C output (compact; no zeroes in table)\n");
		fprintf(stderr, "   svg    Analysis in SVG format\n");
		fprintf(stderr, "   txt    Print FIR filter coefficients\n");
		fprintf(stderr, "\n");
		exit(EXIT_FAILURE);
	}

	int downsample_factor = atoi(argv[1]);
	if (downsample_factor < 1) {
		fprintf(stderr, "invalid <downsample factor> value %d; must be 1 or above\n", downsample_factor);
		exit(EXIT_FAILURE);
	}

	// parse window function
	char window_str[1024];
	{
		char* arg = argv[2];
		char* p = arg;
		while (*p != 0 && *p != ':') p++;
		char* p0 = p;
		size_t n0 = p0 - arg;
		if (memcmp(arg, "none", n0) == 0) {
			if (*p != 0) {
				fprintf(stderr, "\"none\" takes no arguments; see usage\n");
				exit(EXIT_FAILURE);
			}
			snprintf(window_str, sizeof window_str, "no window");
			cfg.window_none_use = 1;
		} else if (memcmp(arg, "kaiser_bessel", n0) == 0) {
			if (*p != ':') {
				fprintf(stderr, "\"kaiser_bessel\" requires an argument; see usage\n");
				exit(EXIT_FAILURE);
			}
			p++;
			char* p1 = p;
			while(*p != 0 && *p != '=') p++;
			char* p2 = p;
			size_t n2 = p2 - p1;
			p++;
			double alpha = 0;
			if (memcmp(p1, "alpha", n2) == 0) {
				alpha = strtod(p, NULL);
				snprintf(window_str, sizeof window_str, "Kaiser-Bessel window (alpha=%.5f)", alpha);
			} else if (memcmp(p1, "att", n2) == 0 || memcmp(p1, "attenuation", n2) == 0) {
				double att = strtod(p, NULL);
				alpha = kaiser_bessel_attenuation_to_alpha(att);
				snprintf(window_str, sizeof window_str, "Kaiser-Bessel window (att=%.1f; alpha=%.5f)", att, alpha);
			} else {
				fprintf(stderr, "invalid kaiser-bessel argument in \"%s\"; see usage\n", arg);
				exit(EXIT_FAILURE);
			}
			cfg.window_kaiser_bessel_use = 1;
			cfg.window_kaiser_bessel_alpha = alpha;
		} else {
			fprintf(stderr, "invalid <window function> value: %s (see usage)\n", arg);
			exit(EXIT_FAILURE);
		}
	}

	int n = atoi(argv[3]);
	if (n < 0) {
		fprintf(stderr, "invalid <n> value %d; must be 0 or above\n", n);
		exit(EXIT_FAILURE);
	}

	cfg.downsample_factor = downsample_factor;
	cfg.n = n;

	int index0 = -n+1;
	int index1 = n-1;

	char* mode = argv[4];
	int do_c0 = strcmp(mode, "c0") == 0;
	int do_c1 = strcmp(mode, "c1") == 0;
	if (do_c0 || do_c1) {
		printf("// AUTO-GENERATED with `%s %s %s %s %s`\n", argv[0], argv[1], argv[2], argv[3], argv[4]);

		int df = cfg.downsample_factor;

		char lcname[1024];
		snprintf(lcname, sizeof lcname, "msd_%dx_downsample", df);
		char ucname[1024];
		snprintf(ucname, sizeof ucname, "MSD_%dX_DOWNSAMPLE", df);

		int skip_zeroes = do_c1;
		printf("static float %s_table[] = {\n", lcname);
		for (int index = index0; index <= index1; index++) {
			double coef = eval_fir(index);
			if (skip_zeroes && coef == 0.0) continue;
			printf("\t%.16f%s\n", coef, index < index1 ? "," : "");
		}
		printf("};\n");
		printf("#define %s_FIR_SZ (%d) // entire FIR table size\n", ucname, cfg.n*2-1);
		printf("#define %s_DELAY (%d)  // delay/latency introduced by filter\n", ucname, cfg.n-1);
		printf("#define %s_OUTPUT_SZ(n,step) (((n)-%s_FIR_SZ)/(step))\n", ucname, ucname);

		if (do_c0) {
			printf("static void %s(float* output, float* input, int n, int step)\n", lcname);
			printf("{\n");
			printf("\tconst int nm = n - %s_FIR_SZ;\n", ucname);
			printf("\tfloat* inp = input;\n");
			printf("\tfloat* outp = output;\n");
			printf("\tfor (int i = 0; i < nm; i+=step) {\n");
			printf("\t\tfloat acc = 0.0f;\n");
			printf("\t\tfor (int j = 0; j < %s_FIR_SZ; j++) {\n", ucname);
			printf("\t\t\tacc += inp[j] * %s_table[j];\n", lcname);
			printf("\t\t}\n");
			printf("\t\t*(outp++) = acc;\n");
			printf("\t\tinp += step;\n");
			printf("\t}\n");
			// XXX assert fails for step>1 ... off-by-one?
			printf("\t//assert((outp-output) == %s_OUTPUT_SZ(n,step));\n", ucname);
			printf("}\n");
		} else if (do_c1) {
			printf("// TODO :-)\n");
		}
	} else if (strcmp(mode, "svg") == 0) {
		// calculate frequency response of FIR filter
		// XXX seems larger FFT sizes reduces "rippling" in the
		// passband... not sure why, there's probably a good
		// mathematical explanation... there always is :) the rippling
		// also gets worse with larger n-values
		const int fft_size = 1<<16;
		struct PP__FFT fft;
		pp__fft_init(&fft, fft_size<<1);
		float* fft_buf = pp__fft_alloc(&fft);
		assert(fft_buf != NULL);

		double coef_max = 0.0;

		for (int index = index0; index <= index1; index++) {
			int fft_buf_index = index;
			if (fft_buf_index < 0) {
				fft_buf_index = (fft_size<<1) + fft_buf_index;
			}
			double coef = eval_fir(index);
			fft_buf[fft_buf_index] = coef; // XXX scaling?

			double coef_abs = fabs(coef);
			if (coef_abs > coef_max) coef_max = coef_abs;
		}

		pp__fft_forward(&fft, fft_buf);

		const int svg_width = 1920;
		const int svg_height = 1080;
		printf("<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d' viewBox='0 0 %d %d'>\n", svg_width, svg_height, svg_width, svg_height);
		printf("<rect fill='black' x='0' y='0' width='%d' height='%d'/>\n", svg_width, svg_height);

		printf("<text x='10' y='30' style='font-size: 15pt; fill: white;'>%d× downsampling // n=%d // %s</text>\n", cfg.downsample_factor, cfg.n, window_str);

		int xmargin = 10;

		// draw FIR / window
		{
			const int fir_height = svg_height/2;
			const int ymid = fir_height / 2;

			double wmax = 0.0;
			for (int index = index0; index <= index1; index++) {
				double w = eval_window(index);
				if (w > wmax) wmax = w;
			}

			double yscale = fir_height/2-10;

			printf("<polyline style='stroke:none;fill:rgba(50,255,200,0.1);' points='");
			printf("%f,%f ", (double)xmargin, (double)ymid);
			for (int index = index0; index <= index1; index++) {
				double t = (double)(index - index0) / (double)(index1 - index0);
				double x = (double)(xmargin + (svg_width-xmargin*2) * t);
				double y = ymid - (eval_window(index)/wmax)*yscale;
				printf("%f,%f ", x, y);
			}
			printf("%f,%f ", (double)(svg_width-xmargin), (double)ymid);
			printf("'/>\n");

			printf("<line x1='%d' y1='%d' x2='%d' y2='%d' style='stroke: rgba(255,255,255,0.5); stroke-width: 2;'/>\n",
				xmargin,fir_height/2,
				svg_width - xmargin, fir_height/2);

			for (int index = index0; index <= index1; index++) {
				double t = (double)(index - index0) / (double)(index1 - index0);
				double x = (double)(xmargin + (svg_width-xmargin*2) * t);
				double coef = eval_fir(index);
				double y1 = ymid;
				double y2 = ymid - (coef / coef_max) * yscale;
				printf("<line x1='%f' y1='%f' x2='%f' y2='%f' style='stroke:rgba(255,255,255,1); stroke-width: 2;'/>\n", x, y1, x, y2);
				if (coef != 0.0) {
					printf("<circle cx='%f' cy='%f' r='5' style='stroke:white;stroke-width:1;fill:none;'/>\n",x, y2);
				} else {
					printf("<circle cx='%f' cy='%f' r='6' style='stroke:none;fill:rgba(255,100,50,0.5);'/>\n",x, y2);
				}
			}
		}

		// draw frequency response
		{
			double ymid = svg_height*0.75;
			printf("<polyline style='fill:none;stroke:rgba(255,255,255,1);stroke-width:2;' points='");
			int istep = (fft_size / svg_width) - 1;
			if (istep < 1) istep = 1;
			for (int i = 1; i < fft_size; i+=istep) {
				float re = fft_buf[i*2];
				float im = fft_buf[i*2+1];
				float mag = sqrtf(re*re + im*im);
				double t = (double)i / (double)fft_size;
				double x = (double)xmargin + (double)(svg_width-xmargin*2)*t;
				double y = (double)svg_height - ((double)(mag) * (double)(svg_height/2));
				printf("%f,%f ", x, y);
			}
			printf("'/>\n");
		}

		printf("</svg>\n");
	} else if (strcmp(mode, "txt") == 0) {
		for (int index = index0; index <= index1; index++) {
			printf("x[%d] = %.16f\n", index, eval_fir(index));
		}
	} else {
		fprintf(stderr, "invalid <mode>; see usage\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
