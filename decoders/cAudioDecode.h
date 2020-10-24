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
  int32_t getNumChannels() { return mNumChannels; }
  int32_t getSampleRate() { return mSampleRate; }
  int32_t getNumSamplesPerFrame() { return mNumSamplesPerFrame; }

  uint8_t* getFramePtr() { return mFramePtr; }
  int32_t getFrameLen() { return mFrameLen; }
  int32_t getNextFrameOffset() { return mFrameLen + mSkip; }

  void setFrame (uint8_t* framePtr, int32_t frameLen);

  bool parseFrame (uint8_t* framePtr, uint8_t* frameLast);
  float* decodeFrame (int frameNum);
  float* decodeFrame (uint8_t* framePtr, int frameLen, int32_t frameNum, uint64_t pts);

  // !!!! statics not quite right, picks up eFrameType
  static eFrameType parseSomeFrames (uint8_t* framePtr, uint8_t* frameLast, int32_t& sampleRate);
  static eFrameType parseAllFrames (uint8_t* framePtr, uint8_t* frameLast, int32_t& sampleRate);

  // !!!! not right !!!!
  inline static uint8_t* mJpegPtr = nullptr;
  inline static int mJpegLen = 0;

private:
  static bool parseId3Tag (uint8_t* framePtr, uint8_t* frameLast);

  // vars
  uint8_t* mFramePtr = nullptr;
  int32_t mFrameLen = 0;
  int32_t mSkip = 0;

  eFrameType mFrameType = eUnknown;
  int32_t mNumChannels = 0;
  int32_t mSampleRate = 0;
  int32_t mNumSamplesPerFrame = 0;

  iAudioDecoder* mAudioDecoder = nullptr;
  };
