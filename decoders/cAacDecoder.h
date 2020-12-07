// cAacdecoder.h
#pragma once
#include "iAudioDecoder.h"

// forward declarations for private data
class cBitStream;
struct sProgConfigElement;
struct sInfoBase;
struct sInfoSbr;

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

  int32_t getNumChannels() { return mNumChannels; }
  int32_t getSampleRate() { return mSampleRate * (mSbrEnabled ? 2 : 1); }
  int32_t getNumSamplesPerFrame() { return mNumSamples; }

  float* decodeFrame (const uint8_t* framePtr, int frameLen, int64_t pts);

private:
  //{{{  private members
  void initSbrState();
  void flush();

  void decodeSingleChannelElement (cBitStream* bsi);
  void decodeChannelPairElement (cBitStream* bsi);
  void decodeLFEChannelElement (cBitStream* bsi);
  void decodeDataStreamElement (cBitStream* bsi);
  void decodeProgramConfigElement (cBitStream* bsi, sProgConfigElement* pce);
  void decodeFillElement (cBitStream* bsi);

  bool unpackADTSHeader (uint8_t*& buffer, int32_t& bitOffset, int32_t& bitsAvail);
  bool decodeNextElement (uint8_t*& buffer, int32_t& bitOffset, int32_t& bitsAvail);
  void decodeNoiselessData (uint8_t*& buffer, int32_t& bitOffset, int32_t& bitsAvail, int32_t channel);
  bool decodeSbrBitstream (int32_t chBase);

  void dequantize (int32_t channel);
  void applyStereoProcess();
  void applyPns (int32_t channel);
  void applyTns (int32_t channel);
  void imdct (int32_t channel, int32_t chOut, float* outBuffer);
  void applySbr (int32_t chBase, float* outBuffer);
  //}}}
  //{{{  private vars
  sInfoBase* mInfoBase;
  sInfoSbr* mInfoSbr;

  // raw decoded data
  void* mRawSampleBuf [AAC_MAX_NCHANS];

  // block information
  int32_t mPrevBlockID;
  int32_t mCurrBlockID;
  int32_t mCurrInstTag;
  uint8_t* mFillBuf;
  int32_t mFillCount;
  int32_t mFillExtType;

  //  info
  int32_t mBitRate;
  int32_t mNumChannels;
  int32_t mSampleRate;
  int32_t mProfile;
  int32_t mFormat;
  int32_t mSbrEnabled;
  int32_t mTnsUsed;
  int32_t mPnsUsed;

  int32_t mNumSamples;

  int64_t mLastPts = -1;
  //}}}
  };
