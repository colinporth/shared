#pragma once
#include "iAudioDecoder.h"

class cAudioDecode {
public:
  static iAudioDecoder* createAudioDecoder (iAudioDecoder::eFrameType frameType);

  static uint8_t* getJpeg (int& len);

  static uint8_t* parseFrame (uint8_t* framePtr, uint8_t* frameLast, int& frameLength);
  static iAudioDecoder::eFrameType parseSomeFrames (uint8_t* framePtr, uint8_t* frameLast, int& sampleRate);

private:
  static bool parseId3Tag (uint8_t* framePtr, uint8_t* frameEnd);
  static uint8_t* parseFrame (uint8_t* framePtr, uint8_t* frameLast,
                              iAudioDecoder::eFrameType& frameType, int& sampleRate, int& frameLength);
  };
