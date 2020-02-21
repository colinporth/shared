#pragma once

#include <stdarg.h>
#include "typedefs.h"

#define MAX_LOG_SIZE    1024
#define MAX_WIDTH      (4096)
#define MAX_HEIGHT     (2304)//MAX_FS_LEVEL51 (36864); MAX_FS_LEVEL51*256/4096 = 2304

typedef void (*PWelsLogCallbackFunc) (void* pCtx, const int32_t iLevel, const char* kpFmt, va_list argv);

typedef struct TagLogContext {
  PWelsLogCallbackFunc pfLog;
  void* pLogCtx;
  void* pCodecInstance;
  } SLogContext;


#ifdef __GNUC__
extern void WelsLog (SLogContext* pCtx, int32_t iLevel, const char* kpFmt, ...) __attribute__ ((__format__ (__printf__,
    3,
    4)));
#else
  extern void WelsLog (SLogContext* pCtx, int32_t iLevel, const char* kpFmt, ...);
#endif
