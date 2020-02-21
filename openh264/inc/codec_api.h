#pragma once
//{{{  includes
#ifndef __cplusplus
  #include <stdbool.h>
#endif
#include "codec_app_def.h"
#include "codec_def.h"
//}}}
#define EXTAPI __cdecl
//{{{
/**   * This page is for openh264 codec API usage.
  *   * For how to use the encoder,please refer to page UsageExampleForEncoder
  *   * For how to use the decoder,please refer to page UsageExampleForDecoder
  *   * For more detail about ISVEncoder,please refer to page ISVCEnoder
  *   * For more detail about ISVDecoder,please refer to page ISVCDecoder
  *
  * Step 1:decoder declaration
  *  //decoder declaration
  *  ISVCDecoder *pSvcDecoder;
  *  //input: encoded bitstream start position; should include start code prefix
  *  unsigned char *pBuf =...;
  *  //input: encoded bit stream length; should include the size of start code prefix
  *  int iSize =...;
  *  //output: [0~2] for Y,U,V buffer for Decoding only
  *  unsigned char *pData[3] =...;
  *  //in-out: for Decoding only: declare and initialize the output buffer info, this should never co-exist with Parsing only
  *  SBufferInfo sDstBufInfo;
  *  memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
  *  //in-out: for Parsing only: declare and initialize the output bitstream buffer info for parse only, this should never co-exist with Decoding only
  *  SParserBsInfo sDstParseInfo;
  *  memset(&sDstParseInfo, 0, sizeof(SParserBsInfo));
  *  sDstParseInfo.pDstBuff = new unsigned char[PARSE_SIZE]; //In Parsing only, allocate enough buffer to save transcoded bitstream for a frame
  *
  * Step 2:decoder creation
  *  CreateDecoder(pSvcDecoder);
  *
  * Step 3:declare required parameter, used to differentiate Decoding only and Parsing only
  *  SDecodingParam sDecParam = {0};
  *  sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
  *  //for Parsing only, the assignment is mandatory
  *  sDecParam.bParseOnly = true;
  *
  * Step 4:initialize the parameter and decoder context, allocate memory
  *  Initialize(&sDecParam);
  *
  * Step 5:do actual decoding process in slice level; this can be done in a loop until data ends
  *  //for Decoding only
  *  iRet = DecodeFrameNoDelay(pBuf, iSize, pData, &sDstBufInfo);
  *  //or
  *  iRet = DecodeFrame2(pBuf, iSize, pData, &sDstBufInfo);
  *  //for Parsing only
  *  iRet = DecodeParser(pBuf, iSize, &sDstParseInfo);
  *  //decode failed
  *  If (iRet != 0){
  *      RequestIDR or something like that.
  *  }
  *  //for Decoding only, pData can be used for render.
  *  if (sDstBufInfo.iBufferStatus==1){
  *      output pData[0], pData[1], pData[2];
  *  }
  * //for Parsing only, sDstParseInfo can be used for, e.g., HW decoding
  *  if (sDstBufInfo.iNalNum > 0){
  *      Hardware decoding sDstParseInfo;
  *  }
  *  //no-delay decoding can be realized by directly calling DecodeFrameNoDelay(), which is the recommended usage.
  *  //no-delay decoding can also be realized by directly calling DecodeFrame2() again with NULL input, as in the following. In this case, decoder would immediately reconstruct the input data. This can also be used similarly for Parsing only. Consequent decoding error and output indication should also be considered as above.
  *  iRet = DecodeFrame2(NULL, 0, pData, &sDstBufInfo);
  *  judge iRet, sDstBufInfo.iBufferStatus ...

  * Step 6:uninitialize the decoder and memory free
  *  Uninitialize();
  *
  * Step 7:destroy the decoder
  *  DestroyDecoder();
*/
//}}}
#ifdef __cplusplus
  //{{{
  class ISVCDecoder {
   public:
    virtual long EXTAPI Initialize (const SDecodingParam* pParam) = 0;
    virtual long EXTAPI Uninitialize() = 0;
    //{{{
    /**
    * @brief   Decode one frame
    * @param   pSrc the h264 stream to be decoded
    * @param   iSrcLen the length of h264 stream
    * @param   ppDst buffer pointer of decoded data (YUV)
    * @param   pStride output stride
    * @param   iWidth output width
    * @param   iHeight output height
    * @return  0 - success; otherwise -failed;
    */
    virtual DECODING_STATE EXTAPI DecodeFrame (const unsigned char* pSrc,
        const int iSrcLen,
        unsigned char** ppDst,
        int* pStride,
        int& iWidth,
        int& iHeight) = 0;
    //}}}
    //{{{
    /**
    * @brief    For slice level DecodeFrameNoDelay() (4 parameters input),
    *           whatever the function return value is, the output data
    *           of I420 format will only be available when pDstInfo->iBufferStatus == 1,.
    *           This function will parse and reconstruct the input frame immediately if it is complete
    *           It is recommended as the main decoding function for H.264/AVC format input
    * @param   pSrc the h264 stream to be decoded
    * @param   iSrcLen the length of h264 stream
    * @param   ppDst buffer pointer of decoded data (YUV)
    * @param   pDstInfo information provided to API(width, height, etc.)
    * @return  0 - success; otherwise -failed;
    */
    virtual DECODING_STATE EXTAPI DecodeFrameNoDelay (const unsigned char* pSrc,
        const int iSrcLen,
        unsigned char** ppDst,
        SBufferInfo* pDstInfo) = 0;
    //}}}
    //{{{
    /**
    * @brief    For slice level DecodeFrame2() (4 parameters input),
    *           whatever the function return value is, the output data
    *           of I420 format will only be available when pDstInfo->iBufferStatus == 1,.
    *           (e.g., in multi-slice cases, only when the whole picture
    *           is completely reconstructed, this variable would be set equal to 1.)
    * @param   pSrc the h264 stream to be decoded
    * @param   iSrcLen the length of h264 stream
    * @param   ppDst buffer pointer of decoded data (YUV)
    * @param   pDstInfo information provided to API(width, height, etc.)
    * @return  0 - success; otherwise -failed;
    */
    virtual DECODING_STATE EXTAPI DecodeFrame2 (const unsigned char* pSrc,
        const int iSrcLen,
        unsigned char** ppDst,
        SBufferInfo* pDstInfo) = 0;
    //}}}
    //{{{
    /**
    * @brief   This function parse input bitstream only, and rewrite possible SVC syntax to AVC syntax
    * @param   pSrc the h264 stream to be decoded
    * @param   iSrcLen the length of h264 stream
    * @param   pDstInfo bit stream info
    * @return  0 - success; otherwise -failed;
    */
    virtual DECODING_STATE EXTAPI DecodeParser (const unsigned char* pSrc,
        const int iSrcLen,
        SParserBsInfo* pDstInfo) = 0;
    //}}}
    //{{{
    /**
    * @brief   This API does not work for now!! This is for future use to support non-I420 color format output.
    * @param   pSrc the h264 stream to be decoded
    * @param   iSrcLen the length of h264 stream
    * @param   pDst buffer pointer of decoded data (YUV)
    * @param   iDstStride output stride
    * @param   iDstLen bit stream info
    * @param   iWidth output width
    * @param   iHeight output height
    * @param   iColorFormat output color format
    * @return  to do ...
    */
    virtual DECODING_STATE EXTAPI DecodeFrameEx (const unsigned char* pSrc,
        const int iSrcLen,
        unsigned char* pDst,
        int iDstStride,
        int& iDstLen,
        int& iWidth,
        int& iHeight,
        int& iColorFormat) = 0;
    //}}}
    //{{{
    /**
    * @brief   Set option for decoder, detail option type, please refer to enumurate DECODER_OPTION.
    * @param   pOption  option for decoder such as OutDataFormat, Eos Flag, EC method, ...
    * @return  CM_RETURN: 0 - success; otherwise - failed;
    */
    virtual long EXTAPI SetOption (DECODER_OPTION eOptionId, void* pOption) = 0;
    //}}}
    //{{{
    /**
    * @brief   Get option for decoder, detail option type, please refer to enumurate DECODER_OPTION.
    * @param   pOption  option for decoder such as OutDataFormat, Eos Flag, EC method, ...
    * @return  CM_RETURN: 0 - success; otherwise - failed;
    */
    virtual long EXTAPI GetOption (DECODER_OPTION eOptionId, void* pOption) = 0;
    //}}}
    virtual ~ISVCDecoder() {}
    };
  //}}}

  extern "C" {
#else
  typedef struct ISVCDecoderVtbl ISVCDecoderVtbl;
  typedef const ISVCDecoderVtbl* ISVCDecoder;
  struct ISVCDecoderVtbl {
  long (*Initialize) (ISVCDecoder*, const SDecodingParam* pParam);
  long (*Uninitialize) (ISVCDecoder*);
  //{{{
  DECODING_STATE (*DecodeFrame) (ISVCDecoder*, const unsigned char* pSrc,
                                 const int iSrcLen,
                                 unsigned char** ppDst,
                                 int* pStride,
                                 int* iWidth,
                                 int* iHeight);
  //}}}
  //{{{
  DECODING_STATE (*DecodeFrameNoDelay) (ISVCDecoder*, const unsigned char* pSrc,
                                  const int iSrcLen,
                                  unsigned char** ppDst,
                                  SBufferInfo* pDstInfo);
  //}}}
  //{{{
  DECODING_STATE (*DecodeFrame2) (ISVCDecoder*, const unsigned char* pSrc,
                                  const int iSrcLen,
                                  unsigned char** ppDst,
                                  SBufferInfo* pDstInfo);
  //}}}
  //{{{
  DECODING_STATE (*DecodeParser) (ISVCDecoder*, const unsigned char* pSrc,
                                  const int iSrcLen,
                                  SParserBsInfo* pDstInfo);
  //}}}
  //{{{
  DECODING_STATE (*DecodeFrameEx) (ISVCDecoder*, const unsigned char* pSrc,
                                   const int iSrcLen,
                                   unsigned char* pDst,
                                   int iDstStride,
                                   int* iDstLen,
                                   int* iWidth,
                                   int* iHeight,
                                   int* iColorFormat);
  //}}}
  long (*SetOption) (ISVCDecoder*, DECODER_OPTION eOptionId, void* pOption);
  long (*GetOption) (ISVCDecoder*, DECODER_OPTION eOptionId, void* pOption);
  };
#endif

typedef void (*WelsTraceCallback) (void* ctx, int level, const char* string);

int WelsGetDecoderCapability (SDecoderCapability* pDecCapability);
long WelsCreateDecoder (ISVCDecoder** ppDecoder);
void WelsDestroyDecoder (ISVCDecoder* pDecoder);
OpenH264Version WelsGetCodecVersion (void);
void WelsGetCodecVersionEx (OpenH264Version *pVersion);

#ifdef __cplusplus
  }
#endif
