// intel only
//{{{  includes
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "cpu_core.h"
//}}}

//{{{
uint32_t WelsCPUFeatureDetect (int32_t* pNumberOfLogicProcessors) {

  uint32_t uiCPU = 0;
  uint32_t uiFeatureA = 0, uiFeatureB = 0, uiFeatureC = 0, uiFeatureD = 0;
  int32_t CacheLineSize = 0;
  int8_t chVendorName[16] = { 0 };
  uint32_t uiMaxCpuidLevel = 0;

  if (!WelsCPUIdVerify()) 
    return 0;

  WelsCPUId (0, &uiFeatureA, (uint32_t*)&chVendorName[0], (uint32_t*)&chVendorName[8], (uint32_t*)&chVendorName[4]);
  uiMaxCpuidLevel = uiFeatureA;
  if (uiMaxCpuidLevel == 0) /* maximum input value for basic cpuid information */
    return 0;

  WelsCPUId (1, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
  if ((uiFeatureD & 0x00800000) == 0)  /* Basic MMX technology is not support in cpu, mean nothing for us so return here */
    return 0;

  uiCPU = WELS_CPU_MMX;
  if (uiFeatureD & 0x02000000) /* SSE technology is identical to AMD MMX extensions */
    uiCPU |= WELS_CPU_MMXEXT | WELS_CPU_SSE;
  if (uiFeatureD & 0x04000000) /* SSE2 support here */
    uiCPU |= WELS_CPU_SSE2;
  if (uiFeatureD & 0x00000001) /* x87 FPU on-chip checking */
    uiCPU |= WELS_CPU_FPU;
  if (uiFeatureD & 0x00008000) /* CMOV instruction checking */
    uiCPU |= WELS_CPU_CMOV;
  if (uiFeatureD & 0x10000000) /* Multi-Threading checking: contains of multiple logic processors */
    uiCPU |= WELS_CPU_HTT;
  if (uiFeatureC & 0x00000001) /* SSE3 support here */
    uiCPU |= WELS_CPU_SSE3;
  if (uiFeatureC & 0x00000200) /* SSSE3 support here */
    uiCPU |= WELS_CPU_SSSE3;
  if (uiFeatureC & 0x00080000) /* SSE4.1 support here, 45nm Penryn processor */
    uiCPU |= WELS_CPU_SSE41;
  if (uiFeatureC & 0x00100000) /* SSE4.2 support here, next generation Nehalem processor */
    uiCPU |= WELS_CPU_SSE42;
  if (WelsCPUSupportAVX (uiFeatureA, uiFeatureC)) /* AVX supported */
    uiCPU |= WELS_CPU_AVX;
  if (WelsCPUSupportFMA (uiFeatureA, uiFeatureC)) /* AVX FMA supported */
    uiCPU |= WELS_CPU_FMA;
  if (uiFeatureC & 0x02000000) /* AES checking */
    uiCPU |= WELS_CPU_AES;
  if (uiFeatureC & 0x00400000) /* MOVBE checking */
    uiCPU |= WELS_CPU_MOVBE;
  if (uiMaxCpuidLevel >= 7) {
    uiFeatureC = 0;
    WelsCPUId (7, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
    if ((uiCPU & WELS_CPU_AVX) && (uiFeatureB & 0x00000020)) /* AVX2 supported */
      uiCPU |= WELS_CPU_AVX2;
    }
  if (pNumberOfLogicProcessors != NULL) {
    if (uiCPU & WELS_CPU_HTT)
      *pNumberOfLogicProcessors = (uiFeatureB & 0x00ff0000) >> 16; // feature bits: 23-16 on returned EBX
     else
      *pNumberOfLogicProcessors = 0;
    if (uiMaxCpuidLevel >= 4) {
      uiFeatureC = 0;
      WelsCPUId (0x4, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
      if (uiFeatureA != 0)
        *pNumberOfLogicProcessors = ((uiFeatureA & 0xfc000000) >> 26) + 1;
      }
    }

  int32_t  family, model;
  WelsCPUId (1, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
  family = ((uiFeatureA >> 8) & 0xf) + ((uiFeatureA >> 20) & 0xff);
  model  = ((uiFeatureA >> 4) & 0xf) + ((uiFeatureA >> 12) & 0xf0);
  if ((family == 6) && (model == 9 || model == 13 || model == 14)) 
    uiCPU &= ~ (WELS_CPU_SSE2 | WELS_CPU_SSE3);

  WelsCPUId (1, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
  CacheLineSize = (uiFeatureB & 0xff00) >> 5; // ((clflush_line_size >> 8) << 3), CLFLUSH_line_size * 8 = CacheLineSize_in_byte
  if (CacheLineSize == 128)
    uiCPU |= WELS_CPU_CACHELINE_128;
  else if (CacheLineSize == 64)
    uiCPU |= WELS_CPU_CACHELINE_64;
  else if (CacheLineSize == 32)
    uiCPU |= WELS_CPU_CACHELINE_32;
  else if (CacheLineSize == 16)
    uiCPU |= WELS_CPU_CACHELINE_16;

  return uiCPU;
  }
//}}}
//{{{
void WelsCPURestore (const uint32_t kuiCPU) {

  if (kuiCPU & (WELS_CPU_MMX | WELS_CPU_MMXEXT | WELS_CPU_3DNOW | WELS_CPU_3DNOWEXT))
    WelsEmms();
  }
//}}}

// #elif defined(HAVE_NEON) //For supporting both android platform and iOS platform
//{{{
//uint32_t WelsCPUFeatureDetect (int32_t* pNumberOfLogicProcessors) {
  //return WELS_CPU_ARMv7 |
         //WELS_CPU_VFPv3 |
         //WELS_CPU_NEON;
//}
//}}}
