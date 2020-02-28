// minimp3.h
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//{{{
#ifdef __cplusplus
  extern "C" {
#endif
//}}}

// struct mp3dec_t
typedef struct {
  uint8_t header[4];
  int free_format_bytes;
  int reserv;
  uint8_t reserv_buf[511];
  float mdct_overlap[2][9*32];
  float qmf_state[15*2*32];
  } mp3dec_t;

// struct mp3dec_frame_info_t
typedef struct {
  int frame_bytes;
  int channels;
  int hz;
  int layer;
  int bitrate_kbps;
  } mp3dec_frame_info_t;

void mp3dec_init (mp3dec_t* dec);
int mp3dec_decode_frame (mp3dec_t* dec, const uint8_t* mp3, int mp3_bytes, float* samples, mp3dec_frame_info_t* info);

//{{{
#ifdef __cplusplus
  }
#endif
//}}}
