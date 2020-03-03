// cMp3Decoder.h
#pragma once
#include "iAudioDecoder.h"

struct sBitStream;
struct L3_gr_info_t;
struct sScratch;

class cMp3Decoder : public iAudioDecoder {
public:
  cMp3Decoder();
  ~cMp3Decoder();

  int32_t getNumChannels() { return channels; }
  int32_t getSampleRate() { return sampleRate; }
  int32_t decodeSingleFrame (uint8_t* inbuf, int32_t bytesLeft, float* outbuf);

private:
  //{{{  private members
  void clear();

  void L3_decode (sScratch* s, L3_gr_info_t* gr_info, int32_t nch);
  void L3_save_reservoir (sScratch* s);
  int32_t L3_restore_reservoir (struct sBitStream* bs, sScratch* s, int32_t main_data_begin);
  //}}}
  //{{{  private vars
  uint8_t header[4] = { 0 };

  int32_t free_format_bytes = 0;

  int32_t reserv = 0;
  uint8_t reserv_buf[511];

  float mdct_overlap[2][9*32];
  float qmf_state[15*2*32];

  int32_t channels = 0;
  int32_t sampleRate = 0;
  //}}}
  };
