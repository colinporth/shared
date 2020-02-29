// aacdec.h
#pragma once

// according to spec (13818-7 section 8.2.2, 14496-3 section 4.5.3)
// max size of input buffer =
//    6144 bits =  768 bytes per SCE or CCE-I
//   12288 bits = 1536 bytes per CPE
//       0 bits =    0 bytes per CCE-D (uses bits from the SCE/CPE/CCE-I it is coupled to)
#define AAC_MAX_NCHANS    2 /* set to default max number of channels  */
#define MAX_NCHANS_ELEM   2 /* max number of channels in any single bitstream element (SCE,CPE,CCE,LFE) */

#define AAC_MAX_NSAMPS    1024
#define AAC_MAINBUF_SIZE  (768 * AAC_MAX_NCHANS)

#define AAC_PROFILE_MP    0
#define AAC_PROFILE_LC    1
#define AAC_PROFILE_SSR   2
#define AAC_NUM_PROFILES  3

struct PSInfoBase;
struct PSInfoSBR;

class cAacDecoder {
public:
  //{{{
  struct BitStreamInfo {
    uint8_t* bytePtr;
    uint32_t iCache;
    int      cachedBits;
    int      nBytes;
    };
  //}}}
  //{{{
  /* sizeof(ProgConfigElement) = 82 bytes (if KEEP_PCE_COMMENTS not defined) */
  struct ProgConfigElement {
    #define MAX_NUM_FCE        15
    #define MAX_NUM_SCE        15
    #define MAX_NUM_BCE        15
    #define MAX_NUM_LCE        3
    #define MAX_NUM_ADE        7
    #define MAX_NUM_CCE        15
    #define MAX_COMMENT_BYTES  255

    uint8_t elemInstTag;        /* element instance tag */
    uint8_t profile;            /* 0 = main, 1 = LC, 2 = SSR, 3 = reserved */
    uint8_t sampRateIdx;        /* sample rate index range = [0, 11] */
    uint8_t numFCE;             /* number of front channel elements (max = 15) */
    uint8_t numSCE;             /* number of side channel elements (max = 15) */
    uint8_t numBCE;             /* number of back channel elements (max = 15) */
    uint8_t numLCE;             /* number of LFE channel elements (max = 3) */
    uint8_t numADE;             /* number of associated data elements (max = 7) */
    uint8_t numCCE;             /* number of valid channel coupling elements (max = 15) */
    uint8_t monoMixdown;        /* mono mixdown: bit 4 = present flag, bits 3-0 = element number */
    uint8_t stereoMixdown;      /* stereo mixdown: bit 4 = present flag, bits 3-0 = element number */
    uint8_t matrixMixdown;      /* matrix mixdown: bit 4 = present flag, bit 3 = unused,
                                         bits 2-1 = index, bit 0 = pseudo-surround enable */
    uint8_t fce [MAX_NUM_FCE];  /* front element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
    uint8_t sce [MAX_NUM_SCE];  /* side element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
    uint8_t bce [MAX_NUM_BCE];  /* back element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
    uint8_t lce [MAX_NUM_LCE];  /* instance tag for LFE elements */
    uint8_t ade [MAX_NUM_ADE];  /* instance tag for ADE elements */
    uint8_t cce [MAX_NUM_BCE];  /* channel coupling elements: bit 4 = switching flag, bits 3-0 = inst tag */

    uint8_t commentBytes;
    uint8_t commentField [MAX_COMMENT_BYTES];
    };
  //}}}
  cAacDecoder();
  ~cAacDecoder();

  int getSampleRate();
  int decode (uint8_t* inbuf, int bytesLeft, float* outbuf);
  int flushCodec();

private:
  int Dequantize (int ch);
  int DeinterleaveShortBlocks (int ch);
  int PNS (int ch);
  int TNSFilter (int ch);
  int StereoProcess();
  int DecodeNoiselessData (uint8_t** buf, int* bitOffset, int* bitsAvail, int ch);
  int UnpackADTSHeader (uint8_t** buf, int* bitOffset, int* bitsAvail);
  int GetADTSChannelMapping (uint8_t* buf, int bitOffset, int bitsAvail);
  int IMDCT (int ch, int chOut, float* outbuf);
  int DecodeSBRData (int chBase, float* outbuf);

  int decodeSingleChannelElement (BitStreamInfo* bsi);
  int decodeChannelPairElement (BitStreamInfo* bsi);
  int decodeLFEChannelElement (BitStreamInfo* bsi);
  int decodeDataStreamElement (BitStreamInfo* bsi);
  int decodeProgramConfigElement (ProgConfigElement* pce, BitStreamInfo* bsi);
  int decodeFillElement (BitStreamInfo* bsi);
  int decodeNextElement (uint8_t** buf, int* bitOffset, int* bitsAvail);
  int DecodeSBRBitstream (int chBase);

  // private vars
  PSInfoBase* psInfoBase;
  PSInfoSBR* psInfoSBR;

  // raw decoded data
  void* rawSampleBuf [AAC_MAX_NCHANS];

  // fill data (can be used for processing SBR or other extensions)
  uint8_t* fillBuf;
  int fillCount;
  int fillExtType;

  // block information
  int prevBlockID;
  int currBlockID;
  int currInstTag;
  int sbDeinterleaveReqd [MAX_NCHANS_ELEM];
  int adtsBlocksLeft;

  // user-accessible info
  int bitRate;
  int nChans;
  int sampRate;
  int profile;
  int format;
  int sbrEnabled;
  int tnsUsed;
  int pnsUsed;
  };
