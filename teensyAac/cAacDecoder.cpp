// cAacDecoder.cpp
//{{{  includes
#include <stdlib.h>
#include <string.h>

#include "cAacDecoder.h"

#include "aaccommon.h"
#include "assembly.h"
#include "cBitStream.h"
#include "huffman.h"
#include "sbr.h"
#include "tables.h"
//}}}
//{{{  defines
#define IS_ADIF(p)    ((p)[0] == 'A' && (p)[1] == 'D' && (p)[2] == 'I' && (p)[3] == 'F')
#define GET_ELE_ID(p) ((AACElementID)(*(p) >> (8-NUM_SYN_ID_BITS)))

#define X0_COEF_2 0xc0000000  /* Q29: -2.0 */
#define X0_OFF_2  0x60000000  /* Q29:  3.0 */
#define Q26_3   0x0c000000  /* Q26:  3.0 */

#define FBITS_LPC_COEFS 20
#define NUM_ITER_INVSQRT  4

/* sqrt(0.5), format = Q31 */
#define SQRTHALF 0x5a82799a
#define SF_OFFSET     100

#define NUM_FFT_SIZES 2

#define FBITS_IN_IMDCT          FBITS_OUT_DQ_OFF        /* number of fraction bits into IMDCT */
#define FBITS_LOST_DCT4         1               /* number of fraction bits lost (>> out) in DCT-IV */
#define FBITS_LOST_WND          1               /* number of fraction bits lost (>> out) in synthesis window (neg = gain frac bits) */
#define FBITS_LOST_IMDCT        (FBITS_LOST_DCT4 + FBITS_LOST_WND)
#define FBITS_OUT_IMDCT         (FBITS_IN_IMDCT - FBITS_LOST_IMDCT)

#define SQRT1_2 0x5a82799a  /* sqrt(1/2) in Q31 */
//}}}

//{{{  helpers
//{{{
void BitReverse (int* inout, int tabidx) {

  #define swapcplx(p0,p1) \
    t = p0; t1 = *(&(p0)+1); p0 = p1; *(&(p0)+1) = *(&(p1)+1); p1 = t; *(&(p1)+1) = t1

  int a, b, t, t1;
  const unsigned char* tab = bitrevtab + bitrevtabOffset[tabidx];
  int nbits = nfftlog2Tab[tabidx];

  int* part0 = inout;
  int* part1 = inout + (1 << nbits);

  while ((a = *tab++) != 0) {
    b = *tab++;
    swapcplx(part0[4*a+0], part0[4*b+0]); /* 0xxx0 <-> 0yyy0 */
    swapcplx(part0[4*a+2], part1[4*b+0]); /* 0xxx1 <-> 1yyy0 */
    swapcplx(part1[4*a+0], part0[4*b+2]); /* 1xxx0 <-> 0yyy1 */
    swapcplx(part1[4*a+2], part1[4*b+2]); /* 1xxx1 <-> 1yyy1 */
    }

  do {
    swapcplx(part0[4*a+2], part1[4*a+0]); /* 0xxx1 <-> 1xxx0 */
    } while ((a = *tab++) != 0);
  }
//}}}
//{{{
// Description: radix-4 trivial pass for decimation-in-time FFT
// Inputs:      buffer of (bit-reversed) samples
//              number of R4 butterflies per group (i.e. nfft / 4)
// Outputs:     processed samples in same buffer
// Notes:       assumes 2 guard bits, gains no integer bits,
//                guard bits out = guard bits in - 2

void R4FirstPass (int* x, int bg) {

  for (; bg != 0; bg--) {
    int ar = x[0] + x[2];
    int br = x[0] - x[2];
    int ai = x[1] + x[3];
    int bi = x[1] - x[3];
    int cr = x[4] + x[6];
    int dr = x[4] - x[6];
    int ci = x[5] + x[7];
    int di = x[5] - x[7];

    /* max per-sample gain = 4.0 (adding 4 inputs together) */
    x[0] = ar + cr;
    x[4] = ar - cr;
    x[1] = ai + ci;
    x[5] = ai - ci;
    x[2] = br + di;
    x[6] = br - di;
    x[3] = bi - dr;
    x[7] = bi + dr;

    x += 8;
    }
  }
//}}}
//{{{
void R8FirstPass (int* x, int bg) {

  for (; bg != 0; bg--) {
    int ar = x[0] + x[2];
    int br = x[0] - x[2];
    int ai = x[1] + x[3];
    int bi = x[1] - x[3];
    int cr = x[4] + x[6];
    int dr = x[4] - x[6];
    int ci = x[5] + x[7];
    int di = x[5] - x[7];

    int sr = ar + cr;
    int ur = ar - cr;
    int si = ai + ci;
    int ui = ai - ci;
    int tr = br - di;
    int vr = br + di;
    int ti = bi + dr;
    int vi = bi - dr;

    ar = x[ 8] + x[10];
    br = x[ 8] - x[10];
    ai = x[ 9] + x[11];
    bi = x[ 9] - x[11];
    cr = x[12] + x[14];
    dr = x[12] - x[14];
    ci = x[13] + x[15];
    di = x[13] - x[15];

    /* max gain of wr/wi/yr/yi vs input = 2 *  (sum of 4 samples >> 1) */
    int wr = (ar + cr) >> 1;
    int yr = (ar - cr) >> 1;
    int wi = (ai + ci) >> 1;
    int yi = (ai - ci) >> 1;

    /* max gain of output vs input = 4 *  (sum of 4 samples >> 1 + sum of 4 samples >> 1) */
    x[ 0] = (sr >> 1) + wr;
    x[ 8] = (sr >> 1) - wr;
    x[ 1] = (si >> 1) + wi;
    x[ 9] = (si >> 1) - wi;
    x[ 4] = (ur >> 1) + yi;
    x[12] = (ur >> 1) - yi;
    x[ 5] = (ui >> 1) - yr;
    x[13] = (ui >> 1) + yr;

    ar = br - di;
    cr = br + di;
    ai = bi + dr;
    ci = bi - dr;

    /* max gain of xr/xi/zr/zi vs input = 4*sqrt(2)/2 = 2*sqrt(2)
     *  (sum of 8 samples, multiply by sqrt(2)/2, implicit >> 1 from Q31) */
    int xr = MULSHIFT32(SQRT1_2, ar - ai);
    int xi = MULSHIFT32(SQRT1_2, ar + ai);
    int zr = MULSHIFT32(SQRT1_2, cr - ci);
    int zi = MULSHIFT32(SQRT1_2, cr + ci);

    /* max gain of output vs input = (2 + 2*sqrt(2) ~= 4.83)
     *  (sum of 4 samples >> 1, plus xr/xi/zr/zi with gain of 2*sqrt(2))
     * in absolute terms, we have max gain of appx 9.656 (4 + 0.707*8)
     *  but we also gain 1 int bit (from MULSHIFT32 or from explicit >> 1) */
    x[ 6] = (tr >> 1) - xr;
    x[14] = (tr >> 1) + xr;
    x[ 7] = (ti >> 1) - xi;
    x[15] = (ti >> 1) + xi;
    x[ 2] = (vr >> 1) + zi;
    x[10] = (vr >> 1) - zi;
    x[ 3] = (vi >> 1) - zr;
    x[11] = (vi >> 1) + zr;

    x += 16;
    }
  }
//}}}
//{{{
void R4Core (int* x, int bg, int gp, int* wtab) {

  for (; bg != 0; gp <<= 2, bg >>= 2) {
    int step = 2*gp;
    int* xptr = x;

    /* max per-sample gain, per group < 1 + 3*sqrt(2) ~= 5.25 if inputs x are full-scale
     * do 3 groups for long block, 2 groups for short block (gain 2 int bits per group)
     *
     * very conservative scaling:
     *   group 1: max gain = 5.25,           int bits gained = 2, gb used = 1 (2^3 = 8)
     *   group 2: max gain = 5.25^2 = 27.6,  int bits gained = 4, gb used = 1 (2^5 = 32)
     *   group 3: max gain = 5.25^3 = 144.7, int bits gained = 6, gb used = 2 (2^8 = 256)
     */
    for (int i = bg; i != 0; i--) {
      int* wptr = wtab;
      for (int j = gp; j != 0; j--) {
        int ar = xptr[0];
        int ai = xptr[1];
        xptr += step;

        /* gain 2 int bits for br/bi, cr/ci, dr/di (MULSHIFT32 by Q30) * gain 1 net GB */
        int ws = wptr[0];
        int wi = wptr[1];
        int br = xptr[0];
        int bi = xptr[1];
        int wd = ws + 2*wi;
        int tr = MULSHIFT32(wi, br + bi);
        br = MULSHIFT32(wd, br) - tr; /* cos*br + sin*bi */
        bi = MULSHIFT32(ws, bi) + tr; /* cos*bi - sin*br */
        xptr += step;

        ws = wptr[2];
        wi = wptr[3];
        int cr = xptr[0];
        int ci = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, cr + ci);
        cr = MULSHIFT32(wd, cr) - tr;
        ci = MULSHIFT32(ws, ci) + tr;
        xptr += step;

        ws = wptr[4];
        wi = wptr[5];
        int dr = xptr[0];
        int di = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, dr + di);
        dr = MULSHIFT32(wd, dr) - tr;
        di = MULSHIFT32(ws, di) + tr;
        wptr += 6;

        tr = ar;
        int ti = ai;
        ar = (tr >> 2) - br;
        ai = (ti >> 2) - bi;
        br = (tr >> 2) + br;
        bi = (ti >> 2) + bi;

        tr = cr;
        ti = ci;
        cr = tr + dr;
        ci = di - ti;
        dr = tr - dr;
        di = di + ti;

        xptr[0] = ar + ci;
        xptr[1] = ai + dr;
        xptr -= step;
        xptr[0] = br - cr;
        xptr[1] = bi - di;
        xptr -= step;
        xptr[0] = ar - ci;
        xptr[1] = ai - dr;
        xptr -= step;
        xptr[0] = br + cr;
        xptr[1] = bi + di;
        xptr += 2;
        }
      xptr += 3*step;
      }
    wtab += 3*step;
    }
  }
//}}}
//{{{
void R4FFT (int tabidx, int* x) {

  int order = nfftlog2Tab[tabidx];
  int nfft = nfftTab[tabidx];

  /* decimation in time */
  BitReverse(x, tabidx);

  if (order & 0x1) {
    /* long block: order = 9, nfft = 512 */
    R8FirstPass(x, nfft >> 3);            /* gain 1 int bit,  lose 2 GB */
    R4Core(x, nfft >> 5, 8, (int *)twidTabOdd);   /* gain 6 int bits, lose 2 GB */
    }
  else {
    /* short block: order = 6, nfft = 64 */
    R4FirstPass(x, nfft >> 2);            /* gain 0 int bits, lose 2 GB */
    R4Core(x, nfft >> 4, 4, (int *)twidTabEven);  /* gain 4 int bits, lose 1 GB */
    }
  }
//}}}
//{{{
void PreMultiply (int tabidx, int* zbuf1)
{
  int i, nmdct, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  zbuf2 = zbuf1 + nmdct - 1;
  csptr = cos4sin4tab + cos4sin4tabOffset[tabidx];

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = nmdct >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0);
    ai2 = *(zbuf1 + 1);
    ai1 = *(zbuf2 + 0);
    ar2 = *(zbuf2 - 1);

    /* gain 2 ints bit from MULSHIFT32 by Q30, but drop 7 or 10 int bits from table scaling of 1/M
     * max per-sample gain (ignoring implicit scaling) = MAX(sin(angle)+cos(angle)) = 1.414
     * i.e. gain 1 GB since worst case is sin(angle) = cos(angle) = 0.707 (Q30), gain 2 from
     *   extra sign bits, and eat one in adding
     */
    t  = MULSHIFT32(sin2a, ar1 + ai1);
    z2 = MULSHIFT32(cps2a, ai1) - t;
    cms2 = cps2a - 2*sin2a;
    z1 = MULSHIFT32(cms2, ar1) + t;
    *zbuf1++ = z1;  /* cos*ar1 + sin*ai1 */
    *zbuf1++ = z2;  /* cos*ai1 - sin*ar1 */

    t  = MULSHIFT32(sin2b, ar2 + ai2);
    z2 = MULSHIFT32(cps2b, ai2) - t;
    cms2 = cps2b - 2*sin2b;
    z1 = MULSHIFT32(cms2, ar2) + t;
    *zbuf2-- = z2;  /* cos*ai2 - sin*ar2 */
    *zbuf2-- = z1;  /* cos*ar2 + sin*ai2 */
  }
}
//}}}
//{{{
void PostMultiply (int tabidx, int* fft1)
{
  int i, nmdct, ar1, ai1, ar2, ai2, skipFactor;
  int t, cms2, cps2, sin2;
  int *fft2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  csptr = cos1sin1tab;
  skipFactor = postSkip[tabidx];
  fft2 = fft1 + nmdct - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin)
   */
  cps2 = *csptr++;
  sin2 = *csptr;
  csptr += skipFactor;
  cms2 = cps2 - 2*sin2;

  for (i = nmdct >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ar2 = *(fft2 - 1);
    ai2 = *(fft2 + 0);

    /* gain 2 ints bit from MULSHIFT32 by Q30
     * max per-sample gain = MAX(sin(angle)+cos(angle)) = 1.414
     * i.e. gain 1 GB since worst case is sin(angle) = cos(angle) = 0.707 (Q30), gain 2 from
     *   extra sign bits, and eat one in adding
     */
    t = MULSHIFT32(sin2, ar1 + ai1);
    *fft2-- = t - MULSHIFT32(cps2, ai1);  /* sin*ar1 - cos*ai1 */
    *fft1++ = t + MULSHIFT32(cms2, ar1);  /* cos*ar1 + sin*ai1 */
    cps2 = *csptr++;
    sin2 = *csptr;
    csptr += skipFactor;

    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    *fft2-- = t - MULSHIFT32(cps2, ai2);  /* sin*ar1 - cos*ai1 */
    cms2 = cps2 - 2*sin2;
    *fft1++ = t + MULSHIFT32(cms2, ar2);  /* cos*ar1 + sin*ai1 */
  }
}
//}}}
//{{{
void PreMultiplyRescale (int tabidx, int* zbuf1, int es)
{
  int i, nmdct, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  zbuf2 = zbuf1 + nmdct - 1;
  csptr = cos4sin4tab + cos4sin4tabOffset[tabidx];

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = nmdct >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0) >> es;
    ai1 = *(zbuf2 + 0) >> es;
    ai2 = *(zbuf1 + 1) >> es;

    t  = MULSHIFT32(sin2a, ar1 + ai1);
    z2 = MULSHIFT32(cps2a, ai1) - t;
    cms2 = cps2a - 2*sin2a;
    z1 = MULSHIFT32(cms2, ar1) + t;
    *zbuf1++ = z1;
    *zbuf1++ = z2;

    ar2 = *(zbuf2 - 1) >> es; /* do here to free up register used for es */

    t  = MULSHIFT32(sin2b, ar2 + ai2);
    z2 = MULSHIFT32(cps2b, ai2) - t;
    cms2 = cps2b - 2*sin2b;
    z1 = MULSHIFT32(cms2, ar2) + t;
    *zbuf2-- = z2;
    *zbuf2-- = z1;

  }
}
//}}}
//{{{
 void PostMultiplyRescale (int tabidx, int* fft1, int es)
{
  int i, nmdct, ar1, ai1, ar2, ai2, skipFactor, z;
  int t, cs2, sin2;
  int *fft2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  csptr = cos1sin1tab;
  skipFactor = postSkip[tabidx];
  fft2 = fft1 + nmdct - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin)
   */
  cs2 = *csptr++;
  sin2 = *csptr;
  csptr += skipFactor;

  for (i = nmdct >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ai2 = *(fft2 + 0);

    t = MULSHIFT32(sin2, ar1 + ai1);
    z = t - MULSHIFT32(cs2, ai1);
    CLIP_2N_SHIFT(z, es);
    *fft2-- = z;
    cs2 -= 2*sin2;
    z = t + MULSHIFT32(cs2, ar1);
    CLIP_2N_SHIFT(z, es);
    *fft1++ = z;

    cs2 = *csptr++;
    sin2 = *csptr;
    csptr += skipFactor;

    ar2 = *fft2;
    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    z = t - MULSHIFT32(cs2, ai2);
    CLIP_2N_SHIFT(z, es);
    *fft2-- = z;
    cs2 -= 2*sin2;
    z = t + MULSHIFT32(cs2, ar2);
    CLIP_2N_SHIFT(z, es);
    *fft1++ = z;
    cs2 += 2*sin2;
  }
}
//}}}
//{{{
void DCT4 (int tabidx, int* coef, int gb)
{
  int es;

  /* fast in-place DCT-IV - adds guard bits if necessary */
  if (gb < GBITS_IN_DCT4) {
    es = GBITS_IN_DCT4 - gb;
    PreMultiplyRescale (tabidx, coef, es);
    R4FFT (tabidx, coef);
    PostMultiplyRescale (tabidx, coef, es);
    }
  else {
    PreMultiply (tabidx, coef);
    R4FFT (tabidx, coef);
    PostMultiply (tabidx, coef);
    }
  }
//}}}

#ifdef AAC_ENABLE_SBR
  //{{{
  void InitSBRState (PSInfoSBR* psInfoSBR) {

    int i, ch;
    unsigned char *c;

    /* clear SBR state structure */
    c = (unsigned char*)psInfoSBR;
    for (i = 0; i < (int)sizeof(PSInfoSBR); i++)
      *c++ = 0;

    /* initialize non-zero state variables */
    for (ch = 0; ch < AAC_MAX_NCHANS; ch++) {
      psInfoSBR->sbrChan[ch].reset = 1;
      psInfoSBR->sbrChan[ch].laPrev = -1;
      }
    }
  //}}}
  //{{{
  void DecWindowOverlapNoClip (int *buf0, int *over0, int *out0, int winTypeCurr, int winTypePrev)
  {
    int in, w0, w1, f0, f1;
    int *buf1, *over1, *out1;
    const int *wndPrev, *wndCurr;

    buf0 += (1024 >> 1);
    buf1  = buf0  - 1;
    out1  = out0 + 1024 - 1;
    over1 = over0 + 1024 - 1;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);
    if (winTypeCurr == winTypePrev) {
      /* cut window loads in half since current and overlap sections use same symmetric window */
      do {
        w0 = *wndPrev++;
        w1 = *wndPrev++;
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        in = *over0;
        *out0++ = in - f0;

        in = *over1;
        *out1-- = in + f1;

        in = *buf1--;
        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    } else {
      /* different windows for current and overlap parts - should still fit in registers on ARM w/o stack spill */
      wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);
      do {
        w0 = *wndPrev++;
        w1 = *wndPrev++;
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        in = *over0;
        *out0++ = in - f0;

        in = *over1;
        *out1-- = in + f1;

        w0 = *wndCurr++;
        w1 = *wndCurr++;
        in = *buf1--;

        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    }
  }
  //}}}
  //{{{
  void DecWindowOverlapShortNoClip (int *buf0, int *over0, int *out0, int winTypeCurr, int winTypePrev)
  {
    int i, in, w0, w1, f0, f1;
    int *buf1, *over1, *out1;
    const int *wndPrev, *wndCurr;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);
    wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);

    /* pcm[0-447] = 0 + overlap[0-447] */
    i = 448;
    do {
      f0 = *over0++;
      f1 = *over0++;
      *out0++ = f0;
      *out0++ = f1;
      i -= 2;
    } while (i);

    /* pcm[448-575] = Wp[0-127] * block0[0-127] + overlap[448-575] */
    out1  = out0 + (128 - 1);
    over1 = over0 + 128 - 1;
    buf0 += 64;
    buf1  = buf0  - 1;
    do {
      w0 = *wndPrev++;  /* W[0], W[1], ...W[63] */
      w1 = *wndPrev++;  /* W[127], W[126], ... W[64] */
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *out0++ = in - f0;

      in = *over1;
      *out1-- = in + f1;

      w0 = *wndCurr++;
      w1 = *wndCurr++;
      in = *buf1--;

      /* save over0/over1 for next short block, in the slots just vacated */
      *over1-- = MULSHIFT32(w0, in);
      *over0++ = MULSHIFT32(w1, in);
    } while (over0 < over1);

    /* pcm[576-703] = Wc[128-255] * block0[128-255] + Wc[0-127] * block1[0-127] + overlap[576-703]
     * pcm[704-831] = Wc[128-255] * block1[128-255] + Wc[0-127] * block2[0-127] + overlap[704-831]
     * pcm[832-959] = Wc[128-255] * block2[128-255] + Wc[0-127] * block3[0-127] + overlap[832-959]
     */
    for (i = 0; i < 3; i++) {
      out0 += 64;
      out1 = out0 + 128 - 1;
      over0 += 64;
      over1 = over0 + 128 - 1;
      buf0 += 64;
      buf1 = buf0 - 1;
      wndCurr -= 128;

      do {
        w0 = *wndCurr++;  /* W[0], W[1], ...W[63] */
        w1 = *wndCurr++;  /* W[127], W[126], ... W[64] */
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        in  = *(over0 - 128); /* from last short block */
        in += *(over0 + 0);   /* from last full frame */
        *out0++ = in - f0;

        in  = *(over1 - 128); /* from last short block */
        in += *(over1 + 0);   /* from last full frame */
        *out1-- = in + f1;

        /* save over0/over1 for next short block, in the slots just vacated */
        in = *buf1--;
        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    }

    /* pcm[960-1023] = Wc[128-191] * block3[128-191] + Wc[0-63]   * block4[0-63] + overlap[960-1023]
     * over[0-63]    = Wc[192-255] * block3[192-255] + Wc[64-127] * block4[64-127]
     */
    out0 += 64;
    over0 -= 832;       /* points at overlap[64] */
    over1 = over0 + 128 - 1;  /* points at overlap[191] */
    buf0 += 64;
    buf1 = buf0 - 1;
    wndCurr -= 128;
    do {
      w0 = *wndCurr++;  /* W[0], W[1], ...W[63] */
      w1 = *wndCurr++;  /* W[127], W[126], ... W[64] */
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in  = *(over0 + 768); /* from last short block */
      in += *(over0 + 896); /* from last full frame */
      *out0++ = in - f0;

      in  = *(over1 + 768); /* from last short block */
      *(over1 - 128) = in + f1;

      in = *buf1--;
      *over1-- = MULSHIFT32(w0, in);  /* save in overlap[128-191] */
      *over0++ = MULSHIFT32(w1, in);  /* save in overlap[64-127] */
    } while (over0 < over1);

    /* over0 now points at overlap[128] */

    /* over[64-191]   = Wc[128-255] * block4[128-255] + Wc[0-127] * block5[0-127]
     * over[192-319]  = Wc[128-255] * block5[128-255] + Wc[0-127] * block6[0-127]
     * over[320-447]  = Wc[128-255] * block6[128-255] + Wc[0-127] * block7[0-127]
     * over[448-576]  = Wc[128-255] * block7[128-255]
     */
    for (i = 0; i < 3; i++) {
      over0 += 64;
      over1 = over0 + 128 - 1;
      buf0 += 64;
      buf1 = buf0 - 1;
      wndCurr -= 128;
      do {
        w0 = *wndCurr++;  /* W[0], W[1], ...W[63] */
        w1 = *wndCurr++;  /* W[127], W[126], ... W[64] */
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        /* from last short block */
        *(over0 - 128) -= f0;
        *(over1 - 128)+= f1;

        in = *buf1--;
        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    }

    /* over[576-1024] = 0 */
    i = 448;
    over0 += 64;
    do {
      *over0++ = 0;
      *over0++ = 0;
      *over0++ = 0;
      *over0++ = 0;
      i -= 4;
    } while (i);
  }
  //}}}
  //{{{
  void DecWindowOverlapLongStopNoClip (int *buf0, int *over0, int *out0, int winTypeCurr, int winTypePrev)
  {
    int i, in, w0, w1, f0, f1;
    int *buf1, *over1, *out1;
    const int *wndPrev, *wndCurr;

    buf0 += (1024 >> 1);
    buf1  = buf0  - 1;
    out1  = out0 + 1024 - 1;
    over1 = over0 + 1024 - 1;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);
    wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);

    i = 448;  /* 2 outputs, 2 overlaps per loop */
    do {
      /* Wn = 0 for n = (0, 1, ... 447) */
      /* Wn = 1 for n = (576, 577, ... 1023) */
      in = *buf0++;
      f1 = in >> 1; /* scale since skipping multiply by Q31 */

      in = *over0;
      *out0++ = in;

      in = *over1;
      *out1-- = in + f1;

      w0 = *wndCurr++;
      w1 = *wndCurr++;
      in = *buf1--;

      *over1-- = MULSHIFT32(w0, in);
      *over0++ = MULSHIFT32(w1, in);
    } while (--i);

    /* do 64 more loops - 2 outputs, 2 overlaps per loop */
    do {
      w0 = *wndPrev++;  /* W[0], W[1], ...W[63] */
      w1 = *wndPrev++;  /* W[127], W[126], ... W[64] */
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *out0++ = in - f0;

      in = *over1;
      *out1-- = in + f1;

      w0 = *wndCurr++;
      w1 = *wndCurr++;
      in = *buf1--;

      *over1-- = MULSHIFT32(w0, in);
      *over0++ = MULSHIFT32(w1, in);
    } while (over0 < over1);
  }
  //}}}
  //{{{
  void DecWindowOverlapLongStartNoClip (int *buf0, int *over0, int *out0, int winTypeCurr, int winTypePrev)
  {
    int i,  in, w0, w1, f0, f1;
    int *buf1, *over1, *out1;
    const int *wndPrev, *wndCurr;

    buf0 += (1024 >> 1);
    buf1  = buf0  - 1;
    out1  = out0 + 1024 - 1;
    over1 = over0 + 1024 - 1;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);
    i = 448;  /* 2 outputs, 2 overlaps per loop */
    do {
      w0 = *wndPrev++;
      w1 = *wndPrev++;
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *out0++ = in - f0;

      in = *over1;
      *out1-- = in + f1;

      in = *buf1--;

      *over1-- = 0;   /* Wn = 0 for n = (2047, 2046, ... 1600) */
      *over0++ = in >> 1; /* Wn = 1 for n = (1024, 1025, ... 1471) */
    } while (--i);

    wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);

    /* do 64 more loops - 2 outputs, 2 overlaps per loop */
    do {
      w0 = *wndPrev++;
      w1 = *wndPrev++;
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *out0++ = in - f0;

      in = *over1;
      *out1-- = in + f1;

      w0 = *wndCurr++;  /* W[0], W[1], ... --> W[255], W[254], ... */
      w1 = *wndCurr++;  /* W[127], W[126], ... --> W[128], W[129], ... */
      in = *buf1--;

      *over1-- = MULSHIFT32(w0, in);  /* Wn = short window for n = (1599, 1598, ... , 1536) */
      *over0++ = MULSHIFT32(w1, in);  /* Wn = short window for n = (1472, 1473, ... , 1535) */
    } while (over0 < over1);
  }
  //}}}
#else
  //{{{
  /**************************************************************************************
   * Function:    DecWindowOverlap
   *
   * Description: apply synthesis window, do overlap-add, clip to 16-bit PCM,
   *                for winSequence LONG-LONG
   *
   * Inputs:      input buffer (output of type-IV DCT)
   *              overlap buffer (saved from last time)
   *              number of channels
   *              window type (sin or KBD) for input buffer
   *              window type (sin or KBD) for overlap buffer
   *
   * Outputs:     one channel, one frame of 16-bit PCM, interleaved by nChans
   *
   * Return:      none
   *
   * Notes:       this processes one channel at a time, but skips every other sample in
   *                the output buffer (pcm) for stereo interleaving
   *              this should fit in registers on ARM
   *
   * TODO:        ARM5E version with saturating overlap/add (QADD)
   *              asm code with free pointer updates, better load scheduling
   **************************************************************************************/
  /*__attribute__ ((section (".data")))*/
  void DecWindowOverlap (int *buf0, int *over0, short *pcm0, int nChans, int winTypeCurr, int winTypePrev)
  {
    int in, w0, w1, f0, f1;
    int *buf1, *over1;
    short *pcm1;
    const int *wndPrev, *wndCurr;

    buf0 += (1024 >> 1);
    buf1  = buf0  - 1;
    pcm1  = pcm0 + (1024 - 1) * nChans;
    over1 = over0 + 1024 - 1;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);
    if (winTypeCurr == winTypePrev) {
      /* cut window loads in half since current and overlap sections use same symmetric window */
      do {
        w0 = *wndPrev++;
        w1 = *wndPrev++;
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        in = *over0;
        *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL1) >> FBITS_OUT_IMDCT );
        pcm0 += nChans;

        in = *over1;
        *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL1) >> FBITS_OUT_IMDCT );
        pcm1 -= nChans;

        in = *buf1--;
        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    } else {
      /* different windows for current and overlap parts - should still fit in registers on ARM w/o stack spill */
      wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);
      do {
        w0 = *wndPrev++;
        w1 = *wndPrev++;
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        in = *over0;
        *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL1) >> FBITS_OUT_IMDCT );
        pcm0 += nChans;

        in = *over1;
        *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL1) >> FBITS_OUT_IMDCT );
        pcm1 -= nChans;

        w0 = *wndCurr++;
        w1 = *wndCurr++;
        in = *buf1--;

        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    }
  }
  //}}}
  //{{{
  /**************************************************************************************
   * Function:    DecWindowOverlapShort
   *
   * Description: apply synthesis window, do overlap-add, clip to 16-bit PCM,
   *                for winSequence EIGHT-SHORT (does all 8 short blocks)
   *
   * Inputs:      input buffer (output of type-IV DCT)
   *              overlap buffer (saved from last time)
   *              number of channels
   *              window type (sin or KBD) for input buffer
   *              window type (sin or KBD) for overlap buffer
   *
   * Outputs:     one channel, one frame of 16-bit PCM, interleaved by nChans
   *
   * Return:      none
   *
   * Notes:       this processes one channel at a time, but skips every other sample in
   *                the output buffer (pcm) for stereo interleaving
   *              this should fit in registers on ARM
   *
   * TODO:        ARM5E version with saturating overlap/add (QADD)
   *              asm code with free pointer updates, better load scheduling
   **************************************************************************************/
   /*__attribute__ ((section (".data"))) */
   void DecWindowOverlapShort (int *buf0, int *over0, short *pcm0, int nChans, int winTypeCurr, int winTypePrev)
  {
    int i, in, w0, w1, f0, f1;
    int *buf1, *over1;
    short *pcm1;
    const int *wndPrev, *wndCurr;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);
    wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);

    /* pcm[0-447] = 0 + overlap[0-447] */
    i = 448;
    do {
      f0 = *over0++;
      f1 = *over0++;
      *pcm0 = CLIPTOSHORT( (f0 + RND_VAL) >> FBITS_OUT_IMDCT ); pcm0 += nChans;
      *pcm0 = CLIPTOSHORT( (f1 + RND_VAL) >> FBITS_OUT_IMDCT ); pcm0 += nChans;
      i -= 2;
    } while (i);

    /* pcm[448-575] = Wp[0-127] * block0[0-127] + overlap[448-575] */
    pcm1  = pcm0 + (128 - 1) * nChans;
    over1 = over0 + 128 - 1;
    buf0 += 64;
    buf1  = buf0  - 1;
    do {
      w0 = *wndPrev++;  /* W[0], W[1], ...W[63] */
      w1 = *wndPrev++;  /* W[127], W[126], ... W[64] */
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL) >> FBITS_OUT_IMDCT );
      pcm0 += nChans;

      in = *over1;
      *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL) >> FBITS_OUT_IMDCT );
      pcm1 -= nChans;

      w0 = *wndCurr++;
      w1 = *wndCurr++;
      in = *buf1--;

      /* save over0/over1 for next short block, in the slots just vacated */
      *over1-- = MULSHIFT32(w0, in);
      *over0++ = MULSHIFT32(w1, in);
    } while (over0 < over1);

    /* pcm[576-703] = Wc[128-255] * block0[128-255] + Wc[0-127] * block1[0-127] + overlap[576-703]
     * pcm[704-831] = Wc[128-255] * block1[128-255] + Wc[0-127] * block2[0-127] + overlap[704-831]
     * pcm[832-959] = Wc[128-255] * block2[128-255] + Wc[0-127] * block3[0-127] + overlap[832-959]
     */
    for (i = 0; i < 3; i++) {
      pcm0 += 64 * nChans;
      pcm1 = pcm0 + (128 - 1) * nChans;
      over0 += 64;
      over1 = over0 + 128 - 1;
      buf0 += 64;
      buf1 = buf0 - 1;
      wndCurr -= 128;

      do {
        w0 = *wndCurr++;  /* W[0], W[1], ...W[63] */
        w1 = *wndCurr++;  /* W[127], W[126], ... W[64] */
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        in  = *(over0 - 128); /* from last short block */
        in += *(over0 + 0);   /* from last full frame */
        *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL) >> FBITS_OUT_IMDCT );
        pcm0 += nChans;

        in  = *(over1 - 128); /* from last short block */
        in += *(over1 + 0);   /* from last full frame */
        *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL) >> FBITS_OUT_IMDCT );
        pcm1 -= nChans;

        /* save over0/over1 for next short block, in the slots just vacated */
        in = *buf1--;
        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    }

    /* pcm[960-1023] = Wc[128-191] * block3[128-191] + Wc[0-63]   * block4[0-63] + overlap[960-1023]
     * over[0-63]    = Wc[192-255] * block3[192-255] + Wc[64-127] * block4[64-127]
     */
    pcm0 += 64 * nChans;
    over0 -= 832;       /* points at overlap[64] */
    over1 = over0 + 128 - 1;  /* points at overlap[191] */
    buf0 += 64;
    buf1 = buf0 - 1;
    wndCurr -= 128;
    do {
      w0 = *wndCurr++;  /* W[0], W[1], ...W[63] */
      w1 = *wndCurr++;  /* W[127], W[126], ... W[64] */
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in  = *(over0 + 768); /* from last short block */
      in += *(over0 + 896); /* from last full frame */
      *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL) >> FBITS_OUT_IMDCT );
      pcm0 += nChans;

      in  = *(over1 + 768); /* from last short block */
      *(over1 - 128) = in + f1;

      in = *buf1--;
      *over1-- = MULSHIFT32(w0, in);  /* save in overlap[128-191] */
      *over0++ = MULSHIFT32(w1, in);  /* save in overlap[64-127] */
    } while (over0 < over1);

    /* over0 now points at overlap[128] */

    /* over[64-191]   = Wc[128-255] * block4[128-255] + Wc[0-127] * block5[0-127]
     * over[192-319]  = Wc[128-255] * block5[128-255] + Wc[0-127] * block6[0-127]
     * over[320-447]  = Wc[128-255] * block6[128-255] + Wc[0-127] * block7[0-127]
     * over[448-576]  = Wc[128-255] * block7[128-255]
     */
    for (i = 0; i < 3; i++) {
      over0 += 64;
      over1 = over0 + 128 - 1;
      buf0 += 64;
      buf1 = buf0 - 1;
      wndCurr -= 128;
      do {
        w0 = *wndCurr++;  /* W[0], W[1], ...W[63] */
        w1 = *wndCurr++;  /* W[127], W[126], ... W[64] */
        in = *buf0++;

        f0 = MULSHIFT32(w0, in);
        f1 = MULSHIFT32(w1, in);

        /* from last short block */
        *(over0 - 128) -= f0;
        *(over1 - 128)+= f1;

        in = *buf1--;
        *over1-- = MULSHIFT32(w0, in);
        *over0++ = MULSHIFT32(w1, in);
      } while (over0 < over1);
    }

    /* over[576-1024] = 0 */
    i = 448;
    over0 += 64;
    do {
      *over0++ = 0;
      *over0++ = 0;
      *over0++ = 0;
      *over0++ = 0;
      i -= 4;
    } while (i);
  }
  //}}}
  //{{{
  /**************************************************************************************
   * Function:    DecWindowOverlapLongStop
   *
   * Description: apply synthesis window, do overlap-add, clip to 16-bit PCM,
   *                for winSequence LONG-STOP
   *
   * Inputs:      input buffer (output of type-IV DCT)
   *              overlap buffer (saved from last time)
   *              number of channels
   *              window type (sin or KBD) for input buffer
   *              window type (sin or KBD) for overlap buffer
   *
   * Outputs:     one channel, one frame of 16-bit PCM, interleaved by nChans
   *
   * Return:      none
   *
   * Notes:       this processes one channel at a time, but skips every other sample in
   *                the output buffer (pcm) for stereo interleaving
   *              this should fit in registers on ARM
   *
   * TODO:        ARM5E version with saturating overlap/add (QADD)
   *              asm code with free pointer updates, better load scheduling
   **************************************************************************************/
   /*__attribute__ ((section (".data")))*/
   void DecWindowOverlapLongStop (int *buf0, int *over0, short *pcm0, int nChans, int winTypeCurr, int winTypePrev)
  {
    int i, in, w0, w1, f0, f1;
    int *buf1, *over1;
    short *pcm1;
    const int *wndPrev, *wndCurr;

    buf0 += (1024 >> 1);
    buf1  = buf0  - 1;
    pcm1  = pcm0 + (1024 - 1) * nChans;
    over1 = over0 + 1024 - 1;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);
    wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);

    i = 448;  /* 2 outputs, 2 overlaps per loop */
    do {
      /* Wn = 0 for n = (0, 1, ... 447) */
      /* Wn = 1 for n = (576, 577, ... 1023) */
      in = *buf0++;
      f1 = in >> 1; /* scale since skipping multiply by Q31 */

      in = *over0;
      *pcm0 = CLIPTOSHORT( (in + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm0 += nChans;

      in = *over1;
      *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm1 -= nChans;

      w0 = *wndCurr++;
      w1 = *wndCurr++;
      in = *buf1--;

      *over1-- = MULSHIFT32(w0, in);
      *over0++ = MULSHIFT32(w1, in);
    } while (--i);

    /* do 64 more loops - 2 outputs, 2 overlaps per loop */
    do {
      w0 = *wndPrev++;  /* W[0], W[1], ...W[63] */
      w1 = *wndPrev++;  /* W[127], W[126], ... W[64] */
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm0 += nChans;

      in = *over1;
      *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm1 -= nChans;

      w0 = *wndCurr++;
      w1 = *wndCurr++;
      in = *buf1--;

      *over1-- = MULSHIFT32(w0, in);
      *over0++ = MULSHIFT32(w1, in);
    } while (over0 < over1);
  }
  //}}}
  //{{{
  /**************************************************************************************
   * Function:    DecWindowOverlapLongStart
   *
   * Description: apply synthesis window, do overlap-add, clip to 16-bit PCM,
   *                for winSequence LONG-START
   *
   * Inputs:      input buffer (output of type-IV DCT)
   *              overlap buffer (saved from last time)
   *              number of channels
   *              window type (sin or KBD) for input buffer
   *              window type (sin or KBD) for overlap buffer
   *
   * Outputs:     one channel, one frame of 16-bit PCM, interleaved by nChans
   *
   * Return:      none
   *
   * Notes:       this processes one channel at a time, but skips every other sample in
   *                the output buffer (pcm) for stereo interleaving
   *              this should fit in registers on ARM
   *
   * TODO:        ARM5E version with saturating overlap/add (QADD)
   *              asm code with free pointer updates, better load scheduling
   **************************************************************************************/
   /*__attribute__ ((section (".data")))*/
   void DecWindowOverlapLongStart (int *buf0, int *over0, short *pcm0, int nChans, int winTypeCurr, int winTypePrev)
  {
    int i,  in, w0, w1, f0, f1;
    int *buf1, *over1;
    short *pcm1;
    const int *wndPrev, *wndCurr;

    buf0 += (1024 >> 1);
    buf1  = buf0  - 1;
    pcm1  = pcm0 + (1024 - 1) * nChans;
    over1 = over0 + 1024 - 1;

    wndPrev = (winTypePrev == 1 ? kbdWindow + kbdWindowOffset[1] : sinWindow + sinWindowOffset[1]);
    i = 448;  /* 2 outputs, 2 overlaps per loop */
    do {
      w0 = *wndPrev++;
      w1 = *wndPrev++;
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm0 += nChans;

      in = *over1;
      *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm1 -= nChans;

      in = *buf1--;

      *over1-- = 0;   /* Wn = 0 for n = (2047, 2046, ... 1600) */
      *over0++ = in >> 1; /* Wn = 1 for n = (1024, 1025, ... 1471) */
    } while (--i);

    wndCurr = (winTypeCurr == 1 ? kbdWindow + kbdWindowOffset[0] : sinWindow + sinWindowOffset[0]);

    /* do 64 more loops - 2 outputs, 2 overlaps per loop */
    do {
      w0 = *wndPrev++;
      w1 = *wndPrev++;
      in = *buf0++;

      f0 = MULSHIFT32(w0, in);
      f1 = MULSHIFT32(w1, in);

      in = *over0;
      *pcm0 = CLIPTOSHORT( (in - f0 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm0 += nChans;

      in = *over1;
      *pcm1 = CLIPTOSHORT( (in + f1 + RND_VAL1) >> FBITS_OUT_IMDCT );
      pcm1 -= nChans;

      w0 = *wndCurr++;  /* W[0], W[1], ... --> W[255], W[254], ... */
      w1 = *wndCurr++;  /* W[127], W[126], ... --> W[128], W[129], ... */
      in = *buf1--;

      *over1-- = MULSHIFT32(w0, in);  /* Wn = short window for n = (1599, 1598, ... , 1536) */
      *over0++ = MULSHIFT32(w1, in);  /* Wn = short window for n = (1472, 1473, ... , 1535) */
    } while (over0 < over1);
  }
  //}}}
#endif

//{{{
int DequantBlock (int* inbuf, int nSamps, int scale) {

  int iSamp, scalef, scalei, x, y, gbMask, shift, tab4[4];
  const int *tab16, *coef;

  if (nSamps <= 0)
    return 0;

  scale -= SF_OFFSET; /* new range = [-100, 156] */
  /* with two's complement numbers, scalei/scalef factorization works for pos and neg values of scale:
   *  [+4...+7] >> 2 = +1, [ 0...+3] >> 2 = 0, [-4...-1] >> 2 = -1, [-8...-5] >> 2 = -2 ...
   *  (-1 & 0x3) = 3, (-2 & 0x3) = 2, (-3 & 0x3) = 1, (0 & 0x3) = 0
   * Example: 2^(-5/4) = 2^(-1) * 2^(-1/4) = 2^-2 * 2^(3/4) */
  tab16 = pow43_14[scale & 0x3];
  scalef = pow14[scale & 0x3];
  scalei = (scale >> 2) + FBITS_OUT_DQ_OFF;

  /* cache first 4 values:
   * tab16[j] = Q28 for j = [0,3]
   * tab4[x] = x^(4.0/3.0) * 2^(0.25*scale), Q(FBITS_OUT_DQ_OFF)
   */
  shift = 28 - scalei;
  if (shift > 31) {
    tab4[0] = tab4[1] = tab4[2] = tab4[3] = 0;
    }
  else if (shift <= 0) {
    shift = -shift;
    if (shift > 31)
      shift = 31;
    for (x = 0; x < 4; x++) {
      y = tab16[x];
      if (y > (0x7fffffff >> shift))
        y = 0x7fffffff;   /* clip (rare) */
      else
        y <<= shift;
      tab4[x] = y;
      }
    }
  else {
    tab4[0] = 0;
    tab4[1] = tab16[1] >> shift;
    tab4[2] = tab16[2] >> shift;
    tab4[3] = tab16[3] >> shift;
    }

  gbMask = 0;
  do {
    iSamp = *inbuf;
    x = FASTABS(iSamp);

    if (x < 4) {
      y = tab4[x];
      }
    else  {

      if (x < 16) {
        /* result: y = Q25 (tab16 = Q25) */
        y = tab16[x];
        shift = 25 - scalei;
        }
      else if (x < 64) {
        /* result: y = Q21 (pow43tab[j] = Q23, scalef = Q30) */
        y = pow43[x-16];
        shift = 21 - scalei;
        y = MULSHIFT32(y, scalef);
        }
      else {
        /* normalize to [0x40000000, 0x7fffffff]
         * input x = [64, 8191] = [64, 2^13-1]
         * ranges:
         *  shift = 7:   64 -  127
         *  shift = 6:  128 -  255
         *  shift = 5:  256 -  511
         *  shift = 4:  512 - 1023
         *  shift = 3: 1024 - 2047
         *  shift = 2: 2048 - 4095
         *  shift = 1: 4096 - 8191 */
        x <<= 17;
        shift = 0;
        if (x < 0x08000000)
          x <<= 4, shift += 4;
        if (x < 0x20000000)
          x <<= 2, shift += 2;
        if (x < 0x40000000)
          x <<= 1, shift += 1;

        coef = (x < SQRTHALF) ? poly43lo : poly43hi;

        /* polynomial */
        y = coef[0];
        y = MULSHIFT32(y, x) + coef[1];
        y = MULSHIFT32(y, x) + coef[2];
        y = MULSHIFT32(y, x) + coef[3];
        y = MULSHIFT32(y, x) + coef[4];
        y = MULSHIFT32(y, pow2frac[shift]) << 3;

        /* fractional scale * result: y = Q21 (pow43tab[j] = Q23, scalef = Q30) */
        y = MULSHIFT32(y, scalef);  /* now y is Q24 */
        shift = 24 - scalei - pow2exp[shift];
        }

      /* integer scale */
      if (shift <= 0) {
        shift = -shift;
        if (shift > 31)
          shift = 31;

        if (y > (0x7fffffff >> shift))
          y = 0x7fffffff;   /* clip (rare) */
        else
          y <<= shift;
        }
      else {
        if (shift > 31)
          shift = 31;
        y >>= shift;
        }
      }

    /* sign and store (gbMask used to count GB's) */
    gbMask |= y;

    /* apply sign */
    iSamp >>= 31;
    y ^= iSamp;
    y -= iSamp;

    *inbuf++ = y;
    } while (--nSamps);

  return gbMask;
  }
//}}}
//{{{
void StereoProcessGroup (int* coefL, int* coefR, const short* sfbTab,
                                int msMaskPres, unsigned char* msMaskPtr, int msMaskOffset, int maxSFB,
                                unsigned char* cbRight, short* sfRight, int* gbCurrent)
{
  int sfb, width, cbIdx, sf, cl, cr, scalef, scalei;
  int gbMaskL, gbMaskR;
  unsigned char msMask;

  msMask = (*msMaskPtr++) >> msMaskOffset;
  gbMaskL = 0;
  gbMaskR = 0;

  for (sfb = 0; sfb < maxSFB; sfb++) {
    width = sfbTab[sfb+1] - sfbTab[sfb];  /* assume >= 0 (see sfBandTabLong/sfBandTabShort) */
    cbIdx = cbRight[sfb];

    if (cbIdx == 14 || cbIdx == 15) {
      /* intensity stereo */
      if (msMaskPres == 1 && (msMask & 0x01))
        cbIdx ^= 0x01;        /* invert_intensity(): 14 becomes 15, or 15 becomes 14 */
      sf = -sfRight[sfb];       /* negative since we use identity 0.5^(x) = 2^(-x) (see spec) */
      cbIdx &= 0x01;          /* choose - or + scale factor */
      scalef = pow14signed[cbIdx][sf & 0x03];
      scalei = (sf >> 2) + 2;     /* +2 to compensate for scalef = Q30 */

      if (scalei > 0) {
        if (scalei > 30)
          scalei = 30;
        do {
          cr = MULSHIFT32(*coefL++, scalef);
          CLIP_2N(cr, 31-scalei);
          cr <<= scalei;
          gbMaskR |= FASTABS(cr);
          *coefR++ = cr;
        } while (--width);
      } else {
        scalei = -scalei;
        if (scalei > 31)
          scalei = 31;
        do {
          cr = MULSHIFT32(*coefL++, scalef) >> scalei;
          gbMaskR |= FASTABS(cr);
          *coefR++ = cr;
        } while (--width);
      }
    } else if ( cbIdx != 13 && ((msMaskPres == 1 && (msMask & 0x01)) || msMaskPres == 2) ) {
      /* mid-side stereo (assumes no GB in inputs) */
      do {
        cl = *coefL;
        cr = *coefR;

        if ( (FASTABS(cl) | FASTABS(cr)) >> 30 ) {
          /* avoid overflow (rare) */
          cl >>= 1;
          sf = cl + (cr >> 1);  CLIP_2N(sf, 30);  sf <<= 1;
          cl = cl - (cr >> 1);  CLIP_2N(cl, 30);  cl <<= 1;
        } else {
          /* usual case */
          sf = cl + cr;
          cl -= cr;
        }

        *coefL++ = sf;
        gbMaskL |= FASTABS(sf);
        *coefR++ = cl;
        gbMaskR |= FASTABS(cl);
      } while (--width);

    } else {
      /* nothing to do */
      coefL += width;
      coefR += width;
    }

    /* get next mask bit (should be branchless on ARM) */
    msMask >>= 1;
    if (++msMaskOffset == 8) {
      msMask = *msMaskPtr++;
      msMaskOffset = 0;
    }
  }

  cl = CLZ(gbMaskL) - 1;
  if (gbCurrent[0] > cl)
    gbCurrent[0] = cl;

  cr = CLZ(gbMaskR) - 1;
  if (gbCurrent[1] > cr)
    gbCurrent[1] = cr;

  return;
}
//}}}

//{{{
unsigned int Get32BitVal (unsigned int* last) {
// Return 32-bit number, uniformly distributed between [0, 2^32)

  // use same coefs as MPEG reference code (classic LCG), unsigned multiply forces reliable wraparound behavior in C
  unsigned int r = *last;
  r = (1664525U * r) + 1013904223U;
  *last = r;
  return r;
  }
//}}}
//{{{
int InvRootR (int r) {

  /* use linear equation for initial guess
   * x0 = -2*r + 3 (so x0 always >= correct answer in range [0.25, 1))
   * xn = Q29 (at every step) */
  int xn = (MULSHIFT32(r, X0_COEF_2) << 2) + X0_OFF_2;
  for (int i = 0; i < NUM_ITER_INVSQRT; i++) {
    int t = MULSHIFT32(xn, xn);          /* Q26 = Q29*Q29 */
    t = Q26_3 - (MULSHIFT32(r, t) << 2); /* Q26 = Q26 - (Q31*Q26 << 1) */
    xn = MULSHIFT32(xn, t) << (6 - 1);   /* Q29 = (Q29*Q26 << 6), and -1 for division by 2 */
    }

  /* clip to range (1.0, 2.0) * (because of rounding, this can converge to xn slightly > 2.0 when r is near 0.25) */
  if (xn >> 30)
    xn = (1 << 30) - 1;
  return xn;
  }
//}}}

//{{{
int ScaleNoiseVector (int* coef, int nVals, int sf)
{
  int i, c, spec, energy, sq, scalef, scalei, invSqrtEnergy, z, gbMask;

  energy = 0;
  for (i = 0; i < nVals; i++) {
    spec = coef[i];

    /* max nVals = max SFB width = 96, so energy can gain < 2^7 bits in accumulation */
    sq = (spec * spec) >> 8;    /* spec*spec range = (-2^30, 2^30) */
    energy += sq;
  }

  /* unless nVals == 1 (or the number generator is broken...), this should not happen */
  if (energy == 0)
    return 0; /* coef[i] must = 0 for i = [0, nVals-1], so gbMask = 0 */

  /* pow(2, sf/4) * pow(2, FBITS_OUT_DQ_OFF) */
  scalef = pow14[sf & 0x3];
  scalei = (sf >> 2) + FBITS_OUT_DQ_OFF;

  /* energy has implied factor of 2^-8 since we shifted the accumulator
   * normalize energy to range [0.25, 1.0), calculate 1/sqrt(1), and denormalize
   *   i.e. divide input by 2^(30-z) and convert to Q30
   *        output of 1/sqrt(i) now has extra factor of 2^((30-z)/2)
   *        for energy > 0, z is an even number between 0 and 28
   * final scaling of invSqrtEnergy:
   *  2^(15 - z/2) to compensate for implicit 2^(30-z) factor in input
   *  +4 to compensate for implicit 2^-8 factor in input
   */
  z = CLZ(energy) - 2;          /* energy has at least 2 leading zeros (see acc loop) */
  z &= 0xfffffffe;            /* force even */
  invSqrtEnergy = InvRootR(energy << z);  /* energy << z must be in range [0x10000000, 0x40000000] */
  scalei -= (15 - z/2 + 4);       /* nInt = 1/sqrt(energy) in Q29 */

  /* normalize for final scaling */
  z = CLZ(invSqrtEnergy) - 1;
  invSqrtEnergy <<= z;
  scalei -= (z - 3 - 2);  /* -2 for scalef, z-3 for invSqrtEnergy */
  scalef = MULSHIFT32(scalef, invSqrtEnergy); /* scalef (input) = Q30, invSqrtEnergy = Q29 * 2^z */
  gbMask = 0;

  if (scalei < 0) {
    scalei = -scalei;
    if (scalei > 31)
      scalei = 31;
    for (i = 0; i < nVals; i++) {
      c = MULSHIFT32(coef[i], scalef) >> scalei;
      gbMask |= FASTABS(c);
      coef[i] = c;
    }
  } else {
    /* for scalei <= 16, no clipping possible (coef[i] is < 2^15 before scaling)
     * for scalei > 16, just saturate exponent (rare)
     *   scalef is close to full-scale (since we normalized invSqrtEnergy)
     * remember, we are just producing noise here
     */
    if (scalei > 16)
      scalei = 16;
    for (i = 0; i < nVals; i++) {
      c = MULSHIFT32(coef[i] << scalei, scalef);
      coef[i] = c;
      gbMask |= FASTABS(c);
    }
  }

  return gbMask;
}
//}}}
//{{{
void GenerateNoiseVector (int* coef, int* last, int nVals) {
  for (int i = 0; i < nVals; i++)
    coef[i] = ((signed int)Get32BitVal((unsigned int *)last)) >> 16;
  }
//}}}
//{{{
void CopyNoiseVector (int* coefL, int* coefR, int nVals) {
  for (int i = 0; i < nVals; i++)
    coefR[i] = coefL[i];
  }
//}}}

//{{{
void DecodeLPCCoefs (int order, int res, signed char* filtCoef, int* a, int* b) {

  const int *invQuantTab;
  if (res == 3)
    invQuantTab = invQuant3;
  else if (res == 4)
    invQuantTab = invQuant4;
  else
    return;

  for (int m = 0; m < order; m++) {
    int t = invQuantTab[filtCoef[m] & 0x0f];  /* t = Q31 */
    for (int i = 0; i < m; i++)
      b[i] = a[i] - (MULSHIFT32(t, a[m-i-1]) << 1);
    for (int i = 0; i < m; i++)
      a[i] = b[i];
    a[m] = t >> (31 - FBITS_LPC_COEFS);
    }
  }
//}}}
//{{{
int FilterRegion (int size, int dir, int order, int* audioCoef, int* a, int* hist) {

  /* init history to 0 every time */
  for (int i = 0; i < order; i++)
    hist[i] = 0;

  U64 sum64;
  sum64.w64 = 0;     /* avoid warning */

  int gbMask = 0;
  int inc = (dir ? -1 : 1);

  do {
    /* sum64 = a0*y[n] = 1.0*y[n] */
    int y = *audioCoef;
    sum64.r.hi32 = y >> (32 - FBITS_LPC_COEFS);
    sum64.r.lo32 = y << FBITS_LPC_COEFS;

    /* sum64 += (a1*y[n-1] + a2*y[n-2] + ... + a[order-1]*y[n-(order-1)]) */
    for (int j = order - 1; j > 0; j--) {
      sum64.w64 = MADD64(sum64.w64, hist[j], a[j]);
      hist[j] = hist[j-1];
      }
    sum64.w64 = MADD64(sum64.w64, hist[0], a[0]);
    y = (sum64.r.hi32 << (32 - FBITS_LPC_COEFS)) | (sum64.r.lo32 >> FBITS_LPC_COEFS);

    /* clip output (rare) */
    int hi32 = sum64.r.hi32;
    if ((hi32 >> 31) != (hi32 >> (FBITS_LPC_COEFS-1)))
      y = (hi32 >> 31) ^ 0x7fffffff;

    hist[0] = y;
    *audioCoef = y;
    audioCoef += inc;
    gbMask |= FASTABS(y);
    } while (--size);

  return gbMask;
  }
//}}}

//{{{
int GetNumChannelsADIF (ProgConfigElement* fhPCE, int nPCE) {

  if (nPCE < 1 || nPCE > MAX_NUM_PCE_ADIF)
    return -1;

  int nChans = 0;
  for (int i = 0; i < nPCE; i++) {
    /* for now: only support LC, no channel coupling */
    if (fhPCE[i].profile != AAC_PROFILE_LC || fhPCE[i].numCCE > 0)
      return -1;

    /* add up number of channels in all channel elements (assume all single-channel) */
    nChans += fhPCE[i].numFCE;
    nChans += fhPCE[i].numSCE;
    nChans += fhPCE[i].numBCE;
    nChans += fhPCE[i].numLCE;

    /* add one more for every element which is a channel pair */
    for (int j = 0; j < fhPCE[i].numFCE; j++)
      if (CHAN_ELEM_IS_CPE(fhPCE[i].fce[j]))
        nChans++;
    for (int j = 0; j < fhPCE[i].numSCE; j++)
      if (CHAN_ELEM_IS_CPE(fhPCE[i].sce[j]))
        nChans++;
    for (int j = 0; j < fhPCE[i].numBCE; j++)
      if (CHAN_ELEM_IS_CPE(fhPCE[i].bce[j]))
        nChans++;
    }

  return nChans;
  }
//}}}
//{{{
int GetSampleRateIdxADIF (ProgConfigElement* fhPCE, int nPCE) {

  if (nPCE < 1 || nPCE > MAX_NUM_PCE_ADIF)
    return -1;

  /* make sure all PCE's have the same sample rate */
  int idx = fhPCE[0].sampRateIdx;
  for (int i = 1; i < nPCE; i++) {
    if (fhPCE[i].sampRateIdx != idx)
      return -1;
    }

  return idx;
  }
//}}}
//{{{
int DecodeProgramConfigElement (cBitStream* bitStream, ProgConfigElement* pce) {

  pce->elemInstTag =   bitStream->GetBits(4);
  pce->profile =       bitStream->GetBits(2);
  pce->sampRateIdx =   bitStream->GetBits(4);
  pce->numFCE =        bitStream->GetBits(4);
  pce->numSCE =        bitStream->GetBits(4);
  pce->numBCE =        bitStream->GetBits(4);
  pce->numLCE =        bitStream->GetBits(2);
  pce->numADE =        bitStream->GetBits(3);
  pce->numCCE =        bitStream->GetBits(4);

  pce->monoMixdown = bitStream->GetBits(1) << 4;  /* present flag */
  if (pce->monoMixdown)
    pce->monoMixdown |= bitStream->GetBits(4);  /* element number */

  pce->stereoMixdown = bitStream->GetBits(1) << 4;  /* present flag */
  if (pce->stereoMixdown)
    pce->stereoMixdown  |= bitStream->GetBits(4); /* element number */

  pce->matrixMixdown = bitStream->GetBits(1) << 4;  /* present flag */
  if (pce->matrixMixdown) {
    pce->matrixMixdown  |= bitStream->GetBits(2) << 1;  /* index */
    pce->matrixMixdown  |= bitStream->GetBits(1);     /* pseudo-surround enable */
    }

  for (int i = 0; i < pce->numFCE; i++) {
    pce->fce[i]  = bitStream->GetBits(1) << 4;  /* is_cpe flag */
    pce->fce[i] |= bitStream->GetBits(4);     /* tag select */
    }

  for (int i = 0; i < pce->numSCE; i++) {
    pce->sce[i]  = bitStream->GetBits(1) << 4;  /* is_cpe flag */
    pce->sce[i] |= bitStream->GetBits(4);     /* tag select */
    }

  for (int i = 0; i < pce->numBCE; i++) {
    pce->bce[i]  = bitStream->GetBits(1) << 4;  /* is_cpe flag */
    pce->bce[i] |= bitStream->GetBits(4);     /* tag select */
    }

  for (int i = 0; i < pce->numLCE; i++)
    pce->lce[i] = bitStream->GetBits(4);      /* tag select */

  for (int i = 0; i < pce->numADE; i++)
    pce->ade[i] = bitStream->GetBits(4);      /* tag select */

  for (int i = 0; i < pce->numCCE; i++) {
    pce->cce[i]  = bitStream->GetBits(1) << 4;  /* independent/dependent flag */
    pce->cce[i] |= bitStream->GetBits(4);     /* tag select */
    }


  bitStream->ByteAlignBitstream();

  /* eat comment bytes and throw away */
  int i = bitStream->GetBits(8);
  while (i--)
    bitStream->GetBits(8);

  return 0;
  }
//}}}

//{{{
void DecodeTNSInfo (cBitStream& bitStream, int winSequence, TNSInfo* ti, signed char* tnsCoef)
{
  int i, w, f, coefBits, compress;
  signed char c, s, n;

  unsigned char *filtLength, *filtOrder, *filtDir;

  filtLength = ti->length;
  filtOrder =  ti->order;
  filtDir =    ti->dir;

  if (winSequence == 2) {
    /* short blocks */
    for (w = 0; w < NWINDOWS_SHORT; w++) {
      ti->numFilt[w] = bitStream.GetBits(1);
      if (ti->numFilt[w]) {
        ti->coefRes[w] = bitStream.GetBits(1) + 3;
        *filtLength =    bitStream.GetBits(4);
        *filtOrder =     bitStream.GetBits(3);
        if (*filtOrder) {
          *filtDir++ =      bitStream.GetBits(1);
          compress =        bitStream.GetBits(1);
          coefBits = (int)ti->coefRes[w] - compress;  /* 2, 3, or 4 */
          s = sgnMask[coefBits - 2];
          n = negMask[coefBits - 2];
          for (i = 0; i < *filtOrder; i++) {
            c = bitStream.GetBits(coefBits);
            if (c & s)  c |= n;
            *tnsCoef++ = c;
          }
        }
        filtLength++;
        filtOrder++;
      }
    }
  } else {
    /* long blocks */
    ti->numFilt[0] = bitStream.GetBits(2);
    if (ti->numFilt[0])
      ti->coefRes[0] = bitStream.GetBits(1) + 3;
    for (f = 0; f < ti->numFilt[0]; f++) {
      *filtLength =      bitStream.GetBits(6);
      *filtOrder =       bitStream.GetBits(5);
      if (*filtOrder) {
        *filtDir++ =     bitStream.GetBits(1);
        compress =       bitStream.GetBits(1);
        coefBits = (int)ti->coefRes[0] - compress;  /* 2, 3, or 4 */
        s = sgnMask[coefBits - 2];
        n = negMask[coefBits - 2];
        for (i = 0; i < *filtOrder; i++) {
          c = bitStream.GetBits(coefBits);
          if (c & s)  c |= n;
          *tnsCoef++ = c;
        }
      }
      filtLength++;
      filtOrder++;
    }
  }
}
//}}}
//{{{
void DecodeICSInfo (cBitStream& bitStream, ICSInfo* icsInfo, int sampRateIdx) {

  int sfb, g, mask;

  icsInfo->icsResBit = bitStream.GetBits(1);
  icsInfo->winSequence = bitStream.GetBits(2);
  icsInfo->winShape = bitStream.GetBits(1);
  if (icsInfo->winSequence == 2) {
    /* short block */
    icsInfo->maxSFB = bitStream.GetBits(4);
    icsInfo->sfGroup = bitStream.GetBits(7);
    icsInfo->numWinGroup = 1;
    icsInfo->winGroupLen[0] = 1;
    mask = 0x40;  /* start with bit 6 */
    for (g = 0; g < 7; g++) {
      if (icsInfo->sfGroup & mask)  {
        icsInfo->winGroupLen[icsInfo->numWinGroup - 1]++;
        }
      else {
        icsInfo->numWinGroup++;
        icsInfo->winGroupLen[icsInfo->numWinGroup - 1] = 1;
        }
      mask >>= 1;
      }
    }
  else {
    /* long block */
    icsInfo->maxSFB = bitStream.GetBits(6);
    icsInfo->predictorDataPresent = bitStream.GetBits(1);
    if (icsInfo->predictorDataPresent) {
      icsInfo->predictorReset = bitStream.GetBits(1);
      if (icsInfo->predictorReset)
        icsInfo->predictorResetGroupNum = bitStream.GetBits(5);
      for (sfb = 0; sfb < MIN(icsInfo->maxSFB, predSFBMax[sampRateIdx]); sfb++)
        icsInfo->predictionUsed[sfb] = bitStream.GetBits(1);
      }
    icsInfo->numWinGroup = 1;
    icsInfo->winGroupLen[0] = 1;
    }
  }
//}}}
//{{{
void DecodePulseInfo (cBitStream& bitStream, PulseInfo* pi) {

  pi->numPulse = bitStream.GetBits(2) + 1;   /* add 1 here */
  pi->startSFB = bitStream.GetBits(6);
  for (int i = 0; i < pi->numPulse; i++) {
    pi->offset[i] = bitStream.GetBits( 5);
    pi->amp[i] = bitStream.GetBits(4);
    }
  }
//}}}
//{{{
void DecodeGainControlInfo (cBitStream& bitStream, int winSequence, GainControlInfo* gainControlInfo) {

  gainControlInfo->maxBand = bitStream.GetBits( 2);
  int maxWin =      (int)gainBits[winSequence][0];
  int locBitsZero = (int)gainBits[winSequence][1];
  int locBits =     (int)gainBits[winSequence][2];

  for (int bd = 1; bd <= gainControlInfo->maxBand; bd++) {
    for (int wd = 0; wd < maxWin; wd++) {
      gainControlInfo->adjNum[bd][wd] = bitStream.GetBits(3);
      for (int ad = 0; ad < gainControlInfo->adjNum[bd][wd]; ad++) {
        gainControlInfo->alevCode[bd][wd][ad] = bitStream.GetBits(4);
        gainControlInfo->alocCode[bd][wd][ad] = bitStream.GetBits((wd == 0 ? locBitsZero : locBits));
        }
      }
   }
  }
//}}}
//{{{
void DecodeICS (cBitStream& bitStream, PSInfoBase* psInfoBase, int ch) {

  auto icsInfo = (ch == 1 && psInfoBase->commonWin == 1) ? &(psInfoBase->icsInfo[0]) : &(psInfoBase->icsInfo[ch]);

  int globalGain = bitStream.GetBits (8);
  if (!psInfoBase->commonWin)
    DecodeICSInfo (bitStream, icsInfo, psInfoBase->sampRateIdx);
  DecodeSectionData (bitStream, icsInfo->winSequence, icsInfo->numWinGroup, icsInfo->maxSFB,
                     psInfoBase->sfbCodeBook[ch]);
  DecodeScaleFactors (bitStream, icsInfo->numWinGroup, icsInfo->maxSFB, globalGain, psInfoBase->sfbCodeBook[ch],
                      psInfoBase->scaleFactors[ch]);

  auto pi = &psInfoBase->pulseInfo[ch];
  pi->pulseDataPresent = bitStream.GetBits(1);
  if (pi->pulseDataPresent)
    DecodePulseInfo (bitStream, pi);

  auto tnsInfo = &psInfoBase->tnsInfo[ch];
  tnsInfo->tnsDataPresent = bitStream.GetBits(1);
  if (tnsInfo->tnsDataPresent)
    DecodeTNSInfo (bitStream, icsInfo->winSequence, tnsInfo, tnsInfo->coef);

  auto gainControlInfo = &psInfoBase->gainControlInfo[ch];
  gainControlInfo->gainControlDataPresent = bitStream.GetBits(1);
  if (gainControlInfo->gainControlDataPresent)
    DecodeGainControlInfo (bitStream, icsInfo->winSequence, gainControlInfo);
  }
//}}}
//}}}

// public
//{{{
cAacDecoder::cAacDecoder() {

  memset (this, 0, sizeof(cAacDecoder));
  psInfoBase = (PSInfoBase*)bigMalloc (sizeof(PSInfoBase), "aacPsInfoBase");
  memset (psInfoBase, 0, sizeof(PSInfoBase));

#ifdef AAC_ENABLE_SBR
  psInfoSBR = (PSInfoSBR*)bigMalloc(sizeof(PSInfoSBR), "aacPsInfoSBR");
  InitSBRState (psInfoSBR);
#endif
  }
//}}}
//{{{
cAacDecoder::~cAacDecoder() {

#ifdef AAC_ENABLE_SBR
  bigFree (psInfoSBR);
#endif

  bigFree (psInfoBase);
  }
//}}}

//{{{
int cAacDecoder::AACSetRawBlockParams (int copyLast, AACFrameInfo *aacFrameInfo)
{
  format = AAC_FF_RAW;
  if (copyLast)
    return SetRawBlockParams (1, 0, 0, 0);
  else
    return SetRawBlockParams (0, aacFrameInfo->nChans, aacFrameInfo->sampRateCore, aacFrameInfo->profile);
}
//}}}
//{{{
int cAacDecoder::AACFlushCodec() {
// reset common state variables which change per-frame
// don't touch state variables which are (usually) constant for entire clip
// (nChans, sampRate, profile, format, sbrEnabled)

  prevBlockID = AAC_ID_INVALID;
  currBlockID = AAC_ID_INVALID;
  currInstTag = -1;
  adtsBlocksLeft = 0;
  tnsUsed = 0;
  pnsUsed = 0;

  // reset internal codec state (flush overlap buffers, etc.)
  memset (psInfoBase->overlap, 0, AAC_MAX_NCHANS * AAC_MAX_NSAMPS * sizeof(int));
  memset (psInfoBase->prevWinShape, 0, AAC_MAX_NCHANS * sizeof(int));

#ifdef AAC_ENABLE_SBR
  InitSBRState (psInfoSBR);
#endif

  return ERR_AAC_NONE;
  }
//}}}
//{{{
// Inputs:      valid AAC decoder instance pointer (HAACDecoder)
//              double pointer to buffer of AAC data
//              pointer to number of valid bytes remaining in inbuf
//              pointer to outbuf, big enough to hold one frame of decoded PCM samples
//                (outbuf must be double-sized if SBR enabled)
// Outputs:     PCM data in outbuf, interleaved LRLRLR... if stereo
//                number of output samples = 1024 per channel (2048 if SBR enabled)
//              updated inbuf pointer
//              updated bytesLeft
// Return:      0 if successful, error code (< 0) if error
// Notes:       inbuf pointer and bytesLeft are not updated until whole frame is
//              successfully decoded, so if ERR_AAC_INDATA_UNDERFLOW is returned
//              just call AACDecode again with more data in inbuf
int cAacDecoder::AACDecode (unsigned char** inbuf, int* bytesLeft, short* outbuf) {

  int err, offset, bitOffset, bitsAvail;
  int ch, baseChan, elementChans;
  unsigned char *inptr;

#ifdef AAC_ENABLE_SBR
  int baseChanSBR;
  int elementChansSBR;
#endif

  /* make local copies (see "Notes" above) */
  inptr = *inbuf;
  bitOffset = 0;
  bitsAvail = (*bytesLeft) << 3;

  /* first time through figure out what the file format is */
  if (format == AAC_FF_Unknown) {
    if (bitsAvail < 32)
      return ERR_AAC_INDATA_UNDERFLOW;

    if (IS_ADIF(inptr)) {
      /* unpack ADIF header */
      format = AAC_FF_ADIF;
      err = UnpackADIFHeader (&inptr, &bitOffset, &bitsAvail);
      if (err)
        return err;
      }
    else {
      /* assume ADTS by default */
      format = AAC_FF_ADTS;
      }
    }

  /* if ADTS, search for start of next frame */
  if (format == AAC_FF_ADTS) {
    /* can have 1-4 raw data blocks per ADTS frame (header only present for first one) */
    if (adtsBlocksLeft == 0) {
      offset = AACFindSyncWord (inptr, bitsAvail >> 3);
      if (offset < 0)
        return ERR_AAC_INDATA_UNDERFLOW;
      inptr += offset;
      bitsAvail -= (offset << 3);

      err = UnpackADTSHeader (&inptr, &bitOffset, &bitsAvail);
      if (err)
        return err;

      if (nChans == -1) {
        /* figure out implicit channel mapping if necessary */
        err = GetADTSChannelMapping (inptr, bitOffset, bitsAvail);
        if (err)
          return err;
        }
      }
    adtsBlocksLeft--;
    }
  else if (format == AAC_FF_RAW) {
    prevBlockID = AAC_ID_INVALID;
    currBlockID = AAC_ID_INVALID;
    currInstTag = -1;
    bitRate = 0;
    sbrEnabled = 0;
    }

  /* check for valid number of channels */
  if (nChans > AAC_MAX_NCHANS || nChans <= 0)
    return ERR_AAC_NCHANS_TOO_HIGH;

  /* will be set later if active in this frame */
  tnsUsed = 0;
  pnsUsed = 0;

  bitOffset = 0;
  baseChan = 0;

#ifdef AAC_ENABLE_SBR
  baseChanSBR = 0;
#endif
  do {
    /* parse next syntactic element */
    err = DecodeNextElement (&inptr, &bitOffset, &bitsAvail);
    if (err)
      return err;

    elementChans = elementNumChans[currBlockID];
    if (baseChan + elementChans > AAC_MAX_NCHANS)
      return ERR_AAC_NCHANS_TOO_HIGH;

    /* noiseless decoder and dequantizer */
    for (ch = 0; ch < elementChans; ch++) {
      err = DecodeNoiselessData (&inptr, &bitOffset, &bitsAvail, ch);
      if (err)
        return err;
      if (Dequantize (ch))
        return ERR_AAC_DEQUANT;
      }

    /* mid-side and intensity stereo */
    if (currBlockID == AAC_ID_CPE)
      if (StereoProcess())
        return ERR_AAC_STEREO_PROCESS;


    /* PNS, TNS, inverse transform */
    for (ch = 0; ch < elementChans; ch++) {
      if (PNS (ch))
        return ERR_AAC_PNS;

      if (TNSFilter (ch))
        return ERR_AAC_TNS;

      if (IMDCT(ch, baseChan + ch, outbuf))
        return ERR_AAC_IMDCT;
      }

  #ifdef AAC_ENABLE_SBR
    //{{{  decode SBR
    if (sbrEnabled && (currBlockID == AAC_ID_FIL || currBlockID == AAC_ID_LFE)) {
      if (currBlockID == AAC_ID_LFE)
        elementChansSBR = elementNumChans[AAC_ID_LFE];
      else if (currBlockID == AAC_ID_FIL && (prevBlockID == AAC_ID_SCE || prevBlockID == AAC_ID_CPE))
        elementChansSBR = elementNumChans[prevBlockID];
      else
        elementChansSBR = 0;

      if (baseChanSBR + elementChansSBR > AAC_MAX_NCHANS)
        return ERR_AAC_SBR_NCHANS_TOO_HIGH;

      /* parse SBR extension data if present (contained in a fill element) */
      if (DecodeSBRBitstream (baseChanSBR))
        return ERR_AAC_SBR_BITSTREAM;

      /* apply SBR */
      if (DecodeSBRData (baseChanSBR, outbuf))
        return ERR_AAC_SBR_DATA;

      baseChanSBR += elementChansSBR;
      }
    //}}}
  #endif

    baseChan += elementChans;
    } while (currBlockID != AAC_ID_END);

  /* byte align after each raw_data_block */
  if (bitOffset) {
    inptr++;
    bitsAvail -= (8-bitOffset);
    bitOffset = 0;
    if (bitsAvail < 0)
      return ERR_AAC_INDATA_UNDERFLOW;
    }

  /* update pointers */
  frameCount++;
  *bytesLeft -= (int)(inptr - *inbuf);
  *inbuf = inptr;

  return ERR_AAC_NONE;
  }
//}}}

// private
//{{{
int cAacDecoder::IMDCT (int ch, int chOut, short* outbuf) {

  int i;
  auto icsInfo = (ch == 1 && psInfoBase->commonWin == 1) ? &(psInfoBase->icsInfo[0]) : &(psInfoBase->icsInfo[ch]);
  outbuf += chOut;

  /* optimized type-IV DCT (operates inplace) */
  if (icsInfo->winSequence == 2) {
    /* 8 short blocks */
    for (i = 0; i < 8; i++)
      DCT4(0, psInfoBase->coef[ch] + i*128, psInfoBase->gbCurrent[ch]);
  } else {
    /* 1 long block */
    DCT4(1, psInfoBase->coef[ch], psInfoBase->gbCurrent[ch]);
  }

#ifdef AAC_ENABLE_SBR
  /* window, overlap-add, don't clip to short (send to SBR decoder)
   * store the decoded 32-bit samples in top half (second AAC_MAX_NSAMPS samples) of coef buffer
   */
  if (icsInfo->winSequence == 0)
    DecWindowOverlapNoClip (psInfoBase->coef[ch], psInfoBase->overlap[chOut], psInfoBase->sbrWorkBuf[ch],
                            icsInfo->winShape, psInfoBase->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 1)
    DecWindowOverlapLongStartNoClip (psInfoBase->coef[ch], psInfoBase->overlap[chOut], psInfoBase->sbrWorkBuf[ch],
                                     icsInfo->winShape, psInfoBase->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 2)
    DecWindowOverlapShortNoClip (psInfoBase->coef[ch], psInfoBase->overlap[chOut], psInfoBase->sbrWorkBuf[ch],
                                 icsInfo->winShape, psInfoBase->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 3)
    DecWindowOverlapLongStopNoClip (psInfoBase->coef[ch], psInfoBase->overlap[chOut], psInfoBase->sbrWorkBuf[ch],
                                    icsInfo->winShape, psInfoBase->prevWinShape[chOut]);
  if (!sbrEnabled) {
    for (i = 0; i < AAC_MAX_NSAMPS; i++) {
      *outbuf = CLIPTOSHORT((psInfoBase->sbrWorkBuf[ch][i] + RND_VAL1) >> FBITS_OUT_IMDCT);
      outbuf += nChans;
    }
  }

  rawSampleBuf[ch] = psInfoBase->sbrWorkBuf[ch];
  rawSampleBytes = sizeof(int);
  rawSampleFBits = FBITS_OUT_IMDCT;
#else
  /* window, overlap-add, round to PCM - optimized for each window sequence */
  if (icsInfo->winSequence == 0)
    DecWindowOverlap (psInfoBase->coef[ch], psInfoBase->overlap[chOut], outbuf, nChans, icsInfo->winShape,
                      psInfoBase->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 1)
    DecWindowOverlapLongStart (psInfoBase->coef[ch], psInfoBase->overlap[chOut], outbuf, nChans, icsInfo->winShape,
                               psInfoBase->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 2)
    DecWindowOverlapShort (psInfoBase->coef[ch], psInfoBase->overlap[chOut], outbuf, nChans, icsInfo->winShape,
                           psInfoBase->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 3)
    DecWindowOverlapLongStop (psInfoBase->coef[ch], psInfoBase->overlap[chOut], outbuf, nChans, icsInfo->winShape,
                              psInfoBase->prevWinShape[chOut]);

  rawSampleBuf[ch] = 0;
  rawSampleBytes = 0;
  rawSampleFBits = 0;
#endif

  psInfoBase->prevWinShape[chOut] = icsInfo->winShape;
  return 0;
  }
//}}}

//{{{
int cAacDecoder::DecodeSBRBitstream (int chBase) {

  if (currBlockID != AAC_ID_FIL ||
      (fillExtType != EXT_SBR_DATA && fillExtType != EXT_SBR_DATA_CRC))
    return ERR_AAC_NONE;

  cBitStream bitStream (fillCount, fillBuf);
  if (bitStream.GetBits (4) != (unsigned int)fillExtType)
    return ERR_AAC_SBR_BITSTREAM;

  if (fillExtType == EXT_SBR_DATA_CRC)
    psInfoSBR->crcCheckWord = bitStream.GetBits(10);

  int headerFlag = bitStream.GetBits(1);
  if (headerFlag) {
    /* get sample rate index for output sample rate (2x base rate) */
    psInfoSBR->sampRateIdx = GetSampRateIdx(2 * sampRate);
    if (psInfoSBR->sampRateIdx < 0 || psInfoSBR->sampRateIdx >= NUM_SAMPLE_RATES)
      return ERR_AAC_SBR_BITSTREAM;
    else if (psInfoSBR->sampRateIdx >= NUM_SAMPLE_RATES_SBR)
      return ERR_AAC_SBR_SINGLERATE_UNSUPPORTED;

    /* reset flag = 1 if header values changed */
    if (UnpackSBRHeader (&bitStream, &(psInfoSBR->sbrHeader[chBase])))
      psInfoSBR->sbrChan[chBase].reset = 1;

    /* first valid SBR header should always trigger CalcFreqTables(), since psi->reset was set in InitSBR() */
    if (psInfoSBR->sbrChan[chBase].reset)
      CalcFreqTables (& (psInfoSBR->sbrHeader[chBase+0]), &(psInfoSBR->sbrFreq[chBase]), psInfoSBR->sampRateIdx);

    /* copy and reset state to right channel for CPE */
    if (prevBlockID == AAC_ID_CPE)
      psInfoSBR->sbrChan[chBase+1].reset = psInfoSBR->sbrChan[chBase+0].reset;
    }

  /* if no header has been received, upsample only */
  if (psInfoSBR->sbrHeader[chBase].count == 0)
    return ERR_AAC_NONE;

  if (prevBlockID == AAC_ID_SCE)
    UnpackSBRSingleChannel (&bitStream, psInfoSBR, chBase);
  else if (prevBlockID == AAC_ID_CPE)
    UnpackSBRChannelPair (&bitStream, psInfoSBR, chBase);
  else
    return ERR_AAC_SBR_BITSTREAM;

  bitStream.ByteAlignBitstream ();

  return ERR_AAC_NONE;
  }
//}}}
//{{{
int cAacDecoder::DecodeSBRData (int chBase, short* outbuf) {

  int ch, qmfaBands, qmfsBands;
  int gbIdx, gbMask;

  /* same header and freq tables for both channels in CPE */
  auto sbrHeader =  &(psInfoSBR->sbrHeader[chBase]);
  auto sbrFreq = &(psInfoSBR->sbrFreq[chBase]);

  /* upsample only if we haven't received an SBR header yet or if we have an LFE block */
  int chBlock;
  int upsampleOnly;
  if (currBlockID == AAC_ID_LFE) {
    chBlock = 1;
    upsampleOnly = 1;
    }
  else if (currBlockID == AAC_ID_FIL) {
    if (prevBlockID == AAC_ID_SCE)
      chBlock = 1;
    else if (prevBlockID == AAC_ID_CPE)
      chBlock = 2;
    else
      return ERR_AAC_NONE;

    upsampleOnly = (sbrHeader->count == 0 ? 1 : 0);
    if (fillExtType != EXT_SBR_DATA && fillExtType != EXT_SBR_DATA_CRC)
      return ERR_AAC_NONE;
    }
  else /* ignore non-SBR blocks */
    return ERR_AAC_NONE;

  if (upsampleOnly) {
    sbrFreq->kStart = 32;
    sbrFreq->numQMFBands = 0;
    }

  for (ch = 0; ch < chBlock; ch++) {
    SBRGrid* sbrGrid = &(psInfoSBR->sbrGrid[chBase + ch]);
    SBRChan* sbrChan = &(psInfoSBR->sbrChan[chBase + ch]);

    if (rawSampleBuf[ch] == 0 || rawSampleBytes != 4)
      return ERR_AAC_SBR_PCM_FORMAT;
    int* inbuf = (int *)rawSampleBuf[ch];
    short* outptr = outbuf + chBase + ch;

    /* restore delay buffers (could use ring buffer or keep in temp buffer for nChans == 1) */
    for (int l = 0; l < HF_GEN; l++) {
      for (int k = 0; k < 64; k++) {
        psInfoSBR->XBuf[l][k][0] = psInfoSBR->XBufDelay[chBase + ch][l][k][0];
        psInfoSBR->XBuf[l][k][1] = psInfoSBR->XBufDelay[chBase + ch][l][k][1];
        }
      }

    /* step 1 - analysis QMF */
    qmfaBands = sbrFreq->kStart;
    for (int l = 0; l < 32; l++) {
      gbMask = QMFAnalysis(inbuf + l*32, psInfoSBR->delayQMFA[chBase + ch], psInfoSBR->XBuf[l + HF_GEN][0],
        rawSampleFBits, &(psInfoSBR->delayIdxQMFA[chBase + ch]), qmfaBands);
      gbIdx = ((l + HF_GEN) >> 5) & 0x01;
      sbrChan->gbMask[gbIdx] |= gbMask; /* gbIdx = (0 if i < 32), (1 if i >= 32) */
      }

    if (upsampleOnly) {
      /* no SBR - just run synthesis QMF to upsample by 2x */
      qmfsBands = 32;
      for (int l = 0; l < 32; l++) {
        /* step 4 - synthesis QMF */
        QMFSynthesis (psInfoSBR->XBuf[l + HF_ADJ][0], psInfoSBR->delayQMFS[chBase + ch],
                      &(psInfoSBR->delayIdxQMFS[chBase + ch]), qmfsBands, outptr, nChans);
        outptr += 64*nChans;
        }
      }
    else {
      /* if previous frame had lower SBR starting freq than current, zero out the synthesized QMF
       *   bands so they aren't used as sources for patching
       * after patch generation, restore from delay buffer
       * can only happen after header reset
       */
      for (int k = sbrFreq->kStartPrev; k < sbrFreq->kStart; k++) {
        for (int l = 0; l < sbrGrid->envTimeBorder[0] + HF_ADJ; l++) {
          psInfoSBR->XBuf[l][k][0] = 0;
          psInfoSBR->XBuf[l][k][1] = 0;
          }
        }

      /* step 2 - HF generation */
      GenerateHighFreq (psInfoSBR, sbrGrid, sbrFreq, sbrChan, ch);

      /* restore SBR bands that were cleared before patch generation (time slots 0, 1 no longer needed) */
      for (int k = sbrFreq->kStartPrev; k < sbrFreq->kStart; k++) {
        for (int l = HF_ADJ; l < sbrGrid->envTimeBorder[0] + HF_ADJ; l++) {
          psInfoSBR->XBuf[l][k][0] = psInfoSBR->XBufDelay[chBase + ch][l][k][0];
          psInfoSBR->XBuf[l][k][1] = psInfoSBR->XBufDelay[chBase + ch][l][k][1];
          }
        }

      /* step 3 - HF adjustment */
      AdjustHighFreq (psInfoSBR, sbrHeader, sbrGrid, sbrFreq, sbrChan, ch);

      /* step 4 - synthesis QMF */
      qmfsBands = sbrFreq->kStartPrev + sbrFreq->numQMFBandsPrev;
      int l;
      for (l = 0; l < sbrGrid->envTimeBorder[0]; l++) {
        /* if new envelope starts mid-frame, use old settings until start of first envelope in this frame */
        QMFSynthesis (psInfoSBR->XBuf[l + HF_ADJ][0], psInfoSBR->delayQMFS[chBase + ch],
                      &(psInfoSBR->delayIdxQMFS[chBase + ch]), qmfsBands, outptr, nChans);
        outptr += 64*nChans;
        }

      qmfsBands = sbrFreq->kStart + sbrFreq->numQMFBands;
      for (     ; l < 32; l++) {
        /* use new settings for rest of frame (usually the entire frame, unless the first envelope starts mid-frame) */
        QMFSynthesis (psInfoSBR->XBuf[l + HF_ADJ][0], psInfoSBR->delayQMFS[chBase + ch],
                      &(psInfoSBR->delayIdxQMFS[chBase + ch]), qmfsBands, outptr, nChans);
        outptr += 64*nChans;
        }
      }

    /* save delay */
    for (int l = 0; l < HF_GEN; l++) {
      for (int k = 0; k < 64; k++) {
        psInfoSBR->XBufDelay[chBase + ch][l][k][0] = psInfoSBR->XBuf[l+32][k][0];
        psInfoSBR->XBufDelay[chBase + ch][l][k][1] = psInfoSBR->XBuf[l+32][k][1];
        }
      }
    sbrChan->gbMask[0] = sbrChan->gbMask[1];
    sbrChan->gbMask[1] = 0;

    if (sbrHeader->count > 0)
      sbrChan->reset = 0;
    }

  sbrFreq->kStartPrev = sbrFreq->kStart;
  sbrFreq->numQMFBandsPrev = sbrFreq->numQMFBands;

  if (nChans > 0 && (chBase + ch) == nChans)
    psInfoSBR->frameCount++;

  return ERR_AAC_NONE;
  }
//}}}

//{{{
int cAacDecoder::StereoProcess()
{
  int gp, win, nSamps, msMaskOffset;
  int *coefL, *coefR;
  unsigned char *msMaskPtr;
  const short *sfbTab;

  /* mid-side and intensity stereo require common_window == 1 (see MPEG4 spec, Correction 2, 2004) */
  if (psInfoBase->commonWin != 1 || currBlockID != AAC_ID_CPE)
    return 0;

  /* nothing to do */
  if (!psInfoBase->msMaskPresent && !psInfoBase->intensityUsed[1])
    return 0;

  auto icsInfo = &(psInfoBase->icsInfo[0]);
  if (icsInfo->winSequence == 2) {
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psInfoBase->sampRateIdx];
    nSamps = NSAMPS_SHORT;
  } else {
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psInfoBase->sampRateIdx];
    nSamps = NSAMPS_LONG;
  }
  coefL = psInfoBase->coef[0];
  coefR = psInfoBase->coef[1];

  /* do fused mid-side/intensity processing for each block (one long or eight short) */
  msMaskOffset = 0;
  msMaskPtr = psInfoBase->msMaskBits;
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      StereoProcessGroup(coefL, coefR, sfbTab, psInfoBase->msMaskPresent,
        msMaskPtr, msMaskOffset, icsInfo->maxSFB, psInfoBase->sfbCodeBook[1] + gp*icsInfo->maxSFB,
        psInfoBase->scaleFactors[1] + gp*icsInfo->maxSFB, psInfoBase->gbCurrent);
      coefL += nSamps;
      coefR += nSamps;
    }
    /* we use one bit per sfb, so there are maxSFB bits for each window group */
    msMaskPtr += (msMaskOffset + icsInfo->maxSFB) >> 3;
    msMaskOffset = (msMaskOffset + icsInfo->maxSFB) & 0x07;
  }

  ASSERT(coefL == psInfoBase->coef[0] + 1024);
  ASSERT(coefR == psInfoBase->coef[1] + 1024);

  return 0;
}
//}}}
//{{{
int cAacDecoder::Dequantize (int ch)
{
  int gp, cb, sfb, win, width, nSamps, gbMask;
  int *coef;
  const short *sfbTab;
  unsigned char *sfbCodeBook;
  short *scaleFactors;

  ICSInfo *icsInfo = (ch == 1 && psInfoBase->commonWin == 1) ? &(psInfoBase->icsInfo[0]) : &(psInfoBase->icsInfo[ch]);
  if (icsInfo->winSequence == 2) {
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psInfoBase->sampRateIdx];
    nSamps = NSAMPS_SHORT;
  } else {
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psInfoBase->sampRateIdx];
    nSamps = NSAMPS_LONG;
  }
  coef = psInfoBase->coef[ch];
  sfbCodeBook = psInfoBase->sfbCodeBook[ch];
  scaleFactors = psInfoBase->scaleFactors[ch];

  psInfoBase->intensityUsed[ch] = 0;
  psInfoBase->pnsUsed[ch] = 0;
  gbMask = 0;
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
        /* dequantize one scalefactor band (not necessary if codebook is intensity or PNS)
         * for zero codebook, still run dequantizer in case non-zero pulse data was added
         */
        cb = (int)(sfbCodeBook[sfb]);
        width = sfbTab[sfb+1] - sfbTab[sfb];
        if (cb >= 0 && cb <= 11)
          gbMask |= DequantBlock(coef, width, scaleFactors[sfb]);
        else if (cb == 13)
          psInfoBase->pnsUsed[ch] = 1;
        else if (cb == 14 || cb == 15)
          psInfoBase->intensityUsed[ch] = 1; /* should only happen if ch == 1 */
        coef += width;
      }
      coef += (nSamps - sfbTab[icsInfo->maxSFB]);
    }
    sfbCodeBook += icsInfo->maxSFB;
    scaleFactors += icsInfo->maxSFB;
  }
  pnsUsed |= psInfoBase->pnsUsed[ch];  /* set flag if PNS used for any channel */

  /* calculate number of guard bits in dequantized data */
  psInfoBase->gbCurrent[ch] = CLZ(gbMask) - 1;

  return ERR_AAC_NONE;
}
//}}}
//{{{
int cAacDecoder::PNS (int ch) {

  int gp, sfb, win, width, nSamps, gb, gbMask;
  int *coef;
  const short *sfbTab;
  unsigned char *sfbCodeBook;
  short *scaleFactors;
  int msMaskOffset, checkCorr, genNew;
  unsigned char msMask;
  unsigned char *msMaskPtr;

  if (!psInfoBase->pnsUsed[ch])
    return 0;

  auto icsInfo = (ch == 1 && psInfoBase->commonWin == 1) ? &(psInfoBase->icsInfo[0]) : &(psInfoBase->icsInfo[ch]);
  if (icsInfo->winSequence == 2) {
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psInfoBase->sampRateIdx];
    nSamps = NSAMPS_SHORT;
    }
  else {
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psInfoBase->sampRateIdx];
    nSamps = NSAMPS_LONG;
    }

  coef = psInfoBase->coef[ch];
  sfbCodeBook = psInfoBase->sfbCodeBook[ch];
  scaleFactors = psInfoBase->scaleFactors[ch];
  checkCorr = (currBlockID == AAC_ID_CPE && psInfoBase->commonWin == 1 ? 1 : 0);

  gbMask = 0;
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      msMaskPtr = psInfoBase->msMaskBits + ((gp*icsInfo->maxSFB) >> 3);
      msMaskOffset = ((gp*icsInfo->maxSFB) & 0x07);
      msMask = (*msMaskPtr++) >> msMaskOffset;

      for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
        width = sfbTab[sfb+1] - sfbTab[sfb];
        if (sfbCodeBook[sfb] == 13) {
          if (ch == 0) {
            /* generate new vector, copy into ch 1 if it's possible that the channels will be correlated
             * if ch 1 has PNS enabled for this SFB but it's uncorrelated (i.e. ms_used == 0),
             *    the copied values will be overwritten when we process ch 1
             */
            GenerateNoiseVector(coef, &psInfoBase->pnsLastVal, width);
            if (checkCorr && psInfoBase->sfbCodeBook[1][gp*icsInfo->maxSFB + sfb] == 13)
              CopyNoiseVector(coef, psInfoBase->coef[1] + (coef - psInfoBase->coef[0]), width);
            }
          else {
            /* generate new vector if no correlation between channels */
            genNew = 1;
            if (checkCorr && psInfoBase->sfbCodeBook[0][gp*icsInfo->maxSFB + sfb] == 13) {
              if ( (psInfoBase->msMaskPresent == 1 && (msMask & 0x01)) || psInfoBase->msMaskPresent == 2 )
                genNew = 0;
              }
            if (genNew)
              GenerateNoiseVector(coef, &psInfoBase->pnsLastVal, width);
            }
          gbMask |= ScaleNoiseVector(coef, width, psInfoBase->scaleFactors[ch][gp*icsInfo->maxSFB + sfb]);
          }
        coef += width;

        /* get next mask bit (should be branchless on ARM) */
        msMask >>= 1;
        if (++msMaskOffset == 8) {
          msMask = *msMaskPtr++;
          msMaskOffset = 0;
          }
        }
      coef += (nSamps - sfbTab[icsInfo->maxSFB]);
      }
    sfbCodeBook += icsInfo->maxSFB;
    scaleFactors += icsInfo->maxSFB;
    }

  /* update guard bit count if necessary */
  gb = CLZ (gbMask) - 1;
  if (psInfoBase->gbCurrent[ch] > gb)
    psInfoBase->gbCurrent[ch] = gb;

  return 0;
  }
//}}}
//{{{
int cAacDecoder::TNSFilter (int ch)
{
  int win, winLen, nWindows, nSFB, filt, bottom, top, order, maxOrder, dir;
  int start, end, size, tnsMaxBand, numFilt, gbMask;
  int *audioCoef;
  unsigned char *filtLength, *filtOrder, *filtRes, *filtDir;
  signed char *filtCoef;
  const unsigned char *tnsMaxBandTab;
  const short *sfbTab;

  auto ti = &psInfoBase->tnsInfo[ch];
  if (!ti->tnsDataPresent)
    return 0;

  auto icsInfo = (ch == 1 && psInfoBase->commonWin == 1) ? &(psInfoBase->icsInfo[0]) : &(psInfoBase->icsInfo[ch]);
  if (icsInfo->winSequence == 2) {
    nWindows = NWINDOWS_SHORT;
    winLen = NSAMPS_SHORT;
    nSFB = sfBandTotalShort[psInfoBase->sampRateIdx];
    maxOrder = tnsMaxOrderShort[profile];
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psInfoBase->sampRateIdx];
    tnsMaxBandTab = tnsMaxBandsShort + tnsMaxBandsShortOffset[profile];
    tnsMaxBand = tnsMaxBandTab[psInfoBase->sampRateIdx];
    }
  else {
    nWindows = NWINDOWS_LONG;
    winLen = NSAMPS_LONG;
    nSFB = sfBandTotalLong[psInfoBase->sampRateIdx];
    maxOrder = tnsMaxOrderLong[profile];
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psInfoBase->sampRateIdx];
    tnsMaxBandTab = tnsMaxBandsLong + tnsMaxBandsLongOffset[profile];
    tnsMaxBand = tnsMaxBandTab[psInfoBase->sampRateIdx];
    }

  if (tnsMaxBand > icsInfo->maxSFB)
    tnsMaxBand = icsInfo->maxSFB;

  filtRes =    ti->coefRes;
  filtLength = ti->length;
  filtOrder =  ti->order;
  filtDir =    ti->dir;
  filtCoef =   ti->coef;

  gbMask = 0;
  audioCoef =  psInfoBase->coef[ch];
  for (win = 0; win < nWindows; win++) {
    bottom = nSFB;
    numFilt = ti->numFilt[win];
    for (filt = 0; filt < numFilt; filt++) {
      top = bottom;
      bottom = top - *filtLength++;
      bottom = MAX(bottom, 0);
      order = *filtOrder++;
      order = MIN(order, maxOrder);

      if (order) {
        start = sfbTab[MIN(bottom, tnsMaxBand)];
        end   = sfbTab[MIN(top, tnsMaxBand)];
        size = end - start;
        if (size > 0) {
          dir = *filtDir++;
          if (dir)
            start = end - 1;

          DecodeLPCCoefs(order, filtRes[win], filtCoef, psInfoBase->tnsLPCBuf, psInfoBase->tnsWorkBuf);
          gbMask |= FilterRegion(size, dir, order, audioCoef + start, psInfoBase->tnsLPCBuf, psInfoBase->tnsWorkBuf);
        }
        filtCoef += order;
      }
    }
    audioCoef += winLen;
  }

  /* update guard bit count if necessary */
  size = CLZ(gbMask) - 1;
  if (psInfoBase->gbCurrent[ch] > size)
    psInfoBase->gbCurrent[ch] = size;

  return 0;
}
//}}}

//{{{
int cAacDecoder::DecodeSingleChannelElement (cBitStream& bitStream) {
  /* read instance tag */
  currInstTag = bitStream.GetBits (NUM_INST_TAG_BITS);
  return 0;
  }
//}}}
//{{{
int cAacDecoder::DecodeChannelPairElement (cBitStream& bitStream)
{
  int sfb, gp, maskOffset;
  unsigned char currBit, *maskPtr;

  auto icsInfo = psInfoBase->icsInfo;

  /* read instance tag */
  currInstTag = bitStream.GetBits(NUM_INST_TAG_BITS);

  /* read common window flag and mid-side info (if present)
   * store msMask bits in psInfoBase->msMaskBits[] as follows:
   *  long blocks -  pack bits for each SFB in range [0, maxSFB) starting with lsb of msMaskBits[0]
   *  short blocks - pack bits for each SFB in range [0, maxSFB), for each group [0, 7]
   * msMaskPresent = 0 means no M/S coding
   *               = 1 means psInfoBase->msMaskBits contains 1 bit per SFB to toggle M/S coding
   *               = 2 means all SFB's are M/S coded (so psInfoBase->msMaskBits is not needed)
   */
  psInfoBase->commonWin = bitStream.GetBits(1);
  if (psInfoBase->commonWin) {
    DecodeICSInfo (bitStream, icsInfo, psInfoBase->sampRateIdx);
    psInfoBase->msMaskPresent = bitStream.GetBits(2);
    if (psInfoBase->msMaskPresent == 1) {
      maskPtr = psInfoBase->msMaskBits;
      *maskPtr = 0;
      maskOffset = 0;
      for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
        for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
          currBit = (unsigned char)(bitStream.GetBits( 1));
          *maskPtr |= currBit << maskOffset;
          if (++maskOffset == 8) {
            maskPtr++;
            *maskPtr = 0;
            maskOffset = 0;
          }
        }
      }
    }
  }

  return 0;
}
//}}}
//{{{
int cAacDecoder::DecodeLFEChannelElement (cBitStream& bitStream) {
  currInstTag = bitStream.GetBits (NUM_INST_TAG_BITS);
  return 0;
  }
//}}}
//{{{
int cAacDecoder::DecodeDataStreamElement (cBitStream& bitStream) {

  currInstTag = bitStream.GetBits(NUM_INST_TAG_BITS);
  unsigned int byteAlign = bitStream.GetBits(1);

  unsigned int dataCount = bitStream.GetBits(8);
  if (dataCount == 255)
    dataCount += bitStream.GetBits(8);

  if (byteAlign)
    bitStream.ByteAlignBitstream();

  psInfoBase->dataCount = dataCount;
  unsigned char* dataBuf = psInfoBase->dataBuf;
  while (dataCount--)
    *dataBuf++ = bitStream.GetBits(8);

  return 0;
  }
//}}}
//{{{
int cAacDecoder::DecodeFillElement (cBitStream& bitStream) {

  unsigned int myFillCount = bitStream.GetBits(4);
  if (myFillCount == 15)
    myFillCount += (bitStream.GetBits(8) - 1);

  psInfoBase->fillCount = myFillCount;
  unsigned char* myFillBuf = psInfoBase->fillBuf;
  while (myFillCount--)
    *myFillBuf++ = bitStream.GetBits(8);

  currInstTag = -1; /* fill elements don't have instance tag */
  fillExtType = 0;

#ifdef AAC_ENABLE_SBR
  // check for SBR, sbrEnabled is sticky (reset each raw_data_block), so for multichannel
  // need to verify that all SCE/CPE/ICCE have valid SBR fill element following, and must upsample by 2 for LFE
  if (psInfoBase->fillCount > 0) {
    fillExtType = (int)((psInfoBase->fillBuf[0] >> 4) & 0x0f);
    if (fillExtType == EXT_SBR_DATA || fillExtType == EXT_SBR_DATA_CRC)
      sbrEnabled = 1;
    }
#endif

  fillBuf = psInfoBase->fillBuf;
  fillCount = psInfoBase->fillCount;
  return 0;
  }
//}}}

//{{{
int cAacDecoder::DecodeNoiselessData (unsigned char** buf, int* bitOffset, int* bitsAvail, int ch) {

  cBitStream bitStream ((*bitsAvail+7) >> 3, *buf);
  bitStream.GetBits (*bitOffset);

  DecodeICS (bitStream, psInfoBase, ch);

  auto icsInfo = (ch == 1 && psInfoBase->commonWin == 1) ? &(psInfoBase->icsInfo[0]) : &(psInfoBase->icsInfo[ch]);
  if (icsInfo->winSequence == 2)
    DecodeSpectrumShort (bitStream, psInfoBase, ch);
  else
    DecodeSpectrumLong (bitStream, psInfoBase, ch);

  int bitsUsed = bitStream.CalcBitsUsed(*buf, *bitOffset);
  *buf += ((bitsUsed + *bitOffset) >> 3);
  *bitOffset = ((bitsUsed + *bitOffset) & 0x07);
  *bitsAvail -= bitsUsed;

  tnsUsed |= psInfoBase->tnsInfo[ch].tnsDataPresent; /* set flag if TNS used for any channel */

  return ERR_AAC_NONE;
  }
//}}}
//{{{
int cAacDecoder::DecodeNextElement (unsigned char **buf, int* bitOffset, int* bitsAvail) {

  cBitStream bitStream ((*bitsAvail + 7) >> 3, *buf);
  bitStream.GetBits (*bitOffset);

  /* read element ID (save last ID for SBR purposes) */
  prevBlockID = currBlockID;
  currBlockID = bitStream.GetBits (NUM_SYN_ID_BITS);

  /* set defaults (could be overwritten by DecodeXXXElement(), depending on currBlockID) */
  psInfoBase->commonWin = 0;

  int err = 0;
  switch (currBlockID) {
    case AAC_ID_SCE:
      err = DecodeSingleChannelElement (bitStream);
      break;
    case AAC_ID_CPE:
      err = DecodeChannelPairElement (bitStream);
      break;
    case AAC_ID_CCE:
      /* TODO - implement CCE decoding */
      break;
    case AAC_ID_LFE:
      err = DecodeLFEChannelElement (bitStream);
      break;
    case AAC_ID_DSE:
      err = DecodeDataStreamElement (bitStream);
      break;
    case AAC_ID_PCE:
      err = DecodeProgramConfigElement (&bitStream, psInfoBase->pce + 0);
      break;
    case AAC_ID_FIL:
      err = DecodeFillElement (bitStream);
      break;
    case AAC_ID_END:
      break;
    }
  if (err)
    return ERR_AAC_SYNTAX_ELEMENT;

  /* update bitstream reader */
  int bitsUsed = bitStream.CalcBitsUsed (*buf, *bitOffset);
  *buf += (bitsUsed + *bitOffset) >> 3;
  *bitOffset = (bitsUsed + *bitOffset) & 0x07;
  *bitsAvail -= bitsUsed;

  if (*bitsAvail < 0)
    return ERR_AAC_INDATA_UNDERFLOW;

  return ERR_AAC_NONE;
  }
//}}}

//{{{
int cAacDecoder::SetRawBlockParams (int copyLast, int nChans, int sampRate, int profile)
{
  int idx;

  if (!copyLast) {
    //profile = profile;
    psInfoBase->nChans = nChans;
    for (idx = 0; idx < NUM_SAMPLE_RATES; idx++) {
      if (sampRate == sampRateTab[idx]) {
        psInfoBase->sampRateIdx = idx;
        break;
      }
    }
    if (idx == NUM_SAMPLE_RATES)
      return ERR_AAC_INVALID_FRAME;
  }
  nChans = psInfoBase->nChans;
  sampRate = sampRateTab[psInfoBase->sampRateIdx];

  /* check validity of header */
  if (psInfoBase->sampRateIdx >= NUM_SAMPLE_RATES || psInfoBase->sampRateIdx < 0 || profile != AAC_PROFILE_LC)
    return ERR_AAC_RAWBLOCK_PARAMS;

  return ERR_AAC_NONE;
}
//}}}

//{{{
int cAacDecoder::UnpackADIFHeader (unsigned char* *buf, int* bitOffset, int* bitsAvail)
{
  int i, bitsUsed;
  ADIFHeader *fhADIF;
  ProgConfigElement *pce;

  cBitStream bitStream ((*bitsAvail + 7) >> 3, *buf);
  bitStream.GetBits (*bitOffset);

  /* unpack ADIF file header */
  fhADIF = &(psInfoBase->fhADIF);
  pce = psInfoBase->pce;

  /* verify that first 32 bits of header are "ADIF" */
  if (bitStream.GetBits(8) != 'A' || bitStream.GetBits(8) != 'D' || bitStream.GetBits(8) != 'I' || bitStream.GetBits(8) != 'F')
    return ERR_AAC_INVALID_ADIF_HEADER;

  /* read ADIF header fields */
  fhADIF->copyBit = bitStream.GetBits(1);
  if (fhADIF->copyBit) {
    for (i = 0; i < ADIF_COPYID_SIZE; i++)
      fhADIF->copyID[i] = bitStream.GetBits(8);
  }
  fhADIF->origCopy = bitStream.GetBits(1);
  fhADIF->home =     bitStream.GetBits(1);
  fhADIF->bsType =   bitStream.GetBits(1);
  fhADIF->bitRate =  bitStream.GetBits(23);
  fhADIF->numPCE =   bitStream.GetBits(4) + 1;  /* add 1 (so range = [1, 16]) */
  if (fhADIF->bsType == 0)
    fhADIF->bufferFull = bitStream.GetBits(20);

  /* parse all program config elements */
  for (i = 0; i < fhADIF->numPCE; i++)
    DecodeProgramConfigElement (&bitStream, pce + i);

  /* byte align */
  bitStream.ByteAlignBitstream();

  /* update codec info */
  psInfoBase->nChans = GetNumChannelsADIF(pce, fhADIF->numPCE);
  psInfoBase->sampRateIdx = GetSampleRateIdxADIF(pce, fhADIF->numPCE);

  /* check validity of header */
  if (psInfoBase->nChans < 0 || psInfoBase->sampRateIdx < 0 || psInfoBase->sampRateIdx >= NUM_SAMPLE_RATES)
    return ERR_AAC_INVALID_ADIF_HEADER;

  /* syntactic element fields will be read from bitstream for each element */
  prevBlockID = AAC_ID_INVALID;
  currBlockID = AAC_ID_INVALID;
  currInstTag = -1;

  /* fill in user-accessible data */
  bitRate = 0;
  nChans = psInfoBase->nChans;
  sampRate = sampRateTab[psInfoBase->sampRateIdx];
  profile = pce[0].profile;
  sbrEnabled = 0;

  /* update bitstream reader */
  bitsUsed = bitStream.CalcBitsUsed(*buf, *bitOffset);
  *buf += (bitsUsed + *bitOffset) >> 3;
  *bitOffset = (bitsUsed + *bitOffset) & 0x07;
  *bitsAvail -= bitsUsed ;
  if (*bitsAvail < 0)
    return ERR_AAC_INDATA_UNDERFLOW;

  return ERR_AAC_NONE;
}
//}}}
//{{{
int cAacDecoder::UnpackADTSHeader (unsigned char** buf, int* bitOffset, int* bitsAvail)
{
  int bitsUsed;
  ADTSHeader *fhADTS;

  fhADTS = &(psInfoBase->fhADTS);

  cBitStream bitStream ((*bitsAvail + 7) >> 3, *buf);
  bitStream.GetBits (*bitOffset);

  /* verify that first 12 bits of header are syncword */
  if (bitStream.GetBits(12) != 0x0fff)
    return ERR_AAC_INVALID_ADTS_HEADER;

  /* fixed fields - should not change from frame to frame */
  fhADTS->id =               bitStream.GetBits(1);
  fhADTS->layer =            bitStream.GetBits(2);
  fhADTS->protectBit =       bitStream.GetBits(1);
  fhADTS->profile =          bitStream.GetBits(2);
  fhADTS->sampRateIdx =      bitStream.GetBits(4);
  fhADTS->privateBit =       bitStream.GetBits(1);
  fhADTS->channelConfig =    bitStream.GetBits(3);
  fhADTS->origCopy =         bitStream.GetBits(1);
  fhADTS->home =             bitStream.GetBits(1);

  /* variable fields - can change from frame to frame */
  fhADTS->copyBit =          bitStream.GetBits(1);
  fhADTS->copyStart =        bitStream.GetBits(1);
  fhADTS->frameLength =      bitStream.GetBits(13);
  fhADTS->bufferFull =       bitStream.GetBits(11);
  fhADTS->numRawDataBlocks = bitStream.GetBits(2) + 1;

  /* note - MPEG4 spec, correction 1 changes how CRC is handled when protectBit == 0 and numRawDataBlocks > 1 */
  if (fhADTS->protectBit == 0)
    fhADTS->crcCheckWord = bitStream.GetBits(16);

  /* byte align */
  bitStream.ByteAlignBitstream(); /* should always be aligned anyway */

  /* check validity of header */
  if (fhADTS->layer != 0 || fhADTS->profile != AAC_PROFILE_LC ||
    fhADTS->sampRateIdx >= NUM_SAMPLE_RATES || fhADTS->channelConfig >= NUM_DEF_CHAN_MAPS)
    return ERR_AAC_INVALID_ADTS_HEADER;

  /* update codec info */
  psInfoBase->sampRateIdx = fhADTS->sampRateIdx;
  if (!psInfoBase->useImpChanMap)
    psInfoBase->nChans = channelMapTab[fhADTS->channelConfig];

  /* syntactic element fields will be read from bitstream for each element */
  prevBlockID = AAC_ID_INVALID;
  currBlockID = AAC_ID_INVALID;
  currInstTag = -1;

  /* fill in user-accessible data (TODO - calc bitrate, handle tricky channel config cases) */
  bitRate = 0;
  nChans = psInfoBase->nChans;
  sampRate = sampRateTab[psInfoBase->sampRateIdx];
  profile = fhADTS->profile;
  sbrEnabled = 0;
  adtsBlocksLeft = fhADTS->numRawDataBlocks;

  /* update bitstream reader */
  bitsUsed = bitStream.CalcBitsUsed(*buf, *bitOffset);
  *buf += (bitsUsed + *bitOffset) >> 3;
  *bitOffset = (bitsUsed + *bitOffset) & 0x07;
  *bitsAvail -= bitsUsed ;
  if (*bitsAvail < 0)
    return ERR_AAC_INDATA_UNDERFLOW;

  return ERR_AAC_NONE;
}
//}}}

//{{{
int cAacDecoder::GetADTSChannelMapping (unsigned char* buf, int bitOffset, int bitsAvail) {

  int nChans = 0;
  do {
    /* parse next syntactic element */
    int err = DecodeNextElement (&buf, &bitOffset, &bitsAvail);
    if (err)
      return err;

    int elementChans = elementNumChans[currBlockID];
    nChans += elementChans;

    for (int ch = 0; ch < elementChans; ch++) {
      err = DecodeNoiselessData (&buf, &bitOffset, &bitsAvail, ch);
      if (err)
        return err;
      }
    } while (currBlockID != AAC_ID_END);

  if (nChans <= 0)
    return ERR_AAC_CHANNEL_MAP;

  /* update number of channels in codec state and user-accessible info structs */
  psInfoBase->nChans = nChans;
  nChans = psInfoBase->nChans;
  psInfoBase->useImpChanMap = 1;

  return ERR_AAC_NONE;
  }
//}}}
//{{{
int cAacDecoder::AACFindSyncWord (unsigned char* buf, int nBytes) {

  int i;
  /* find byte-aligned syncword (12 bits = 0xFFF) */
  for (i = 0; i < nBytes - 1; i++) {
    if ( (buf[i+0] & SYNCWORDH) == SYNCWORDH && (buf[i+1] & SYNCWORDL) == SYNCWORDL )
      return i;
    }

  return -1;
  }
//}}}
