#ifndef EXPAND_PICTURE_H
#define EXPAND_PICTURE_H

#include "typedefs.h"

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#define PADDING_LENGTH 32 // reference extension

//#if defined(X86_ASM)
void ExpandPictureLuma_sse2 (uint8_t* pDst,
                             const int32_t kiStride,
                             const int32_t kiPicW,
                             const int32_t kiPicH);
void ExpandPictureChromaAlign_sse2 (uint8_t* pDst,
                                    const int32_t kiStride,
                                    const int32_t kiPicW,
                                    const int32_t kiPicH);
void ExpandPictureChromaUnalign_sse2 (uint8_t* pDst,
                                      const int32_t kiStride,
                                      const int32_t kiPicW,
                                      const int32_t kiPicH);
//#endif//X86_ASM

#if defined(HAVE_NEON)
void ExpandPictureLuma_neon (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW, const int32_t kiPicH);
void ExpandPictureChroma_neon (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW, const int32_t kiPicH);
#endif
#if defined(HAVE_NEON_AARCH64)
void ExpandPictureLuma_AArch64_neon (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW, const int32_t kiPicH);
void ExpandPictureChroma_AArch64_neon (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW,
                                       const int32_t kiPicH);
#endif

typedef void (*PExpandPictureFunc) (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW, const int32_t kiPicH);

typedef struct TagExpandPicFunc {
  PExpandPictureFunc pfExpandLumaPicture;
  PExpandPictureFunc pfExpandChromaPicture[2];
} SExpandPicFunc;


void ExpandReferencingPicture (uint8_t* pData[3], int32_t iWidth, int32_t iHeight, int32_t iStride[3],
                               PExpandPictureFunc pExpLuma, PExpandPictureFunc pExpChrom[2]);

void InitExpandPictureFunc (SExpandPicFunc* pExpandPicFunc, const uint32_t kuiCPUFlags);

#if defined(__cplusplus)
}
#endif//__cplusplus

#endif
