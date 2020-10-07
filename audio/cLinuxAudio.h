// cLinuxAudio.h
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#include "iAudio.h"
//}}}

class cAudio : public iAudio {
public:
  //{{{
  cAudio (int srcChannels, int srcSampleRate, int latency, bool int16) {

    int err = snd_pcm_open (&mHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
      printf ("audOpen - snd_pcm_open error: %s\n", snd_strerror (err));

    //SND_PCM_FORMAT_FLOAT
    err = snd_pcm_set_params (mHandle,
                              int16 ? SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_FLOAT,
                              SND_PCM_ACCESS_RW_INTERLEAVED,
                              2,      // channels
                              48000,  // sampleRate
                              1,      // softResample
                              latency);
    if (err < 0)
      printf ("audOpen - snd_pcm_set_params  error: %s\n", snd_strerror(err));
    }
  //}}}
  //{{{
  virtual ~cAudio() {
    snd_pcm_close (mHandle);
    }
  //}}}

  int getDstChannels() { return 2; }
  int getDstSampleRate() { return 480000; }
  int getDstChannelMask() { return 0xFF; }
  int getSrcChannels() { return 2; }

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
  float getDefaultVolume() { return 0.7f; };
  float getMaxVolume() { return 1.0f; }

  bool getMixedFL() { return true; }
  bool getMixedFR() { return true; }
  bool getMixedC()  { return true; }
  bool getMixedW()  { return true; }
  bool getMixedBL() { return true; }
  bool getMixedBR() { return true; }
  eMixDown getMixDown() { return mMixDown; }
  void setMixDown (eMixDown mixDown) { mMixDown = mixDown; }

  //{{{
  void play (int srcChannels, void* srcSamples, int srcNumSamples, float pitch) {

    snd_pcm_sframes_t frames = snd_pcm_writei (mHandle, srcSamples, srcNumSamples);

    if (frames < 0)
      frames = snd_pcm_recover (mHandle, frames, 0);

    if (frames < 0)
      printf ("audPlay - snd_pcm_writei failed: %s\n", snd_strerror(frames));
    }
  //}}}

  float mVolume = 0.8f;

private:
  snd_pcm_t* mHandle;
  eMixDown mMixDown = eBestMix;
  };
