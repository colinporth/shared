#ifndef WELS_DECODE_MB_AUX_H__
#define WELS_DECODE_MB_AUX_H__

#include "typedefs.h"
#include "macros.h"

namespace WelsDec {

void IdctResAddPred_c (uint8_t* pPred, const int32_t kiStride, int16_t* pRs);
void IdctResAddPred8x8_c (uint8_t* pPred, const int32_t kiStride, int16_t* pRs);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

//#if defined(X86_ASM)
void IdctResAddPred_mmx (uint8_t* pPred, const int32_t kiStride, int16_t* pRs);
//#endif//X86_ASM

#if defined(HAVE_NEON)
void IdctResAddPred_neon (uint8_t* pred, const int32_t stride, int16_t* rs);
#endif

#if defined(HAVE_NEON_AARCH64)
void IdctResAddPred_AArch64_neon (uint8_t* pred, const int32_t stride, int16_t* rs);
#endif


#if defined(__cplusplus)
}
#endif//__cplusplus

void GetI4LumaIChromaAddrTable (int32_t* pBlockOffset, const int32_t kiYStride, const int32_t kiUVStride);

} // namespace WelsDec

#endif//WELS_DECODE_MB_AUX_H__
