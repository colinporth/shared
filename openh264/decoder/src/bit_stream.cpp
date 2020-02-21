//{{{
#include "bit_stream.h"
#include "error_code.h"
//}}}

namespace WelsDec {
//{{{
inline uint32_t GetValue4Bytes (uint8_t* pDstNal) {

  return (pDstNal[0] << 24) | (pDstNal[1] << 16) | (pDstNal[2] << 8) | (pDstNal[3]);
  }
//}}}
//{{{
int32_t InitReadBits (PBitStringAux pBitString, intX_t iEndOffset) {

  if (pBitString->pCurBuf >= (pBitString->pEndBuf - iEndOffset)) 
    return ERR_INFO_INVALID_ACCESS;

  pBitString->uiCurBits  = GetValue4Bytes (pBitString->pCurBuf);
  pBitString->pCurBuf  += 4;
  pBitString->iLeftBits = -16;
  return ERR_NONE;
  }
//}}}
//{{{
int32_t DecInitBits (PBitStringAux pBitString, const uint8_t* kpBuf, const int32_t kiSize) {

  const int32_t kiSizeBuf = (kiSize + 7) >> 3;
  uint8_t* pTmp = (uint8_t*)kpBuf;

  if (NULL == pTmp)
    return ERR_INFO_INVALID_ACCESS;

  pBitString->pStartBuf = pTmp;            // buffer to start position
  pBitString->pEndBuf = pTmp + kiSizeBuf;  // buffer + length
  pBitString->iBits = kiSize;              // count bits of overall bitstreaming inputindex;
  pBitString->pCurBuf = pBitString->pStartBuf;
  int32_t iErr = InitReadBits (pBitString, 0);

  if (iErr)
    return iErr;
  return ERR_NONE;
  }
//}}}
//{{{
void RBSP2EBSP (uint8_t* pDstBuf, uint8_t* pSrcBuf, const int32_t kiSize) {

  uint8_t* pSrcPointer = pSrcBuf;
  uint8_t* pDstPointer = pDstBuf;
  uint8_t* pSrcEnd = pSrcBuf + kiSize;
  int32_t iZeroCount = 0;

  while (pSrcPointer < pSrcEnd) {
    if (iZeroCount == 2 && *pSrcPointer <= 3) {
      //add the code 0x03
      *pDstPointer++ = 3;
      iZeroCount = 0;
      }

    if (*pSrcPointer == 0)
      ++ iZeroCount;
    else
      iZeroCount = 0;
    *pDstPointer++ = *pSrcPointer++;
    }
  }
//}}}
} // namespace WelsDec
