#ifndef WELS_DECODE_SLICE_H__
#define WELS_DECODE_SLICE_H__

#include "decoder_context.h"

namespace WelsDec {

int32_t WelsActualDecodeMbCavlcISlice (PWelsDecoderContext pCtx);
int32_t WelsDecodeMbCavlcISlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag);

int32_t WelsActualDecodeMbCavlcPSlice (PWelsDecoderContext pCtx);
int32_t WelsDecodeMbCavlcPSlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag);
typedef int32_t (*PWelsDecMbFunc) (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag);

int32_t WelsDecodeMbCabacISlice(PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag);
int32_t WelsDecodeMbCabacPSlice(PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag);
int32_t WelsDecodeMbCabacISliceBaseMode0(PWelsDecoderContext pCtx, uint32_t& uiEosFlag);
int32_t WelsDecodeMbCabacPSliceBaseMode0(PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiEosFlag);

int32_t WelsTargetSliceConstruction (PWelsDecoderContext pCtx); //construction based on slice

int32_t WelsDecodeSlice (PWelsDecoderContext pCtx, bool bFirstSliceInLayer, PNalUnit pNalCur);

int32_t WelsTargetMbConstruction (PWelsDecoderContext pCtx);

int32_t WelsMbIntraPredictionConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer, bool bOutput);
int32_t WelsMbInterSampleConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer,
                                       uint8_t* pDstY, uint8_t* pDstU, uint8_t* pDstV, int32_t iStrideL, int32_t iStrideC);
int32_t WelsMbInterConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer);
void WelsLumaDcDequantIdct (int16_t* pBlock, int32_t iQp,PWelsDecoderContext pCtx);
int32_t WelsMbInterPrediction (PWelsDecoderContext pCtx, PDqLayer pCurLayer);
void WelsChromaDcIdct (int16_t* pBlock);

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

//#if defined(X86_ASM)
void WelsBlockZero16x16_sse2(int16_t * block, int32_t stride);
void WelsBlockZero8x8_sse2(int16_t * block, int32_t stride);
//#endif

#if defined(HAVE_NEON)
void WelsBlockZero16x16_neon(int16_t * block, int32_t stride);
void WelsBlockZero8x8_neon(int16_t * block, int32_t stride);
#endif

#if defined(HAVE_NEON_AARCH64)
void WelsBlockZero16x16_AArch64_neon(int16_t * block, int32_t stride);
void WelsBlockZero8x8_AArch64_neon(int16_t * block, int32_t stride);
#endif
#ifdef __cplusplus
}
#endif//__cplusplus

void WelsBlockFuncInit (SBlockFunc* pFunc,  int32_t iCpu);
void WelsBlockZero16x16_c(int16_t * block, int32_t stride);
void WelsBlockZero8x8_c(int16_t * block, int32_t stride);

} // namespace WelsDec

#endif //WELS_DECODE_SLICE_H__


