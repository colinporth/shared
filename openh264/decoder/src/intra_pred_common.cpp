//{{{

#include "ls_defines.h"
#include "cpu_core.h"
#include "intra_pred_common.h"
//}}}

//{{{
void WelsI16x16LumaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride) {
  uint8_t i = 15;
  const int8_t* kpSrc = (int8_t*)&pRef[-kiStride];
  const uint64_t kuiT1 = LD64 (kpSrc);
  const uint64_t kuiT2 = LD64 (kpSrc + 8);
  uint8_t* pDst = pPred;

  do {
    ST64 (pDst  , kuiT1);
    ST64 (pDst + 8, kuiT2);
    pDst += 16;
  } while (i-- > 0);
}
//}}}
//{{{
void WelsI16x16LumaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride) {
  int32_t iStridex15 = (kiStride << 4) - kiStride;
  int32_t iPredStride = 16;
  int32_t iPredStridex15 = 240; //(iPredStride<<4)-iPredStride;
  uint8_t i = 15;

  do {
    const uint8_t kuiSrc8 = pRef[iStridex15 - 1];
    const uint64_t kuiV64 = (uint64_t) (0x0101010101010101ULL * kuiSrc8);
    ST64 (&pPred[iPredStridex15], kuiV64);
    ST64 (&pPred[iPredStridex15 + 8], kuiV64);

    iStridex15 -= kiStride;
    iPredStridex15 -= iPredStride;
  } while (i-- > 0);
}
//}}}
