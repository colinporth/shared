// iAudioDecoder.h
#pragma once

class iAudioDecoder {
public:
  virtual ~iAudioDecoder() {}
  virtual int getSampleRate() = 0;
  virtual int decode (uint8_t* inbuf, int bytesLeft, float* outbuf) = 0;
  };
