// cAacdecoder.h
#pragma once
#include "iAudioDecoder.h"

// forward declarations for private data
class cBitStream;
struct sProgConfigElement;
struct sPSInfoBase;
struct sPSInfoSBR;

class cAacDecoder : public iAudioDecoder {
public:
  //{{{  defines
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
  //}}}
  cAacDecoder();
  ~cAacDecoder();

  int32_t getNumChannels() { return numChannels; }
  int32_t getSampleRate() { return sampleRate * (sbrEnabled ? 2 : 1); }
  int32_t getNumSamples() { return numSamples; }

  float* decodeFrame (const uint8_t* framePtr, int32_t frameLen, int32_t frameNum);

private:
  //{{{  private members
  void flush();

  void decodeSingleChannelElement (cBitStream* bsi);
  void decodeChannelPairElement (cBitStream* bsi);
  void decodeLFEChannelElement (cBitStream* bsi);
  void decodeDataStreamElement (cBitStream* bsi);
  void decodeProgramConfigElement (cBitStream* bsi, sProgConfigElement* pce);
  void decodeFillElement (cBitStream* bsi);
  bool decodeNextElement (uint8_t*& buf, int32_t& bitOffset, int32_t& bitsAvail);
  bool decodeSbrBitstream (int32_t chBase);

  void decodeNoiselessData (uint8_t*& buf, int32_t& bitOffset, int32_t& bitsAvail, int32_t channel);
  void dequantize (int32_t channel);
  void applyStereoProcess();
  void applyPns (int32_t channel);
  void applyTns (int32_t channel);
  void imdct (int32_t channel, int32_t chOut, float* outbuf);
  void applySbr (int32_t chBase, float* outbuf);

  bool unpackADTSHeader (uint8_t*& buf, int32_t& bitOffset, int32_t& bitsAvail);
  //}}}
  //{{{  private vars
  sPSInfoBase* psInfoBase;
  sPSInfoSBR* psInfoSBR;

  // raw decoded data
  void* rawSampleBuf [AAC_MAX_NCHANS];

  // fill data (can be used for processing SBR or other extensions)
  uint8_t* fillBuf;
  int32_t fillCount;
  int32_t fillExtType;

  // block information
  int32_t prevBlockID;
  int32_t currBlockID;
  int32_t currInstTag;
  int32_t sbDeinterleaveReqd [MAX_NCHANS_ELEM];
  int32_t adtsBlocksLeft;

  //  info
  int32_t bitRate;
  int32_t numChannels;
  int32_t sampleRate;
  int32_t profile;
  int32_t format;
  int32_t sbrEnabled;
  int32_t tnsUsed;
  int32_t pnsUsed;

  int32_t numSamples;

  int32_t mLastFrameNum = -1;
  //}}}
  };
