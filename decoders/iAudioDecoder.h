// iAudioDecoder.h
#pragma once
#include <stdint.h>

class iAudioDecoder {
public:
  virtual ~iAudioDecoder() {}

  virtual int getNumChannels() = 0;
  virtual int getSampleRate() = 0;
  virtual int decodeFrame (uint8_t* inbuf, int bytesLeft, float* outbuf, int frameNum) = 0;
  };
