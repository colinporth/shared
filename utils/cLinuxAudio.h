// cLinuxAudio.h
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include "iAudio.h"
//}}}

class cLinuxAudio : public iAudio {
public:
  //{{{
  cLinuxAudio() {
    mSilence = (int16_t*)malloc (2048*4);
    memset (mSilence, 0, 2048*4);
    }
  //}}}
  //{{{
  virtual ~cLinuxAudio() {
    free (mSilence);
    }
  //}}}

  //{{{
  float getVolume() {
    return mVolume;
    }
  //}}}
  //{{{
  void setVolume (float volume) {
    if (volume < 0)
      mVolume = 0;
    else if (volume > 100.0f)
      mVolume = 100.0f;
    else
      mVolume = volume;
    }
  //}}}

  //{{{
  void audOpen (int sampleFreq, int bitsPerSample, int channels) {

    int err = snd_pcm_open (&mHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
      printf ("audOpen - snd_pcm_open error: %s\n", snd_strerror (err));

    err = snd_pcm_set_params (mHandle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 48000, 1, 500000);
    if (err < 0)
      printf ("audOpen - snd_pcm_set_params  error: %s\n", snd_strerror(err));
    }
  //}}}
  //{{{
  void audPlay (int16_t* src, int len, float pitch) {

    if (!src)
      src = mSilence;

    snd_pcm_sframes_t frames = snd_pcm_writei (mHandle, src, len/4);
    if (frames < 0)
      frames = snd_pcm_recover (mHandle, frames, 0);
    if (frames < 0)
      printf("audPlay - snd_pcm_writei failed: %s\n", snd_strerror(frames));
    }
  //}}}
  //{{{
  void audSilence() {
    audPlay (mSilence, 4096, 1.0f);
    }
  //}}}
  //{{{
  void audClose() {
    snd_pcm_close (mHandle);
    }
  //}}}

  float mVolume = 0.8f;

private:
  snd_pcm_t* mHandle;
  int16_t* mSilence;
  };
