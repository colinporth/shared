#include <stdlib.h>
#include <string.h>
#include "coder.h"

//{{{
/**************************************************************************************
 * Function:    AllocateBuffers
 *
 * Description: allocate all the memory needed for the AAC decoder
 *
 * Inputs:      none
 *
 * Outputs:     none
 *
 * Return:      pointer to AACDecInfo structure, cleared to all 0's (except for
 *                pointer to platform-specific data structure)
 *
 * Notes:       if one or more mallocs fail, function frees any buffers already
 *                allocated before returning
 **************************************************************************************/
AACDecInfo* AllocateBuffers() {

  AACDecInfo *aacDecInfo;

  aacDecInfo = (AACDecInfo *)malloc(sizeof(AACDecInfo));
  if (!aacDecInfo)
    return 0;
  ClearBuffer(aacDecInfo, sizeof(AACDecInfo));

  aacDecInfo->psInfoBase = malloc(sizeof(PSInfoBase));
  if (!aacDecInfo->psInfoBase) {
    FreeBuffers(aacDecInfo);
    return 0;
    }
  ClearBuffer(aacDecInfo->psInfoBase, sizeof(PSInfoBase));

  return aacDecInfo;
  }
//}}}
//{{{
/**************************************************************************************
 * Function:    ClearBuffer
 *
 * Description: fill buffer with 0's
 *
 * Inputs:      pointer to buffer
 *              number of bytes to fill with 0
 *
 * Outputs:     cleared buffer
 *
 * Return:      none
 *
 * Notes:       slow, platform-independent equivalent to memset(buf, 0, nBytes)
 **************************************************************************************/
 void ClearBuffer (void* buf, int nBytes)
{
/*  int i;
  unsigned char *cbuf = (unsigned char *)buf;

  for (i = 0; i < nBytes; i++)
    cbuf[i] = 0;

  return;
  */
  memset(buf, 0, nBytes);
}
//}}}

#ifndef SAFE_FREE
#define SAFE_FREE(x)  {if (x) free(x);  (x) = 0;} /* helper macro */
#endif
//{{{
/**************************************************************************************
 * Function:    FreeBuffers
 *
 * Description: frees all the memory used by the AAC decoder
 *
 * Inputs:      pointer to initialized AACDecInfo structure
 *
 * Outputs:     none
 *
 * Return:      none
 *
 * Notes:       safe to call even if some buffers were not allocated (uses SAFE_FREE)
 **************************************************************************************/
void FreeBuffers (AACDecInfo* aacDecInfo)
{
  if (!aacDecInfo)
    return;

  SAFE_FREE(aacDecInfo->psInfoBase);
  SAFE_FREE(aacDecInfo);
}
//}}}
