// cFFmpegAacDecoder.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>

#include "cFFmpegAacDecoder.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}
constexpr bool kMoreLog = false;

cFFmpegAacDecoder::cFFmpegAacDecoder() {
  }
cFFmpegAacDecoder::~cFFmpegAacDecoder() {
  }

int32_t cFFmpegAacDecoder::getNumChannels() { return 2; }
int32_t cFFmpegAacDecoder::getSampleRate() { return 48000; }
int32_t cFFmpegAacDecoder::getNumSamplesPerFrame() { return 1024; }

//{{{
float* cFFmpegAacDecoder::decodeFrame (const uint8_t* framePtr, int32_t frameLen, int32_t frameNum) {

  float* outBuffer = (float*)malloc (2 * 1024 * sizeof(float));
  return outBuffer;
  }
//}}}
