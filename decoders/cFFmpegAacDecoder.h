// cFFmpegAacdecoder.h
#pragma once
#include "iAudioDecoder.h"

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;

class cFFmpegAacDecoder : public iAudioDecoder {
public:
  cFFmpegAacDecoder (eAudioFrameType frameType);
  ~cFFmpegAacDecoder();

  int32_t getNumChannels() { return mChannels; }
  int32_t getSampleRate() { return mSampleRate; }
  int32_t getNumSamplesPerFrame() { return mSamplesPerFrame; }

  float* decodeFrame (const uint8_t* framePtr, int32_t frameLen, int32_t frameNum);

private:
  int32_t mChannels = 0;
  int32_t mSampleRate = 0;
  int32_t mSamplesPerFrame = 0;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;
  };
