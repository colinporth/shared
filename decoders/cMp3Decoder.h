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

  int32_t getNumChannels() { return channels; }
  int32_t getSampleRate() { return sampleRate; }
  int32_t decodeSingleFrame (uint8_t* inBuffer, int32_t bytesLeft, float* outBuffer);

private:
  //{{{  private members
  void clear();

  void decode (sScratch* s, sGranule* granule, int32_t nch);
  void save_reservoir (sScratch* s);
  int32_t restore_reservoir (class cBitStream* bitStream, sScratch* s, int32_t mainDataBegin);
  //}}}
  //{{{  private vars
  uint8_t header[4] = { 0 };
  int32_t channels = 0;
  int32_t sampleRate = 0;

  int32_t reservoir = 0;
  uint8_t reservoirBuf[511];

  float mdctOverlap[2][9*32];
  float qmfState[15*2*32];
  //}}}
  };
