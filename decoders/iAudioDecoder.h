// iAudioDecoder.h
#pragma once
#include <stdint.h>

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm } ;

class iAudioDecoder {
public:
  virtual ~iAudioDecoder() {}

  virtual int32_t getNumChannels() = 0;
  virtual int32_t getSampleRate() = 0;
  virtual int32_t getNumSamplesPerFrame() = 0;

  virtual float* decodeFrame (const uint8_t* inbuf, int bytesLeft, int64_t pts) = 0;
  };
