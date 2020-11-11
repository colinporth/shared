#pragma once
#include "iAudioDecoder.h"

class cAudioParser {
public:
  static uint8_t* getJpeg (int& len);

  static uint8_t* parseFrame (uint8_t* framePtr, uint8_t* frameLast, int& frameLength);
  static eAudioFrameType parseSomeFrames (uint8_t* framePtr, uint8_t* frameLast, int& numChannels, int& sampleRate);

private:
  static bool parseId3Tag (uint8_t* framePtr, uint8_t* frameEnd);
  static uint8_t* parseFrame (uint8_t* framePtr, uint8_t* frameLast,
                              eAudioFrameType& frameType, int& numChannels, int& sampleRate, int& frameLength);
  };
