// iAudioDecoder.h
#pragma once

class iAudioDecoder {
public:
  virtual int decode (uint8_t* inbuf, int bytesLeft, float* outbuf) = 0;
  };
