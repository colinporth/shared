// iAudioDecoder.h
#pragma once
#include <stdint.h>

class iAudioDecoder {
public:
  virtual ~iAudioDecoder() {}

  virtual int32_t getNumChannels() = 0;
  virtual int32_t getSampleRate() = 0;
  virtual int32_t getNumSamples() = 0;
  virtual float* decodeFrame (uint8_t* inbuf, int bytesLeft, int frameNum) = 0;
  };
