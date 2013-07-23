/* fix_fft.c - Fixed-point in-place Fast Fourier Transform  */
/*
  All data are fixed-point short integers, in which -32768
  to +32768 represent -1.0 to +1.0 respectively. Integer
  arithmetic is used for speed, instead of the more natural
  floating-point.

  For the forward FFT (time -> freq), fixed scaling is
  performed to prevent arithmetic overflow, and to map a 0dB
  sine/cosine wave (i.e. amplitude = 32767) to two -6dB freq
  coefficients. The return value is always 0.

  Written by:  Tom Roberts  11/8/89
  Made portable:  Malcolm Slaney 12/15/94 malcolm@interval.com
  Enhanced:  Dimitrios P. Bouras  14 Jun 2006 dbouras@ieee.org
*/

#include <string.h>

#define N_WAVE      64    /* full length of Sinewave[] */
#define LOG2_N_WAVE 6      /* log2(N_WAVE) */

/*
  Henceforth "short" implies 16-bit word. If this is not
  the case in your architecture, please replace "short"
  with a type definition which *is* a 16-bit word.
*/
/*
  Since we only use 3/4 of N_WAVE, we define only
  this many samples, in order to conserve data space.
*/
short Sinewave[N_WAVE-N_WAVE/4] = {
  0,      3211,     6392,   9512,  12539,  15446,  18204,  20787,
  23170,  25330,   27245,  28898,  30273,  31357,  32138,  32610,
  32768,  32610,   32138,  31357,  30273,  28898,  27245,  25330,
  23170,  20787,   18204,  15446,  12539,   9512,   6392,   3211,
  0,      -3211,   -6392,  -9512, -12539, -15446, -18204, -20787,
  -23170, -25330, -27245, -28898, -30273, -31357, -32138, -32610,
};

/*
  FIX_MPY() - fixed-point multiplication & scaling.
  Scaling ensures that result remains 16-bit.
*/
#define FIX_MPY(a, b)	(((a*b) >> 15) + (((a*b) >> 14) & 0x01))

/*
  fix_fft() - perform forward fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m
*/
void fix_fft(short fr[], short fi[], short m) {
  int mr, nn, i, j, l, k, istep, n;
  short qr, qi, tr, ti, wr, wi;

  n = 1 << m;

  /* max FFT size = N_WAVE */
  if (n > N_WAVE) {
    return;
  }

  mr = 0;
  nn = n - 1;

  /* decimation in time - re-order data */
  for (m = 1; m <= nn; m += 1) {
    l = n;
    do {
      l >>= 1;
    } while (mr+l >= n);

    mr = (mr & (l-1)) + l;

    if (mr > m) {
      tr = fr[m];
      fr[m] = fr[mr];
      fr[mr] = tr;
      ti = fi[m];
      fi[m] = fi[mr];
      fi[mr] = ti;
    }
  }

  l = 1;
  k = LOG2_N_WAVE-1;
  while (l < n) {
    /*
      fixed scaling, for proper normalisation --
      there will be log2(n) passes, so this results
      in an overall factor of 1/n, distributed to
      maximise arithmetic accuracy.
    */
    /*
      it may not be obvious, but the shift will be
      performed on each data point exactly once,
      during this pass.
    */
    istep = l << 1;
    for (m=0; m<l; ++m) {
      j = m << k;
      /* 0 <= j < N_WAVE/2 */
      wr =  Sinewave[j+N_WAVE/4] >> 1;
      wi = -Sinewave[j]; wi >>= 1;

      for (i=m; i<n; i+=istep) {
	j = i + l;
	tr = FIX_MPY(wr,fr[j]) - FIX_MPY(wi,fi[j]);
	ti = FIX_MPY(wr,fi[j]) + FIX_MPY(wi,fr[j]);
	qr = fr[i] >> 1;
	qi = fi[i] >> 1;

	fr[j] = qr - tr;
	fi[j] = qi - ti;
	fr[i] = qr + tr;
	fi[i] = qi + ti;
      }
    }
    --k;
    l = istep;
  }
}

/**
 * Perform a 32-point Fast Fourier Transform, returning the magnitude at `index`.
 * Note the FFT is performed 'in-place' on the data passed to 'real'.
 */
int fft_32(short real[], short index) {
  short imag[32]; /* Prepare a blank imaginary array */
  memset(imag, 0, 32*sizeof(short));

  /* Do the FFT */
  fix_fft(real, imag, 5);

  /* Return the magnitude at `index` */
  return real[index]*real[index] + imag[index]*imag[index];
}
