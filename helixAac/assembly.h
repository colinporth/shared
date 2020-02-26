#pragma once
/**************************************************************************************
 * MULSHIFT32(x, y)    signed multiply of two 32-bit integers (x and y),
 *                     returns top 32-bits of 64-bit result
 * CLIPTOSHORT(x)      convert 32-bit integer to 16-bit short,
 *                     clipping to [-32768, 32767]
 * FASTABS(x)          branchless absolute value of signed integer x
 * CLZ(x)              count leading zeros on signed integer x
 * MADD64(sum64, x, y) 64-bit multiply accumulate: sum64 += (x*y)
 **************************************************************************************/

//{{{
static __inline int MULSHIFT32(int x, int y) {
	__int64 x64 = x;
	__int64 y64 = y;
	__int64 res = x64 * y64;
	return res >> 32;
	}
//}}}
//{{{
static __inline short CLIPTOSHORT(int x) {
/* clip to [-32768, 32767] */
	int sign = x >> 31;
	if (sign != (x >> 15))
		x = sign ^ ((1 << 15) - 1);
	return (short)x;
	}
//}}}
//{{{
static __inline int FASTABS(int x) {
	int sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;
	return x;
	}
//}}}
//{{{
static __inline int CLZ(int x) {
/* count leading zeros with binary search */
	if (!x)
		return 32;
	int numZeros = 1;
	if (!((unsigned int)x >> 16)) { numZeros += 16; x <<= 16; }
	if (!((unsigned int)x >> 24)) { numZeros +=  8; x <<=  8; }
	if (!((unsigned int)x >> 28)) { numZeros +=  4; x <<=  4; }
	if (!((unsigned int)x >> 30)) { numZeros +=  2; x <<=  2; }
	numZeros -= ((unsigned int)x >> 31);
	return numZeros;
	}
//}}}

//{{{
typedef union _U64 {
	__int64 w64;
	struct {
		unsigned int lo32;
		signed int   hi32;
		} r;
	} U64;
//}}}
//{{{
static __inline __int64 MADD64(__int64 sum64, int x, int y) {
	sum64 += (__int64)x * (__int64)y;
	return sum64;
	}
//}}}

//{{{
#define CLIP_2N(y, n) {            \
	int sign = (y) >> 31;            \
	if (sign != (y) >> (n))  {       \
		(y) = sign ^ ((1 << (n)) - 1); \
		}                              \
	}
//}}}
//{{{
#define CLIP_2N_SHIFT(y, n) {       \
	int sign = (y) >> 31;             \
	if (sign != (y) >> (30 - (n)))  { \
		(y) = sign ^ (0x3fffffff);      \
		}                               \
	else {                            \
		(y) = (y) << (n);               \
		}                               \
	}
//}}}
