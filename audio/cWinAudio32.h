// cWinAudio32.h
#pragma once
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <xaudio2.h>
#include <vector>

#include "iAudio.h"
//}}}

class cAudio : public iAudio {
public:
  cAudio (int srcChannels, int srcSampleRate, int latency, bool int16);
  virtual ~cAudio();

  int getDstChannels() { return mDstChannels; }
  int getDstSampleRate() { return mDstSampleRate; }
  int getDstChannelMask() { return mDstChannelMask; }

  int getSrcChannels() { return mSrcChannels; }

  float getVolume() { return mDstVolume; }
  float getDefaultVolume() { return kDefaultVolume; }
  float getMaxVolume() { return kMaxVolume; }
  void setVolume (float volume);

  bool getMixedFL() { return (mMixDown == eBestMix) || (mMixDown == eFLFR); }
  bool getMixedFR() { return (mMixDown == eBestMix) || (mMixDown == eFLFR); }
  bool getMixedC()  { return (mMixDown == eBestMix) || (mMixDown == eCentre); }
  bool getMixedW()  { return (mMixDown == eBestMix) || (mMixDown == eWoofer); }
  bool getMixedBL() { return (mMixDown == eBestMix) || (mMixDown == eBLBR); }
  bool getMixedBR() { return (mMixDown == eBestMix) || (mMixDown == eBLBR); }
  eMixDown getMixDown() { return mMixDown; }
  void setMixDown (eMixDown mixDown) { mMixDown = mixDown; }

  void play (int srcChannels, void* srcSamples, int srcNumSamples, float pitch);

private:
  const float kMaxVolume = 3.f;
  const float kDefaultVolume = 0.8f;
  //{{{
  class cAudio2VoiceCallback : public IXAudio2VoiceCallback {
  public:
    cAudio2VoiceCallback() : mBufferEndEvent (CreateEvent (NULL, FALSE, FALSE, NULL)) {}
    ~cAudio2VoiceCallback() { CloseHandle (mBufferEndEvent); }

    // overrides
    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) override {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
    STDMETHOD_(void, OnStreamEnd)() override {}
    STDMETHOD_(void, OnBufferStart)(void*) override {}
    STDMETHOD_(void, OnBufferEnd)(void*) override { SetEvent (mBufferEndEvent); }
    STDMETHOD_(void, OnLoopEnd)(void*) override {}
    STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

    void wait() {
      WaitForSingleObject (mBufferEndEvent, INFINITE);
      }

    HANDLE mBufferEndEvent;
    };
  //}}}

  void open (int srcChannels, int srcSampleRate);
  void close();

  // vars
  IXAudio2* mXAudio2;
  IXAudio2MasteringVoice* mMasteringVoice;
  IXAudio2SourceVoice* mSourceVoice;
  cAudio2VoiceCallback mVoiceCallback;

  float mDstVolume;
  eMixDown mMixDown = eBestMix;
  eMixDown mLastMixDown = eNoMix;

  int mDstChannels = 0;
  int mDstSampleRate = 0;
  int mDstChannelMask = 0;

  int mSrcChannels = 0;
  int mSrcSampleRate = 0;

  // buffers
  float* mSilence = nullptr;

  int mBufferIndex = 0;
  std::vector<XAUDIO2_BUFFER> mBuffers;
  };
