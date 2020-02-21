
#ifndef WELS_CODEC_TRACE
#define WELS_CODEC_TRACE

#include <stdarg.h>
#include "typedefs.h"
#include "utils.h"
#include "codec_app_def.h"
#include "codec_api.h"

class welsCodecTrace {
 public:
  welsCodecTrace();
  ~welsCodecTrace();

  void SetCodecInstance (void* pCodecInstance);
  void SetTraceLevel (const int32_t kiLevel);
  void SetTraceCallback (WelsTraceCallback func);
  void SetTraceCallbackContext (void* pCtx);

 private:
  static void StaticCodecTrace (void* pCtx, const int32_t kiLevel, const char* kpStrFormat, va_list vl);
  void CodecTrace (const int32_t kiLevel, const char* kpStrFormat, va_list vl);

  int32_t       m_iTraceLevel;
  WelsTraceCallback m_fpTrace;
  void*         m_pTraceCtx;
 public:

  SLogContext m_sLogCtx;
};

#endif //WELS_CODEC_TRACE
