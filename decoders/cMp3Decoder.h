// cMp3.h - minimp3 wrapper
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
//}}}

#define MINIMP3_IMPLEMENTATION
#include "miniMp3.h"

class cMp3Decoder {
public:
  //{{{
  cMp3Decoder() {
    mp3dec_init (&dec);
    }
  //}}}

  //{{{
  uint8_t* getJpeg (int& jpegLen) {
    jpegLen = mJpegLen;
    return mJpegBuf;
    }
  //}}}
  //{{{
  unsigned findId3tag (uint8_t* buf, int bufLen) {
  // check for ID3 tag

    auto ptr = buf;
    auto tag = ((*ptr)<<24) | (*(ptr+1)<<16) | (*(ptr+2)<<8) | *(ptr+3);

    if (tag == 0x49443303)  {
      // got ID3 tag
      auto tagSize = (*(ptr+6)<<21) | (*(ptr+7)<<14) | (*(ptr+8)<<7) | *(ptr+9);
      cLog::log (LOGINFO3, "cMp3decoder - %c%c%c ver:%d %02x flags:%02x tagSize:%d",
                           *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), tagSize);
      ptr += 10;

      while (ptr < buf + tagSize) {
        auto tag = ((*ptr)<<24) | (*(ptr+1)<<16) | (*(ptr+2)<<8) | *(ptr+3);
        auto frameSize = (*(ptr+4)<<24) | (*(ptr+5)<<16) | (*(ptr+6)<<8) | (*(ptr+7));
        if (!frameSize)
          break;

        auto frameFlags1 = *(ptr+8);
        auto frameFlags2 = *(ptr+9);
        cLog::log (LOGINFO3, "cMp3decoder - tag %c%c%c%c - %02x %02x - frameSize:%d",
                            *ptr, *(ptr+1), *(ptr+2), *(ptr+3), frameFlags1, frameFlags2, frameSize);
        //for (auto i = 0; i < (tag == 0x41504943 ? 11 : frameSize); i++)
        //  printf ("%c", *(ptr+10+i));
        //printf ("\n");
        //for (auto i = 0; i < (frameSize < 32 ? frameSize : 32); i++)
        //  printf ("%02x ", *(ptr+10+i));
        //printf ("\n");

        if (tag == 0x41504943) {
          cLog::log (LOGINFO3, "cMp3decoder - jpeg tag found");
          mJpegLen = frameSize - 14;
          mJpegBuf =  (uint8_t*)malloc (mJpegLen);
          memcpy (mJpegBuf, ptr + 10 + 14, mJpegLen);
          }
        ptr += frameSize + 10;
        }
      return tagSize + 10;
      }
    else
      return 0;
    }
  //}}}

  //{{{
  int decodeFrame (uint8_t* buf, unsigned bufLen, int16_t* samples) {

    mp3dec_decode_frame (&dec, buf, bufLen, samples, &info);

    return info.frame_bytes;
    }
  //}}}

private:
  const uint16_t mp3_freq_tab[3] = { 44100, 48000, 32000 };
  //{{{
  const uint16_t mp3_bitrate_tab[2][15] = {
    {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    };
  //}}}

  uint8_t* mJpegBuf = nullptr;
  int mJpegLen = 0;

  mp3dec_t dec;
  mp3dec_frame_info_t info;
  };
