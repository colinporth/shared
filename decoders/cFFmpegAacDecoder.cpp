// cFFmpegAacDecoder.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>

#include "cFFmpegAacDecoder.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  }

using namespace std;
using namespace chrono;
//}}}

//{{{
cFFmpegAacDecoder::cFFmpegAacDecoder (eFrameType frameType) {

  AVCodecID streamType;

  switch (frameType) {
    case iAudioDecoder::eFrameType::eMp3:
      streamType =  AV_CODEC_ID_MP3;
      break;

    case iAudioDecoder::eFrameType::eAacAdts:
      streamType =  AV_CODEC_ID_AAC;
      break;

    case iAudioDecoder::eFrameType::eAacLatm:
      streamType =  AV_CODEC_ID_AAC_LATM;
      break;

    default:
      cLog::log (LOGERROR, "unknown cFFmpegAacDecoder frameType %d", frameType);
      return;
    }

  mAvParser = av_parser_init (streamType);
  mAvCodec = avcodec_find_decoder (streamType);
  mAvContext = avcodec_alloc_context3 (mAvCodec);
  avcodec_open2 (mAvContext, mAvCodec, NULL);
  }
//}}}
//{{{
cFFmpegAacDecoder::~cFFmpegAacDecoder() {

  if (mAvContext)
    avcodec_close (mAvContext);
  if (mAvParser)
    av_parser_close (mAvParser);
  }
//}}}

//{{{
float* cFFmpegAacDecoder::decodeFrame (const uint8_t* framePtr, int32_t frameLen, int32_t frameNum) {

  float* outBuffer = nullptr;

  AVPacket avPacket;
  av_init_packet (&avPacket);
  auto avFrame = av_frame_alloc();

  int64_t pts = 0;
  auto pesPtr = framePtr;
  auto pesSize = frameLen;
  while (pesSize) {
    auto bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket.data, &avPacket.size,
                                       pesPtr, (int)pesSize, pts, AV_NOPTS_VALUE, 0);
    avPacket.pts = pts;
    pesPtr += bytesUsed;
    pesSize -= bytesUsed;
    if (avPacket.size) {
      auto ret = avcodec_send_packet (mAvContext, &avPacket);
      while (ret >= 0) {
        ret = avcodec_receive_frame (mAvContext, avFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
          break;

        if (avFrame->nb_samples > 0) {
          switch (mAvContext->sample_fmt) {
            //{{{
            case AV_SAMPLE_FMT_FLTP: { // 32bit float planar, copy to interleaved, mix down 5.1

              mChannels = 2;
              mSampleRate = avFrame->sample_rate;
              mSamplesPerFrame = avFrame->nb_samples;

              outBuffer = (float*)malloc (mChannels * mSamplesPerFrame * sizeof(float));
              float* dstPtr = outBuffer;

              float* srcPtr0 = (float*)avFrame->data[0];
              float* srcPtr1 = (float*)avFrame->data[1];
              if (avFrame->channels == 6) { // 5.1
                float* srcPtr2 = (float*)avFrame->data[2];
                float* srcPtr3 = (float*)avFrame->data[3];
                float* srcPtr4 = (float*)avFrame->data[4];
                float* srcPtr5 = (float*)avFrame->data[5];
                for (auto sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++ + *srcPtr2++ + *srcPtr4 + *srcPtr5; // left loud
                  *dstPtr++ = *srcPtr1++ + *srcPtr3++ + *srcPtr4++ + *srcPtr5++; // right loud
                  }
                }
              else // stereo
                for (auto sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++;
                  *dstPtr++ = *srcPtr1++;
                  }
                }
              break;
            //}}}
            //{{{
            case AV_SAMPLE_FMT_S16P: // 16bit signed planar, copy scale and copy to interleaved
              mChannels =  avFrame->channels;
              mSampleRate = avFrame->sample_rate;
              mSamplesPerFrame = avFrame->nb_samples;
              outBuffer = (float*)malloc (avFrame->channels * avFrame->nb_samples * sizeof(float));

              for (auto channel = 0; channel < avFrame->channels; channel++) {
                auto dstPtr = outBuffer + channel;
                auto srcPtr = (short*)avFrame->data[channel];
                for (auto sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr = *srcPtr++ / (float)0x8000;
                  dstPtr += mChannels;
                  }
                }
              break;
            //}}}
            default:;
            }

          }
        av_frame_unref (avFrame);
        }
      }
    }

  av_frame_free (&avFrame);
  return outBuffer;
  }
//}}}
