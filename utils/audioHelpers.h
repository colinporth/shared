// audioHelper.h
#pragma once
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#include <string>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libavutil/timestamp.h>
  #include <libswresample/swresample.h>
  }

#include "cLog.h"
//}}}

class cWavWriter {
public:
  //{{{
  cWavWriter (std::string filename, WAVEFORMATEX* waveFormatEx) {

    mFilename = filename;
    open ((char*)filename.c_str(), waveFormatEx);
    }
  //}}}
  //{{{
  ~cWavWriter() {
    finish();
    }
  //}}}

  bool getOk() { return mOk; }
  //{{{
  void write (float* data, int numSamples) {

    LONG bytesToWrite = numSamples * mBlockAlign;
    auto bytesWritten = mmioWrite (file, reinterpret_cast<PCHAR>(data), bytesToWrite);

    if (bytesWritten == bytesToWrite)
      framesWritten +=  bytesToWrite / mBlockAlign;
    else
      cLog::log (LOGERROR, "mmioWrite failed - wrote only %u of %u bytes", bytesWritten, bytesToWrite);
    }
  //}}}

private:
  //{{{
  void open (char* filename, WAVEFORMATEX* waveFormatEx) {

    MMIOINFO mi = { 0 };
    file = mmioOpen (filename, &mi, MMIO_READWRITE | MMIO_CREATE);

    // make a RIFF/WAVE chunk
    ckRIFF.ckid = MAKEFOURCC ('R', 'I', 'F', 'F');
    ckRIFF.fccType = MAKEFOURCC ('W', 'A', 'V', 'E');

    MMRESULT result = mmioCreateChunk (file, &ckRIFF, MMIO_CREATERIFF);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioCreateChunk (\"RIFF/WAVE\") failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    // make a 'fmt ' chunk (within the RIFF/WAVE chunk)
    MMCKINFO chunk;
    chunk.ckid = MAKEFOURCC ('f', 'm', 't', ' ');
    result = mmioCreateChunk (file, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioCreateChunk (\"fmt \") failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    // write the WAVEFORMATEX data to it
    LONG lBytesInWfx = sizeof(WAVEFORMATEX) + waveFormatEx->cbSize;
    LONG lBytesWritten = mmioWrite (file, reinterpret_cast<PCHAR>(const_cast<LPWAVEFORMATEX>(waveFormatEx)), lBytesInWfx);
    if (lBytesWritten != lBytesInWfx) {
      //{{{
      cLog::log (LOGERROR, "mmioWrite (fmt data) wrote %u bytes; expected %u bytes", lBytesWritten, lBytesInWfx);
      return;
      }
      //}}}

    // ascend from the 'fmt ' chunk
    result = mmioAscend(file, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioAscend (\"fmt \" failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    // make a 'fact' chunk whose data is (DWORD)0
    chunk.ckid = MAKEFOURCC ('f', 'a', 'c', 't');
    result = mmioCreateChunk (file, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioCreateChunk (\"fmt \") failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    // write (DWORD)0 to it
    // this is cleaned up later
    framesWritten = 0;
    lBytesWritten = mmioWrite (file, reinterpret_cast<PCHAR>(&framesWritten), sizeof(framesWritten));
    if (lBytesWritten != sizeof (framesWritten)) {
      //{{{
      cLog::log (LOGERROR, "mmioWrite(fact data) wrote %u bytes; expected %u bytes", lBytesWritten, (UINT32)sizeof(framesWritten));
      return;
      }
      //}}}

    // ascend from the 'fact' chunk
    result = mmioAscend (file, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioAscend (\"fact\" failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    // make a 'data' chunk and leave the data pointer there
    ckData.ckid = MAKEFOURCC ('d', 'a', 't', 'a');
    result = mmioCreateChunk (file, &ckData, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioCreateChunk(\"data\") failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    mBlockAlign = waveFormatEx->nBlockAlign;
    mOk = true;
    }
  //}}}
  //{{{
  void finish() {

    MMRESULT result = mmioAscend (file, &ckData, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioAscend(\"data\" failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    result = mmioAscend (file, &ckRIFF, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioAscend(\"RIFF/WAVE\" failed: MMRESULT = 0x%08x", result);
      return;
      }
      //}}}

    result = mmioClose (file, 0);
    file = NULL;
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioClose failed: MMSYSERR = %u", result);
      return;
      }
      //}}}

    // everything went well... fixup the fact chunk in the file

    // reopen the file in read/write mode
    MMIOINFO mi = { 0 };
    file = mmioOpen ((char*)mFilename.c_str(), &mi, MMIO_READWRITE);
    if (NULL == file) {
      //{{{
      cLog::log (LOGERROR, "mmioOpen failed");
      return;
      }
      //}}}

    // descend into the RIFF/WAVE chunk
    MMCKINFO ckRIFF1 = {0};
    ckRIFF1.ckid = MAKEFOURCC ('W', 'A', 'V', 'E'); // this is right for mmioDescend
    result = mmioDescend (file, &ckRIFF1, NULL, MMIO_FINDRIFF);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioDescend(\"WAVE\") failed: MMSYSERR = %u", result);
      return;
      }
      //}}}

    // descend into the fact chunk
    MMCKINFO ckFact = {0};
    ckFact.ckid = MAKEFOURCC ('f', 'a', 'c', 't');
    result = mmioDescend (file, &ckFact, &ckRIFF1, MMIO_FINDCHUNK);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioDescend(\"fact\") failed: MMSYSERR = %u", result);
      return;
      }
      //}}}

    // write framesWritten to the fact chunk
    LONG bytesWritten = mmioWrite (file, reinterpret_cast<PCHAR>(&framesWritten), sizeof(framesWritten));
    if (bytesWritten != sizeof (framesWritten)) {
      //{{{
      cLog::log (LOGERROR, "Updating the fact chunk wrote %u bytes; expected %u", bytesWritten, (UINT32)sizeof(framesWritten));
      return;
      }
      //}}}

    // ascend out of the fact chunk
    result = mmioAscend (file, &ckFact, 0);
    if (MMSYSERR_NOERROR != result) {
      //{{{
      cLog::log (LOGERROR, "mmioAscend(\"fact\") failed: MMSYSERR = %u", result);
      return;
      }
      //}}}

    mmioClose (file, 0);
    }
  //}}}
  //{{{  private vars
  std::string mFilename;
  HMMIO file;

  MMCKINFO ckRIFF = { 0 };
  MMCKINFO ckData = { 0 };

  bool mOk = false;

  int mBlockAlign = 0;
  int framesWritten = 0;
  //}}}
  };


class cAacWriter {
public:
  //{{{
  cAacWriter (std::string filename, int channels, int sampleRate, int bitRate) {

    if (openFile (filename.c_str(), channels, sampleRate, bitRate))
      if (avformat_write_header (mFormatContext, NULL) < 0)
        mOk = true;
    }
  //}}}
  //{{{
  ~cAacWriter() {

    bool flushingEncoder = true;
    while (flushingEncoder)
      encodeFrame (NULL, flushingEncoder);

    av_write_trailer (mFormatContext);
    }
  //}}}

  bool getOk() { return mOk; }
  int getChannels() { return mCodecContext->channels; }
  int getSampleRate() { return mCodecContext->sample_rate; }
  int getBitRate() { return (int)mCodecContext->bit_rate; }
  int getFrameSize() { return mCodecContext->frame_size; }

  //{{{
  void write (float* data, int numSamples) {

    AVFrame* frame = av_frame_alloc();

    frame->nb_samples = mCodecContext->frame_size;
    frame->channel_layout = mCodecContext->channel_layout;
    frame->format = mCodecContext->sample_fmt;
    frame->sample_rate = mCodecContext->sample_rate;
    av_frame_get_buffer (frame, 0);

    auto samplesL = (float*)frame->data[0];
    auto samplesR = (float*)frame->data[1];
    for (int i = 0; i < getFrameSize(); i++) {
      *samplesL++ = *data++;
      *samplesR++ = *data++;
      }

    bool hasData;
    encodeFrame (frame, hasData);

    av_frame_free (&frame);
    }
  //}}}

private:
  //{{{
  bool openFile (const char* filename, int channels, int sampleRate, int bitRate) {

    // find the encoder by name
    AVCodec* codec = avcodec_find_encoder (AV_CODEC_ID_AAC);
    mCodecContext = avcodec_alloc_context3 (codec);

    // set basic encoder parameters
    mCodecContext->channels = channels;
    mCodecContext->channel_layout = av_get_default_channel_layout (channels);
    mCodecContext->sample_rate = sampleRate;
    mCodecContext->sample_fmt = codec->sample_fmts[0];
    mCodecContext->bit_rate = bitRate;
    mCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    // Open the output file to write to it
    AVIOContext* ioContext;
    int error = avio_open (&ioContext, filename, AVIO_FLAG_WRITE);

    // Create a new format context for the output container format
    mFormatContext = avformat_alloc_context();
    mFormatContext->pb = ioContext;
    mFormatContext->oformat = av_guess_format (NULL, filename, NULL);

    // Create a new audio stream in the output file container
    AVStream* stream = avformat_new_stream (mFormatContext, NULL);
    stream->time_base.den = sampleRate;
    stream->time_base.num = 1;

    // Some container formats (like MP4) require global headers to be present
    // Mark the encoder so that it behaves accordingly
    if (mFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
      mCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Open the encoder for the audio stream to use it later
    //auto res = av_opt_set (codecContext->priv_data, "profile", "aac_he", 0);
    //printf ("setopt %x", res);
    error = avcodec_open2 (mCodecContext, codec, NULL);
    error = avcodec_parameters_from_context (stream->codecpar, mCodecContext);

    return true;
    }
  //}}}
  //{{{
  bool encodeFrame (AVFrame* frame, bool& hasData) {

    bool ok = false;
    hasData = false;

    // Packet used for temporary storage
    AVPacket packet;
    av_init_packet (&packet);
    packet.data = NULL;
    packet.size = 0;

    // Set a timestamp based on the sample rate for the container
    if (frame) {
      frame->pts = mPts;
      mPts += frame->nb_samples;
      }

    // Send the audio frame stored in the temporary packet to the encoder
    // The output audio stream encoder is used to do this The encoder signals that it has nothing more to encode
    int error = avcodec_send_frame (mCodecContext, frame);
    if (error == AVERROR_EOF) {
      ok = true;
      goto cleanup;
      }
    else if (error < 0) {
      cLog::log (LOGERROR, "error send packet for encoding");
      goto cleanup;
      }

    // Receive one encoded frame from the encoder
    // If the encoder asks for more data to be able to provide an encoded frame, return indicating that no data is present
    error = avcodec_receive_packet (mCodecContext, &packet);
    if ((error == AVERROR(EAGAIN)) || (error == AVERROR_EOF))
      ok = true;
    else if (error < 0)
      cLog::log (LOGERROR, "error encoding frame");
    else {
      ok = true;
      hasData = true;
      }

    // Write one audio frame from the temporary packet to the output file
    if (hasData)
      if (av_write_frame (mFormatContext, &packet) < 0) {
        ok = false;
        cLog::log (LOGERROR, "error write frame");
        }

  cleanup:
    av_packet_unref (&packet);
    return ok;
    }
  //}}}
  //{{{  private vars
  AVCodecContext* mCodecContext = NULL;
  AVFormatContext* mFormatContext = NULL;

  bool mOk = false;

  int64_t mPts = 0;
  //}}}
  };
