#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include "typedefs.h"

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct _timeb SWelsTime;

int32_t  WelsSnprintf (char* buffer,  int32_t sizeOfBuffer,  const char* format, ...);
char* WelsStrncpy (char* dest, int32_t sizeInBytes, const char* src);
char* WelsStrcat (char* dest, uint32_t sizeInBytes, const char* src);
int32_t  WelsVsnprintf (char* buffer, int32_t sizeOfBuffer, const char* format, va_list argptr);

int32_t WelsGetTimeOfDay (SWelsTime* tp);
int32_t WelsStrftime (char* buffer, int32_t size, const char* format, const SWelsTime* tp);
uint16_t WelsGetMillisecond (const SWelsTime* tp);

#ifdef __cplusplus
  }
#endif
