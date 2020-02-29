// iAudioDecoder.h
#pragma once

class iAudioDecoder {
public:
  virtual ~iAudioDecoder() {}

  // sampleRate may change after decode for aac HE
  virtual int getSampleRate() = 0;

  virtual int decodeSingleFrame (uint8_t* inbuf, int bytesLeft, float* outbuf) = 0;
  };
