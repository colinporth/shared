// cWinAudio.h
#pragma once
//{{{  includes
#include <stdint.h>
#include <xaudio2.h>
#include "iAudio.h"
//}}}
const int kMaxBuffers = 6;

class cWinAudio : public iAudio {
public:
  cWinAudio();
  virtual cWinAudio::~cWinAudio();

  void audOpen (int sampleRate, int bitsPerSample, int channels);
  void audPlay (int16_t* src, int len, float pitch);
  void audSilence (int samples);
  void audClose();

  int getOutChannels() { return mOutChannels; }
  int getOutSampleRate() { return mOutSampleRate; }
  int getOutChannelMask() { return mOutChannelMask; }

  float getVolume() { return mVolume; }
  void setVolume (float volume);

  // public only for widgets to access directly
  float mVolume;

private:
  //{{{  const
  const int kBytesPerChannel = 2;
  const int kMaxChannels = 6;
  const int kMaxSilenceSamples = 1152;
  //}}}
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

  // vars
  IXAudio2* mXAudio2;
  IXAudio2MasteringVoice* mMasteringVoice;
  IXAudio2SourceVoice* mSourceVoice;
  cAudio2VoiceCallback mVoiceCallback;

  int mOutChannels = 0;
  int mOutSampleRate = 0;
  int mOutChannelMask = 0;

  int mInChannels = 0;

  // buffers
  XAUDIO2_BUFFER mBuffer;

  int16_t* mSilence = nullptr;

  int mBufferIndex = 0;
  uint8_t* mBuffers [kMaxBuffers];
  };
