// cFFmpegAacdecoder.h
#pragma once
#include "iAudioDecoder.h"

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;

class cFFmpegAudioDecoder : public iAudioDecoder {
public:
  cFFmpegAudioDecoder (eAudioFrameType frameType);
  ~cFFmpegAudioDecoder();

  int32_t getNumChannels() { return mChannels; }
  int32_t getSampleRate() { return mSampleRate; }
  int32_t getNumSamplesPerFrame() { return mSamplesPerFrame; }

  float* decodeFrame (const uint8_t* framePtr, int frameLen, int64_t pts);

private:
  int32_t mChannels = 0;
  int32_t mSampleRate = 0;
  int32_t mSamplesPerFrame = 0;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;

  int64_t mLastPts = -1;
  };
