// cAacdecoder.h
#pragma once
#include "iAudioDecoder.h"

class cFFmpegAacDecoder : public iAudioDecoder {
public:
  //{{{  defines
  // according to spec (13818-7 section 8.2.2, 14496-3 section 4.5.3)
  // max size of input buffer =
  //    6144 bits =  768 bytes per SCE or CCE-I
  //   12288 bits = 1536 bytes per CPE
  //       0 bits =    0 bytes per CCE-D (uses bits from the SCE/CPE/CCE-I it is coupled to)
  #define AAC_MAX_NCHANS    2 /* set to default max number of channels  */
  #define MAX_NCHANS_ELEM   2 /* max number of channels in any single bitstream element (SCE,CPE,CCE,LFE) */

  #define AAC_MAX_NSAMPS    1024
  #define AAC_MAINBUF_SIZE  (768 * AAC_MAX_NCHANS)

  #define AAC_PROFILE_MP    0
  #define AAC_PROFILE_LC    1
  #define AAC_PROFILE_SSR   2
  #define AAC_NUM_PROFILES  3
  //}}}
  cFFmpegAacDecoder();
  ~cFFmpegAacDecoder();

  int32_t getNumChannels();
  int32_t getSampleRate();
  int32_t getNumSamplesPerFrame();

  float* decodeFrame (const uint8_t* framePtr, int32_t frameLen, int32_t frameNum);

private:
  };
