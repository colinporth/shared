// cMp3Decoder.h
#pragma once
#include "iAudioDecoder.h"

class cBitStream;
struct sScratch;
struct sGranule;

class cMp3Decoder : public iAudioDecoder {
public:
  cMp3Decoder();
  ~cMp3Decoder();

  int32_t getNumChannels() { return mNumChannels; }
  int32_t getSampleRate() { return mSampleRate; }
  int32_t decodeSingleFrame (uint8_t* inBuffer, int32_t bytesLeft, float* outBuffer);

private:
  //{{{  private members
  void clear();

  void decode (sScratch* s, sGranule* granule, int32_t nch);
  void save_reservoir (sScratch* s);
  int32_t restore_reservoir (class cBitStream* bitStream, sScratch* s, int32_t mainDataBegin);
  //}}}
  //{{{  private vars
  uint8_t mHeader[4] = { 0 };
  int32_t mNumChannels = 0;
  int32_t mSampleRate = 0;

  int32_t mReservoir = 0;
  uint8_t mReservoirBuffer[511];

  float mMdctOverlap[2][9*32];
  float mQmfState[15*2*32];
  //}}}
  };
