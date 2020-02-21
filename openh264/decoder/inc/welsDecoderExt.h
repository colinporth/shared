#pragma once
//{{{
#include "codec_api.h"
#include "codec_app_def.h"
#include "decoder_context.h"
#include "welsCodecTrace.h"
#include "cpu.h"
//}}}

class ISVCDecoder;
namespace WelsDec {

class CWelsDecoder : public ISVCDecoder {
public:
  CWelsDecoder (void);
  virtual ~CWelsDecoder();

  virtual long EXTAPI Initialize (const SDecodingParam* pParam);
  virtual long EXTAPI Uninitialize();

  virtual DECODING_STATE EXTAPI DecodeFrame (const unsigned char* kpSrc, const int kiSrcLen, unsigned char** ppDst,
      int* pStride,
      int& iWidth,
      int& iHeight);

  virtual DECODING_STATE EXTAPI DecodeFrameNoDelay (const unsigned char* kpSrc, const int kiSrcLen, unsigned char** ppDst,
      SBufferInfo* pDstInfo);

  virtual DECODING_STATE EXTAPI DecodeFrame2 (const unsigned char* kpSrc, const int kiSrcLen, unsigned char** ppDst,
      SBufferInfo* pDstInfo);
  virtual DECODING_STATE EXTAPI DecodeParser (const unsigned char* kpSrc, const int kiSrcLen, SParserBsInfo* pDstInfo);
  virtual DECODING_STATE EXTAPI DecodeFrameEx (const unsigned char* kpSrc, const int kiSrcLen, unsigned char* pDst,
      int iDstStride,
      int& iDstLen,
      int& iWidth,
      int& iHeight,
      int& color_format);

  virtual long EXTAPI SetOption (DECODER_OPTION eOptID, void* pOption);
  virtual long EXTAPI GetOption (DECODER_OPTION eOptID, void* pOption);

private:
  PWelsDecoderContext     m_pDecContext;
  welsCodecTrace*         m_pWelsTrace;

  int32_t InitDecoder (const SDecodingParam* pParam);
  void UninitDecoder (void);
  int32_t ResetDecoder();
 };

} // namespace WelsDec
