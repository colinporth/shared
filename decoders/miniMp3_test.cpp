#define _CRT_SECURE_NO_WARNINGS

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#define MINIMP3_SKIP_ID3V1
//#define MINIMP3_ONLY_SIMD
//#define MINIMP3_NONSTANDARD_BUT_LOGICAL
#include "miniMp3_ex.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

//{{{
static char* wav_header (int hz, int ch, int bips, int data_bytes) {

  static char hdr[45] = "RIFFsizeWAVEfmt \x10\0\0\0\1\0ch_hz_abpsbabsdatasize";
  unsigned long nAvgBytesPerSec = bips*ch*hz >> 3;
  unsigned int nBlockAlign      = bips*ch >> 3;

  *(int32_t *)(void*)(hdr + 0x04) = 44 + data_bytes - 8;   /* File size - 8 */
  *(int16_t *)(void*)(hdr + 0x14) = 1;                     /* Integer PCM format */
  *(int16_t *)(void*)(hdr + 0x16) = ch;
  *(int32_t *)(void*)(hdr + 0x18) = hz;
  *(int32_t *)(void*)(hdr + 0x1C) = nAvgBytesPerSec;
  *(int16_t *)(void*)(hdr + 0x20) = nBlockAlign;
  *(int16_t *)(void*)(hdr + 0x22) = bips;
  *(int32_t *)(void*)(hdr + 0x28) = data_bytes;

  return hdr;
  }
//}}}

//{{{
static void decode_file (const char *input_file_name, FILE *file_out, const int wave_out) {

  mp3dec_t mp3d;
  mp3dec_file_info_t info;
  if (mp3dec_load (&mp3d, input_file_name, &info, 0, 0) == 0)  {
    printf ("%d %d %d %d\n", info.hz, (int)info.samples, info.channels, info.layer);

  #ifdef MINIMP3_FLOAT_OUTPUT
    int16_t* buffer = malloc (info.samples*sizeof(int16_t));
    mp3dec_f32_to_s16 (info.buffer, buffer, info.samples);
    free (info.buffer);
  #else
    int16_t* buffer = info.buffer;
  #endif

    fwrite (wav_header (0, 0, 0, 0), 1, 44, file_out);

    if (info.samples) {
      fwrite(buffer, info.samples, sizeof(int16_t), file_out);
      free(buffer);
      }

    int data_bytes = ftell (file_out) - 44;
    rewind (file_out);
    fwrite (wav_header (info.hz, info.channels, 16, data_bytes), 1, 44, file_out);
    }
  }
//}}}

int main (int argc, char *argv[]) {

  char* input_file_name  = (argc > 1) ? argv[1] : NULL;
  if (!input_file_name) {
    printf ("error: no file names given\n");
    return 1;
    }

  char* output_file_name = (argc > 2) ? argv[2] : NULL;
  if (!output_file_name) {
    printf ("error: no file names given\n");
    return 1;
    }
  FILE* file_out = fopen (output_file_name, "wb");

  decode_file (input_file_name, file_out, 1);

  if (file_out)
    fclose (file_out);

  return 0;
  }
