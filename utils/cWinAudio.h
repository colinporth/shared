// cWinAudio.h
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>

#include <xaudio2.h>
#pragma comment(lib,"Xaudio2.lib")

#include "iAudio.h"
//}}}
const int kMaxBuffers = 6;

class cWinAudio : public iAudio {
public:
  //{{{
  cWinAudio() {

    memset (&mBuffer, 0, sizeof (XAUDIO2_BUFFER));

    mSilence = (int16_t*)malloc (kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);
    memset (mSilence, 0, kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);

    // guess initial buffer allocation
    for (auto i = 0; i < kMaxBuffers; i++)
      mBuffers [i] = (uint8_t*)malloc (kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);
    }
  //}}}
  //{{{
  virtual ~cWinAudio() {
    audClose();
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

    mChannels = channels;

    // create XAudio2 engine.
    if (XAudio2Create (&mXAudio2) != S_OK) {
      cLog::log (LOGERROR, "cWinAudio - XAudio2Create failed");
      return;
      }

    if (mXAudio2->CreateMasteringVoice (&mMasteringVoice, channels, sampleFreq) != S_OK) {
      cLog::log (LOGERROR, "cWinAudio - CreateMasteringVoice failed");
      return;
      }

    DWORD dwChannelMask;
    mMasteringVoice->GetChannelMask (&dwChannelMask);

    XAUDIO2_VOICE_DETAILS details;
    mMasteringVoice->GetVoiceDetails (&details);
    auto masterChannelMask = dwChannelMask;
    auto masterChannels = details.InputChannels;
    auto masterRate = details.InputSampleRate;
    cLog::log (LOGINFO, "cWinAudio - audOpen mask:" + hex(masterChannelMask) +
                         " ch:" + hex(masterChannels) +
                         " rate:" + dec(masterRate));

    WAVEFORMATEX waveformatex;
    memset ((void*)&waveformatex, 0, sizeof (WAVEFORMATEX));
    waveformatex.wFormatTag      = WAVE_FORMAT_PCM;
    waveformatex.wBitsPerSample  = bitsPerSample;
    waveformatex.nChannels       = channels;
    waveformatex.nSamplesPerSec  = (unsigned long)sampleFreq;
    waveformatex.nBlockAlign     = channels * bitsPerSample / 8;
    waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * channels * bitsPerSample/8;

    if (mXAudio2->CreateSourceVoice (&mSourceVoice, &waveformatex,
                                     0, XAUDIO2_DEFAULT_FREQ_RATIO, &mVoiceCallback, nullptr, nullptr) != S_OK) {
      cLog::log (LOGERROR, "CreateSourceVoice failed");
      return;
      }

    mSourceVoice->Start();
    }
  //}}}
  //{{{
  void audPlay (int16_t* src, int len, float pitch) {

    if (!src)
      src = mSilence;

    // copy data, it can be reused before we play it, could reverse if needed
    if (mVolume == 1.0f)
      memcpy (mBuffers[mBufferIndex], src, len);
    else {
      // crude volume, must be able to set mastering voice volume instead ************
      auto dst = (int16_t*)mBuffers[mBufferIndex];
      for (auto i = 0; i < len / 2; i++)
        *dst++ = (int16_t)(*src++ * mVolume);
      }

    // queue buffer
    mBuffer.AudioBytes = len;
    mBuffer.pAudioData = mBuffers[mBufferIndex];
    if (mSourceVoice->SubmitSourceBuffer (&mBuffer) != S_OK) {
      cLog::log (LOGERROR, "XAudio2 - SubmitSourceBuffer failed");
      return;
      }

    // cycle buffers
    mBufferIndex = (mBufferIndex + 1) % kMaxBuffers;

    if ((pitch > 0.005f) && (pitch < 4.0f))
      mSourceVoice->SetFrequencyRatio (pitch, XAUDIO2_COMMIT_NOW);

    // if none left block waiting for free buffer
    XAUDIO2_VOICE_STATE voiceState;
    mSourceVoice->GetState (&voiceState);
    if (voiceState.BuffersQueued >= kMaxBuffers)
      mVoiceCallback.wait();
    }
  //}}}
  //{{{
  void audSilence (int samples) {
    audPlay (mSilence, mChannels * samples * kBytesPerChannel, 1.0f);
    }
  //}}}
  //{{{
  void audClose() {

    free (mSilence);

    for (auto i = 0; i < kMaxBuffers; i++)
      free (mBuffers[i]);

    mSourceVoice->Stop();
    mXAudio2->Release();
    }
  //}}}

  float mVolume = 0.8f;

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

  int mChannels = 0;

  // buffers
  XAUDIO2_BUFFER mBuffer;

  int16_t* mSilence = nullptr;

  int mBufferIndex = 0;
  uint8_t* mBuffers [kMaxBuffers];
  };
