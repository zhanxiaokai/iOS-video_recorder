#ifndef MAYER_H
#define MAYER_H

#define REAL float

class MayerFft
{

	public :

static void mayer_realfft(int n, REAL *real);
static void mayer_realifft(int n, REAL *real);
static void mayer_fft(int n, REAL *real, REAL *imag);
static void mayer_ifft(int n, REAL *real, REAL *imag);

	private:
static void mayer_fht(REAL *fz, int n);

const static REAL halsec[20];
const static REAL costab[20];
const static REAL sintab[20];
static REAL coswrk[20];
static REAL sinwrk[20];

};

#endif
