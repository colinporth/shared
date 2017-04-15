// cAacDecoder.h
#pragma once
#define AAC_MAX_NCHANS    2 // set to default max number of channels  */
#define AAC_MAX_NSAMPS    1024

#define AAC_NUM_PROFILES  3
#define AAC_PROFILE_MP    0
#define AAC_PROFILE_LC    1
#define AAC_PROFILE_SSR   2

//{{{  error enum
enum {
  ERR_AAC_NONE                          =   0,
  ERR_AAC_INDATA_UNDERFLOW              =  -1,
  ERR_AAC_NULL_POINTER                  =  -2,
  ERR_AAC_INVALID_ADTS_HEADER           =  -3,
  ERR_AAC_INVALID_ADIF_HEADER           =  -4,
  ERR_AAC_INVALID_FRAME                 =  -5,
  ERR_AAC_MPEG4_UNSUPPORTED             =  -6,
  ERR_AAC_CHANNEL_MAP                   =  -7,
  ERR_AAC_SYNTAX_ELEMENT                =  -8,

  ERR_AAC_DEQUANT                       =  -9,
  ERR_AAC_STEREO_PROCESS                = -10,
  ERR_AAC_PNS                           = -11,
  ERR_AAC_SHORT_BLOCK_DEINT             = -12,
  ERR_AAC_TNS                           = -13,
  ERR_AAC_IMDCT                         = -14,
  ERR_AAC_NCHANS_TOO_HIGH               = -15,

  ERR_AAC_SBR_INIT                      = -16,
  ERR_AAC_SBR_BITSTREAM                 = -17,
  ERR_AAC_SBR_DATA                      = -18,
  ERR_AAC_SBR_PCM_FORMAT                = -19,
  ERR_AAC_SBR_NCHANS_TOO_HIGH           = -20,
  ERR_AAC_SBR_SINGLERATE_UNSUPPORTED    = -21,

  ERR_AAC_RAWBLOCK_PARAMS               = -22,

  ERR_AAC_UNKNOWN           = -9999
};
//}}}
//{{{  struct AACFrameInfo
typedef struct _AACFrameInfo {
  int bitRate;
  int nChans;
  int sampRateCore;
  int sampRateOut;
  int outputSamps;
  int profile;
  int tnsUsed;
  int pnsUsed;
  } AACFrameInfo;
//}}}
class cBitStream;
typedef struct _PSInfoBase PSInfoBase;
typedef struct _PSInfoSBR PSInfoSBR;

class cAacDecoder {
public:
  cAacDecoder();
  ~cAacDecoder();

  int AACDecode (unsigned char** inbuf, int* bytesLeft, short* outbuf);

  int AACSetRawBlockParams (int copyLast, AACFrameInfo* aacFrameInfo);
  int AACFlushCodec();

  //sampRateOut = sampRate * (sbrEnabled ? 2 : 1);
  //outputSamps = nChans * AAC_MAX_NSAMPS * (sbrEnabled ? 2 : 1);
  //{{{  public vars
  int bitRate;
  int nChans;
  int sampRate;
  int profile;
  int format;
  int sbrEnabled;
  int tnsUsed;
  int pnsUsed;
  int frameCount;
  //}}}

private:
  int IMDCT (int ch, int chOut, short *outbuf);

  int DecodeSBRBitstream (int chBase);
  int DecodeSBRData (int chBase, short* outbuf);

  int StereoProcess();
  int Dequantize (int ch);
  int PNS (int ch);
  int TNSFilter (int ch);

  int DecodeSingleChannelElement (cBitStream& bitStream);
  int DecodeChannelPairElement (cBitStream& bitStream);
  int DecodeLFEChannelElement (cBitStream& bitStream);
  int DecodeDataStreamElement (cBitStream& bitStream);
  int DecodeFillElement (cBitStream& bitStream);

  int DecodeNoiselessData (unsigned char** buf, int* bitOffset, int* bitsAvail, int ch);
  int DecodeNextElement (unsigned char** buf, int* bitOffset, int* bitsAvail);
  int UnpackADIFHeader (unsigned char* *buf, int* bitOffset, int* bitsAvail);
  int SetRawBlockParams (int copyLast, int nChans, int sampRate, int profile);

  int UnpackADTSHeader (unsigned char** buf, int* bitOffset, int* bitsAvail);
  int GetADTSChannelMapping (unsigned char* buf, int bitOffset, int bitsAvail);
  int AACFindSyncWord (unsigned char* buf, int nBytes);

  //{{{  private vars
  PSInfoBase* psInfoBase; /* baseline MPEG-4 LC decoding */
  PSInfoSBR* psInfoSBR;  /* MPEG-4 SBR decoding */

  /* raw decoded data, before rounding to 16-bit PCM (for postprocessing such as SBR) */
  void* rawSampleBuf[AAC_MAX_NCHANS];
  int rawSampleBytes;
  int rawSampleFBits;

  /* fill data (can be used for processing SBR or other extensions) */
  unsigned char* fillBuf;
  int fillCount;
  int fillExtType;

  /* block information */
  int prevBlockID;
  int currBlockID;
  int currInstTag;
  int adtsBlocksLeft;
  //}}}
  };
