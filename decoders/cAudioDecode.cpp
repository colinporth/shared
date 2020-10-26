// cAudioDecode.cpp
//{{{  includes
#include <cstring>

#include "cAudioDecode.h"

#include "cAacDecoder.h"
#include "cMp3Decoder.h"

#include "../utils/cLog.h"
//}}}

// public
//{{{
cAudioDecode::cAudioDecode (eFrameType frameType) {

  switch (frameType) {
    case eMp3:
      mAudioDecoder = new cMp3Decoder();
      break;
    case eAac:
      mAudioDecoder = new cAacDecoder();
      break;
    default:;
    }
  }
//}}}
//{{{
cAudioDecode::~cAudioDecode() {

  if (mAudioDecoder)
    delete (mAudioDecoder);
  }
//}}}

//{{{
void cAudioDecode::setFrame (uint8_t* framePtr, int32_t frameLen) {

  mFramePtr = framePtr;
  mFrameLen = frameLen;
  }
//}}}

//{{{
bool cAudioDecode::parseFrame (uint8_t* framePtr, uint8_t* frameLast) {
// dumb mp3 / aacAdts / wav / id3Tag parser

  mFramePtr = nullptr;
  mFrameLen = 0;
  mFrameType = eUnknown;
  mSkip = 0;
  mSampleRate = 0;

  while ((frameLast - framePtr) >= 6) {
    if ((framePtr[0] == 0xFF) && ((framePtr[1] & 0xF0) == 0xF0)) {
      // syncWord found
      if ((framePtr[1] & 0x06) == 0) {
        //{{{  got aac header
        // Header consists of 7 or 9 bytes (without or with CRC).
        // AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)
        //
        // A 12  syncword 0xFFF, all bits must be 1
        // B  1  MPEG Version: 0 for MPEG-4, 1 for MPEG-2
        // C  2  Layer: always 0
        // D  1  protection absent, Warning, set to 1 if there is no CRC and 0 if there is CRC
        // E  2  profile, the MPEG-4 Audio Object Type minus 1
        // F  4  MPEG-4 Sampling Frequency Index (15 is forbidden)
        // G  1  private bit, guaranteed never to be used by MPEG, set to 0 when encoding, ignore when decoding
        // H  3  MPEG-4 Channel Configuration (in the case of 0, the channel configuration is sent via an inband PCE)
        // I  1  originality, set to 0 when encoding, ignore when decoding
        // J  1  home, set to 0 when encoding, ignore when decoding
        // K  1  copyrighted id bit, the next bit of a centrally registered copyright identifier, set to 0 when encoding, ignore when decoding
        // L  1  copyright id start, signals that this frame's copyright id bit is the first bit of the copyright id, set to 0 when encoding, ignore when decoding
        // M 13  frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)
        // O 11  buffer fullness
        // P  2  Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame
        // Q 16  CRC if protection absent is 0

        const int sampleRates[16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,0,0};

        mSampleRate = sampleRates [(framePtr[2] & 0x3c) >> 2];

        // return aacFrame & size
        mFramePtr = framePtr;
        mFrameLen = (((unsigned int)framePtr[3] & 0x3) << 11) | (((unsigned int)framePtr[4]) << 3) | (framePtr[5] >> 5);
        mFrameType = eAac;

        // check for enough bytes for frame body
        return (framePtr + mFrameLen) <= frameLast;
        }
        //}}}
      else {
        //{{{  got mp3 header
        // AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
        //{{{  A 31:21  Frame sync (all bits must be set)
        //}}}
        //{{{  B 20:19  MPEG Audio version ID
        //   0 - MPEG Version 2.5 (later extension of MPEG 2)
        //   01 - reserved
        //   10 - MPEG Version 2 (ISO/IEC 13818-3)
        //   11 - MPEG Version 1 (ISO/IEC 11172-3)
        //   Note: MPEG Version 2.5 was added lately to the MPEG 2 standard.
        // It is an extension used for very low bitrate files, allowing the use of lower sampling frequencies.
        // If your decoder does not support this extension, it is recommended for you to use 12 bits for synchronization
        // instead of 11 bits.
        //}}}
        //{{{  C 18:17  Layer description
        //   00 - reserved
        //   01 - Layer III
        //   10 - Layer II
        //   11 - Layer I
        //}}}
        //{{{  D    16  Protection bit
        //   0 - Protected by CRC (16bit CRC follows header)
        //   1 - Not protected
        //}}}
        //{{{  E 15:12  Bitrate index
        // V1 - MPEG Version 1
        // V2 - MPEG Version 2 and Version 2.5
        // L1 - Layer I
        // L2 - Layer II
        // L3 - Layer III
        //   bits  V1,L1   V1,L2  V1,L3  V2,L1  V2,L2L3
        //   0000   free    free   free   free   free
        //   0001  32  32  32  32  8
        //   0010  64  48  40  48  16
        //   0011  96  56  48  56  24
        //   0100  128 64  56  64  32
        //   0101  160 80  64  80  40
        //   0110  192 96  80  96  48
        //   0111  224 112 96  112 56
        //   1000  256 128 112 128 64
        //   1001  288 160 128 144 80
        //   1010  320 192 160 160 96
        //   1011  352 224 192 176 112
        //   1100  384 256 224 192 128
        //   1101  416 320 256 224 144
        //   1110  448 384 320 256 160
        //   1111  bad bad bad bad bad
        //   values are in kbps
        //}}}
        //{{{  F 11:10  Sampling rate index
        //   bits  MPEG1     MPEG2    MPEG2.5
        //    00  44100 Hz  22050 Hz  11025 Hz
        //    01  48000 Hz  24000 Hz  12000 Hz
        //    10  32000 Hz  16000 Hz  8000 Hz
        //    11  reserv.   reserv.   reserv.
        //}}}
        //{{{  G     9  Padding bit
        //   0 - frame is not padded
        //   1 - frame is padded with one extra slot
        //   Padding is used to exactly fit the bitrate.
        // As an example: 128kbps 44.1kHz layer II uses a lot of 418 bytes
        // and some of 417 bytes long frames to get the exact 128k bitrate.
        // For Layer I slot is 32 bits long, for Layer II and Layer III slot is 8 bits long.
        //}}}
        //{{{  H     8  Private bit. This one is only informative.
        //}}}
        //{{{  I   7:6  Channel Mode
        //   00 - Stereo
        //   01 - Joint stereo (Stereo)
        //   10 - Dual channel (2 mono channels)
        //   11 - Single channel (Mono)
        //}}}
        //{{{  K     3  Copyright
        //   0 - Audio is not copyrighted
        //   1 - Audio is copyrighted
        //}}}
        //{{{  L     2  Original
        //   0 - Copy of original media
        //   1 - Original media
        //}}}
        //{{{  M   1:0  emphasis
        //   00 - none
        //   01 - 50/15 ms
        //   10 - reserved
        //   11 - CCIT J.17
        //}}}

        const uint32_t bitRates[16] = {     0,  32000,  40000, 48000,
                                        56000,  64000,  80000,  96000,
                                       112000, 128000, 160000, 192000,
                                       224000, 256000, 320000,      0};

        const int sampleRates[4] = { 44100, 48000, 32000, 0};

        //uint8_t version = (framePtr[1] & 0x08) >> 3;
        uint8_t layer = (framePtr[1] & 0x06) >> 1;
        //uint8_t errp = framePtr[1] & 0x01;

        uint8_t bitrateIndex = (framePtr[2] & 0xf0) >> 4;
        uint8_t sampleRateIndex = (framePtr[2] & 0x0c) >> 2;
        uint8_t pad = (framePtr[2] & 0x02) >> 1;
        //uint8_t priv = (framePtr[2] & 0x01);

        //uint8_t mode = (framePtr[3] & 0xc0) >> 6;
        //uint8_t modex = (framePtr[3] & 0x30) >> 4;
        //uint8_t copyright = (framePtr[3] & 0x08) >> 3;
        //uint8_t original = (framePtr[3] & 0x04) >> 2;
        //uint8_t emphasis = (framePtr[3] & 0x03);

        uint32_t bitrate = bitRates[bitrateIndex];
        mSampleRate = sampleRates[sampleRateIndex];

        int scale = (layer == 3) ? 48 : 144;
        int size = (bitrate * scale) / mSampleRate;
        if (pad)
          size++;

        // return mp3Frame & size
        mFramePtr = framePtr;
        mFrameLen = size;
        mFrameType = eMp3;

        // check for enough bytes for frame body
        return (framePtr + mFrameLen) <= frameLast;
        }
        //}}}
      }
    else if (framePtr[0] == 'R' && framePtr[1] == 'I' && framePtr[2] == 'F' && framePtr[3] == 'F') {
      //{{{  got wav header, dumb but explicit parser
      framePtr += 4;

      //uint32_t riffSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
      framePtr += 4;

      if ((framePtr[0] == 'W') && (framePtr[1] == 'A') && (framePtr[2] == 'V') && (framePtr[3] == 'E')) {
        framePtr += 4;

        if ((framePtr[0] == 'f') && (framePtr[1] == 'm') && (framePtr[2] == 't') && (framePtr[3] == ' ')) {
          framePtr += 4;

          uint32_t fmtSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
          framePtr += 4;

          //uint16_t audioFormat = framePtr[0] + (framePtr[1] << 8);
          //int numChannels = framePtr[2] + (framePtr[3] << 8);
          mSampleRate = framePtr[4] + (framePtr[5] << 8) + (framePtr[6] << 16) + (framePtr[7] << 24);
          //int blockAlign = framePtr[12] + (framePtr[13] << 8);
          framePtr += fmtSize;

          while (!((framePtr[0] == 'd') && (framePtr[1] == 'a') && (framePtr[2] == 't') && (framePtr[3] == 'a'))) {
            framePtr += 4;

            uint32_t chunkSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
            framePtr += 4 + chunkSize;
            }
          framePtr += 4;

          //uint32_t dataSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
          framePtr += 4;

          mFramePtr = framePtr;
          mFrameLen = int(frameLast - framePtr);

          mFrameType = eWav;
          return true;
          }
        }

      return false;
      }
      //}}}
    else if (framePtr[0] == 'I' && framePtr[1] == 'D' && framePtr[2] == '3') {
      //{{{  got id3 header
      auto tagSize = (framePtr[6] << 21) | (framePtr[7] << 14) | (framePtr[8] << 7) | framePtr[9];

      // return tag & size
      mFramePtr = framePtr;
      mFrameLen = 10 + tagSize;
      mFrameType = eId3Tag;

      // check for enough bytes for frame body
      return (framePtr + mFrameLen) <= frameLast;
      }
      //}}}
    else {
      mSkip++;
      framePtr++;
      }
    }

  return false;
  }
//}}}
//{{{
float* cAudioDecode::decodeFrame (int32_t frameNum) {
// decode parser frame to samples using codec context, fixup song samplerate and numSamplesPerFrame

  auto samples = mAudioDecoder->decodeFrame (mFramePtr, mFrameLen, frameNum);

  mNumChannels = mAudioDecoder->getNumChannels();
  mSampleRate = mAudioDecoder->getSampleRate();
  mNumSamplesPerFrame = mAudioDecoder->getNumSamplesPerFrame();

  return samples;
  }
//}}}
//{{{
float* cAudioDecode::decodeFrame (uint8_t* framePtr, int frameLen, int32_t frameNum, uint64_t pts) {
// decode parser frame to samples using codec context, fixup song samplerate and numSamplesPerFrame

  auto samples = mAudioDecoder->decodeFrame (framePtr, frameLen, frameNum);

  mNumChannels = mAudioDecoder->getNumChannels();
  mSampleRate = mAudioDecoder->getSampleRate();
  mNumSamplesPerFrame = mAudioDecoder->getNumSamplesPerFrame();

  return samples;
  }
//}}}

// static
//{{{
cAudioDecode::eFrameType cAudioDecode::parseSomeFrames (uint8_t* framePtr, uint8_t* frameEnd, int32_t& sampleRate) {
// return fameType

  eFrameType frameType = eUnknown;
  sampleRate = 0;

  cAudioDecode decode;
  while (decode.parseFrame (framePtr, frameEnd) && ((frameType == eUnknown) || (frameType == eId3Tag))) {
    if (decode.mFrameType == decode.eId3Tag) {
      if (parseId3Tag (framePtr, frameEnd))
        cLog::log (LOGINFO, "parseFrames found jpeg");
      }
    else {
      frameType = decode.mFrameType;
      sampleRate = decode.mSampleRate;
      }

    // onto next frame
    framePtr += decode.mSkip + decode.mFrameLen;
    }

  cLog::log (LOGINFO, "parseSomeFrames type:%d sampleRate:%d", frameType, sampleRate);

  return frameType;
  }
//}}}
//{{{
cAudioDecode::eFrameType cAudioDecode::parseAllFrames (uint8_t* framePtr, uint8_t* frameEnd, int32_t& sampleRate) {
// return frameType

  eFrameType frameType = eUnknown;
  sampleRate = 0;

  int numTags = 0;
  int numFrames = 0;
  int numLostSync = 0;

  cAudioDecode decode;
  while (decode.parseFrame (framePtr, frameEnd)) {
    if (decode.mFrameType == decode.eId3Tag) {
      if (parseId3Tag (framePtr, frameEnd))
        cLog::log (LOGINFO, "parseFrames found jpeg");
      numTags++;
      }
    else {
      frameType = decode.mFrameType;
      sampleRate = decode.mSampleRate;
      numFrames++;
      }
    // onto next frame
    framePtr += decode.mSkip + decode.mFrameLen;
    numLostSync += decode.mSkip;
    }

  cLog::log (LOGINFO, "parseAllFrames f:%d tags:%d lost:%d type:%d sampleRate:%d",
                      numFrames, numLostSync, numTags, frameType, sampleRate);

  return frameType;
  }
//}}}

// private:
//{{{
bool cAudioDecode::parseId3Tag (uint8_t* framePtr, uint8_t* frameEnd) {
// look for ID3 Jpeg tag

  auto ptr = framePtr;
  auto tag = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

  if (tag == 0x49443303)  {
    // ID3 tag
    auto tagSize = (ptr[6] << 21) | (ptr[7] << 14) | (ptr[8] << 7) | ptr[9];
    cLog::log (LOGINFO, "parseId3Tag - %c%c%c ver:%d %02x flags:%02x tagSize:%d",
                         ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], tagSize);
    ptr += 10;

    while (ptr < framePtr + tagSize) {
      auto tag = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
      auto frameSize = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
      if (!frameSize)
        break;

      auto frameFlags1 = ptr[8];
      auto frameFlags2 = ptr[9];
      std::string info;
      for (auto i = 0; i < (frameSize < 40 ? frameSize : 40); i++)
        if ((ptr[10+i] >= 0x20) && (ptr[10+i] < 0x7F))
          info += ptr[10+i];

      cLog::log (LOGINFO, "parseId3Tag - %c%c%c%c %02x %02x %d %s",
                           ptr[0], ptr[1], ptr[2], ptr[3], frameFlags1, frameFlags2, frameSize, info.c_str());

      if (tag == 0x41504943) {
        // APIC tag
        cLog::log (LOGINFO3, "parseId3Tag - APIC jpeg tag found");
        mJpegLen = frameSize - 14;
        mJpegPtr = (uint8_t*)malloc (mJpegLen);
        if (mJpegPtr)
          memcpy (mJpegPtr, ptr + 10 + 14, mJpegLen);
        return true;
        }

      ptr += frameSize + 10;
      }
    }

  return false;
  }
//}}}
