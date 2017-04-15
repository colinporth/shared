#pragma once

#include "neaacdec.h"
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
  //{{{  win32 basic types
  typedef unsigned __int64 uint64_t;
  typedef unsigned __int32 uint32_t;
  typedef unsigned __int16 uint16_t;
  typedef unsigned __int8 uint8_t;
  typedef signed __int64 int64_t;
  typedef signed __int32 int32_t;
  typedef signed __int16 int16_t;
  typedef signed __int8  int8_t;

  #define faad_malloc malloc
  #define faad_free free
  //}}}
#else
  //{{{  stm32 
  #include "FreeRtos.h"

  #define faad_malloc pvPortMalloc
  #define faad_free vPortFree
  //}}}
#endif

typedef float float32_t;
typedef float real_t;

//#define SBR_DEC
//#define PS_DEC
//#define LD_DEC
//#define LTP_DEC
//#define SSR_DEC

#define INLINE __inline
#define ALIGN

#ifndef max
  #define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
  #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define DIV_R(A, B) ((A)/(B))
#define DIV_C(A, B) ((A)/(B))

#define qmf_t complex_t
#define QMF_RE(A) RE(A)
#define QMF_IM(A) IM(A)

#define MUL_R(A,B) ((A)*(B))
#define MUL_C(A,B) ((A)*(B))
#define MUL_F(A,B) ((A)*(B))

#define REAL_CONST(A) ((real_t)(A))
#define COEF_CONST(A) ((real_t)(A))
#define Q2_CONST(A) ((real_t)(A))
#define FRAC_CONST(A) ((real_t)(A)) /* pure fractional part */

/* Complex multiplication */
static INLINE void ComplexMult(real_t *y1, real_t *y2, real_t x1, real_t x2, real_t c1, real_t c2) {
  *y1 = MUL_F(x1, c1) + MUL_F(x2, c2);
  *y2 = MUL_F(x2, c1) - MUL_F(x1, c2);
  }

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2 /* PI/2 */
  #define M_PI_2 1.57079632679489661923
#endif

#define sin sinf
#define cos cosf
#define log logf
#define exp expf
#define floor floorf
#define ceil ceilf
#define sqrt sqrtf

#define lrintf(f) ((int32_t)(f))

typedef real_t complex_t[2];
#define RE(A) A[0]
#define IM(A) A[1]

#define DEBUGDEC
#define DEBUGVAR(A,B,C)

uint8_t cpu_has_sse();
uint32_t ne_rng (uint32_t *__r1, uint32_t *__r2);
uint32_t wl_min_lzc (uint32_t x);
uint8_t get_sr_index (const uint32_t samplerate);
uint8_t max_pred_sfb (const uint8_t sr_index);
uint8_t max_tns_sfb (const uint8_t sr_index, const uint8_t object_type, const uint8_t is_short);
uint32_t get_sample_rate (const uint8_t sr_index);
int8_t can_decode_ot (const uint8_t object_type);
