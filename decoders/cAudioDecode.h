// cAudioDecode.h
#pragma once
#include "iAudioDecoder.h"

class cAudioDecode {
public:
  enum eFrameType { eUnknown, eId3Tag, eWav, eMp3, eAac } ;

  cAudioDecode() {}
  cAudioDecode (eFrameType frameType);
  ~cAudioDecode();

  // gets
  eFrameType getFrameType() { return mFrameType; }
  int getNumChannels() { return mNumChannels; }
  int getSampleRate() { return mSampleRate; }

  uint8_t* getFramePtr() { return mFramePtr; }
  int getFrameLen() { return mFrameLen; }
  int getNextFrameOffset() { return mFrameLen + mSkip; }

  void setFrame (uint8_t* framePtr, int frameLen);

  bool parseFrame (uint8_t* framePtr, uint8_t* frameLast);
  int decodeFrame (float* samples, bool jumped);

  // !!!! statics not quite right, picks up eFrameType
  static eFrameType parseSomeFrames (uint8_t* framePtr, uint8_t* frameLast, int& sampleRate);
  static eFrameType parseAllFrames (uint8_t* framePtr, uint8_t* frameLast, int& sampleRate);

  // !!!! not right !!!!
  inline static uint8_t* mJpegPtr = nullptr;
  inline static int mJpegLen = 0;

private:
  static bool parseId3Tag (uint8_t* framePtr, uint8_t* frameLast);

  // vars
  uint8_t* mFramePtr = nullptr;
  int mFrameLen = 0;
  int mSkip = 0;

  eFrameType mFrameType = eUnknown;
  int mNumChannels = 0;
  int mSampleRate = 0;

  iAudioDecoder* mAudioDecoder = nullptr;
  };
