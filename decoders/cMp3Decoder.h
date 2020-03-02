// cMp3Decoder.h
#pragma once
#include "iAudioDecoder.h"

struct sBitStream;
struct L3_gr_info_t;
struct mp3dec_scratch_t;

class cMp3Decoder : public iAudioDecoder {
public:
  cMp3Decoder();
  ~cMp3Decoder();

  int getNumChannels() { return channels; }
  int getSampleRate() { return sampleRate; }
  int decodeSingleFrame (uint8_t* inbuf, int bytesLeft, float* outbuf);

private:
  //{{{  private members
  void clear();

  void L3_decode (mp3dec_scratch_t* s, L3_gr_info_t* gr_info, int nch);
  void L3_save_reservoir (mp3dec_scratch_t* s);
  int L3_restore_reservoir (sBitStream* bs, mp3dec_scratch_t* s, int main_data_begin);
  //}}}
  //{{{  private vars
  uint8_t header[4] = { 0 };

  int free_format_bytes = 0;

  int reserv = 0;
  uint8_t reserv_buf[511];

  float mdct_overlap[2][9*32];
  float qmf_state[15*2*32];

  int channels = 0;
  int sampleRate = 0;
  //}}}
  };
