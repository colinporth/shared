// sbr.cpp
//{{{  includes
#include "sbr.h"

#include "aaccommon.h"
#include "tables.h"
#include "cBitStream.h"
#include "huffman.h"
//}}}
//{{{  defines
#define FBITS_GLIM_BOOST  24
#define FBITS_QLIM_BOOST  14
#define GBOOST_MAX        0x2830afd3  /* Q28, 1.584893192 squared */
#define ACC_SCALE         6

#define FBITS_LPCOEFS     29  /* Q29 for range of (-4, 4) */
#define MAG_16            (16 * (1 << (32 - (2*(32-FBITS_LPCOEFS)))))   /* i.e. 16 in Q26 format */
#define RELAX_COEF        0x7ffff79c  /* 1.0 / (1.0 + 1e-6), Q31 */

#define Q28_2             0x20000000  /* Q28: 2.0 */
#define Q28_15            0x30000000  /* Q28: 1.5 */
#define LOG2_EXP_INV      0x58b90bfc  /* 1/log2(e), Q31 */
#define NUM_ITER_IRN      5
//}}}

#ifdef _WIN32
  //{{{
  void CVKernel1 (int* XBuf, int* accBuf) {

    U64 p01re, p01im, p12re, p12im, p11re, p22re;
    int n, x0re, x0im, x1re, x1im;

    x0re = XBuf[0];
    x0im = XBuf[1];
    XBuf += (2*64);
    x1re = XBuf[0];
    x1im = XBuf[1];
    XBuf += (2*64);

    p01re.w64 = p01im.w64 = 0;
    p12re.w64 = p12im.w64 = 0;
    p11re.w64 = 0;
    p22re.w64 = 0;

    p12re.w64 = MADD64(p12re.w64,  x1re, x0re);
    p12re.w64 = MADD64(p12re.w64,  x1im, x0im);
    p12im.w64 = MADD64(p12im.w64,  x0re, x1im);
    p12im.w64 = MADD64(p12im.w64, -x0im, x1re);
    p22re.w64 = MADD64(p22re.w64,  x0re, x0re);
    p22re.w64 = MADD64(p22re.w64,  x0im, x0im);
    for (n = (NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6); n != 0; n--) {
      /* 4 input, 3*2 acc, 1 ptr, 1 loop counter = 12 registers (use same for x0im, -x0im) */
      x0re = x1re;
      x0im = x1im;
      x1re = XBuf[0];
      x1im = XBuf[1];

      p01re.w64 = MADD64(p01re.w64,  x1re, x0re);
      p01re.w64 = MADD64(p01re.w64,  x1im, x0im);
      p01im.w64 = MADD64(p01im.w64,  x0re, x1im);
      p01im.w64 = MADD64(p01im.w64, -x0im, x1re);
      p11re.w64 = MADD64(p11re.w64,  x0re, x0re);
      p11re.w64 = MADD64(p11re.w64,  x0im, x0im);

      XBuf += (2*64);
      }

    /* these can be derived by slight changes to account for boundary conditions */
    p12re.w64 += p01re.w64;
    p12re.w64 = MADD64(p12re.w64, x1re, -x0re);
    p12re.w64 = MADD64(p12re.w64, x1im, -x0im);
    p12im.w64 += p01im.w64;
    p12im.w64 = MADD64(p12im.w64, x0re, -x1im);
    p12im.w64 = MADD64(p12im.w64, x0im,  x1re);
    p22re.w64 += p11re.w64;
    p22re.w64 = MADD64(p22re.w64, x0re, -x0re);
    p22re.w64 = MADD64(p22re.w64, x0im, -x0im);

    accBuf[0]  = p01re.r.lo32;  accBuf[1]  = p01re.r.hi32;
    accBuf[2]  = p01im.r.lo32;  accBuf[3]  = p01im.r.hi32;
    accBuf[4]  = p11re.r.lo32;  accBuf[5]  = p11re.r.hi32;
    accBuf[6]  = p12re.r.lo32;  accBuf[7]  = p12re.r.hi32;
    accBuf[8]  = p12im.r.lo32;  accBuf[9]  = p12im.r.hi32;
    accBuf[10] = p22re.r.lo32;  accBuf[11] = p22re.r.hi32;
    }
  //}}}
  //{{{
  void CVKernel2 (int* XBuf, int* accBuf) {

    int x0re = XBuf[0];
    int x0im = XBuf[1];
    XBuf += (2*64);
    int x1re = XBuf[0];
    int x1im = XBuf[1];
    XBuf += (2*64);

    U64 p02re;
    p02re.w64 = 0;
    U64 p02im;
    p02im.w64 = 0;
    for (int n = (NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6); n != 0; n--) {
      /* 6 input, 2*2 acc, 1 ptr, 1 loop counter = 12 registers (use same for x0im, -x0im) */
      int x2re = XBuf[0];
      int x2im = XBuf[1];

      p02re.w64 = MADD64(p02re.w64,  x2re, x0re);
      p02re.w64 = MADD64(p02re.w64,  x2im, x0im);
      p02im.w64 = MADD64(p02im.w64,  x0re, x2im);
      p02im.w64 = MADD64(p02im.w64, -x0im, x2re);

      x0re = x1re;
      x0im = x1im;
      x1re = x2re;
      x1im = x2im;
      XBuf += (2*64);
      }

    accBuf[0] = p02re.r.lo32;
    accBuf[1] = p02re.r.hi32;
    accBuf[2] = p02im.r.lo32;
    accBuf[3] = p02im.r.hi32;
    }
  //}}}
  //{{{
  void QMFAnalysisConv (int* cTab, int* delay, int dIdx, int* uBuf) {

    int k, dOff;
    int *cPtr0, *cPtr1;
    U64 u64lo, u64hi;

    dOff = dIdx*32 + 31;
    cPtr0 = cTab;
    cPtr1 = cTab + 33*5 - 1;

    /* special first pass since we need to flip sign to create cTab[384], cTab[512] */
    u64lo.w64 = 0;
    u64hi.w64 = 0;
    u64lo.w64 = MADD64(u64lo.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr1--,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64, -(*cPtr1--), delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr1--,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64, -(*cPtr1--), delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr1--,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}

    uBuf[0]  = u64lo.r.hi32;
    uBuf[32] = u64hi.r.hi32;
    uBuf++;
    dOff--;

    /* max gain for any sample in uBuf, after scaling by cTab, ~= 0.99
     * so we can just sum the uBuf values with no overflow problems */
    for (k = 1; k <= 31; k++) {
      u64lo.w64 = 0;
      u64hi.w64 = 0;
      u64lo.w64 = MADD64(u64lo.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}

      uBuf[0]  = u64lo.r.hi32;
      uBuf[32] = u64hi.r.hi32;
      uBuf++;
      dOff--;
      }
    }
  //}}}
  //{{{
  void QMFSynthesisConv (int* cPtr, int* delay, int dIdx, short* outbuf, int nChans) {

    int k, dOff0, dOff1;
    U64 sum64;

    dOff0 = (dIdx)*128;
    dOff1 = dOff0 - 1;
    if (dOff1 < 0)
      dOff1 += 1280;

    /* scaling note: total gain of coefs (cPtr[0]-cPtr[9] for any k) is < 2.0, so 1 GB in delay values is adequate */
    for (k = 0; k <= 63; k++) {
      sum64.w64 = 0;
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}

      dOff0++;
      dOff1--;
      *outbuf = CLIPTOSHORT((sum64.r.hi32 + RND_VAL) >> FBITS_OUT_QMFS);
      outbuf += nChans;
      }
    }
  //}}}
#endif

#ifdef __linux__
  //{{{
  void CVKernel1 (int* XBuf, int* accBuf) {

    U64 p01re, p01im, p12re, p12im, p11re, p22re;
    int n, x0re, x0im, x1re, x1im;

    x0re = XBuf[0];
    x0im = XBuf[1];
    XBuf += (2*64);
    x1re = XBuf[0];
    x1im = XBuf[1];
    XBuf += (2*64);

    p01re.w64 = p01im.w64 = 0;
    p12re.w64 = p12im.w64 = 0;
    p11re.w64 = 0;
    p22re.w64 = 0;

    p12re.w64 = MADD64(p12re.w64,  x1re, x0re);
    p12re.w64 = MADD64(p12re.w64,  x1im, x0im);
    p12im.w64 = MADD64(p12im.w64,  x0re, x1im);
    p12im.w64 = MADD64(p12im.w64, -x0im, x1re);
    p22re.w64 = MADD64(p22re.w64,  x0re, x0re);
    p22re.w64 = MADD64(p22re.w64,  x0im, x0im);
    for (n = (NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6); n != 0; n--) {
      /* 4 input, 3*2 acc, 1 ptr, 1 loop counter = 12 registers (use same for x0im, -x0im) */
      x0re = x1re;
      x0im = x1im;
      x1re = XBuf[0];
      x1im = XBuf[1];

      p01re.w64 = MADD64(p01re.w64,  x1re, x0re);
      p01re.w64 = MADD64(p01re.w64,  x1im, x0im);
      p01im.w64 = MADD64(p01im.w64,  x0re, x1im);
      p01im.w64 = MADD64(p01im.w64, -x0im, x1re);
      p11re.w64 = MADD64(p11re.w64,  x0re, x0re);
      p11re.w64 = MADD64(p11re.w64,  x0im, x0im);

      XBuf += (2*64);
      }

    /* these can be derived by slight changes to account for boundary conditions */
    p12re.w64 += p01re.w64;
    p12re.w64 = MADD64(p12re.w64, x1re, -x0re);
    p12re.w64 = MADD64(p12re.w64, x1im, -x0im);
    p12im.w64 += p01im.w64;
    p12im.w64 = MADD64(p12im.w64, x0re, -x1im);
    p12im.w64 = MADD64(p12im.w64, x0im,  x1re);
    p22re.w64 += p11re.w64;
    p22re.w64 = MADD64(p22re.w64, x0re, -x0re);
    p22re.w64 = MADD64(p22re.w64, x0im, -x0im);

    accBuf[0]  = p01re.r.lo32;  accBuf[1]  = p01re.r.hi32;
    accBuf[2]  = p01im.r.lo32;  accBuf[3]  = p01im.r.hi32;
    accBuf[4]  = p11re.r.lo32;  accBuf[5]  = p11re.r.hi32;
    accBuf[6]  = p12re.r.lo32;  accBuf[7]  = p12re.r.hi32;
    accBuf[8]  = p12im.r.lo32;  accBuf[9]  = p12im.r.hi32;
    accBuf[10] = p22re.r.lo32;  accBuf[11] = p22re.r.hi32;
    }
  //}}}
  //{{{
  void CVKernel2 (int* XBuf, int* accBuf) {

    int x0re = XBuf[0];
    int x0im = XBuf[1];
    XBuf += (2*64);
    int x1re = XBuf[0];
    int x1im = XBuf[1];
    XBuf += (2*64);

    U64 p02re;
    p02re.w64 = 0;
    U64 p02im;
    p02im.w64 = 0;
    for (int n = (NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6); n != 0; n--) {
      /* 6 input, 2*2 acc, 1 ptr, 1 loop counter = 12 registers (use same for x0im, -x0im) */
      int x2re = XBuf[0];
      int x2im = XBuf[1];

      p02re.w64 = MADD64(p02re.w64,  x2re, x0re);
      p02re.w64 = MADD64(p02re.w64,  x2im, x0im);
      p02im.w64 = MADD64(p02im.w64,  x0re, x2im);
      p02im.w64 = MADD64(p02im.w64, -x0im, x2re);

      x0re = x1re;
      x0im = x1im;
      x1re = x2re;
      x1im = x2im;
      XBuf += (2*64);
      }

    accBuf[0] = p02re.r.lo32;
    accBuf[1] = p02re.r.hi32;
    accBuf[2] = p02im.r.lo32;
    accBuf[3] = p02im.r.hi32;
    }
  //}}}
  //{{{
  void QMFAnalysisConv (int* cTab, int* delay, int dIdx, int* uBuf) {

    int k, dOff;
    int *cPtr0, *cPtr1;
    U64 u64lo, u64hi;

    dOff = dIdx*32 + 31;
    cPtr0 = cTab;
    cPtr1 = cTab + 33*5 - 1;

    /* special first pass since we need to flip sign to create cTab[384], cTab[512] */
    u64lo.w64 = 0;
    u64hi.w64 = 0;
    u64lo.w64 = MADD64(u64lo.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64,  *cPtr0++,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr1--,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64, -(*cPtr1--), delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr1--,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64lo.w64 = MADD64(u64lo.w64, -(*cPtr1--), delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}
    u64hi.w64 = MADD64(u64hi.w64,  *cPtr1--,   delay[dOff]);  dOff -= 32; if (dOff < 0) {dOff += 320;}

    uBuf[0]  = u64lo.r.hi32;
    uBuf[32] = u64hi.r.hi32;
    uBuf++;
    dOff--;

    /* max gain for any sample in uBuf, after scaling by cTab, ~= 0.99
     * so we can just sum the uBuf values with no overflow problems */
    for (k = 1; k <= 31; k++) {
      u64lo.w64 = 0;
      u64hi.w64 = 0;
      u64lo.w64 = MADD64(u64lo.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr0++, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64lo.w64 = MADD64(u64lo.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}
      u64hi.w64 = MADD64(u64hi.w64, *cPtr1--, delay[dOff]); dOff -= 32; if (dOff < 0) {dOff += 320;}

      uBuf[0]  = u64lo.r.hi32;
      uBuf[32] = u64hi.r.hi32;
      uBuf++;
      dOff--;
      }
    }
  //}}}
  //{{{
  void QMFSynthesisConv (int* cPtr, int* delay, int dIdx, short* outbuf, int nChans) {

    int k, dOff0, dOff1;
    U64 sum64;

    dOff0 = (dIdx)*128;
    dOff1 = dOff0 - 1;
    if (dOff1 < 0)
      dOff1 += 1280;

    /* scaling note: total gain of coefs (cPtr[0]-cPtr[9] for any k) is < 2.0, so 1 GB in delay values is adequate */
    for (k = 0; k <= 63; k++) {
      sum64.w64 = 0;
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff0]); dOff0 -= 256; if (dOff0 < 0) {dOff0 += 1280;}
      sum64.w64 = MADD64(sum64.w64, *cPtr++, delay[dOff1]); dOff1 -= 256; if (dOff1 < 0) {dOff1 += 1280;}

      dOff0++;
      dOff1--;
      *outbuf = CLIPTOSHORT((sum64.r.hi32 + RND_VAL) >> FBITS_OUT_QMFS);
      outbuf += nChans;
      }
    }
  //}}}
#endif

//{{{  helpers
//{{{
void BubbleSort (unsigned char* v, int nItems) {

  int i;
  unsigned char t;
  while (nItems >= 2) {
    for (i = 0; i < nItems-1; i++) {
      if (v[i+1] < v[i]) {
        t = v[i+1];
        v[i+1] = v[i];
        v[i] = t;
        }
      }
    nItems--;
    }
  }
//}}}
//{{{
unsigned char VMin (unsigned char* v, int nItems) {

  unsigned char vMin = v[0];
  for (int i = 1; i < nItems; i++)
    if (v[i] < vMin)
      vMin = v[i];
  return vMin;
  }
//}}}
//{{{
unsigned char VMax (unsigned char* v, int nItems) {

  unsigned char vMax = v[0];
  for (int i = 1; i < nItems; i++)
    if (v[i] > vMax)
      vMax = v[i];
  return vMax;
  }
//}}}

//{{{
int CalcFreqMasterScaleZero (unsigned char *freqMaster, int alterScale, int k0, int k2) {

  int nMaster, k, nBands, k2Achieved, dk, vDk[64], k2Diff;
  if (alterScale) {
    dk = 2;
    nBands = 2 * ((k2 - k0 + 2) >> 2);
    }
  else {
    dk = 1;
    nBands = 2 * ((k2 - k0) >> 1);
    }

  if (nBands <= 0)
    return 0;

  k2Achieved = k0 + nBands * dk;
  k2Diff = k2 - k2Achieved;
  for (k = 0; k < nBands; k++)
    vDk[k] = dk;

  if (k2Diff > 0) {
    k = nBands - 1;
    while (k2Diff) {
      vDk[k]++;
      k--;
      k2Diff--;
      }
    }
  else if (k2Diff < 0) {
    k = 0;
    while (k2Diff) {
      vDk[k]--;
      k++;
      k2Diff++;
      }
    }

  nMaster = nBands;
  freqMaster[0] = k0;
  for (k = 1; k <= nBands; k++)
    freqMaster[k] = freqMaster[k-1] + vDk[k-1];

  return nMaster;
  }
//}}}
//{{{
int CalcFreqMaster (unsigned char *freqMaster, int freqScale, int alterScale, int k0, int k2) {

  int bands, twoRegions, k, k1, t, vLast, vCurr, pCurr;
  int invWarp, nBands0, nBands1, change;
  unsigned char vDk1Min, vDk0Max;
  unsigned char *vDelta;

  if (freqScale < 1 || freqScale > 3)
    return -1;

  bands = mBandTab[freqScale - 1];
  invWarp = invWarpTab[alterScale];

  /* tested for all k0 = [5, 64], k2 = [k0, 64] */
  if (k2*10000 > 22449*k0) {
    twoRegions = 1;
    k1 = 2*k0;
    }
  else {
    twoRegions = 0;
    k1 = k2;
    }

  /* tested for all k0 = [5, 64], k1 = [k0, 64], freqScale = [1,3] */
  t = (log2Tab[k1] - log2Tab[k0]) >> 3;       /* log2(k1/k0), Q28 to Q25 */
  nBands0 = 2 * (((bands * t) + (1 << 24)) >> 25);  /* multiply by bands/2, round to nearest int (mBandTab has factor of 1/2 rolled in) */

  /* tested for all valid combinations of k0, k1, nBands (from sampRate, freqScale, alterScale)
   * roundoff error can be a problem with fixpt (e.g. pCurr = 12.499999 instead of 12.50003)
   *   because successive multiplication always undershoots a little bit, but this
   *   doesn't occur in any of the ratios we encounter from the valid k0/k1 bands in the spec */
  t = RatioPowInv(k1, k0, nBands0);
  pCurr = k0 << 24;
  vLast = k0;
  vDelta = freqMaster + 1;  /* operate in-place */
  for (k = 0; k < nBands0; k++) {
    pCurr = MULSHIFT32(pCurr, t) << 8;  /* keep in Q24 */
    vCurr = (pCurr + (1 << 23)) >> 24;
    vDelta[k] = (vCurr - vLast);
    vLast = vCurr;
    }

  /* sort the deltas and find max delta for first region */
  BubbleSort(vDelta, nBands0);
  vDk0Max = VMax(vDelta, nBands0);

  /* fill master frequency table with bands from first region */
  freqMaster[0] = k0;
  for (k = 1; k <= nBands0; k++)
    freqMaster[k] += freqMaster[k-1];

  /* if only one region, then the table is complete */
  if (!twoRegions)
    return nBands0;

  /* tested for all k1 = [10, 64], k2 = [k0, 64], freqScale = [1,3] */
  t = (log2Tab[k2] - log2Tab[k1]) >> 3;   /* log2(k1/k0), Q28 to Q25 */
  t = MULSHIFT32(bands * t, invWarp) << 2;  /* multiply by bands/2, divide by warp factor, keep Q25 */
  nBands1 = 2 * ((t + (1 << 24)) >> 25);    /* round to nearest int */

  /* see comments above for calculations in first region */
  t = RatioPowInv(k2, k1, nBands1);
  pCurr = k1 << 24;
  vLast = k1;
  vDelta = freqMaster + nBands0 + 1;  /* operate in-place */
  for (k = 0; k < nBands1; k++) {
    pCurr = MULSHIFT32(pCurr, t) << 8;  /* keep in Q24 */
    vCurr = (pCurr + (1 << 23)) >> 24;
    vDelta[k] = (vCurr - vLast);
    vLast = vCurr;
    }

  /* sort the deltas, adjusting first and last if the second region has smaller deltas than the first */
  vDk1Min = VMin(vDelta, nBands1);
  if (vDk1Min < vDk0Max) {
    BubbleSort(vDelta, nBands1);
    change = vDk0Max - vDelta[0];
    if (change > ((vDelta[nBands1 - 1] - vDelta[0]) >> 1))
       change = ((vDelta[nBands1 - 1] - vDelta[0]) >> 1);
    vDelta[0] += change;
    vDelta[nBands1-1] -= change;
    }
  BubbleSort(vDelta, nBands1);

  /* fill master frequency table with bands from second region * Note: freqMaster[nBands0] = k1 */
  for (k = 1; k <= nBands1; k++)
    freqMaster[k + nBands0] += freqMaster[k + nBands0 - 1];

  return (nBands0 + nBands1);
  }
//}}}
//{{{
int CalcFreqHigh (unsigned char *freqHigh, unsigned char *freqMaster, int nMaster, int crossOverBand) {

  int nHigh = nMaster - crossOverBand;
  for (int k = 0; k <= nHigh; k++)
    freqHigh[k] = freqMaster[k + crossOverBand];
  return nHigh;
  }
//}}}
//{{{
int CalcFreqLow (unsigned char *freqLow, unsigned char *freqHigh, int nHigh) {

  int nLow = nHigh - (nHigh >> 1);
  freqLow[0] = freqHigh[0];
  int oddFlag = nHigh & 0x01;

  for (int k = 1; k <= nLow; k++)
    freqLow[k] = freqHigh[2*k - oddFlag];

  return nLow;
  }
//}}}
//{{{
int CalcFreqNoise (unsigned char *freqNoise, unsigned char *freqLow, int nLow, int kStart, int k2, int noiseBands) {

  int lTop = log2Tab[k2];
  int lBottom = log2Tab[kStart];
  int nQ = noiseBands*((lTop - lBottom) >> 2);  /* Q28 to Q26, noiseBands = [0,3] */
  nQ = (nQ + (1 << 25)) >> 26;
  if (nQ < 1)
    nQ = 1;

  int iLast = 0;
  freqNoise[0] = freqLow[0];
  for (int k = 1; k <= nQ; k++) {
    int i = iLast + (nLow - iLast) / (nQ + 1 - k);  /* truncating division */
    freqNoise[k] = freqLow[i];
    iLast = i;
    }

  return nQ;
  }
//}}}
//{{{
int BuildPatches (unsigned char *patchNumSubbands, unsigned char *patchStartSubband, unsigned char *freqMaster,
                         int nMaster, int k0, int kStart, int numQMFBands, int sampRateIdx) {

  int i, j, k;
  int msb, sb, usb, numPatches, goalSB, oddFlag;

  msb = k0;
  usb = kStart;
  numPatches = 0;
  goalSB = goalSBTab[sampRateIdx];

  if (nMaster == 0) {
    patchNumSubbands[0] = 0;
    patchStartSubband[0] = 0;
    return 0;
    }

  if (goalSB < kStart + numQMFBands) {
    k = 0;
    for (i = 0; freqMaster[i] < goalSB; i++)
      k = i+1;
  } else {
    k = nMaster;
  }

  do {
    j = k+1;
    do {
      j--;
      sb = freqMaster[j];
      oddFlag = (sb - 2 + k0) & 0x01;
      } while (sb > k0 - 1 + msb - oddFlag);

    patchNumSubbands[numPatches] = MAX(sb - usb, 0);
    patchStartSubband[numPatches] = k0 - oddFlag - patchNumSubbands[numPatches];

    /* from MPEG reference code - slightly different from spec */
    if ((patchNumSubbands[numPatches] < 3) && (numPatches > 0))
      break;

    if (patchNumSubbands[numPatches] > 0) {
      usb = sb;
      msb = sb;
      numPatches++;
      }
    else
      msb = kStart;

    if (freqMaster[k] - sb < 3)
      k = nMaster;
    } while (sb != (kStart + numQMFBands) && numPatches <= MAX_NUM_PATCHES);

  return numPatches;
  }
//}}}
//{{{
int FindFreq (unsigned char *freq, int nFreq, unsigned char val) {

  for (int k = 0; k < nFreq; k++) {
    if (freq[k] == val)
      return 1;
    }

  return 0;
  }
//}}}
//{{{
void RemoveFreq (unsigned char *freq, int nFreq, int removeIdx) {

  int k;
  if (removeIdx >= nFreq)
    return;

  for (k = removeIdx; k < nFreq - 1; k++)
    freq[k] = freq[k+1];
  }
//}}}
//{{{
int CalcFreqLimiter (unsigned char *freqLimiter, unsigned char *patchNumSubbands, unsigned char *freqLow,
                            int nLow, int kStart, int limiterBands, int numPatches) {

  int k, bands, nLimiter, nOctaves;
  int limBandsPerOctave[3] = {120, 200, 300};   /* [1.2, 2.0, 3.0] * 100 */
  unsigned char patchBorders[MAX_NUM_PATCHES + 1];

  /* simple case */
  if (limiterBands == 0) {
    freqLimiter[0] = freqLow[0] - kStart;
    freqLimiter[1] = freqLow[nLow] - kStart;
    return 1;
    }

  bands = limBandsPerOctave[limiterBands - 1];
  patchBorders[0] = kStart;

  /* from MPEG reference code - slightly different from spec (top border) */
  for (k = 1; k < numPatches; k++)
    patchBorders[k] = patchBorders[k-1] + patchNumSubbands[k-1];
  patchBorders[k] = freqLow[nLow];

  for (k = 0; k <= nLow; k++)
    freqLimiter[k] = freqLow[k];

  for (k = 1; k < numPatches; k++)
    freqLimiter[k+nLow] = patchBorders[k];

  k = 1;
  nLimiter = nLow + numPatches - 1;
  BubbleSort(freqLimiter, nLimiter + 1);

  while (k <= nLimiter) {
    nOctaves = log2Tab[freqLimiter[k]] - log2Tab[freqLimiter[k-1]]; /* Q28 */
    nOctaves = (nOctaves >> 9) * bands; /* Q19, max bands = 300 < 2^9 */
    if (nOctaves < (49 << 19)) {    /* compare with 0.49*100, in Q19 */
      if (freqLimiter[k] == freqLimiter[k-1] || FindFreq(patchBorders, numPatches + 1, freqLimiter[k]) == 0) {
        RemoveFreq(freqLimiter, nLimiter + 1, k);
        nLimiter--;
      } else if (FindFreq(patchBorders, numPatches + 1, freqLimiter[k-1]) == 0) {
        RemoveFreq(freqLimiter, nLimiter + 1, k-1);
        nLimiter--;
      } else {
        k++;
      }
    } else {
      k++;
    }
  }

  /* store limiter boundaries as offsets from kStart */
  for (k = 0; k <= nLimiter; k++)
    freqLimiter[k] -= kStart;

  return nLimiter;
  }
//}}}

//{{{
void CalcNoiseDivFactors (int q, int *qp1Inv, int *qqp1Inv) {

  int z, qp1, t, s;

  /* 1 + Q_orig */
  qp1  = (q >> 1);
  qp1 += (1 << (FBITS_OUT_DQ_NOISE - 1));   /* >> 1 to avoid overflow when adding 1.0 */
  z = CLZ(qp1) - 1;             /* z <= 31 - FBITS_OUT_DQ_NOISE */
  qp1 <<= z;                  /* Q(FBITS_OUT_DQ_NOISE + z) = Q31 * 2^-(31 - (FBITS_OUT_DQ_NOISE + z)) */
  t = InvRNormalized(qp1) << 1;       /* Q30 * 2^(31 - (FBITS_OUT_DQ_NOISE + z)), guaranteed not to overflow */

  /* normalize to Q31 */
  s = (31 - (FBITS_OUT_DQ_NOISE - 1) - z - 1);  /* clearly z >= 0, z <= (30 - (FBITS_OUT_DQ_NOISE - 1)) */
  *qp1Inv = (t >> s);               /* s = [0, 31 - FBITS_OUT_DQ_NOISE] */
  *qqp1Inv = MULSHIFT32(t, q) << (32 - FBITS_OUT_DQ_NOISE - s);
  }
//}}}
//{{{
int GetSMapped (SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan, int env, int band, int la) {

  int bandStart, bandEnd, oddFlag, r;

  if (sbrGrid->freqRes[env]) {
    /* high resolution */
    bandStart = band;
    bandEnd = band+1;
    }
  else {
    /* low resolution (see CalcFreqLow() for mapping) */
    oddFlag = sbrFreq->nHigh & 0x01;
    bandStart = (band > 0 ? 2*band - oddFlag : 0);    /* starting index for freqLow[band] */
    bandEnd = 2*(band+1) - oddFlag;           /* ending index for freqLow[band+1] */
    }

  /* sMapped = 1 if sIndexMapped == 1 for any frequency in this band */
  for (band = bandStart; band < bandEnd; band++) {
    if (sbrChan->addHarmonic[1][band]) {
      r = ((sbrFreq->freqHigh[band+1] + sbrFreq->freqHigh[band]) >> 1);
      if (env >= la || sbrChan->addHarmonic[0][r] == 1)
        return 1;
      }
    }

  return 0;
  }
//}}}
//{{{
void CopyCouplingGrid (SBRGrid *sbrGridLeft, SBRGrid *sbrGridRight) {

  int env, noiseFloor;

  sbrGridRight->frameClass =     sbrGridLeft->frameClass;
  sbrGridRight->ampResFrame =    sbrGridLeft->ampResFrame;
  sbrGridRight->pointer =        sbrGridLeft->pointer;

  sbrGridRight->numEnv =         sbrGridLeft->numEnv;
  for (env = 0; env < sbrGridLeft->numEnv; env++) {
    sbrGridRight->envTimeBorder[env] = sbrGridLeft->envTimeBorder[env];
    sbrGridRight->freqRes[env] =       sbrGridLeft->freqRes[env];
  }
  sbrGridRight->envTimeBorder[env] = sbrGridLeft->envTimeBorder[env]; /* borders are [0, numEnv] inclusive */

  sbrGridRight->numNoiseFloors = sbrGridLeft->numNoiseFloors;
  for (noiseFloor = 0; noiseFloor <= sbrGridLeft->numNoiseFloors; noiseFloor++)
    sbrGridRight->noiseTimeBorder[noiseFloor] = sbrGridLeft->noiseTimeBorder[noiseFloor];

  /* numEnvPrev, numNoiseFloorsPrev, freqResPrev are updated in DecodeSBREnvelope() and DecodeSBRNoise() */
  }
//}}}
//{{{
void CopyCouplingInverseFilterMode (int numNoiseFloorBands, unsigned char *modeLeft, unsigned char *modeRight) {

  int band;
  for (band = 0; band < numNoiseFloorBands; band++)
    modeRight[band] = modeLeft[band];
  }
//}}}

//{{{
void EstimateEnvelope (PSInfoSBR *psi, SBRHeader *sbrHeader, SBRGrid *sbrGrid, SBRFreq *sbrFreq, int env)
{
  int i, m, iStart, iEnd, xre, xim, nScale, expMax;
  int p, n, mStart, mEnd, invFact, t;
  int *XBuf;
  U64 eCurr;
  unsigned char *freqBandTab;

  /* estimate current envelope */
  iStart = sbrGrid->envTimeBorder[env] + HF_ADJ;
  iEnd =   sbrGrid->envTimeBorder[env+1] + HF_ADJ;
  if (sbrGrid->freqRes[env]) {
    n = sbrFreq->nHigh;
    freqBandTab = sbrFreq->freqHigh;
  } else {
    n = sbrFreq->nLow;
    freqBandTab = sbrFreq->freqLow;
  }

  /* ADS should inline MADD64 (smlal) properly, but check to make sure */
  expMax = 0;
  if (sbrHeader->interpFreq) {
    for (m = 0; m < sbrFreq->numQMFBands; m++) {
      eCurr.w64 = 0;
      XBuf = psi->XBuf[iStart][sbrFreq->kStart + m];
      for (i = iStart; i < iEnd; i++) {
        /* scale to int before calculating power (precision not critical, and avoids overflow) */
        xre = (*XBuf) >> FBITS_OUT_QMFA;  XBuf += 1;
        xim = (*XBuf) >> FBITS_OUT_QMFA;  XBuf += (2*64 - 1);
        eCurr.w64 = MADD64(eCurr.w64, xre, xre);
        eCurr.w64 = MADD64(eCurr.w64, xim, xim);
      }

      /* eCurr.w64 is now Q(64 - 2*FBITS_OUT_QMFA) (64-bit word)
       * if energy is too big to fit in 32-bit word (> 2^31) scale down by power of 2
       */
      nScale = 0;
      if (eCurr.r.hi32) {
        nScale = (32 - CLZ(eCurr.r.hi32)) + 1;
        t  = (int)(eCurr.r.lo32 >> nScale);   /* logical (unsigned) >> */
        t |= eCurr.r.hi32 << (32 - nScale);
      } else if (eCurr.r.lo32 >> 31) {
        nScale = 1;
        t  = (int)(eCurr.r.lo32 >> nScale);   /* logical (unsigned) >> */
      } else {
        t  = (int)eCurr.r.lo32;
      }

      invFact = invBandTab[(iEnd - iStart)-1];
      psi->eCurr[m] = MULSHIFT32(t, invFact);
      psi->eCurrExp[m] = nScale + 1;  /* +1 for invFact = Q31 */
      if (psi->eCurrExp[m] > expMax)
        expMax = psi->eCurrExp[m];
    }
  } else {
    for (p = 0; p < n; p++) {
      mStart = freqBandTab[p];
      mEnd =   freqBandTab[p+1];
      eCurr.w64 = 0;
      for (i = iStart; i < iEnd; i++) {
        XBuf = psi->XBuf[i][mStart];
        for (m = mStart; m < mEnd; m++) {
          xre = (*XBuf++) >> FBITS_OUT_QMFA;
          xim = (*XBuf++) >> FBITS_OUT_QMFA;
          eCurr.w64 = MADD64(eCurr.w64, xre, xre);
          eCurr.w64 = MADD64(eCurr.w64, xim, xim);
        }
      }

      nScale = 0;
      if (eCurr.r.hi32) {
        nScale = (32 - CLZ(eCurr.r.hi32)) + 1;
        t  = (int)(eCurr.r.lo32 >> nScale);   /* logical (unsigned) >> */
        t |= eCurr.r.hi32 << (32 - nScale);
      } else if (eCurr.r.lo32 >> 31) {
        nScale = 1;
        t  = (int)(eCurr.r.lo32 >> nScale);   /* logical (unsigned) >> */
      } else {
        t  = (int)eCurr.r.lo32;
      }

      invFact = invBandTab[(iEnd - iStart)-1];
      invFact = MULSHIFT32(invBandTab[(mEnd - mStart)-1], invFact) << 1;
      t = MULSHIFT32(t, invFact);

      for (m = mStart; m < mEnd; m++) {
        psi->eCurr[m - sbrFreq->kStart] = t;
        psi->eCurrExp[m - sbrFreq->kStart] = nScale + 1;  /* +1 for invFact = Q31 */
      }
      if (psi->eCurrExp[mStart - sbrFreq->kStart] > expMax)
        expMax = psi->eCurrExp[mStart - sbrFreq->kStart];
    }
  }
  psi->eCurrExpMax = expMax;
}
//}}}
//{{{
void CalcMaxGain (PSInfoSBR *psi, SBRHeader *sbrHeader, SBRGrid *sbrGrid, SBRFreq *sbrFreq,
                         int ch, int env, int lim, int fbitsDQ) {

  int m, mStart, mEnd, q, z, r;
  int sumEOrigMapped, sumECurr, gainMax, eOMGainMax, envBand;
  unsigned char eCurrExpMax;
  unsigned char *freqBandTab;

  mStart = sbrFreq->freqLimiter[lim];   /* these are offsets from kStart */
  mEnd =   sbrFreq->freqLimiter[lim + 1];
  freqBandTab = (sbrGrid->freqRes[env] ? sbrFreq->freqHigh : sbrFreq->freqLow);

  /* calculate max gain to apply to signal in this limiter band */
  sumECurr = 0;
  sumEOrigMapped = 0;
  eCurrExpMax = psi->eCurrExpMax;
  eOMGainMax = psi->eOMGainMax;
  envBand = psi->envBand;
  for (m = mStart; m < mEnd; m++) {
    /* map current QMF band to appropriate envelope band */
    if (m == freqBandTab[envBand + 1] - sbrFreq->kStart) {
      envBand++;
      eOMGainMax = psi->envDataDequant[ch][env][envBand] >> ACC_SCALE;  /* summing max 48 bands */
      }
    sumEOrigMapped += eOMGainMax;

    /* easy test for overflow on ARM */
    sumECurr += (psi->eCurr[m] >> (eCurrExpMax - psi->eCurrExp[m]));
    if (sumECurr >> 30) {
      sumECurr >>= 1;
      eCurrExpMax++;
      }
    }
  psi->eOMGainMax = eOMGainMax;
  psi->envBand = envBand;

  psi->gainMaxFBits = 30; /* Q30 tables */
  if (sumECurr == 0) {
    /* any non-zero numerator * 1/EPS_0 is > G_MAX */
    gainMax = (sumEOrigMapped == 0 ? limGainTab[sbrHeader->limiterGains] : 0x80000000);
    }
  else if (sumEOrigMapped == 0) {
    /* 1/(any non-zero denominator) * EPS_0 * limGainTab[x] is appx. 0 */
    gainMax = 0;
    }
  else {
    /* sumEOrigMapped = Q(fbitsDQ - ACC_SCALE), sumECurr = Q(-eCurrExpMax) */
    gainMax = limGainTab[sbrHeader->limiterGains];
    if (sbrHeader->limiterGains != 3) {
      q = MULSHIFT32(sumEOrigMapped, gainMax);  /* Q(fbitsDQ - ACC_SCALE - 2), gainMax = Q30  */
      z = CLZ(sumECurr) - 1;
      r = InvRNormalized(sumECurr << z);  /* in =  Q(z - eCurrExpMax), out = Q(29 + 31 - z + eCurrExpMax) */
      gainMax = MULSHIFT32(q, r);     /* Q(29 + 31 - z + eCurrExpMax + fbitsDQ - ACC_SCALE - 2 - 32) */
      psi->gainMaxFBits = 26 - z + eCurrExpMax + fbitsDQ - ACC_SCALE;
      }
    }

  psi->sumEOrigMapped = sumEOrigMapped;
  psi->gainMax = gainMax;
  }
//}}}
//{{{
void CalcComponentGains (PSInfoSBR *psi, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan,
                                int ch, int env, int lim, int fbitsDQ) {

  int d, m, mStart, mEnd, q, qm, noiseFloor, sIndexMapped;
  int shift, eCurr, maxFlag, gainMax, gainMaxFBits;
  int gain, sm, z, r, fbitsGain, gainScale;
  unsigned char *freqBandTab;

  mStart = sbrFreq->freqLimiter[lim];   /* these are offsets from kStart */
  mEnd =   sbrFreq->freqLimiter[lim + 1];

  gainMax = psi->gainMax;
  gainMaxFBits = psi->gainMaxFBits;

  d = (env == psi->la || env == sbrChan->laPrev ? 0 : 1);
  freqBandTab = (sbrGrid->freqRes[env] ? sbrFreq->freqHigh : sbrFreq->freqLow);

  /* figure out which noise floor this envelope is in (only 1 or 2 noise floors allowed) */
  noiseFloor = 0;
  if (sbrGrid->numNoiseFloors == 2 && sbrGrid->noiseTimeBorder[1] <= sbrGrid->envTimeBorder[env])
    noiseFloor++;

  psi->sumECurrGLim = 0;
  psi->sumSM = 0;
  psi->sumQM = 0;
  /* calculate energy of noise to add in this limiter band */
  for (m = mStart; m < mEnd; m++) {
    if (m == sbrFreq->freqNoise[psi->noiseFloorBand + 1] - sbrFreq->kStart) {
      /* map current QMF band to appropriate noise floor band (NOTE: freqLimiter[0] == freqLow[0] = freqHigh[0]) */
      psi->noiseFloorBand++;
      CalcNoiseDivFactors(psi->noiseDataDequant[ch][noiseFloor][psi->noiseFloorBand], &(psi->qp1Inv), &(psi->qqp1Inv));
      }
    if (m == sbrFreq->freqHigh[psi->highBand + 1] - sbrFreq->kStart)
      psi->highBand++;
    if (m == freqBandTab[psi->sBand + 1] - sbrFreq->kStart) {
      psi->sBand++;
      psi->sMapped = GetSMapped(sbrGrid, sbrFreq, sbrChan, env, psi->sBand, psi->la);
      }

    /* get sIndexMapped for this QMF subband */
    sIndexMapped = 0;
    r = ((sbrFreq->freqHigh[psi->highBand+1] + sbrFreq->freqHigh[psi->highBand]) >> 1);
    if (m + sbrFreq->kStart == r) {
      /* r = center frequency, deltaStep = (env >= la || sIndexMapped'(r, numEnv'-1) == 1) */
      if (env >= psi->la || sbrChan->addHarmonic[0][r] == 1)
        sIndexMapped = sbrChan->addHarmonic[1][psi->highBand];
      }

    /* save sine flags from last envelope in this frame:
     *   addHarmonic[0][0...63] = saved sine present flag from previous frame, for each QMF subband
     *   addHarmonic[1][0...nHigh-1] = addHarmonic bit from current frame, for each high-res frequency band
     * from MPEG reference code - slightly different from spec
     *   (sIndexMapped'(m,LE'-1) can still be 0 when numEnv == psi->la)
     */
    if (env == sbrGrid->numEnv - 1) {
      if (m + sbrFreq->kStart == r)
        sbrChan->addHarmonic[0][m + sbrFreq->kStart] = sbrChan->addHarmonic[1][psi->highBand];
      else
        sbrChan->addHarmonic[0][m + sbrFreq->kStart] = 0;
      }

    gain = psi->envDataDequant[ch][env][psi->sBand];
    qm = MULSHIFT32(gain, psi->qqp1Inv) << 1;
    sm = (sIndexMapped ? MULSHIFT32(gain, psi->qp1Inv) << 1 : 0);

    /* three cases: (sMapped == 0 && delta == 1), (sMapped == 0 && delta == 0), (sMapped == 1) */
    if (d == 1 && psi->sMapped == 0)
      gain = MULSHIFT32(psi->qp1Inv, gain) << 1;
    else if (psi->sMapped != 0)
      gain = MULSHIFT32(psi->qqp1Inv, gain) << 1;

    /* gain, qm, sm = Q(fbitsDQ), gainMax = Q(fbitsGainMax) */
    eCurr = psi->eCurr[m];
    if (eCurr) {
      z = CLZ(eCurr) - 1;
      r = InvRNormalized(eCurr << z);   /* in = Q(z - eCurrExp), out = Q(29 + 31 - z + eCurrExp) */
      gainScale = MULSHIFT32(gain, r);  /* out = Q(29 + 31 - z + eCurrExp + fbitsDQ - 32) */
      fbitsGain = 29 + 31 - z + psi->eCurrExp[m] + fbitsDQ - 32;
      }
    else {
      /* if eCurr == 0, then gain is unchanged (divide by EPS = 1) */
      gainScale = gain;
      fbitsGain = fbitsDQ;
      }

    /* see if gain for this band exceeds max gain */
    maxFlag = 0;
    if (gainMax != (int)0x80000000) {
      if (fbitsGain >= gainMaxFBits) {
        shift = MIN(fbitsGain - gainMaxFBits, 31);
        maxFlag = ((gainScale >> shift) > gainMax ? 1 : 0);
        }
     else {
        shift = MIN(gainMaxFBits - fbitsGain, 31);
        maxFlag = (gainScale > (gainMax >> shift) ? 1 : 0);
        }
      }

    if (maxFlag) {
      /* gainScale > gainMax, calculate ratio with 32/16 division */
      q = 0;
      r = gainScale;  /* guaranteed > 0, else maxFlag could not have been set */
      z = CLZ(r);
      if (z < 16) {
        q = 16 - z;
        r >>= q;  /* out = Q(fbitsGain - q) */
        }

      z = CLZ(gainMax) - 1;
      r = (gainMax << z) / r;   /* out = Q((fbitsGainMax + z) - (fbitsGain - q)) */
      q = (gainMaxFBits + z) - (fbitsGain - q); /* r = Q(q) */
      if (q > 30) {
        r >>= MIN(q - 30, 31);
        }
      else {
        z = MIN(30 - q, 30);
        CLIP_2N_SHIFT(r, z);  /* let r = Q30 since range = [0.0, 1.0) (clip to 0x3fffffff = 0.99999) */
        }

      qm = MULSHIFT32(qm, r) << 2;
      gain = MULSHIFT32(gain, r) << 2;
      psi->gLimBuf[m] = gainMax;
      psi->gLimFbits[m] = gainMaxFBits;
      }
    else {
      psi->gLimBuf[m] = gainScale;
      psi->gLimFbits[m] = fbitsGain;
      }

    /* sumSM, sumQM, sumECurrGLim = Q(fbitsDQ - ACC_SCALE) */
    psi->smBuf[m] = sm;
    psi->sumSM += (sm >> ACC_SCALE);
    psi->qmLimBuf[m] = qm;
    if (env != psi->la && env != sbrChan->laPrev && sm == 0)
      psi->sumQM += (qm >> ACC_SCALE);

    /* eCurr * gain^2 same as gain^2, before division by eCurr
     * (but note that gain != 0 even if eCurr == 0, since it's divided by eps) */
    if (eCurr)
      psi->sumECurrGLim += (gain >> ACC_SCALE);
    }
  }
//}}}
//{{{
void ApplyBoost (PSInfoSBR *psi, SBRFreq *sbrFreq, int lim, int fbitsDQ) {

  int m, mStart, mEnd, q, z, r;
  int sumEOrigMapped, gBoost;

  mStart = sbrFreq->freqLimiter[lim];   /* these are offsets from kStart */
  mEnd =   sbrFreq->freqLimiter[lim + 1];

  sumEOrigMapped = psi->sumEOrigMapped >> 1;
  r = (psi->sumECurrGLim >> 1) + (psi->sumSM >> 1) + (psi->sumQM >> 1); /* 1 GB fine (sm and qm are mutually exclusive in acc) */
  if (r < (1 << (31-28))) {
    /* any non-zero numerator * 1/EPS_0 is > GBOOST_MAX
     * round very small r to zero to avoid scaling problems
     */
    gBoost = (sumEOrigMapped == 0 ? (1 << 28) : GBOOST_MAX);
    z = 0;
  } else if (sumEOrigMapped == 0) {
    /* 1/(any non-zero denominator) * EPS_0 is appx. 0 */
    gBoost = 0;
    z = 0;
  } else {
    /* numerator (sumEOrigMapped) and denominator (r) have same Q format (before << z) */
    z = CLZ(r) - 1; /* z = [0, 27] */
    r = InvRNormalized(r << z);
    gBoost = MULSHIFT32(sumEOrigMapped, r);
  }

  /* gBoost = Q(28 - z) */
  if (gBoost > (GBOOST_MAX >> z)) {
    gBoost = GBOOST_MAX;
    z = 0;
  }
  gBoost <<= z; /* gBoost = Q28, minimum 1 GB */

  /* convert gain, noise, sinusoids to fixed Q format, clipping if necessary
   *   (rare, usually only happens at very low bitrates, introduces slight
   *    distortion into final HF mapping, but should be inaudible)
   */
  for (m = mStart; m < mEnd; m++) {
    /* let gLimBoost = Q24, since in practice the max values are usually 16 to 20
     *   unless limiterGains == 3 (limiter off) and eCurr ~= 0 (i.e. huge gain, but only
     *   because the envelope has 0 power anyway)
     */
    q = MULSHIFT32(psi->gLimBuf[m], gBoost) << 2; /* Q(gLimFbits) * Q(28) --> Q(gLimFbits[m]-2) */
    r = SqrtFix(q, psi->gLimFbits[m] - 2, &z);
    z -= FBITS_GLIM_BOOST;
    if (z >= 0) {
      psi->gLimBoost[m] = r >> MIN(z, 31);
    } else {
      z = MIN(30, -z);
      CLIP_2N_SHIFT(r, z);
      psi->gLimBoost[m] = r;
    }

    q = MULSHIFT32(psi->qmLimBuf[m], gBoost) << 2;  /* Q(fbitsDQ) * Q(28) --> Q(fbitsDQ-2) */
    r = SqrtFix(q, fbitsDQ - 2, &z);
    z -= FBITS_QLIM_BOOST;    /* << by 14, since integer sqrt of x < 2^16, and we want to leave 1 GB */
    if (z >= 0) {
      psi->qmLimBoost[m] = r >> MIN(31, z);
    } else {
      z = MIN(30, -z);
      CLIP_2N_SHIFT(r, z);
      psi->qmLimBoost[m] = r;
    }

    q = MULSHIFT32(psi->smBuf[m], gBoost) << 2;   /* Q(fbitsDQ) * Q(28) --> Q(fbitsDQ-2) */
    r = SqrtFix(q, fbitsDQ - 2, &z);
    z -= FBITS_OUT_QMFA;    /* justify for adding to signal (xBuf) later */
    if (z >= 0) {
      psi->smBoost[m] = r >> MIN(31, z);
    } else {
      z = MIN(30, -z);
      CLIP_2N_SHIFT(r, z);
      psi->smBoost[m] = r;
    }
  }
}
//}}}
//{{{
void CalcGain (PSInfoSBR *psi, SBRHeader *sbrHeader, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan,
                      int ch, int env) {

  int lim, fbitsDQ;

  /* initialize to -1 so that mapping limiter bands to env/noise bands works right on first pass */
  psi->envBand        = -1;
  psi->noiseFloorBand = -1;
  psi->sBand          = -1;
  psi->highBand       = -1;

  fbitsDQ = (FBITS_OUT_DQ_ENV - psi->envDataDequantScale[ch][env]); /* Q(29 - optional scalefactor) */
  for (lim = 0; lim < sbrFreq->nLimiter; lim++) {
    /* the QMF bands are divided into lim regions (consecutive, non-overlapping) */
    CalcMaxGain(psi, sbrHeader, sbrGrid, sbrFreq, ch, env, lim, fbitsDQ);
    CalcComponentGains(psi, sbrGrid, sbrFreq, sbrChan, ch, env, lim, fbitsDQ);
    ApplyBoost(psi, sbrFreq, lim, fbitsDQ);
  }
}
//}}}
//{{{
void MapHF (PSInfoSBR *psi, SBRHeader *sbrHeader, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan,
                   int env, int hfReset) {

  int noiseTabIndex, sinIndex, gainNoiseIndex, hSL;
  int i, iStart, iEnd, m, idx, j, s, n, smre, smim;
  int gFilt, qFilt, xre, xim, gbMask, gbIdx;
  int *XBuf;

  noiseTabIndex =   sbrChan->noiseTabIndex;
  sinIndex =        sbrChan->sinIndex;
  gainNoiseIndex =  sbrChan->gainNoiseIndex;  /* oldest entries in filter delay buffer */

  if (hfReset)
    noiseTabIndex = 2;  /* starts at 1, double since complex */
  hSL = (sbrHeader->smoothMode ? 0 : 4);

  if (hfReset) {
    for (i = 0; i < hSL; i++) {
      for (m = 0; m < sbrFreq->numQMFBands; m++) {
        sbrChan->gTemp[gainNoiseIndex][m] = psi->gLimBoost[m];
        sbrChan->qTemp[gainNoiseIndex][m] = psi->qmLimBoost[m];
      }
      gainNoiseIndex++;
      if (gainNoiseIndex == MAX_NUM_SMOOTH_COEFS)
        gainNoiseIndex = 0;
    }
  }

  iStart = sbrGrid->envTimeBorder[env];
  iEnd =   sbrGrid->envTimeBorder[env+1];
  for (i = iStart; i < iEnd; i++) {
    /* save new values in temp buffers (delay)
     * we only store MAX_NUM_SMOOTH_COEFS most recent values,
     *   so don't keep storing the same value over and over
     */
    if (i - iStart < MAX_NUM_SMOOTH_COEFS) {
      for (m = 0; m < sbrFreq->numQMFBands; m++) {
        sbrChan->gTemp[gainNoiseIndex][m] = psi->gLimBoost[m];
        sbrChan->qTemp[gainNoiseIndex][m] = psi->qmLimBoost[m];
      }
    }

    /* see 4.6.18.7.6 */
    XBuf = psi->XBuf[i + HF_ADJ][sbrFreq->kStart];
    gbMask = 0;
    for (m = 0; m < sbrFreq->numQMFBands; m++) {
      if (env == psi->la || env == sbrChan->laPrev) {
        /* no smoothing filter for gain, and qFilt = 0 (only need to do once) */
        if (i == iStart) {
          psi->gFiltLast[m] = sbrChan->gTemp[gainNoiseIndex][m];
          psi->qFiltLast[m] = 0;
        }
      } else if (hSL == 0) {
        /* no smoothing filter for gain, (only need to do once) */
        if (i == iStart) {
          psi->gFiltLast[m] = sbrChan->gTemp[gainNoiseIndex][m];
          psi->qFiltLast[m] = sbrChan->qTemp[gainNoiseIndex][m];
        }
      } else {
        /* apply smoothing filter to gain and noise (after MAX_NUM_SMOOTH_COEFS, it's always the same) */
        if (i - iStart < MAX_NUM_SMOOTH_COEFS) {
          gFilt = 0;
          qFilt = 0;
          idx = gainNoiseIndex;
          for (j = 0; j < MAX_NUM_SMOOTH_COEFS; j++) {
            /* sum(abs(hSmoothCoef[j])) for all j < 1.0 */
            gFilt += MULSHIFT32(sbrChan->gTemp[idx][m], hSmoothCoef[j]);
            qFilt += MULSHIFT32(sbrChan->qTemp[idx][m], hSmoothCoef[j]);
            idx--;
            if (idx < 0)
              idx += MAX_NUM_SMOOTH_COEFS;
          }
          psi->gFiltLast[m] = gFilt << 1; /* restore to Q(FBITS_GLIM_BOOST) (gain of filter < 1.0, so no overflow) */
          psi->qFiltLast[m] = qFilt << 1; /* restore to Q(FBITS_QLIM_BOOST) */
        }
      }

      if (psi->smBoost[m] != 0) {
        /* add scaled signal and sinusoid, don't add noise (qFilt = 0) */
        smre = psi->smBoost[m];
        smim = smre;

        /* sinIndex:  [0] xre += sm   [1] xim += sm*s   [2] xre -= sm   [3] xim -= sm*s  */
        s = (sinIndex >> 1);  /* if 2 or 3, flip sign to subtract sm */
        s <<= 31;
        smre ^= (s >> 31);
        smre -= (s >> 31);
        s ^= ((m + sbrFreq->kStart) << 31);
        smim ^= (s >> 31);
        smim -= (s >> 31);

        /* if sinIndex == 0 or 2, smim = 0; if sinIndex == 1 or 3, smre = 0 */
        s = sinIndex << 31;
        smim &= (s >> 31);
        s ^= 0x80000000;
        smre &= (s >> 31);

        noiseTabIndex += 2;   /* noise filtered by 0, but still need to bump index */
      } else {
        /* add scaled signal and scaled noise */
        qFilt = psi->qFiltLast[m];
        n = noiseTab[noiseTabIndex++];
        smre = MULSHIFT32(n, qFilt) >> (FBITS_QLIM_BOOST - 1 - FBITS_OUT_QMFA);

        n = noiseTab[noiseTabIndex++];
        smim = MULSHIFT32(n, qFilt) >> (FBITS_QLIM_BOOST - 1 - FBITS_OUT_QMFA);
      }
      noiseTabIndex &= 1023;  /* 512 complex numbers */

      gFilt = psi->gFiltLast[m];
      xre = MULSHIFT32(gFilt, XBuf[0]);
      xim = MULSHIFT32(gFilt, XBuf[1]);
      CLIP_2N_SHIFT(xre, 32 - FBITS_GLIM_BOOST);
      CLIP_2N_SHIFT(xim, 32 - FBITS_GLIM_BOOST);

      xre += smre;  *XBuf++ = xre;
      xim += smim;  *XBuf++ = xim;

      gbMask |= FASTABS(xre);
      gbMask |= FASTABS(xim);
    }
    /* update circular buffer index */
    gainNoiseIndex++;
    if (gainNoiseIndex == MAX_NUM_SMOOTH_COEFS)
      gainNoiseIndex = 0;

    sinIndex++;
    sinIndex &= 3;

    /* ensure MIN_GBITS_IN_QMFS guard bits in output
     * almost never occurs in practice, but checking here makes synth QMF logic very simple
     */
    if (gbMask >> (31 - MIN_GBITS_IN_QMFS)) {
      XBuf = psi->XBuf[i + HF_ADJ][sbrFreq->kStart];
      for (m = 0; m < sbrFreq->numQMFBands; m++) {
        xre = XBuf[0];  xim = XBuf[1];
        CLIP_2N(xre, (31 - MIN_GBITS_IN_QMFS));
        CLIP_2N(xim, (31 - MIN_GBITS_IN_QMFS));
        *XBuf++ = xre;  *XBuf++ = xim;
      }
      CLIP_2N(gbMask, (31 - MIN_GBITS_IN_QMFS));
    }
    gbIdx = ((i + HF_ADJ) >> 5) & 0x01;
    sbrChan->gbMask[gbIdx] |= gbMask;
  }
  sbrChan->noiseTabIndex =  noiseTabIndex;
  sbrChan->sinIndex =       sinIndex;
  sbrChan->gainNoiseIndex = gainNoiseIndex;
}
//}}}

//{{{
int CalcCovariance1 (int* XBuf, int* p01reN, int* p01imN, int* p12reN, int* p12imN, int* p11reN, int* p22reN) {

  int accBuf[2*6];
  int n, z, s, loShift, hiShift, gbMask;
  U64 p01re, p01im, p12re, p12im, p11re, p22re;

  CVKernel1(XBuf, accBuf);
  p01re.r.lo32 = accBuf[0]; p01re.r.hi32 = accBuf[1];
  p01im.r.lo32 = accBuf[2]; p01im.r.hi32 = accBuf[3];
  p11re.r.lo32 = accBuf[4]; p11re.r.hi32 = accBuf[5];
  p12re.r.lo32 = accBuf[6]; p12re.r.hi32 = accBuf[7];
  p12im.r.lo32 = accBuf[8]; p12im.r.hi32 = accBuf[9];
  p22re.r.lo32 = accBuf[10];  p22re.r.hi32 = accBuf[11];

  /* 64-bit accumulators now have 2*FBITS_OUT_QMFA fraction bits
   * want to scale them down to integers (32-bit signed, Q0)
   *   with scale factor of 2^n, n >= 0
   * leave 2 GB's for calculating determinant, so take top 30 non-zero bits
   */
  gbMask  = ((p01re.r.hi32) ^ (p01re.r.hi32 >> 31)) | ((p01im.r.hi32) ^ (p01im.r.hi32 >> 31));
  gbMask |= ((p12re.r.hi32) ^ (p12re.r.hi32 >> 31)) | ((p12im.r.hi32) ^ (p12im.r.hi32 >> 31));
  gbMask |= ((p11re.r.hi32) ^ (p11re.r.hi32 >> 31)) | ((p22re.r.hi32) ^ (p22re.r.hi32 >> 31));
  if (gbMask == 0) {
    s = p01re.r.hi32 >> 31; gbMask  = (p01re.r.lo32 ^ s) - s;
    s = p01im.r.hi32 >> 31; gbMask |= (p01im.r.lo32 ^ s) - s;
    s = p12re.r.hi32 >> 31; gbMask |= (p12re.r.lo32 ^ s) - s;
    s = p12im.r.hi32 >> 31; gbMask |= (p12im.r.lo32 ^ s) - s;
    s = p11re.r.hi32 >> 31; gbMask |= (p11re.r.lo32 ^ s) - s;
    s = p22re.r.hi32 >> 31; gbMask |= (p22re.r.lo32 ^ s) - s;
    z = 32 + CLZ(gbMask);
  } else {
    gbMask  = FASTABS(p01re.r.hi32) | FASTABS(p01im.r.hi32);
    gbMask |= FASTABS(p12re.r.hi32) | FASTABS(p12im.r.hi32);
    gbMask |= FASTABS(p11re.r.hi32) | FASTABS(p22re.r.hi32);
    z = CLZ(gbMask);
  }

  n = 64 - z; /* number of non-zero bits in bottom of 64-bit word */
  if (n <= 30) {
    loShift = (30 - n);
    *p01reN = p01re.r.lo32 << loShift;  *p01imN = p01im.r.lo32 << loShift;
    *p12reN = p12re.r.lo32 << loShift;  *p12imN = p12im.r.lo32 << loShift;
    *p11reN = p11re.r.lo32 << loShift;  *p22reN = p22re.r.lo32 << loShift;
    return -(loShift + 2*FBITS_OUT_QMFA);
  } else if (n < 32 + 30) {
    loShift = (n - 30);
    hiShift = 32 - loShift;
    *p01reN = (p01re.r.hi32 << hiShift) | (p01re.r.lo32 >> loShift);
    *p01imN = (p01im.r.hi32 << hiShift) | (p01im.r.lo32 >> loShift);
    *p12reN = (p12re.r.hi32 << hiShift) | (p12re.r.lo32 >> loShift);
    *p12imN = (p12im.r.hi32 << hiShift) | (p12im.r.lo32 >> loShift);
    *p11reN = (p11re.r.hi32 << hiShift) | (p11re.r.lo32 >> loShift);
    *p22reN = (p22re.r.hi32 << hiShift) | (p22re.r.lo32 >> loShift);
    return (loShift - 2*FBITS_OUT_QMFA);
  } else {
    hiShift = n - (32 + 30);
    *p01reN = p01re.r.hi32 >> hiShift;  *p01imN = p01im.r.hi32 >> hiShift;
    *p12reN = p12re.r.hi32 >> hiShift;  *p12imN = p12im.r.hi32 >> hiShift;
    *p11reN = p11re.r.hi32 >> hiShift;  *p22reN = p22re.r.hi32 >> hiShift;
    return (32 - 2*FBITS_OUT_QMFA - hiShift);
  }

  return 0;
}
//}}}
//{{{
int CalcCovariance2 (int* XBuf, int* p02reN, int* p02imN) {

  U64 p02re, p02im;
  int n, z, s, loShift, hiShift, gbMask;
  int accBuf[2*2];

  CVKernel2(XBuf, accBuf);
  p02re.r.lo32 = accBuf[0];
  p02re.r.hi32 = accBuf[1];
  p02im.r.lo32 = accBuf[2];
  p02im.r.hi32 = accBuf[3];

  /* 64-bit accumulators now have 2*FBITS_OUT_QMFA fraction bits
   * want to scale them down to integers (32-bit signed, Q0)
   *   with scale factor of 2^n, n >= 0
   * leave 1 GB for calculating determinant, so take top 30 non-zero bits
   */
  gbMask  = ((p02re.r.hi32) ^ (p02re.r.hi32 >> 31)) | ((p02im.r.hi32) ^ (p02im.r.hi32 >> 31));
  if (gbMask == 0) {
    s = p02re.r.hi32 >> 31; gbMask  = (p02re.r.lo32 ^ s) - s;
    s = p02im.r.hi32 >> 31; gbMask |= (p02im.r.lo32 ^ s) - s;
    z = 32 + CLZ(gbMask);
  } else {
    gbMask  = FASTABS(p02re.r.hi32) | FASTABS(p02im.r.hi32);
    z = CLZ(gbMask);
  }
  n = 64 - z; /* number of non-zero bits in bottom of 64-bit word */

  if (n <= 30) {
    loShift = (30 - n);
    *p02reN = p02re.r.lo32 << loShift;
    *p02imN = p02im.r.lo32 << loShift;
    return -(loShift + 2*FBITS_OUT_QMFA);
  } else if (n < 32 + 30) {
    loShift = (n - 30);
    hiShift = 32 - loShift;
    *p02reN = (p02re.r.hi32 << hiShift) | (p02re.r.lo32 >> loShift);
    *p02imN = (p02im.r.hi32 << hiShift) | (p02im.r.lo32 >> loShift);
    return (loShift - 2*FBITS_OUT_QMFA);
  } else {
    hiShift = n - (32 + 30);
    *p02reN = p02re.r.hi32 >> hiShift;
    *p02imN = p02im.r.hi32 >> hiShift;
    return (32 - 2*FBITS_OUT_QMFA - hiShift);
  }

  return 0;
}
//}}}
//{{{
void CalcLPCoefs (int* XBuf, int* a0re, int* a0im, int* a1re, int* a1im, int gb) {

  int zFlag, n1, n2, nd, d, dInv, tre, tim;
  int p01re, p01im, p02re, p02im, p12re, p12im, p11re, p22re;

  /* pre-scale to avoid overflow - probably never happens in practice (see QMFA)
   *   max bit growth per accumulator = 38*2 = 76 mul-adds (X * X)
   *   using 64-bit MADD, so if X has n guard bits, X*X has 2n+1 guard bits
   *   gain 1 extra sign bit per multiply, so ensure ceil(log2(76/2) / 2) = 3 guard bits on inputs
   */
  if (gb < 3) {
    nd = 3 - gb;
    for (n1 = (NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6 + 2); n1 != 0; n1--) {
      XBuf[0] >>= nd; XBuf[1] >>= nd;
      XBuf += (2*64);
    }
    XBuf -= (2*64*(NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6 + 2));
  }

  /* calculate covariance elements */
  n1 = CalcCovariance1(XBuf, &p01re, &p01im, &p12re, &p12im, &p11re, &p22re);
  n2 = CalcCovariance2(XBuf, &p02re, &p02im);

  /* normalize everything to larger power of 2 scalefactor, call it n1 */
  if (n1 < n2) {
    nd = MIN(n2 - n1, 31);
    p01re >>= nd; p01im >>= nd;
    p12re >>= nd; p12im >>= nd;
    p11re >>= nd; p22re >>= nd;
    n1 = n2;
  } else if (n1 > n2) {
    nd = MIN(n1 - n2, 31);
    p02re >>= nd; p02im >>= nd;
  }

  /* calculate determinant of covariance matrix (at least 1 GB in pXX) */
  d = MULSHIFT32(p12re, p12re) + MULSHIFT32(p12im, p12im);
  d = MULSHIFT32(d, RELAX_COEF) << 1;
  d = MULSHIFT32(p11re, p22re) - d;

  zFlag = 0;
  *a0re = *a0im = 0;
  *a1re = *a1im = 0;
  if (d > 0) {
    /* input =   Q31  d    = Q(-2*n1 - 32 + nd) = Q31 * 2^(31 + 2*n1 + 32 - nd)
     * inverse = Q29  dInv = Q29 * 2^(-31 - 2*n1 - 32 + nd) = Q(29 + 31 + 2*n1 + 32 - nd)
     *
     * numerator has same Q format as d, since it's sum of normalized squares
     * so num * inverse = Q(-2*n1 - 32) * Q(29 + 31 + 2*n1 + 32 - nd)
     *                  = Q(29 + 31 - nd), drop low 32 in MULSHIFT32
     *                  = Q(29 + 31 - 32 - nd) = Q(28 - nd)
     */
    nd = CLZ(d) - 1;
    d <<= nd;
    dInv = InvRNormalized(d);

    /* 1 GB in pXX */
    tre = MULSHIFT32(p01re, p12re) - MULSHIFT32(p01im, p12im) - MULSHIFT32(p02re, p11re);
    tre = MULSHIFT32(tre, dInv);
    tim = MULSHIFT32(p01re, p12im) + MULSHIFT32(p01im, p12re) - MULSHIFT32(p02im, p11re);
    tim = MULSHIFT32(tim, dInv);

    /* if d is extremely small, just set coefs to 0 (would have poor precision anyway) */
    if (nd > 28 || (FASTABS(tre) >> (28 - nd)) >= 4 || (FASTABS(tim) >> (28 - nd)) >= 4) {
      zFlag = 1;
    } else {
      *a1re = tre << (FBITS_LPCOEFS - 28 + nd); /* i.e. convert Q(28 - nd) to Q(29) */
      *a1im = tim << (FBITS_LPCOEFS - 28 + nd);
    }
  }

  if (p11re) {
    /* input =   Q31  p11re = Q(-n1 + nd) = Q31 * 2^(31 + n1 - nd)
     * inverse = Q29  dInv  = Q29 * 2^(-31 - n1 + nd) = Q(29 + 31 + n1 - nd)
     *
     * numerator is Q(-n1 - 3)
     * so num * inverse = Q(-n1 - 3) * Q(29 + 31 + n1 - nd)
     *                  = Q(29 + 31 - 3 - nd), drop low 32 in MULSHIFT32
     *                  = Q(29 + 31 - 3 - 32 - nd) = Q(25 - nd)
     */
    nd = CLZ(p11re) - 1;  /* assume positive */
    p11re <<= nd;
    dInv = InvRNormalized(p11re);

    /* a1re, a1im = Q29, so scaled by (n1 + 3) */
    tre = (p01re >> 3) + MULSHIFT32(p12re, *a1re) + MULSHIFT32(p12im, *a1im);
    tre = -MULSHIFT32(tre, dInv);
    tim = (p01im >> 3) - MULSHIFT32(p12im, *a1re) + MULSHIFT32(p12re, *a1im);
    tim = -MULSHIFT32(tim, dInv);

    if (nd > 25 || (FASTABS(tre) >> (25 - nd)) >= 4 || (FASTABS(tim) >> (25 - nd)) >= 4) {
      zFlag = 1;
    } else {
      *a0re = tre << (FBITS_LPCOEFS - 25 + nd); /* i.e. convert Q(25 - nd) to Q(29) */
      *a0im = tim << (FBITS_LPCOEFS - 25 + nd);
    }
  }

  /* see 4.6.18.6.2 - if magnitude of a0 or a1 >= 4 then a0 = a1 = 0
   * i.e. a0re < 4, a0im < 4, a1re < 4, a1im < 4
   * Q29*Q29 = Q26
   */
  if (zFlag || MULSHIFT32(*a0re, *a0re) + MULSHIFT32(*a0im, *a0im) >= MAG_16 || MULSHIFT32(*a1re, *a1re) + MULSHIFT32(*a1im, *a1im) >= MAG_16) {
    *a0re = *a0im = 0;
    *a1re = *a1im = 0;
  }

  /* no need to clip - we never changed the XBuf data, just used it to calculate a0 and a1 */
  if (gb < 3) {
    nd = 3 - gb;
    for (n1 = (NUM_TIME_SLOTS*SAMPLES_PER_SLOT + 6 + 2); n1 != 0; n1--) {
      XBuf[0] <<= nd; XBuf[1] <<= nd;
      XBuf += (2*64);
    }
  }
}
//}}}

//{{{
int DequantizeEnvelope (int nBands, int ampRes, signed char* envQuant, int* envDequant) {

  int exp, expMax, i, scalei;

  if (nBands <= 0)
    return 0;

  /* scan for largest dequant value (do separately from envelope decoding to keep code cleaner) */
  expMax = 0;
  for (i = 0; i < nBands; i++) {
    if (envQuant[i] > expMax)
      expMax = envQuant[i];
    }

  /* dequantized envelope gains
   *   envDequant = 64*2^(envQuant / alpha) = 2^(6 + envQuant / alpha)
   *     if ampRes == 0, alpha = 2 and range of envQuant = [0, 127]
   *     if ampRes == 1, alpha = 1 and range of envQuant = [0, 63]
   * also if coupling is on, envDequant is scaled by something in range [0, 2]
   * so range of envDequant = [2^6, 2^69] (no coupling), [2^6, 2^70] (with coupling)
   *
   * typical range (from observation) of envQuant/alpha = [0, 27] --> largest envQuant ~= 2^33
   * output: Q(29 - (6 + expMax))
   *
   * reference: 14496-3:2001(E)/4.6.18.3.5 and 14496-4:200X/FPDAM8/5.6.5.1.2.1.5
   */
  if (ampRes) {
    do {
      exp = *envQuant++;
      scalei = MIN(expMax - exp, 31);
      *envDequant++ = envDQTab[0] >> scalei;
      } while (--nBands);

    return (6 + expMax);
    }
  else {
    expMax >>= 1;
    do {
      exp = *envQuant++;
      scalei = MIN(expMax - (exp >> 1), 31);
      *envDequant++ = envDQTab[exp & 0x01] >> scalei;
      } while (--nBands);

    return (6 + expMax);
    }
  }
//}}}
//{{{
void DequantizeNoise (int nBands, signed char* noiseQuant, int* noiseDequant) {

  int exp, scalei;
  if (nBands <= 0)
    return;

  /* dequantize noise floor gains (4.6.18.3.5):
   *   noiseDequant = 2^(NOISE_FLOOR_OFFSET - noiseQuant)
   *
   * range of noiseQuant = [0, 30] (see 4.6.18.3.6), NOISE_FLOOR_OFFSET = 6
   *   so range of noiseDequant = [2^-24, 2^6]
   */
  do {
    exp = *noiseQuant++;
    scalei = NOISE_FLOOR_OFFSET - exp + FBITS_OUT_DQ_NOISE; /* 6 + 24 - exp, exp = [0,30] */

    if (scalei < 0)
      *noiseDequant++ = 0;
    else if (scalei < 30)
      *noiseDequant++ = 1 << scalei;
    else
      *noiseDequant++ = 0x3fffffff; /* leave 2 GB */

    } while (--nBands);
  }
//}}}

//{{{
void PreMultiply64 (int* zbuf1) {

  int i, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  zbuf2 = zbuf1 + 64 - 1;
  csptr = cos4sin4tab64;

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = 64 >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0);
    ai2 = *(zbuf1 + 1);
    ai1 = *(zbuf2 + 0);
    ar2 = *(zbuf2 - 1);

    /* gain 2 ints bit from MULSHIFT32 by Q30
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
void BitReverse32 (int* inout) {

  int t, t1;

  // swap RE{p0} with RE{p1} and IM{P0} with IM{P1}
  #define swapcplx(p0,p1) \
    t = p0; t1 = *(&(p0)+1); p0 = p1; *(&(p0)+1) = *(&(p1)+1); p1 = t; *(&(p1)+1) = t1

  swapcplx(inout[2],  inout[32]);
  swapcplx(inout[4],  inout[16]);
  swapcplx(inout[6],  inout[48]);
  swapcplx(inout[10], inout[40]);
  swapcplx(inout[12], inout[24]);
  swapcplx(inout[14], inout[56]);
  swapcplx(inout[18], inout[36]);
  swapcplx(inout[22], inout[52]);
  swapcplx(inout[26], inout[44]);
  swapcplx(inout[30], inout[60]);
  swapcplx(inout[38], inout[50]);
  swapcplx(inout[46], inout[58]);
  }
//}}}
//{{{
void R8FirstPass32 (int* r0)
{
  #define SQRT1_2 0x5a82799a

  int r1, r2, r3, r4, r5, r6, r7;
  int r8, r9, r10, r11, r12, r14;

  /* number of passes = fft size / 8 = 32 / 8 = 4 */
  r1 = (32 >> 3);
  do {

    r2 = r0[8];
    r3 = r0[9];
    r4 = r0[10];
    r5 = r0[11];
    r6 = r0[12];
    r7 = r0[13];
    r8 = r0[14];
    r9 = r0[15];

    r10 = r2 + r4;
    r11 = r3 + r5;
    r12 = r6 + r8;
    r14 = r7 + r9;

    r2 -= r4;
    r3 -= r5;
    r6 -= r8;
    r7 -= r9;

    r4 = r2 - r7;
    r5 = r2 + r7;
    r8 = r3 - r6;
    r9 = r3 + r6;

    r2 = r4 - r9;
    r3 = r4 + r9;
    r6 = r5 - r8;
    r7 = r5 + r8;

    r2 = MULSHIFT32(SQRT1_2, r2); /* can use r4, r5, r8, or r9 for constant and lo32 scratch reg */
    r3 = MULSHIFT32(SQRT1_2, r3);
    r6 = MULSHIFT32(SQRT1_2, r6);
    r7 = MULSHIFT32(SQRT1_2, r7);

    r4 = r10 + r12;
    r5 = r10 - r12;
    r8 = r11 + r14;
    r9 = r11 - r14;

    r10 = r0[0];
    r11 = r0[2];
    r12 = r0[4];
    r14 = r0[6];

    r10 += r11;
    r12 += r14;

    r4 >>= 1;
    r10 += r12;
    r4 += (r10 >> 1);
    r0[ 0] = r4;
    r4 -= (r10 >> 1);
    r4 = (r10 >> 1) - r4;
    r0[ 8] = r4;

    r9 >>= 1;
    r10 -= 2*r12;
    r4 = (r10 >> 1) + r9;
    r0[ 4] = r4;
    r4 = (r10 >> 1) - r9;
    r0[12] = r4;
    r10 += r12;

    r10 -= 2*r11;
    r12 -= 2*r14;

    r4 =  r0[1];
    r9 =  r0[3];
    r11 = r0[5];
    r14 = r0[7];

    r4 += r9;
    r11 += r14;

    r8 >>= 1;
    r4 += r11;
    r8 += (r4 >> 1);
    r0[ 1] = r8;
    r8 -= (r4 >> 1);
    r8 = (r4 >> 1) - r8;
    r0[ 9] = r8;

    r5 >>= 1;
    r4 -= 2*r11;
    r8 = (r4 >> 1) - r5;
    r0[ 5] = r8;
    r8 = (r4 >> 1) + r5;
    r0[13] = r8;
    r4 += r11;

    r4 -= 2*r9;
    r11 -= 2*r14;

    r9 = r10 - r11;
    r10 += r11;
    r14 = r4 + r12;
    r4 -= r12;

    r5 = (r10 >> 1) + r7;
    r8 = (r4 >> 1) - r6;
    r0[ 2] = r5;
    r0[ 3] = r8;

    r5 = (r9 >> 1) - r2;
    r8 = (r14 >> 1) - r3;
    r0[ 6] = r5;
    r0[ 7] = r8;

    r5 = (r10 >> 1) - r7;
    r8 = (r4 >> 1) + r6;
    r0[10] = r5;
    r0[11] = r8;

    r5 = (r9 >> 1) + r2;
    r8 = (r14 >> 1) + r3;
    r0[14] = r5;
    r0[15] = r8;

    r0 += 16;
    r1--;
  } while (r1 != 0);
}
//}}}
//{{{
void R4Core32 (int* r0)
{
  int r2, r3, r4, r5, r6, r7;
  int r8, r9, r10, r12, r14;
  int *r1;

  r1 = (int *)twidTabOdd32;
  r10 = 8;
  do {
    /* can use r14 for lo32 scratch register in all MULSHIFT32 */
    r2 = r1[0];
    r3 = r1[1];
    r4 = r0[16];
    r5 = r0[17];
    r12 = r4 + r5;
    r12 = MULSHIFT32(r3, r12);
    r5  = MULSHIFT32(r2, r5) + r12;
    r2 += 2*r3;
    r4  = MULSHIFT32(r2, r4) - r12;

    r2 = r1[2];
    r3 = r1[3];
    r6 = r0[32];
    r7 = r0[33];
    r12 = r6 + r7;
    r12 = MULSHIFT32(r3, r12);
    r7  = MULSHIFT32(r2, r7) + r12;
    r2 += 2*r3;
    r6  = MULSHIFT32(r2, r6) - r12;

    r2 = r1[4];
    r3 = r1[5];
    r8 = r0[48];
    r9 = r0[49];
    r12 = r8 + r9;
    r12 = MULSHIFT32(r3, r12);
    r9  = MULSHIFT32(r2, r9) + r12;
    r2 += 2*r3;
    r8  = MULSHIFT32(r2, r8) - r12;

    r2 = r0[0];
    r3 = r0[1];

    r12 = r6 + r8;
    r8  = r6 - r8;
    r14 = r9 - r7;
    r9  = r9 + r7;

    r6 = (r2 >> 2) - r4;
    r7 = (r3 >> 2) - r5;
    r4 += (r2 >> 2);
    r5 += (r3 >> 2);

    r2 = r4 + r12;
    r3 = r5 + r9;
    r0[0] = r2;
    r0[1] = r3;
    r2 = r6 - r14;
    r3 = r7 - r8;
    r0[16] = r2;
    r0[17] = r3;
    r2 = r4 - r12;
    r3 = r5 - r9;
    r0[32] = r2;
    r0[33] = r3;
    r2 = r6 + r14;
    r3 = r7 + r8;
    r0[48] = r2;
    r0[49] = r3;

    r0 += 2;
    r1 += 6;
    r10--;
  } while (r10 != 0);
}
//}}}
//{{{
void PostMultiply64 (int* fft1, int nSampsOut) {

  int i, ar1, ai1, ar2, ai2;
  int t, cms2, cps2, sin2;
  int *fft2;
  const int *csptr;

  csptr = cos1sin1tab64;
  fft2 = fft1 + 64 - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin)/2, sin2 = sin/2, cms2 = (cos-sin)/2
   */
  cps2 = *csptr++;
  sin2 = *csptr++;
  cms2 = cps2 - 2*sin2;

  for (i = (nSampsOut + 3) >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ar2 = *(fft2 - 1);
    ai2 = *(fft2 + 0);

    /* gain 2 int bits (multiplying by Q30), max gain = sqrt(2) */
    t = MULSHIFT32(sin2, ar1 + ai1);
    *fft2-- = t - MULSHIFT32(cps2, ai1);
    *fft1++ = t + MULSHIFT32(cms2, ar1);

    cps2 = *csptr++;
    sin2 = *csptr++;

    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    *fft2-- = t - MULSHIFT32(cps2, ai2);
    cms2 = cps2 - 2*sin2;
    *fft1++ = t + MULSHIFT32(cms2, ar2);
    }
  }
//}}}
//{{{
void FFT32C (int* x, int nSampsOut) {

  PreMultiply64 (x);  /* 2 GB in, 3 GB out */

  /* decimation in time */
  BitReverse32 (x);

  /* 32-point complex FFT */
  R8FirstPass32 (x); /* gain 1 int bit,  lose 2 GB (making assumptions about input) */
  R4Core32 (x);      /* gain 2 int bits, lose 0 GB (making assumptions about input) */

  PostMultiply64 (x, nSampsOut);  /* 1 GB in, 2 GB out */
  }
//}}}

//{{{
void UnpackSBRGrid (cBitStream* bitStream, SBRHeader* sbrHeader, SBRGrid* sbrGrid) {

  int numEnvRaw, env, rel, pBits, border, middleBorder=0;
  unsigned char relBordLead[MAX_NUM_ENV], relBordTrail[MAX_NUM_ENV];
  unsigned char relBorder0[3], relBorder1[3], relBorder[3];
  unsigned char numRelBorder0, numRelBorder1, numRelBorder, numRelLead=0, numRelTrail;
  unsigned char absBordLead=0, absBordTrail=0, absBorder;

  sbrGrid->ampResFrame = sbrHeader->ampRes;
  sbrGrid->frameClass = bitStream->GetBits(2);
  switch (sbrGrid->frameClass) {

  case SBR_GRID_FIXFIX:
    numEnvRaw = bitStream->GetBits(2);
    sbrGrid->numEnv = (1 << numEnvRaw);
    if (sbrGrid->numEnv == 1)
      sbrGrid->ampResFrame = 0;

    sbrGrid->freqRes[0] = bitStream->GetBits(1);
    for (env = 1; env < sbrGrid->numEnv; env++)
       sbrGrid->freqRes[env] = sbrGrid->freqRes[0];

    absBordLead =  0;
    absBordTrail = NUM_TIME_SLOTS;
    numRelLead =   sbrGrid->numEnv - 1;
    numRelTrail =  0;

    /* numEnv = 1, 2, or 4 */
    if (sbrGrid->numEnv == 1)   border = NUM_TIME_SLOTS / 1;
    else if (sbrGrid->numEnv == 2)  border = NUM_TIME_SLOTS / 2;
    else              border = NUM_TIME_SLOTS / 4;

    for (rel = 0; rel < numRelLead; rel++)
      relBordLead[rel] = border;

    middleBorder = (sbrGrid->numEnv >> 1);

    break;

  case SBR_GRID_FIXVAR:
    absBorder = bitStream->GetBits(2) + NUM_TIME_SLOTS;
    numRelBorder = bitStream->GetBits(2);
    sbrGrid->numEnv = numRelBorder + 1;
    for (rel = 0; rel < numRelBorder; rel++)
      relBorder[rel] = 2*bitStream->GetBits(2) + 2;

    pBits = cLog2[sbrGrid->numEnv + 1];
    sbrGrid->pointer = bitStream->GetBits(pBits);

    for (env = sbrGrid->numEnv - 1; env >= 0; env--)
      sbrGrid->freqRes[env] = bitStream->GetBits(1);

    absBordLead =  0;
    absBordTrail = absBorder;
    numRelLead =   0;
    numRelTrail =  numRelBorder;

    for (rel = 0; rel < numRelTrail; rel++)
      relBordTrail[rel] = relBorder[rel];

    if (sbrGrid->pointer > 1)     middleBorder = sbrGrid->numEnv + 1 - sbrGrid->pointer;
    else                middleBorder = sbrGrid->numEnv - 1;

    break;

  case SBR_GRID_VARFIX:
    absBorder = bitStream->GetBits(2);
    numRelBorder = bitStream->GetBits(2);
    sbrGrid->numEnv = numRelBorder + 1;
    for (rel = 0; rel < numRelBorder; rel++)
      relBorder[rel] = 2*bitStream->GetBits(2) + 2;

    pBits = cLog2[sbrGrid->numEnv + 1];
    sbrGrid->pointer = bitStream->GetBits(pBits);

    for (env = 0; env < sbrGrid->numEnv; env++)
      sbrGrid->freqRes[env] = bitStream->GetBits(1);

    absBordLead =  absBorder;
    absBordTrail = NUM_TIME_SLOTS;
    numRelLead =   numRelBorder;
    numRelTrail =  0;

    for (rel = 0; rel < numRelLead; rel++)
      relBordLead[rel] = relBorder[rel];

    if (sbrGrid->pointer == 0)      middleBorder = 1;
    else if (sbrGrid->pointer == 1)   middleBorder = sbrGrid->numEnv - 1;
    else                middleBorder = sbrGrid->pointer - 1;

    break;

  case SBR_GRID_VARVAR:
    absBordLead =   bitStream->GetBits(2);  /* absBorder0 */
    absBordTrail =  bitStream->GetBits(2) + NUM_TIME_SLOTS; /* absBorder1 */
    numRelBorder0 = bitStream->GetBits(2);
    numRelBorder1 = bitStream->GetBits(2);

    sbrGrid->numEnv = numRelBorder0 + numRelBorder1 + 1;

    for (rel = 0; rel < numRelBorder0; rel++)
      relBorder0[rel] = 2*bitStream->GetBits(2) + 2;

    for (rel = 0; rel < numRelBorder1; rel++)
      relBorder1[rel] = 2*bitStream->GetBits(2) + 2;

    pBits = cLog2[numRelBorder0 + numRelBorder1 + 2];
    sbrGrid->pointer = bitStream->GetBits(pBits);

    for (env = 0; env < sbrGrid->numEnv; env++)
      sbrGrid->freqRes[env] = bitStream->GetBits(1);

    numRelLead =  numRelBorder0;
    numRelTrail = numRelBorder1;

    for (rel = 0; rel < numRelLead; rel++)
      relBordLead[rel] = relBorder0[rel];

    for (rel = 0; rel < numRelTrail; rel++)
      relBordTrail[rel] = relBorder1[rel];

    if (sbrGrid->pointer > 1)
      middleBorder = sbrGrid->numEnv + 1 - sbrGrid->pointer;
    else
      middleBorder = sbrGrid->numEnv - 1;

    break;
  }

  /* build time border vector */
  sbrGrid->envTimeBorder[0] = absBordLead * SAMPLES_PER_SLOT;

  rel = 0;
  border = absBordLead;
  for (env = 1; env <= numRelLead; env++) {
    border += relBordLead[rel++];
    sbrGrid->envTimeBorder[env] = border * SAMPLES_PER_SLOT;
  }

  rel = 0;
  border = absBordTrail;
  for (env = sbrGrid->numEnv - 1; env > numRelLead; env--) {
    border -= relBordTrail[rel++];
    sbrGrid->envTimeBorder[env] = border * SAMPLES_PER_SLOT;
  }

  sbrGrid->envTimeBorder[sbrGrid->numEnv] = absBordTrail * SAMPLES_PER_SLOT;

  if (sbrGrid->numEnv > 1) {
    sbrGrid->numNoiseFloors = 2;
    sbrGrid->noiseTimeBorder[0] = sbrGrid->envTimeBorder[0];
    sbrGrid->noiseTimeBorder[1] = sbrGrid->envTimeBorder[middleBorder];
    sbrGrid->noiseTimeBorder[2] = sbrGrid->envTimeBorder[sbrGrid->numEnv];
  } else {
    sbrGrid->numNoiseFloors = 1;
    sbrGrid->noiseTimeBorder[0] = sbrGrid->envTimeBorder[0];
    sbrGrid->noiseTimeBorder[1] = sbrGrid->envTimeBorder[1];
  }
}
//}}}
//{{{
void UnpackDeltaTimeFreq (cBitStream* bitStream, int numEnv, unsigned char* deltaFlagEnv,
                                 int numNoiseFloors, unsigned char *deltaFlagNoise) {

  int env, noiseFloor;
  for (env = 0; env < numEnv; env++)
    deltaFlagEnv[env] = bitStream->GetBits(1);

  for (noiseFloor = 0; noiseFloor < numNoiseFloors; noiseFloor++)
    deltaFlagNoise[noiseFloor] = bitStream->GetBits(1);
  }
//}}}
//{{{
void UnpackInverseFilterMode (cBitStream* bitStream, int numNoiseFloorBands, unsigned char* mode) {

  int n;
  for (n = 0; n < numNoiseFloorBands; n++)
    mode[n] = bitStream->GetBits(2);
  }
//}}}
//{{{
void UnpackSinusoids (cBitStream* bitStream, int nHigh, int addHarmonicFlag, unsigned char* addHarmonic) {

  int n = 0;
  if (addHarmonicFlag) {
    for (  ; n < nHigh; n++)
      addHarmonic[n] = bitStream->GetBits(1);
    }

  /* zero out unused bands */
  for (     ; n < MAX_QMF_BANDS; n++)
    addHarmonic[n] = 0;
  }
//}}}
//}}}

//{{{
int CalcFreqTables (SBRHeader* sbrHeader, SBRFreq *sbrFreq, int sampRateIdx) {

  int k0 = k0Tab[sampRateIdx][sbrHeader->startFreq];

  int k2;
  if (sbrHeader->stopFreq == 14)
    k2 = 2*k0;
  else if (sbrHeader->stopFreq == 15)
    k2 = 3*k0;
  else
    k2 = k2Tab[sampRateIdx][sbrHeader->stopFreq];
  if (k2 > 64)
    k2 = 64;

  /* calculate master frequency table */
  if (sbrHeader->freqScale == 0)
    sbrFreq->nMaster = CalcFreqMasterScaleZero(sbrFreq->freqMaster, sbrHeader->alterScale, k0, k2);
  else
    sbrFreq->nMaster = CalcFreqMaster(sbrFreq->freqMaster, sbrHeader->freqScale, sbrHeader->alterScale, k0, k2);

  /* calculate high frequency table and related parameters */
  sbrFreq->nHigh = CalcFreqHigh(sbrFreq->freqHigh, sbrFreq->freqMaster, sbrFreq->nMaster, sbrHeader->crossOverBand);
  sbrFreq->numQMFBands = sbrFreq->freqHigh[sbrFreq->nHigh] - sbrFreq->freqHigh[0];
  sbrFreq->kStart = sbrFreq->freqHigh[0];

  /* calculate low frequency table */
  sbrFreq->nLow = CalcFreqLow(sbrFreq->freqLow, sbrFreq->freqHigh, sbrFreq->nHigh);

  /* calculate noise floor frequency table */
  sbrFreq->numNoiseFloorBands = CalcFreqNoise(sbrFreq->freqNoise, sbrFreq->freqLow, sbrFreq->nLow, sbrFreq->kStart, k2, sbrHeader->noiseBands);

  /* calculate limiter table */
  sbrFreq->numPatches = BuildPatches(sbrFreq->patchNumSubbands, sbrFreq->patchStartSubband, sbrFreq->freqMaster,
    sbrFreq->nMaster, k0, sbrFreq->kStart, sbrFreq->numQMFBands, sampRateIdx);
  sbrFreq->nLimiter = CalcFreqLimiter(sbrFreq->freqLimiter, sbrFreq->patchNumSubbands, sbrFreq->freqLow, sbrFreq->nLow, sbrFreq->kStart,
    sbrHeader->limiterBands, sbrFreq->numPatches);

  return 0;
  }
//}}}
//{{{
void AdjustHighFreq (PSInfoSBR* psi, SBRHeader *sbrHeader, SBRGrid* sbrGrid, SBRFreq* sbrFreq, SBRChan* sbrChan, int ch) {

  int i, env, hfReset;
  unsigned char frameClass, pointer;

  frameClass = sbrGrid->frameClass;
  pointer  = sbrGrid->pointer;

  /* derive la from table 4.159 */
  if ((frameClass == SBR_GRID_FIXVAR || frameClass == SBR_GRID_VARVAR) && pointer > 0)
    psi->la = sbrGrid->numEnv + 1 - pointer;
  else if (frameClass == SBR_GRID_VARFIX && pointer > 1)
    psi->la = pointer - 1;
  else
    psi->la = -1;

  /* for each envelope, estimate gain and adjust SBR QMF bands */
  hfReset = sbrChan->reset;
  for (env = 0; env < sbrGrid->numEnv; env++) {
    EstimateEnvelope(psi, sbrHeader, sbrGrid, sbrFreq, env);
    CalcGain(psi, sbrHeader, sbrGrid, sbrFreq, sbrChan, ch, env);
    MapHF(psi, sbrHeader, sbrGrid, sbrFreq, sbrChan, env, hfReset);
    hfReset = 0;  /* only set for first envelope after header reset */
    }

  /* set saved sine flags to 0 for QMF bands outside of current frequency range */
  for (i = 0; i < sbrFreq->freqLimiter[0] + sbrFreq->kStart; i++)
    sbrChan->addHarmonic[0][i] = 0;
  for (i = sbrFreq->freqLimiter[sbrFreq->nLimiter] + sbrFreq->kStart; i < 64; i++)
    sbrChan->addHarmonic[0][i] = 0;
  sbrChan->addHarmonicFlag[0] = sbrChan->addHarmonicFlag[1];

  /* save la for next frame */
  if (psi->la == sbrGrid->numEnv)
    sbrChan->laPrev = 0;
  else
    sbrChan->laPrev = -1;
  }
//}}}
//{{{
void GenerateHighFreq (PSInfoSBR* psInfoSbr, SBRGrid* sbrGrid, SBRFreq* sbrFreq, SBRChan* sbrChan, int ch) {

  int band, newBW, c, t, gb, gbMask, gbIdx;
  int currPatch, p, x, k, g, i, iStart, iEnd, bw, bwsq;
  int a0re, a0im, a1re, a1im;
  int x1re, x1im, x2re, x2im;
  int ACCre, ACCim;
  int *XBufLo, *XBufHi;

  /* calculate array of chirp factors */
  for (band = 0; band < sbrFreq->numNoiseFloorBands; band++) {
    c = sbrChan->chirpFact[band]; /* previous (bwArray') */
    newBW = newBWTab[sbrChan->invfMode[0][band]][sbrChan->invfMode[1][band]];

    /* weighted average of new and old (can't overflow - total gain = 1.0) */
    if (newBW < c)
      t = MULSHIFT32(newBW, 0x60000000) + MULSHIFT32(0x20000000, c);  /* new is smaller: 0.75*new + 0.25*old */
    else
      t = MULSHIFT32(newBW, 0x74000000) + MULSHIFT32(0x0c000000, c);  /* new is larger: 0.90625*new + 0.09375*old */
    t <<= 1;

    if (t < 0x02000000) /* below 0.015625, clip to 0 */
      t = 0;
    if (t > 0x7f800000) /* clip to 0.99609375 */
      t = 0x7f800000;

    /* save curr as prev for next time */
    sbrChan->chirpFact[band] = t;
    sbrChan->invfMode[0][band] = sbrChan->invfMode[1][band];
    }

  iStart = sbrGrid->envTimeBorder[0] + HF_ADJ;
  iEnd =   sbrGrid->envTimeBorder[sbrGrid->numEnv] + HF_ADJ;

  /* generate new high freqs from low freqs, patches, and chirp factors */
  k = sbrFreq->kStart;
  g = 0;
  bw = sbrChan->chirpFact[g];
  bwsq = MULSHIFT32(bw, bw) << 1;

  gbMask = (sbrChan->gbMask[0] | sbrChan->gbMask[1]); /* older 32 | newer 8 */
  gb = CLZ(gbMask) - 1;

  for (currPatch = 0; currPatch < sbrFreq->numPatches; currPatch++) {
    for (x = 0; x < sbrFreq->patchNumSubbands[currPatch]; x++) {
      /* map k to corresponding noise floor band */
      if (k >= sbrFreq->freqNoise[g+1]) {
        g++;
        bw = sbrChan->chirpFact[g];   /* Q31 */
        bwsq = MULSHIFT32(bw, bw) << 1; /* Q31 */
        }

      p = sbrFreq->patchStartSubband[currPatch] + x;  /* low QMF band */
      XBufHi = psInfoSbr->XBuf[iStart][k];
      if (bw) {
        CalcLPCoefs(psInfoSbr->XBuf[0][p], &a0re, &a0im, &a1re, &a1im, gb);

        a0re = MULSHIFT32(bw, a0re);  /* Q31 * Q29 = Q28 */
        a0im = MULSHIFT32(bw, a0im);
        a1re = MULSHIFT32(bwsq, a1re);
        a1im = MULSHIFT32(bwsq, a1im);

        XBufLo = psInfoSbr->XBuf[iStart-2][p];

        x2re = XBufLo[0]; /* RE{XBuf[n-2]} */
        x2im = XBufLo[1]; /* IM{XBuf[n-2]} */
        XBufLo += (64*2);

        x1re = XBufLo[0]; /* RE{XBuf[n-1]} */
        x1im = XBufLo[1]; /* IM{XBuf[n-1]} */
        XBufLo += (64*2);

        for (i = iStart; i < iEnd; i++) {
          /* a0re/im, a1re/im are Q28 with at least 1 GB,
           *   so the summing for AACre/im is fine (1 GB in, plus 1 from MULSHIFT32)
           */
          ACCre = MULSHIFT32(x2re, a1re) - MULSHIFT32(x2im, a1im);
          ACCim = MULSHIFT32(x2re, a1im) + MULSHIFT32(x2im, a1re);
          x2re = x1re;
          x2im = x1im;

          ACCre += MULSHIFT32(x1re, a0re) - MULSHIFT32(x1im, a0im);
          ACCim += MULSHIFT32(x1re, a0im) + MULSHIFT32(x1im, a0re);
          x1re = XBufLo[0]; /* RE{XBuf[n]} */
          x1im = XBufLo[1]; /* IM{XBuf[n]} */
          XBufLo += (64*2);

          /* lost 4 fbits when scaling by a0re/im, a1re/im (Q28) */
          CLIP_2N_SHIFT(ACCre, 4);
          ACCre += x1re;
          CLIP_2N_SHIFT(ACCim, 4);
          ACCim += x1im;

          XBufHi[0] = ACCre;
          XBufHi[1] = ACCim;
          XBufHi += (64*2);

          /* update guard bit masks */
          gbMask  = FASTABS(ACCre);
          gbMask |= FASTABS(ACCim);
          gbIdx = (i >> 5) & 0x01;  /* 0 if i < 32, 1 if i >= 32 */
          sbrChan->gbMask[gbIdx] |= gbMask;
          }
        }
      else {
        XBufLo = (int *)psInfoSbr->XBuf[iStart][p];
        for (i = iStart; i < iEnd; i++) {
          XBufHi[0] = XBufLo[0];
          XBufHi[1] = XBufLo[1];
          XBufLo += (64*2);
          XBufHi += (64*2);
          }
        }
      k++;  /* high QMF band */
      }
    }
  }
//}}}

//{{{
void DecodeSBREnvelope (cBitStream* bitStream, PSInfoSBR* psi, SBRGrid* sbrGrid, SBRFreq* sbrFreq, SBRChan* sbrChan, int ch) {

  int huffIndexTime, huffIndexFreq, env, envStartBits, band, nBands, sf, lastEnv;
  int freqRes, freqResPrev, dShift, i;

  if (psi->couplingFlag && ch) {
    dShift = 1;
    if (sbrGrid->ampResFrame) {
      huffIndexTime = HuffTabSBR_tEnv30b;
      huffIndexFreq = HuffTabSBR_fEnv30b;
      envStartBits = 5;
    } else {
      huffIndexTime = HuffTabSBR_tEnv15b;
      huffIndexFreq = HuffTabSBR_fEnv15b;
      envStartBits = 6;
    }
  } else {
    dShift = 0;
    if (sbrGrid->ampResFrame) {
      huffIndexTime = HuffTabSBR_tEnv30;
      huffIndexFreq = HuffTabSBR_fEnv30;
      envStartBits = 6;
    } else {
      huffIndexTime = HuffTabSBR_tEnv15;
      huffIndexFreq = HuffTabSBR_fEnv15;
      envStartBits = 7;
    }
  }

  /* range of envDataQuant[] = [0, 127] (see comments in DequantizeEnvelope() for reference) */
  for (env = 0; env < sbrGrid->numEnv; env++) {
    nBands =      (sbrGrid->freqRes[env] ? sbrFreq->nHigh : sbrFreq->nLow);
    freqRes =     (sbrGrid->freqRes[env]);
    freqResPrev = (env == 0 ? sbrGrid->freqResPrev : sbrGrid->freqRes[env-1]);
    lastEnv =     (env == 0 ? sbrGrid->numEnvPrev-1 : env-1);
    if (lastEnv < 0)
      lastEnv = 0;  /* first frame */

    if (sbrChan->deltaFlagEnv[env] == 0) {
      /* delta coding in freq */
      sf = bitStream->GetBits(envStartBits) << dShift;
      sbrChan->envDataQuant[env][0] = sf;
      for (band = 1; band < nBands; band++) {
        sf = DecodeOneSymbol(bitStream, huffIndexFreq) << dShift;
        sbrChan->envDataQuant[env][band] = sf + sbrChan->envDataQuant[env][band-1];
      }
    } else if (freqRes == freqResPrev) {
      /* delta coding in time - same freq resolution for both frames */
      for (band = 0; band < nBands; band++) {
        sf = DecodeOneSymbol(bitStream, huffIndexTime) << dShift;
        sbrChan->envDataQuant[env][band] = sf + sbrChan->envDataQuant[lastEnv][band];
      }
    } else if (freqRes == 0 && freqResPrev == 1) {
      /* delta coding in time - low freq resolution for new frame, high freq resolution for old frame */
      for (band = 0; band < nBands; band++) {
        sf = DecodeOneSymbol(bitStream, huffIndexTime) << dShift;
        sbrChan->envDataQuant[env][band] = sf;
        for (i = 0; i < sbrFreq->nHigh; i++) {
          if (sbrFreq->freqHigh[i] == sbrFreq->freqLow[band]) {
            sbrChan->envDataQuant[env][band] += sbrChan->envDataQuant[lastEnv][i];
            break;
          }
        }
      }
    } else if (freqRes == 1 && freqResPrev == 0) {
      /* delta coding in time - high freq resolution for new frame, low freq resolution for old frame */
      for (band = 0; band < nBands; band++) {
        sf = DecodeOneSymbol(bitStream, huffIndexTime) << dShift;
        sbrChan->envDataQuant[env][band] = sf;
        for (i = 0; i < sbrFreq->nLow; i++) {
          if (sbrFreq->freqLow[i] <= sbrFreq->freqHigh[band] && sbrFreq->freqHigh[band] < sbrFreq->freqLow[i+1] ) {
            sbrChan->envDataQuant[env][band] += sbrChan->envDataQuant[lastEnv][i];
            break;
          }
        }
      }
    }

    /* skip coupling channel */
    if (ch != 1 || psi->couplingFlag != 1)
      psi->envDataDequantScale[ch][env] = DequantizeEnvelope(nBands, sbrGrid->ampResFrame, sbrChan->envDataQuant[env], psi->envDataDequant[ch][env]);
  }
  sbrGrid->numEnvPrev = sbrGrid->numEnv;
  sbrGrid->freqResPrev = sbrGrid->freqRes[sbrGrid->numEnv-1];
}
//}}}
//{{{
void DecodeSBRNoise (cBitStream* bitStream, PSInfoSBR* psi, SBRGrid* sbrGrid, SBRFreq* sbrFreq, SBRChan* sbrChan, int ch) {

  int huffIndexTime, huffIndexFreq, noiseFloor, band, dShift, sf, lastNoiseFloor;
  if (psi->couplingFlag && ch) {
    dShift = 1;
    huffIndexTime = HuffTabSBR_tNoise30b;
    huffIndexFreq = HuffTabSBR_fNoise30b;
  } else {
    dShift = 0;
    huffIndexTime = HuffTabSBR_tNoise30;
    huffIndexFreq = HuffTabSBR_fNoise30;
  }

  for (noiseFloor = 0; noiseFloor < sbrGrid->numNoiseFloors; noiseFloor++) {
    lastNoiseFloor = (noiseFloor == 0 ? sbrGrid->numNoiseFloorsPrev-1 : noiseFloor-1);
    if (lastNoiseFloor < 0)
      lastNoiseFloor = 0; /* first frame */

    if (sbrChan->deltaFlagNoise[noiseFloor] == 0) {
      /* delta coding in freq */
      sbrChan->noiseDataQuant[noiseFloor][0] = bitStream->GetBits(5) << dShift;
      for (band = 1; band < sbrFreq->numNoiseFloorBands; band++) {
        sf = DecodeOneSymbol(bitStream, huffIndexFreq) << dShift;
        sbrChan->noiseDataQuant[noiseFloor][band] = sf + sbrChan->noiseDataQuant[noiseFloor][band-1];
      }
    } else {
      /* delta coding in time */
      for (band = 0; band < sbrFreq->numNoiseFloorBands; band++) {
        sf = DecodeOneSymbol(bitStream, huffIndexTime) << dShift;
        sbrChan->noiseDataQuant[noiseFloor][band] = sf + sbrChan->noiseDataQuant[lastNoiseFloor][band];
      }
    }

    /* skip coupling channel */
    if (ch != 1 || psi->couplingFlag != 1)
      DequantizeNoise(sbrFreq->numNoiseFloorBands, sbrChan->noiseDataQuant[noiseFloor], psi->noiseDataDequant[ch][noiseFloor]);
  }
  sbrGrid->numNoiseFloorsPrev = sbrGrid->numNoiseFloors;
}
//}}}
//{{{
void UncoupleSBREnvelope (PSInfoSBR *psi, SBRGrid* sbrGrid, SBRFreq* sbrFreq, SBRChan* sbrChanR) {

  int env, band, nBands, scalei, E_1;

  scalei = (sbrGrid->ampResFrame ? 0 : 1);
  for (env = 0; env < sbrGrid->numEnv; env++) {
    nBands = (sbrGrid->freqRes[env] ? sbrFreq->nHigh : sbrFreq->nLow);
    psi->envDataDequantScale[1][env] = psi->envDataDequantScale[0][env]; /* same scalefactor for L and R */
    for (band = 0; band < nBands; band++) {
      /* clip E_1 to [0, 24] (scalefactors approach 0 or 2) */
      E_1 = sbrChanR->envDataQuant[env][band] >> scalei;
      if (E_1 < 0)  E_1 = 0;
      if (E_1 > 24) E_1 = 24;

      /* envDataDequant[0] has 1 GB, so << by 2 is okay */
      psi->envDataDequant[1][env][band] = MULSHIFT32(psi->envDataDequant[0][env][band], dqTabCouple[24 - E_1]) << 2;
      psi->envDataDequant[0][env][band] = MULSHIFT32(psi->envDataDequant[0][env][band], dqTabCouple[E_1]) << 2;
    }
  }
}
//}}}
//{{{
void UncoupleSBRNoise (PSInfoSBR* psi, SBRGrid* sbrGrid, SBRFreq* sbrFreq, SBRChan* sbrChanR) {

  int noiseFloor, band, Q_1;

  for (noiseFloor = 0; noiseFloor < sbrGrid->numNoiseFloors; noiseFloor++) {
    for (band = 0; band < sbrFreq->numNoiseFloorBands; band++) {
      /* Q_1 should be in range [0, 24] according to 4.6.18.3.6, but check to make sure */
      Q_1 = sbrChanR->noiseDataQuant[noiseFloor][band];
      if (Q_1 < 0)  Q_1 = 0;
      if (Q_1 > 24) Q_1 = 24;

      /* noiseDataDequant[0] has 1 GB, so << by 2 is okay */
      psi->noiseDataDequant[1][noiseFloor][band] = MULSHIFT32(psi->noiseDataDequant[0][noiseFloor][band], dqTabCouple[24 - Q_1]) << 2;
      psi->noiseDataDequant[0][noiseFloor][band] = MULSHIFT32(psi->noiseDataDequant[0][noiseFloor][band], dqTabCouple[Q_1]) << 2;
    }
  }
}
//}}}

//{{{
int InvRNormalized (int r) {
// r =   [0.5, 1.0) 1/r = (1.0, 2.0] so use 1.5 as initial guess  xn = xn*(2.0 - r*xn) */

  int xn = Q28_15;
  for (int i = NUM_ITER_IRN; i != 0; i--) {
    int t = MULSHIFT32(r, xn);    /* Q31*Q29 = Q28 */
    t = Q28_2 - t;                /* Q28 */
    xn = MULSHIFT32(xn, t) << 4;  /* Q29*Q28 << 4 = Q29 */
    }
  return xn;
  }
//}}}
//{{{
int RatioPowInv (int a, int b, int c) {

  if (a < 1 || b < 1 || c < 1 || a > 64 || b > 64 || c > 64 || a < b)
    return 0;

  int lna = MULSHIFT32(log2Tab[a], LOG2_EXP_INV) << 1;  /* ln(a), Q28 */
  int lnb = MULSHIFT32(log2Tab[b], LOG2_EXP_INV) << 1;  /* ln(b), Q28 */
  int p = (lna - lnb) / c;  /* Q28 */

  /* sum in Q24 */
  int y = (1 << 24);
  int t = p >> 4;   /* t = p^1 * 1/1! (Q24)*/
  y += t;

  for (int i = 2; i <= NUM_TERMS_RPI; i++) {
    t = MULSHIFT32(invTab[i-1], t) << 2;
    t = MULSHIFT32(p, t) << 4;  /* t = p^i * 1/i! (Q24) */
    y += t;
    }

  return y;
  }
//}}}
//{{{
int SqrtFix (int q, int fBitsIn, int *fBitsOut) {

  if (q <= 0) {
    *fBitsOut = fBitsIn;
    return 0;
    }

  /* force even fBitsIn */
  int z = fBitsIn & 0x01;
  q >>= z;
  fBitsIn -= z;

  /* for max precision, normalize to [0x20000000, 0x7fffffff] */
  z = (CLZ(q) - 1);
  z >>= 1;
  q <<= (2*z);

  /* choose initial bounds */
  int lo = 1;
  if (q >= 0x10000000)
    lo = 16384; /* (int)sqrt(0x10000000) */
  int hi = 46340;   /* (int)sqrt(0x7fffffff) */

  /* do binary search with 32x32->32 multiply test */
  do {
    int mid = (lo + hi) >> 1;
    if (mid*mid > q)
      hi = mid - 1;
    else
      lo = mid + 1;
    } while (hi >= lo);
  lo--;

  *fBitsOut = ((fBitsIn + 2*z) >> 1);
  return lo;
  }
//}}}

//{{{
int QMFAnalysis (int* inbuf, int* delay, int* XBuf, int fBitsIn, int* delayIdx, int qmfaBands) {

  int n, y, shift, gbMask;
  int *delayPtr, *uBuf, *tBuf;

  /* use XBuf[128] as temp buffer for reordering */
  uBuf = XBuf;    /* first 64 samples */
  tBuf = XBuf + 64; /* second 64 samples */

  /* overwrite oldest PCM with new PCM
   * delay[n] has 1 GB after shifting (either << or >>)
   */
  delayPtr = delay + (*delayIdx * 32);
  if (fBitsIn > FBITS_IN_QMFA) {
    shift = MIN(fBitsIn - FBITS_IN_QMFA, 31);
    for (n = 32; n != 0; n--) {
      y = (*inbuf) >> shift;
      inbuf++;
      *delayPtr++ = y;
    }
  } else {
    shift = MIN(FBITS_IN_QMFA - fBitsIn, 30);
    for (n = 32; n != 0; n--) {
      y = *inbuf++;
      CLIP_2N_SHIFT(y, shift);
      *delayPtr++ = y;
    }
  }

  QMFAnalysisConv((int *)cTabA, delay, *delayIdx, uBuf);

  /* uBuf has at least 2 GB right now (1 from clipping to Q(FBITS_IN_QMFA), one from
   *   the scaling by cTab (MULSHIFT32(*delayPtr--, *cPtr++), with net gain of < 1.0)
   * TODO - fuse with QMFAnalysisConv to avoid separate reordering
   */
    tBuf[2*0 + 0] = uBuf[0];
    tBuf[2*0 + 1] = uBuf[1];
    for (n = 1; n < 31; n++) {
        tBuf[2*n + 0] = -uBuf[64-n];
        tBuf[2*n + 1] =  uBuf[n+1];
    }
    tBuf[2*31 + 1] =  uBuf[32];
    tBuf[2*31 + 0] = -uBuf[33];

  FFT32C (tBuf, qmfaBands*2);

  /* TODO - roll into PostMultiply (if enough registers) */
  gbMask = 0;
  for (n = 0; n < qmfaBands; n++) {
    XBuf[2*n+0] =  tBuf[ n + 0];  /* implicit scaling of 2 in our output Q format */
    gbMask |= FASTABS(XBuf[2*n+0]);
    XBuf[2*n+1] = -tBuf[63 - n];
    gbMask |= FASTABS(XBuf[2*n+1]);
  }

  /* fill top section with zeros for HF generation */
  for (    ; n < 64; n++) {
    XBuf[2*n+0] = 0;
    XBuf[2*n+1] = 0;
  }

  *delayIdx = (*delayIdx == NUM_QMF_DELAY_BUFS - 1 ? 0 : *delayIdx + 1);

  /* minimum of 2 GB in output */
  return gbMask;
}
//}}}
//{{{
void QMFSynthesis (int* inbuf, int* delay, int* delayIdx, int qmfsBands, short* outbuf, int nChans) {

  int n, a0, a1, b0, b1, dOff0, dOff1, dIdx;
  int *tBufLo, *tBufHi;

  dIdx = *delayIdx;
  tBufLo = delay + dIdx*128 + 0;
  tBufHi = delay + dIdx*128 + 127;

  /* reorder inputs to DCT-IV, only use first qmfsBands (complex) samples
   * TODO - fuse with PreMultiply64 to avoid separate reordering steps */
  for (n = 0; n < qmfsBands >> 1; n++) {
    a0 = *inbuf++;
    b0 = *inbuf++;
    a1 = *inbuf++;
    b1 = *inbuf++;
    *tBufLo++ = a0;
    *tBufLo++ = a1;
    *tBufHi-- = b0;
    *tBufHi-- = b1;
    }
  if (qmfsBands & 0x01) {
    a0 = *inbuf++;
    b0 = *inbuf++;
    *tBufLo++ = a0;
        *tBufHi-- = b0;
        *tBufLo++ = 0;
    *tBufHi-- = 0;
    n++;
    }
  for (     ; n < 32; n++) {
    *tBufLo++ = 0;
    *tBufHi-- = 0;
    *tBufLo++ = 0;
    *tBufHi-- = 0;
    }

  tBufLo = delay + dIdx*128 + 0;
  tBufHi = delay + dIdx*128 + 64;

    /* 3 GB in, 1 GB out */
  FFT32C (tBufLo, 64);
  FFT32C (tBufHi,64);

  /* could fuse with PostMultiply64 to avoid separate pass */
  dOff0 = dIdx*128;
  dOff1 = dIdx*128 + 64;
  for (n = 32; n != 0; n--) {
    a0 =  (*tBufLo++);
    a1 =  (*tBufLo++);
    b0 =  (*tBufHi++);
    b1 = -(*tBufHi++);
    delay[dOff0++] = (b0 - a0);
    delay[dOff0++] = (b1 - a1);
    delay[dOff1++] = (b0 + a0);
    delay[dOff1++] = (b1 + a1);
    }

  QMFSynthesisConv ((int *)cTabS, delay, dIdx, outbuf, nChans);

  *delayIdx = (*delayIdx == NUM_QMF_DELAY_BUFS - 1 ? 0 : *delayIdx + 1);
  }
//}}}

//{{{
int GetSampRateIdx (int sampRate) {

  for (int idx = 0; idx < NUM_SAMPLE_RATES; idx++)
    if (sampRate == sampRateTab[idx])
      return idx;

  return -1;
  }
//}}}
//{{{
int UnpackSBRHeader (cBitStream* bitStream, SBRHeader* sbrHeader) {

  /* save previous values so we know whether to reset decoder */
  SBRHeader sbrHeaderPrev;
  sbrHeaderPrev.startFreq =     sbrHeader->startFreq;
  sbrHeaderPrev.stopFreq =      sbrHeader->stopFreq;
  sbrHeaderPrev.freqScale =     sbrHeader->freqScale;
  sbrHeaderPrev.alterScale =    sbrHeader->alterScale;
  sbrHeaderPrev.crossOverBand = sbrHeader->crossOverBand;
  sbrHeaderPrev.noiseBands =    sbrHeader->noiseBands;

  sbrHeader->ampRes =        bitStream->GetBits(1);
  sbrHeader->startFreq =     bitStream->GetBits(4);
  sbrHeader->stopFreq =      bitStream->GetBits(4);
  sbrHeader->crossOverBand = bitStream->GetBits(3);
  sbrHeader->resBitsHdr =    bitStream->GetBits(2);
  sbrHeader->hdrExtra1 =     bitStream->GetBits(1);
  sbrHeader->hdrExtra2 =     bitStream->GetBits(1);

  if (sbrHeader->hdrExtra1) {
    sbrHeader->freqScale =    bitStream->GetBits(2);
    sbrHeader->alterScale =   bitStream->GetBits(1);
    sbrHeader->noiseBands =   bitStream->GetBits(2);
  } else {
    /* defaults */
    sbrHeader->freqScale =    2;
    sbrHeader->alterScale =   1;
    sbrHeader->noiseBands =   2;
  }

  if (sbrHeader->hdrExtra2) {
    sbrHeader->limiterBands = bitStream->GetBits(2);
    sbrHeader->limiterGains = bitStream->GetBits(2);
    sbrHeader->interpFreq =   bitStream->GetBits(1);
    sbrHeader->smoothMode =   bitStream->GetBits(1);
  } else {
    /* defaults */
    sbrHeader->limiterBands = 2;
    sbrHeader->limiterGains = 2;
    sbrHeader->interpFreq =   1;
    sbrHeader->smoothMode =   1;
  }
  sbrHeader->count++;

  /* if any of these have changed from previous frame, reset the SBR module */
  if (sbrHeader->startFreq != sbrHeaderPrev.startFreq || sbrHeader->stopFreq != sbrHeaderPrev.stopFreq ||
    sbrHeader->freqScale != sbrHeaderPrev.freqScale || sbrHeader->alterScale != sbrHeaderPrev.alterScale ||
    sbrHeader->crossOverBand != sbrHeaderPrev.crossOverBand || sbrHeader->noiseBands != sbrHeaderPrev.noiseBands
    )
    return -1;
  else
    return 0;
}
//}}}
//{{{
void UnpackSBRSingleChannel (cBitStream* bitStream, PSInfoSBR* psi, int chBase) {

  int bitsLeft;
  SBRHeader *sbrHeader = &(psi->sbrHeader[chBase]);
  SBRGrid *sbrGridL = &(psi->sbrGrid[chBase+0]);
  SBRFreq *sbrFreq =  &(psi->sbrFreq[chBase]);
  SBRChan *sbrChanL = &(psi->sbrChan[chBase+0]);

  psi->dataExtra = bitStream->GetBits(1);
  if (psi->dataExtra)
    psi->resBitsData = bitStream->GetBits(4);

  UnpackSBRGrid(bitStream, sbrHeader, sbrGridL);
  UnpackDeltaTimeFreq(bitStream, sbrGridL->numEnv, sbrChanL->deltaFlagEnv, sbrGridL->numNoiseFloors, sbrChanL->deltaFlagNoise);
  UnpackInverseFilterMode(bitStream, sbrFreq->numNoiseFloorBands, sbrChanL->invfMode[1]);

  DecodeSBREnvelope(bitStream, psi, sbrGridL, sbrFreq, sbrChanL, 0);
  DecodeSBRNoise(bitStream, psi, sbrGridL, sbrFreq, sbrChanL, 0);

  sbrChanL->addHarmonicFlag[1] = bitStream->GetBits(1);
  UnpackSinusoids(bitStream, sbrFreq->nHigh, sbrChanL->addHarmonicFlag[1], sbrChanL->addHarmonic[1]);

  psi->extendedDataPresent = bitStream->GetBits(1);
  if (psi->extendedDataPresent) {
    psi->extendedDataSize = bitStream->GetBits(4);
    if (psi->extendedDataSize == 15)
      psi->extendedDataSize += bitStream->GetBits(8);

    bitsLeft = 8 * psi->extendedDataSize;

    /* get ID, unpack extension info, do whatever is necessary with it... */
    while (bitsLeft > 0) {
      bitStream->GetBits(8);
      bitsLeft -= 8;
    }
  }
}
//}}}
//{{{
void UnpackSBRChannelPair (cBitStream* bitStream, PSInfoSBR* psi, int chBase) {

  int bitsLeft;
  SBRHeader *sbrHeader = &(psi->sbrHeader[chBase]);
  SBRGrid *sbrGridL = &(psi->sbrGrid[chBase+0]), *sbrGridR = &(psi->sbrGrid[chBase+1]);
  SBRFreq *sbrFreq =  &(psi->sbrFreq[chBase]);
  SBRChan *sbrChanL = &(psi->sbrChan[chBase+0]), *sbrChanR = &(psi->sbrChan[chBase+1]);

  psi->dataExtra = bitStream->GetBits(1);
  if (psi->dataExtra) {
    psi->resBitsData = bitStream->GetBits(4);
    psi->resBitsData = bitStream->GetBits(4);
    }

  psi->couplingFlag = bitStream->GetBits(1);
  if (psi->couplingFlag) {
    UnpackSBRGrid(bitStream, sbrHeader, sbrGridL);
    CopyCouplingGrid(sbrGridL, sbrGridR);

    UnpackDeltaTimeFreq(bitStream, sbrGridL->numEnv, sbrChanL->deltaFlagEnv, sbrGridL->numNoiseFloors, sbrChanL->deltaFlagNoise);
    UnpackDeltaTimeFreq(bitStream, sbrGridR->numEnv, sbrChanR->deltaFlagEnv, sbrGridR->numNoiseFloors, sbrChanR->deltaFlagNoise);

    UnpackInverseFilterMode(bitStream, sbrFreq->numNoiseFloorBands, sbrChanL->invfMode[1]);
    CopyCouplingInverseFilterMode(sbrFreq->numNoiseFloorBands, sbrChanL->invfMode[1], sbrChanR->invfMode[1]);

    DecodeSBREnvelope(bitStream, psi, sbrGridL, sbrFreq, sbrChanL, 0);
    DecodeSBRNoise(bitStream, psi, sbrGridL, sbrFreq, sbrChanL, 0);
    DecodeSBREnvelope(bitStream, psi, sbrGridR, sbrFreq, sbrChanR, 1);
    DecodeSBRNoise(bitStream, psi, sbrGridR, sbrFreq, sbrChanR, 1);

    /* pass RIGHT sbrChan struct */
    UncoupleSBREnvelope(psi, sbrGridL, sbrFreq, sbrChanR);
    UncoupleSBRNoise(psi, sbrGridL, sbrFreq, sbrChanR);
    }
  else {
    UnpackSBRGrid(bitStream, sbrHeader, sbrGridL);
    UnpackSBRGrid(bitStream, sbrHeader, sbrGridR);
    UnpackDeltaTimeFreq(bitStream, sbrGridL->numEnv, sbrChanL->deltaFlagEnv, sbrGridL->numNoiseFloors, sbrChanL->deltaFlagNoise);
    UnpackDeltaTimeFreq(bitStream, sbrGridR->numEnv, sbrChanR->deltaFlagEnv, sbrGridR->numNoiseFloors, sbrChanR->deltaFlagNoise);
    UnpackInverseFilterMode(bitStream, sbrFreq->numNoiseFloorBands, sbrChanL->invfMode[1]);
    UnpackInverseFilterMode(bitStream, sbrFreq->numNoiseFloorBands, sbrChanR->invfMode[1]);

    DecodeSBREnvelope(bitStream, psi, sbrGridL, sbrFreq, sbrChanL, 0);
    DecodeSBREnvelope(bitStream, psi, sbrGridR, sbrFreq, sbrChanR, 1);
    DecodeSBRNoise(bitStream, psi, sbrGridL, sbrFreq, sbrChanL, 0);
    DecodeSBRNoise(bitStream, psi, sbrGridR, sbrFreq, sbrChanR, 1);
    }

  sbrChanL->addHarmonicFlag[1] = bitStream->GetBits(1);
  UnpackSinusoids(bitStream, sbrFreq->nHigh, sbrChanL->addHarmonicFlag[1], sbrChanL->addHarmonic[1]);

  sbrChanR->addHarmonicFlag[1] = bitStream->GetBits(1);
  UnpackSinusoids(bitStream, sbrFreq->nHigh, sbrChanR->addHarmonicFlag[1], sbrChanR->addHarmonic[1]);

  psi->extendedDataPresent = bitStream->GetBits(1);
  if (psi->extendedDataPresent) {
    psi->extendedDataSize = bitStream->GetBits(4);
    if (psi->extendedDataSize == 15)
      psi->extendedDataSize += bitStream->GetBits(8);

    bitsLeft = 8 * psi->extendedDataSize;

    /* get ID, unpack extension info, do whatever is necessary with it... */
    while (bitsLeft > 0) {
      bitStream->GetBits(8);
      bitsLeft -= 8;
     }
    }
  }
//}}}
