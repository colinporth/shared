//{{{
#include "copy_mb.h"
#include "macros.h"
#include "ls_defines.h"
//}}}

//{{{
void WelsCopy4x4_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  const int32_t kiSrcStride2 = iStrideS << 1;
  const int32_t kiSrcStride3 = iStrideS + kiSrcStride2;
  const int32_t kiDstStride2 = iStrideD << 1;
  const int32_t kiDstStride3 = iStrideD + kiDstStride2;

  ST32 (pDst,                LD32 (pSrc));
  ST32 (pDst + iStrideD,     LD32 (pSrc + iStrideS));
  ST32 (pDst + kiDstStride2, LD32 (pSrc + kiSrcStride2));
  ST32 (pDst + kiDstStride3, LD32 (pSrc + kiSrcStride3));
}
//}}}
//{{{
void WelsCopy8x4_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  WelsCopy4x4_c (pDst, iStrideD, pSrc, iStrideS);
  WelsCopy4x4_c (pDst + 4, iStrideD, pSrc + 4, iStrideS);
}
//}}}
//{{{
void WelsCopy4x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  WelsCopy4x4_c (pDst, iStrideD, pSrc, iStrideS);
  WelsCopy4x4_c (pDst + (iStrideD << 2), iStrideD, pSrc + (iStrideS << 2), iStrideS);
}
//}}}
//{{{
void WelsCopy8x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  int32_t i;
  for (i = 0; i < 4; i++) {
    ST32 (pDst,                 LD32 (pSrc));
    ST32 (pDst + 4 ,            LD32 (pSrc + 4));
    ST32 (pDst + iStrideD,      LD32 (pSrc + iStrideS));
    ST32 (pDst + iStrideD + 4 , LD32 (pSrc + iStrideS + 4));
    pDst += iStrideD << 1;
    pSrc += iStrideS << 1;
  }
}
//}}}
//{{{
void WelsCopy8x16_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  int32_t i;
  for (i = 0; i < 8; ++i) {
    ST32 (pDst,                 LD32 (pSrc));
    ST32 (pDst + 4 ,            LD32 (pSrc + 4));
    ST32 (pDst + iStrideD,      LD32 (pSrc + iStrideS));
    ST32 (pDst + iStrideD + 4 , LD32 (pSrc + iStrideS + 4));
    pDst += iStrideD << 1;
    pSrc += iStrideS << 1;
  }
}
//}}}
//{{{
void WelsCopy16x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  int32_t i;
  for (i = 0; i < 8; i++) {
    ST32 (pDst,         LD32 (pSrc));
    ST32 (pDst + 4 ,    LD32 (pSrc + 4));
    ST32 (pDst + 8 ,    LD32 (pSrc + 8));
    ST32 (pDst + 12 ,   LD32 (pSrc + 12));
    pDst += iStrideD ;
    pSrc += iStrideS;
  }
}
//}}}
//{{{
void WelsCopy16x16_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS) {
  int32_t i;
  for (i = 0; i < 16; i++) {
    ST32 (pDst,         LD32 (pSrc));
    ST32 (pDst + 4 ,    LD32 (pSrc + 4));
    ST32 (pDst + 8 ,    LD32 (pSrc + 8));
    ST32 (pDst + 12 ,   LD32 (pSrc + 12));
    pDst += iStrideD ;
    pSrc += iStrideS;
  }
}
//}}}
