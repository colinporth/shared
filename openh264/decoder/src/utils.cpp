//{{{
#include "utils.h"
#include "crt_util_safe_x.h" // Safe CRT routines like utils for cross platforms
#include "codec_app_def.h"
//}}}

//{{{
void WelsLog (SLogContext* logCtx, int32_t iLevel, const char* kpFmt, ...) {

  va_list vl;
  char pTraceTag[MAX_LOG_SIZE] = {0};

  switch (iLevel) {
    case WELS_LOG_ERROR:
      WelsSnprintf (pTraceTag, MAX_LOG_SIZE, "[OpenH264] this = 0x%p, Error:", logCtx->pCodecInstance);
      break;
    case WELS_LOG_WARNING:
      WelsSnprintf (pTraceTag, MAX_LOG_SIZE, "[OpenH264] this = 0x%p, Warning:", logCtx->pCodecInstance);
      break;
    case WELS_LOG_INFO:
      WelsSnprintf (pTraceTag, MAX_LOG_SIZE, "[OpenH264] this = 0x%p, Info:", logCtx->pCodecInstance);
      break;
    case WELS_LOG_DEBUG:
      WelsSnprintf (pTraceTag, MAX_LOG_SIZE, "[OpenH264] this = 0x%p, Debug:", logCtx->pCodecInstance);
      break;
    default:
      WelsSnprintf (pTraceTag, MAX_LOG_SIZE, "[OpenH264] this = 0x%p, Detail:", logCtx->pCodecInstance);
      break;
    }

  WelsStrcat (pTraceTag, MAX_LOG_SIZE, kpFmt);
  va_start (vl, kpFmt);

  logCtx->pfLog (logCtx->pLogCtx, iLevel, pTraceTag, vl);
  va_end (vl);
  }
//}}}
