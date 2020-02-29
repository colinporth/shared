// minimp3.h
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "iAudioDecoder.h"

class cMp3Decoder : public iAudioDecoder {
public:
  //{{{  defines
  #define MAX_FREE_FORMAT_FRAME_SIZE  2304  /* more than ISO spec's */
  #define MAX_L3_FRAME_PAYLOAD_BYTES  MAX_FREE_FORMAT_FRAME_SIZE /* MUST be >= 320000/8/32000*1152 = 1440 */
  #define MAX_BITRESERVOIR_BYTES      511
  //}}}
  //{{{  struct sBitStream
  typedef struct {
    const uint8_t* buf;
    int pos;
    int limit;
    } sBitStream;
  //}}}
  //{{{  struct L3_gr_info_t
  typedef struct {
    const uint8_t* sfbtab;
    uint16_t part_23_length;
    uint16_t big_values;
    uint16_t scalefac_compress;
    uint8_t  global_gain;
    uint8_t  block_type;
    uint8_t  mixed_block_flag;
    uint8_t  n_long_sfb;
    uint8_t  n_short_sfb;
    uint8_t  table_select [3];
    uint8_t  region_count [3];
    uint8_t  subblock_gain [3];
    uint8_t  preflag;
    uint8_t  scalefac_scale;
    uint8_t  count1_table;
    uint8_t  scfsi;
    } L3_gr_info_t;
  //}}}
  //{{{  struct mp3dec_scratch_t
  typedef struct {
    sBitStream   bs;
    uint8_t      maindata [MAX_BITRESERVOIR_BYTES + MAX_L3_FRAME_PAYLOAD_BYTES];
    L3_gr_info_t gr_info [4];
    float        grbuf [2][576];
    float        scf [40];
    float        syn [18 + 15][2*32];
    uint8_t      ist_pos [2][39];
    } mp3dec_scratch_t;
  //}}}

  cMp3Decoder();
  int getSampleRate();
  int decode (uint8_t* inbuf, int bytesLeft, float* outbuf);

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

  int sampleRate = 0;
  //}}}
  };
