//{{{  includes
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include "macros.h"
#include "crt_util_safe_x.h" // Safe CRT routines like utils for cross platforms
//}}}

//{{{
int32_t WelsSnprintf (char* pBuffer,  int32_t iSizeOfBuffer, const char* kpFormat, ...) {
  va_list  pArgPtr;
  int32_t  iRc;

  va_start (pArgPtr, kpFormat);

  iRc = vsnprintf_s (pBuffer, iSizeOfBuffer, _TRUNCATE, kpFormat, pArgPtr);
  if (iRc < 0)
    iRc = iSizeOfBuffer;

  va_end (pArgPtr);

  return iRc;
}
//}}}
//{{{
char* WelsStrncpy (char* pDest, int32_t iSizeInBytes, const char* kpSrc) {
  strncpy_s (pDest, iSizeInBytes, kpSrc, _TRUNCATE);

  return pDest;
}
//}}}
//{{{
int32_t WelsVsnprintf (char* pBuffer, int32_t iSizeOfBuffer, const char* kpFormat, va_list pArgPtr) {
  int32_t iRc = vsnprintf_s (pBuffer, iSizeOfBuffer, _TRUNCATE, kpFormat, pArgPtr);
  if (iRc < 0)
    iRc = iSizeOfBuffer;
  return iRc;
}
//}}}
//{{{
char* WelsStrcat (char* pDest, uint32_t uiSizeInBytes, const char* kpSrc) {
  uint32_t uiCurLen = (uint32_t) strlen (pDest);
  if (uiSizeInBytes > uiCurLen)
    return WelsStrncpy (pDest + uiCurLen, uiSizeInBytes - uiCurLen, kpSrc);
  return pDest;
}
//}}}

//{{{
int32_t WelsGetTimeOfDay (SWelsTime* pTp) {
  return _ftime_s (pTp);
}
//}}}
//{{{
int32_t WelsStrftime (char* pBuffer, int32_t iSize, const char* kpFormat, const SWelsTime* kpTp) {
  struct tm   sTimeNow;
  int32_t iRc;

  localtime_s (&sTimeNow, &kpTp->time);

  iRc = (int32_t)strftime (pBuffer, iSize, kpFormat, &sTimeNow);
  if (iRc == 0)
    pBuffer[0] = '\0';
  return iRc;
}
//}}}
//{{{
uint16_t WelsGetMillisecond (const SWelsTime* kpTp) {
  return kpTp->millitm;
}
//}}}
