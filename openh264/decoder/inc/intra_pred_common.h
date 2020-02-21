#ifndef INTRA_PRED_COMMON_H
#define INTRA_PRED_COMMON_H

#include "typedefs.h"


void WelsI16x16LumaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);


#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

//#if defined(X86_ASM)
//for intra-prediction ASM functions
void WelsI16x16LumaPredV_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredH_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
//#endif//X86_ASM

#if defined(HAVE_NEON)
void WelsI16x16LumaPredV_neon (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredH_neon (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
#endif//HAVE_NEON

#if defined(HAVE_NEON_AARCH64)
void WelsI16x16LumaPredV_AArch64_neon (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredH_AArch64_neon (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
#endif//HAVE_NEON_AARCH64
#if defined(__cplusplus)
}
#endif//__cplusplus
#endif//



