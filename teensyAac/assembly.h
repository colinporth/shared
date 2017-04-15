// assembly.h - possible assembly routines
#pragma once
// MADD64(sum64, x, y)  64-bit multiply accumulate: sum64 += (x*y)
// MULSHIFT32(x, y)     signed multiply of two 32-bit integers (x and y), returns top 32-bits of 64-bit result
// FASTABS(x)           branchless absolute value of signed integer x
// CLZ(x)               count leading zeros on signed integer x
// REV16(value)         reverse byte order (16 bit)
// REV32(value)         reverse byte order (32 bit)
// CLIPTOSHORT(x)       convert 32-bit integer to 16-bit short,  clipping to [-32768, 32767]
// CLIP_2N(val, n)      clip to [-2^n, 2^n-1], valid range of n = [1, 30]
// CLIP_2N_SHIFT(y, n)  y <<= n, clipping to range [-2^30, 2^30 - 1] (i.e. output has one guard bit)

#ifdef _WIN32
  #include <stdint.h>
  #pragma warning( disable : 4035 ) // complains about inline asm not returning a value
  #pragma warning( disable : 4838 ) // complains about narrowing conversion

  typedef int64_t Word64;
  //{{{
  typedef union _U64 {
    Word64 w64;
    struct {
      /* x86 = little endian */
      unsigned int lo32;
      signed int   hi32;
      } r;
    } U64;
  //}}}
  //{{{
  /* returns 64-bit value in [edx:eax] */
  static __inline Word64 MADD64 (Word64 sum64, int x, int y) {
    return sum64 + ((Word64)x * (Word64)y);
    }
  //}}}
  //{{{
  static __inline int MULSHIFT32 (int x, int y) {
    return (int)(((long long)x * (long long)y) >> 32);

    //__asm {
      //mov   eax, x
      //imul  y
      //mov   eax, edx
      //}
    //}
    }
  //}}}
  //{{{
  static __inline int FASTABS (int x) {
    int sign = x >> (sizeof(int) * 8 - 1);
    x ^= sign;
    x -= sign;
    return x;
    }
  //}}}
  //{{{
  static __inline int CLZ (int x) {

    int numZeros;
    if (!x)
      return 32;

    /* count leading zeros with binary search */
    numZeros = 1;
    if (!((unsigned int)x >> 16)) {
      numZeros += 16;
      x <<= 16;
      }

    if (!((unsigned int)x >> 24)) {
      numZeros +=  8;
      x <<=  8;
      }

    if (!((unsigned int)x >> 28)) {
      numZeros +=  4;
      x <<=  4;
      }

    if (!((unsigned int)x >> 30)) {
      numZeros +=  2;
      x <<=  2;
      }

    numZeros -= ((unsigned int)x >> 31);

    return numZeros;
    }
  //}}}
  //{{{
  static __inline unsigned int REV16 (unsigned int value) {
    return (value << 8) + (value >> 8);
    }
  //}}}
  //{{{
  static __inline unsigned int REV32 (unsigned int value) {
    return (value << 24) + ((value & 0xFF00) << 8) + ((value & 0xFF0000) >> 8) + (value >> 24);
    }
  //}}}
  //{{{
  static __inline short CLIPTOSHORT (int x) {

    int sign = x >> 31;
    if (sign != (x >> 15))
      x = sign ^ ((1 << 15) - 1);
    return (short)x;
    }
  //}}}
#elif __linux__
  typedef long long Word64;
  //{{{
  static __inline__ int MULSHIFT32(int x, int y)
  {
      int z;

      z = (Word64)x * (Word64)y >> 32;

    return z;
  }
  //}}}
  //{{{
  static __inline short CLIPTOSHORT(int x)
  {
    int sign;

    /* clip to [-32768, 32767] */
    sign = x >> 31;
    if (sign != (x >> 15))
      x = sign ^ ((1 << 15) - 1);

    return (short)x;
  }
  //}}}
  //{{{
  static __inline int FASTABS(int x)
  {
    int sign;

    sign = x >> (sizeof(int) * 8 - 1);
    x ^= sign;
    x -= sign;

    return x;
  }
  //}}}
  //{{{
  static __inline int CLZ(int x)
  {
    int numZeros;

    if (!x)
      return 32;

    /* count leading zeros with binary search (function should be 17 ARM instructions total) */
    numZeros = 1;
    if (!((unsigned int)x >> 16)) { numZeros += 16; x <<= 16; }
    if (!((unsigned int)x >> 24)) { numZeros +=  8; x <<=  8; }
    if (!((unsigned int)x >> 28)) { numZeros +=  4; x <<=  4; }
    if (!((unsigned int)x >> 30)) { numZeros +=  2; x <<=  2; }

    numZeros -= ((unsigned int)x >> 31);

    return numZeros;
  }
  //}}}
  //{{{
  static __inline unsigned int REV16 (unsigned int value) {
    return (value << 8) + (value >> 8);
    }
  //}}}
  //{{{
  static __inline unsigned int REV32 (unsigned int value) {
    return (value << 24) + ((value & 0xFF00) << 8) + ((value & 0xFF0000) >> 8) + (value >> 24);
    }
  //}}}
  //{{{
  typedef union _U64 {
    Word64 w64;
    struct {
      /* x86 = little endian */
      unsigned int lo32;
      signed int   hi32;
    } r;
  } U64;
  //}}}
  //{{{
  static __inline Word64 MADD64(Word64 sum64, int x, int y)
  {
    sum64 += (Word64)x * (Word64)y;

    return sum64;
  }
  //}}}

#else // stm32
  #include <stdlib.h>
  typedef long long Word64;
  //{{{
  typedef union _U64 {
    Word64 w64;
    struct {
      /* little endian */
      unsigned int lo32;
      signed int   hi32;
      } r;
    } U64;
  //}}}
  //{{{
  static inline Word64 MADD64 (Word64 sum64, int x, int y) {
    U64 u;
    u.w64 = sum64;
    __asm ("smlal %0,%1,%2,%3" : "+&r" (u.r.lo32), "+&r" (u.r.hi32) : "r" (x), "r" (y) : "cc");
    return u.w64;
    }
  //}}}
  //{{{
  static inline int MULSHIFT32 (int x, int y) {
    int zlow;
    __asm ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (y) : "r" (x), "1" (y) : "cc");
    return y;
    }
  //}}}
  #define FASTABS(x) abs(x)
  #define CLZ(x) __builtin_clz(x)
  //{{{
  static inline unsigned int REV16 (unsigned int value) {
    __asm ("rev16 %0, %1" : "=r" (value) : "r" (value) );
    return(value);
    }
  //}}}
  //{{{
  static inline unsigned int REV32 (unsigned int value) {
    __asm ("rev %0, %1" : "=r" (value) : "r" (value) );
    return(value);
    }
  //}}}
  //{{{
  static inline short CLIPTOSHORT (int x) {
    __asm ("ssat %0, #16, %1" : "=r" (x) : "r" (x));
    return x;
    }
  //}}}
#endif

//{{{
#define CLIP_2N(val, n) { \
  if ((val) >> 31 != (val) >> (n)) \
    (val) = ((val) >> 31) ^ ((1 << (n)) - 1);  \
  }
//}}}
//{{{
#define CLIP_2N_SHIFT(y, n) { \
  int sign = (y) >> 31; \
  if (sign != (y) >> (30 - (n))) \
    (y) = sign ^ (0x3fffffff); \
  else \
    (y) = (y) << (n); \
  }
//}}}
