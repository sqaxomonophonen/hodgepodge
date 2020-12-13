#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define F0_MIN (120)
#define F0_MAX (1200)

#define FIR_WIDTH (50)
#define FIR_SZ (FIR_WIDTH*2-1)

#define WAVEFORM_SZ (1<<7)

#define PI (3.141592653589793)
#define PI2 (PI*2.0)

#define DEBUG

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

static void map_audio(const char* path, float** samples, int* n_samples)
{
	assert(samples != NULL);
	assert(n_samples != NULL);

	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	struct stat st;
	if (fstat(fd, &st) == -1) {
		perror(path);
		exit(1);
	}

	void* mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED) {
		perror(path);
		exit(1);
	}

	close(fd);

	*samples = mem;
	*n_samples = (int)(st.st_size / sizeof(**samples));

	#ifdef DEBUG
	fprintf(stderr, "%s: %d samples\n", path, *n_samples);
	#endif
}

static inline float pp__n_center(int n)
{
	return (float)n * 0.5f - 0.5f;
}

static void pp__parabolic_regression(int n, float* ys, float* b_out, float* c_out)
{
	// consider the parabola:
	//
	//   y = a + b*x + c*x^2
	//
	// we want to find [a b c] so that the parabola best fits our x,y input
	// values. x values are given implicitly; they're sequential and
	// centered around x=0, so x[n-1]+1=x[n], and the sum of x values is 0.
	// to find [a b c] we need to solve the normal equations:
	//
	//   sum[y]     = a*n        + b*sum[x]   + c*sum[x^2]
	//   sum[x*y]   = a*sum[x]   + b*sum[x^2] + c*sum[x^3]
	//   sum[x^2*y] = a*sum[x^2] + b*sum[x^3] + c*sum[x^4]
	//
	// where sum[expression] is the sum of the expression for all input
	// values.


	// start by calculating sums ...
	//float sum_x = 0.0f;
	float sum_y = 0.0f;    // sum[y]
	float sum_xx = 0.0f;   // sum[x^2]
	float sum_xxx = 0.0f;  // sum[x^3]
	float sum_xxxx = 0.0f; // sum[x^4]
	float sum_xy = 0.0f;   // sum[x*y]
	float sum_xxy = 0.0f;  // sum[x^2*y]
	float center = pp__n_center(n);
	for (int i = 0; i < n; i++) {
		float x   = (float)i - center;
		float y   = ys[i];
		float xx  = x*x;
		//sum_x    += x;
		sum_y    += y;
		sum_xx   += xx;
		sum_xxx  += xx*x;
		sum_xxxx += xx*xx;
		sum_xy   += x*y;
		sum_xxy  += xx*y;
	}


	// now arrange the normal equations into matrix form:
	//
	//  B = A*X
	//
	// where B is:
	//    sum[y]
	//    sum[x*y]
	//    sum[x^2*y]
	//
	// and A is:
	//    n        sum[x]     sum[x^2]
	//    sum[x]   sum[x^2]   sum[x^3]
	//    sum[x^2] sum[x^3]   sum[x^4]
	//
	// and we need to find X:
	//    a
	//    b
	//    c
	//
	// the solution is X = inverse(A) * B


	// matrix A
	//    a11    a12    a13
	//    a21    a22    a23
	//    a31    a32    a33
	float a11 = n;
	//float a12 = sum_x; // sum_x=0
	float a13 = sum_xx;
	//float a21 = sum_x; // sum_x=0
	float a22 = sum_xx;
	float a23 = sum_xxx;
	float a31 = sum_xx;
	float a32 = sum_xxx;
	float a33 = sum_xxxx;


	// calculate the inverse of matrix of A
	//
	//   inverse(A) = 1/determinant(A) * cofactor_matrix(A)
	//
	float determinant_a =
		     a11 * (  a22*a33   - a23*a32)
		// - a12 * (  a21*a33   - a23*a31)
		   + a13 * (/*a21*a32*/ - a22*a31);
	float one_over_determinant_a = 1.0f / determinant_a;
	//float ia11 = (   a22*a33     - a23*a32   ) * one_over_determinant_a;
	//float ia12 = ( /*a21*a33*/   - a23*a31   ) * one_over_determinant_a;
	//float ia13 = ( /*a21*a32*/   - a22*a31   ) * one_over_determinant_a;
	float ia21 = ( /*a12*a33*/   - a13*a32   ) * one_over_determinant_a;
	float ia22 = (   a11*a33     - a13*a31   ) * one_over_determinant_a;
	float ia23 = (   a11*a32   /*- a12*a31*/ ) * one_over_determinant_a;
	float ia31 = ( /*a12*a23*/   - a13*a22   ) * one_over_determinant_a;
	float ia32 = (   a11*a23   /*- a13*a21*/ ) * one_over_determinant_a;
	float ia33 = (   a11*a22   /*- a12*a21*/ ) * one_over_determinant_a;


	// matrix B
	float b1 = sum_y;
	float b2 = sum_xy;
	float b3 = sum_xxy;


	// now calculate X = inverse(A) * B
	//float a = ia11*b1 + ia12*b2 + ia13*b3;
	float b = ia21*b1 + ia22*b2 + ia23*b3;
	float c = ia31*b1 + ia32*b2 + ia33*b3;


	// finally return requested values
	//if (a_out) *a_out = a;
	if (b_out) *b_out = b;
	if (c_out) *c_out = c;
}

static inline float pp__parabola_pivot(int n, float* ys)
{
	float b, c;
	pp__parabolic_regression(n, ys, &b, &c);
	return -b / (2*c) + pp__n_center(n);
}

struct output {
	int ok;
	float nsdf_best_score;
	float nsdf_best_frequency;
	float fdomain[WAVEFORM_SZ-2];
};

static inline float nsinc(float x)
{
	if (fabsf(x) < 1e-5) return 1.0; // XXX probably ok? :)
	const float nx = PI*x;
	return sinf(nx)/nx;
}

#if 0
static inline float nsinc_lpf(float x, float ratio)
{
	//if (ratio > 1.0f) ratio = 1.0f; // XXX leave it to caller maybe? same for multiple consequtive calls
	return ratio * nsinc(ratio * x);
}
#endif

#if 1
static inline int resample_is_within_bounds(int index, int n)
{
	const int margin = 1000; // XXX this is STUPIIIID
	return index >= margin && index < (n-margin);
}

static inline void resample(int n_out, float* out, float n_in, float* in)
{
	const float interpolate_step = n_in / (float)n_out;

	// cutoff (1.0 = nyquist)
	float cut = (float)n_out / n_in;
	if (cut > 1.0f) cut = 1.0f;

	float interpolate_position_f = 0.0f;

	float u = 0.0f;
	float ustep = 1.0f / (float)n_out;
	for (int output_index = 0; output_index < n_out; output_index++) {
		float sums[2];
		for (int piece = 0; piece < 2; piece++) {
			float interpolate_position_fpi = interpolate_position_f + n_in * (float)piece;
			float interpolate_position_r = roundf(interpolate_position_fpi);
			int interpolate_position = (int)interpolate_position_r;
			float sum = 0.0f;
			float* inp = &in[interpolate_position - FIR_WIDTH + 1];
			float shift = interpolate_position_r - interpolate_position_fpi;
			for (int k = 0; k < FIR_SZ; k++) {
				float s = inp[k];
				float t = (float)(k - FIR_WIDTH + 1) + shift;
				float h = cut*nsinc(t*cut);
				float tri = 1.0f - (fabsf(t) * (1.0f / (float)(FIR_WIDTH)));
				sum += s * h * tri;
			}
			sums[piece] = sum;
		}
		out[output_index] = sums[0]*u + sums[1]*(1.0f-u);
		u += ustep;
		interpolate_position_f += interpolate_step;
	}
}
#endif

#if 0
static inline int resample_is_within_bounds(int index, int n)
{
	return index >= (FIR_WIDTH-1) && index < (n-FIR_WIDTH+1);
}

static inline void resample(int n_out, float* out, float n_in, float* in)
{
	const float interpolate_step = n_in / (float)n_out;

	// cutoff (1.0 = nyquist)
	float cut = (float)n_out / n_in;
	if (cut > 1.0f) cut = 1.0f;

	float interpolate_position_f = 0.0f;

	for (int output_index = 0; output_index < n_out; output_index++) {
		float interpolate_position_r = roundf(interpolate_position_f);
		int interpolate_position = (int)interpolate_position_r;
		float sum = 0.0f;
		float* inp = &in[interpolate_position - FIR_WIDTH + 1];
		float shift = interpolate_position_r - interpolate_position_f;
		for (int k = 0; k < FIR_SZ; k++) {
			float s = inp[k];
			float t = (float)(k - FIR_WIDTH + 1) + shift;
			float h = cut*nsinc(t*cut);
			float tri = 1.0f - (fabsf(t) * (1.0f / (float)(FIR_WIDTH)));
			sum += s * h * tri;
		}
		out[output_index] = sum;
		interpolate_position_f += interpolate_step;
	}
}
#endif

#if 0
static inline void resample(int n_out, float* out, float n_in, float* in)
{
	// drop-sample interpolator for debugging
	const float interpolate_step = n_in / (float)n_out;
	float interpolate_position_f = 0.0f;
	for (int output_index = 0; output_index < n_out; output_index++) {
		float interpolate_position_r = roundf(interpolate_position_f);
		int interpolate_position = (int)interpolate_position_r;
		out[output_index] = in[interpolate_position];
		interpolate_position_f += interpolate_step;
	}
}
#endif

static inline float angle_between_vectors(float x0, float y0, float x1, float y1)
{
	//angle = atan2( a.x*b.y - a.y*b.x, a.x*b.x + a.y*b.y );
	return atan2f(
		x0*y1 - y0*x1,
		x0*x1 + y0*y1
	);
}

static inline int f8(float f)
{
	return (int)fmaxf(0.0f, fminf(255.0f, f*255.0f));
}

static inline void angle_to_hue(float angle, int* red, int* green, int* blue)
{
	while (angle < 0) angle += 2*PI;
	while (angle > 2*PI) angle -= 2*PI;
	float hf = angle * (3.0/PI);
	int hi = (int)floorf(hf);
	float tp = hf - (float)hi;
	float tm = 1.0f - tp;

	int pos = f8(tp);
	int neg = f8(tm);


	switch (hi) {
	case 0:
		*red = 255;
		*green = pos;
		*blue = 0;
		break;
	case 1:
		*red = neg;
		*green = 255;
		*blue = 0;
		break;
	case 2:
		*red = 0;
		*green = 255;
		*blue = pos;
		break;
	case 3:
		*red = 0;
		*green = neg;
		*blue = 255;
		break;
	case 4:
		*red = pos;
		*green = 0;
		*blue = 255;
		break;
	default:
	case 5:
		*red = 255;
		*green = 0;
		*blue = neg;
		break;
	}
}


int main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <sample rate> <raw audio file>\n", argv[0]);
		fprintf(stderr, "   <sample rate>     Raw integer sample rate, e.g. 12000\n");
		fprintf(stderr, "   <raw audio file>  Raw audio in mono, f32, native endian\n");
		fprintf(stderr, "You can use sox for this, e.g.:\n");
		fprintf(stderr, " $ sox test.wav -r 12000 --bits 32 --encoding floating-point --endian little test12khz.raw\n");
		fprintf(stderr, " $ %s 12000 test12khz.raw\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int sample_rate = atoi(argv[1]);
	assert(sample_rate >= 8000 && sample_rate < 200000);

	float* samples;
	int n_samples;

	map_audio(argv[2], &samples, &n_samples);

	const int output_rate_hz = 200;
	const int samples_per_frame = sample_rate / output_rate_hz;

	// for NSDF to function properly I need at least two cycles of a waveform
	const int nsdf_sz = sample_rate / (F0_MIN/2);

	float* nsdf = calloc(nsdf_sz, sizeof *nsdf);
	assert(nsdf != NULL);

	int index_max = n_samples - nsdf_sz;

	const int n_outputs = (index_max / samples_per_frame) + 1;
	struct output* outputs = calloc(n_outputs, sizeof *outputs);
	assert(outputs != NULL);

	const int fft_size = WAVEFORM_SZ;
	struct PP__FFT fft;
	pp__fft_init(&fft, fft_size);
	float* fft_buf = pp__fft_alloc(&fft);
	assert(fft_buf != NULL);

	int n_iterations = 0;
	for (int index = 0; index < index_max; index += samples_per_frame) {

		float* cursor = &samples[index];

		// perform NSDF
		for (int i = 0; i < nsdf_sz; i++) {
			float rsum = 0;
			float msum = 0;
			for (int j = 0; j < (nsdf_sz - i); j++) {
				float a = cursor[j];
				float b = cursor[j+i];
				rsum += a*b;
				msum += a*a + b*b;
			}
			nsdf[i] = (2*rsum) / msum;
		}

		// MPM peak finding
		float nsdf_best_score = 0.0f;
		float nsdf_best_frequency = 0.0f;
		{
			int i = 0;

			// advance to first positive-slope crossing
			for (; i < nsdf_sz && nsdf[i] > 0; i++) {}
			for (; i < nsdf_sz && nsdf[i] <= 0; i++) {}

			const int i0 = i;

			// find overall max and use it to calculate threshold
			float x_max = 0;
			for (i = i0; i < nsdf_sz; i++) {
				float x = nsdf[i];
				if (x > x_max) x_max = x;
			}
			const float k = 0.80; // magical constant woooOOoo
			const float threshold = x_max * k;

			// first highest point within threshold is our f0 guess
			for (i = i0; i < nsdf_sz && nsdf[i] < threshold; i++) {}
			x_max = 0;
			int i_max = 0;
			int i_end = (nsdf_sz*5)/6; // skip garbage at the end?
			for (; i < i_end && nsdf[i] >= threshold; i++) {
				float x = nsdf[i];
				if (x > x_max) {
					x_max = x;
					i_max = i;
				}
			}

			// refine location of peak
			const int parabola_window_size = 5;
			if (x_max > 0 && i_max >= (parabola_window_size/2) && i_max < (nsdf_sz-(parabola_window_size/2))) {
				float better_i = pp__parabola_pivot(5, &nsdf[i_max-(parabola_window_size/2)]) + (float)i_max - (float)(parabola_window_size/2);
				nsdf_best_score = x_max;
				nsdf_best_frequency = (float)sample_rate / better_i;
			}
		}

		int ok =
			   (nsdf_best_score > 0)
			&& (nsdf_best_frequency >= F0_MIN)
			&& (nsdf_best_frequency <= F0_MAX);

		ok = ok && resample_is_within_bounds(index, n_samples);

		if (ok) {
			#if 0
			const int magic_frame = 784;
			//const int magic_frame = 10208;
			if (n_iterations == magic_frame) {
				float f0 = nsdf_best_frequency;
				float period = (float)sample_rate / f0;
				resample(fft_size, fft_buf, period, cursor);
			}
			#endif

			float f0 = nsdf_best_frequency;
			float period = (float)sample_rate / f0;
			resample(fft_size, fft_buf, period, cursor);
			pp__fft_forward(&fft, fft_buf);

			#if 0
			if (nsdf_best_score > 0.98) {
				printf("f0=%f\n", nsdf_best_frequency);
				for (int k = 1; k < fft_size-1; k+=2) {
					printf("  #%d: %f + %fi\n", k, fft_buf[k], fft_buf[k+1]);
				}
				assert(!"NO");
			}
			#endif
		}

		{
			struct output* o = &outputs[n_iterations];
			o->ok = ok;
			o->nsdf_best_frequency = nsdf_best_frequency;
			o->nsdf_best_score = nsdf_best_score;
			memcpy(o->fdomain, fft_buf+1, sizeof(o->fdomain));
		}

		#ifdef DEBUG
		fprintf(stderr, "%d/%d (%d%%) i:%d  %c  NSDF[freq=%.1f;score=%.2f]\n",
			index, index_max, index*100/index_max, n_iterations,
			ok ? 'X' : '.',
			nsdf_best_frequency, nsdf_best_score);
		#endif

		n_iterations++;
	}

	{
		const int pitch_height = 500;
		const int harmonic_height = 48;
		const int hspacing = 4;
		const int n_harmonics = (WAVEFORM_SZ-2)/2;
		const int img_width = n_iterations;
		const int img_height = pitch_height + hspacing + (harmonic_height+hspacing) * n_harmonics;
		const int ncomp = 3;
		const int stride = img_width*ncomp;
		//printf("%d × %d\n", img_width, img_height);

		unsigned char* img = calloc(ncomp*img_width*img_height, 1);
		assert(img != NULL);

		for (int index = 0; index < n_iterations; index++) {
			struct output* o = &outputs[index];
			if (!o->ok) continue;

			const int x = index;

			{
				for (int y = 0; y < pitch_height; y++) {
					unsigned char* pixel = &img[x*ncomp + y*stride];
					pixel[0] = 0;
					pixel[1] = 10;
					pixel[2] = 50;
				}

				const float fmin = 0.0f;
				const float fmax = 1000.0f;
				int y = pitch_height - (int)(((o->nsdf_best_frequency - fmin) / (fmax-fmin)) * (float)pitch_height);
				if (y < 0) y = 0;
				if (y >= pitch_height) y = pitch_height;

				unsigned char* pixel = &img[x*ncomp + y*stride];
				pixel[0] = 255;
				pixel[1] = 255;
				pixel[2] = 255;
			}

			for (int i = 0; i < n_harmonics; i++) {
				int y0 = pitch_height + hspacing + (harmonic_height+hspacing) * i;
				int first_harmonic = i == 0;

				float re = o->fdomain[i*2];
				float im = o->fdomain[i*2+1];
				float mag = sqrtf(re*re + im*im);

				int r,g,b;
				if (first_harmonic) {
					angle_to_hue(0, &r, &g, &b);
				} else {
					float angle = angle_between_vectors(o->fdomain[0], o->fdomain[1], re, im);
					angle_to_hue(angle, &r, &g, &b);
				}


				float ymag = (logf(mag + 1e-6f) + 2.0f) * 0.2f;
				if (ymag < 0.0f) ymag = 0.0f;
				if (ymag > 1.0f) ymag = 1.0f;

				int iymag = (int)(ymag * harmonic_height);

				for (int j = 0; j < harmonic_height; j++) {
					if (j <= (harmonic_height-iymag)) continue;
					int y = y0+j;

					unsigned char* pixel = &img[x*ncomp + y*stride];
					pixel[0] = r;
					pixel[1] = g;
					pixel[2] = b;
				}
			}
		}

		stbi_write_png("out.png", img_width, img_height, ncomp, img, stride);
	}



	#if 0
	// visualization; analysis->svg
	{
		//const int svg_width = 1920*6;
		const int svg_width = n_iterations;
		const int svg_height = 1080;
		printf("<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d' viewBox='0 0 %d %d'>\n", svg_width, svg_height, svg_width, svg_height);
		printf("<rect fill='black' x='0' y='0' width='%d' height='%d'/>\n", svg_width, svg_height);

		{
			const int h = 400;
			printf("<rect style='fill:rgba(0,0,255,0.1);' x='0' y='0' width='%d' height='%d'/>\n", svg_width, h);
			for (int i = 0; i < n_iterations; i++) {
				struct output* o = &outputs[i];

				float t = (float)i / (float)n_iterations;
				float x = (float)svg_width * t;

				if (o->ok) {
					const float fmin = 0.0f;
					const float fmax = 1000.0f;
					float yn = (o->nsdf_best_frequency - fmin) / (fmax-fmin);
					int oob = 0;
					if (yn < 0.0f) {
						yn = 0.0f;
						oob = 1;
					}
					if (yn > 1.0f) {
						yn = 1.0f;
						oob = 1;
					}
					float y = (float)h - yn*(float)h;

					int red;
					int green;
					int blue;
					float alpha = o->nsdf_best_score;
					float radius;
					if (oob) {
						red = 255;
						green = 0;
						blue = 0;
						radius = 2.5f;
					} else {
						red = 255;
						green = 255;
						blue = 255;
						radius = 1.0f;
					}

					printf("<circle cx='%f' cy='%f' r='%.1f' style='fill:rgba(%d,%d,%d,%f)'/>\n", x, y, radius, red, green, blue, alpha);

					//for (int i = 0; i < 16; i++) {
					//}
				}
			}
		}

		#if 0
		{
			printf("<polyline style='stroke:white;stroke-width:1;fill:none;' points='");
			const int n = fft_size*2;
			for (int i = 0; i <= (n); i++) {
				float x = (float)i * (1920.0f / (float)(n));
				float y = 700.0f + fft_buf[i%fft_size] * 1000.0f;
				printf("%.2f,%.2f ", x, y);
			}
			printf("'/>\n");
		}
		#endif

		printf("<text x='10' y='30' style='font-size: 15pt; fill: white;'>%d iterations</text>\n", n_iterations);

		printf("</svg>\n");
	}
	#endif

	return 0;
}
