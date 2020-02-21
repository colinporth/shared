//{{{  includes
#include <windows.h>
#include <tchar.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "crt_util_safe_x.h" // Safe CRT routines like utils for cross platforms

#include "welsCodecTrace.h"
#include "utils.h"
//}}}

//{{{
static void welsStderrTrace (void* ctx, int level, const char* string) {
  fprintf (stderr, "%s\n", string);
  }
//}}}

//{{{
welsCodecTrace::welsCodecTrace() {

  m_iTraceLevel = WELS_LOG_DEFAULT;
  m_fpTrace = welsStderrTrace;
  m_pTraceCtx = NULL;

  m_sLogCtx.pLogCtx = this;
  m_sLogCtx.pfLog = StaticCodecTrace;
  m_sLogCtx.pCodecInstance = NULL;
}
//}}}
//{{{
welsCodecTrace::~welsCodecTrace() {
  m_fpTrace = NULL;
}
//}}}

//{{{
void welsCodecTrace::StaticCodecTrace (void* pCtx, const int32_t iLevel, const char* Str_Format, va_list vl) {
  welsCodecTrace* self = (welsCodecTrace*) pCtx;
  self->CodecTrace (iLevel, Str_Format, vl);
}
//}}}
//{{{
void welsCodecTrace::CodecTrace (const int32_t iLevel, const char* Str_Format, va_list vl) {

  if (m_iTraceLevel < iLevel) {
    return;
  }

  char pBuf[MAX_LOG_SIZE] = {0};
  WelsVsnprintf (pBuf, MAX_LOG_SIZE, Str_Format, vl); // confirmed_safe_unsafe_usage
  if (m_fpTrace) {
    m_fpTrace (m_pTraceCtx, iLevel, pBuf);
  }
}
//}}}

//{{{
void welsCodecTrace::SetCodecInstance (void* pCodecInstance) {
  m_sLogCtx.pCodecInstance = pCodecInstance;
}
//}}}

//{{{
void welsCodecTrace::SetTraceLevel (const int32_t iLevel) {
  if (iLevel >= 0)
    m_iTraceLevel = iLevel;
}
//}}}
//{{{
void welsCodecTrace::SetTraceCallback (WelsTraceCallback func) {
  m_fpTrace = func;
}
//}}}
//{{{
void welsCodecTrace::SetTraceCallbackContext (void* ctx) {
  m_pTraceCtx = ctx;
}
//}}}
