// cWinAudio.h
#pragma once
//{{{  includes
#include <stdio.h>
#include <stdint.h>

#include <xaudio2.h>
#pragma comment(lib,"Xaudio2.lib")

#include "iAudio.h"
//}}}
#define NUM_BUFFERS 6

//{{{
class cAudio2VoiceCallback : public IXAudio2VoiceCallback {
public:
  cAudio2VoiceCallback() : hBufferEndEvent (CreateEvent (NULL, FALSE, FALSE, NULL)) {}
  ~cAudio2VoiceCallback() { CloseHandle (hBufferEndEvent); }

  STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) override {}
  STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
  STDMETHOD_(void, OnStreamEnd)() override {}
  STDMETHOD_(void, OnBufferStart)(void*) override {}
  STDMETHOD_(void, OnBufferEnd)(void*) override { SetEvent (hBufferEndEvent); }
  STDMETHOD_(void, OnLoopEnd)(void*) override {}
  STDMETHOD_(void, OnVoiceError)(void*, HRESULT) override {}

  HANDLE hBufferEndEvent;
  };
//}}}

class cWinAudio : public iAudio {
public:
  //{{{
  cWinAudio() {
    mSilence = (int16_t*)malloc (1024*2*6);
    memset (mSilence, 0, 1024*2*6);
    }
  //}}}
  //{{{
  virtual ~cWinAudio() {
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

    // create the XAudio2 engine.
    HRESULT hr = XAudio2Create (&mxAudio2);
    if (hr != S_OK) {
      cLog::log (LOGERROR, "cWinAudio - XAudio2Create failed");
      return;
      }

    hr = mxAudio2->CreateMasteringVoice (&mxAudio2MasteringVoice, channels, sampleFreq);
    if (hr != S_OK) {
      cLog::log (LOGERROR, "cWinAudio - CreateMasteringVoice failed");
      return;
      }

    DWORD dwChannelMask;
    hr = mxAudio2MasteringVoice->GetChannelMask (&dwChannelMask);

    XAUDIO2_VOICE_DETAILS details;
    mxAudio2MasteringVoice->GetVoiceDetails (&details);
    auto masterChannelMask = dwChannelMask;
    auto masterChannels = details.InputChannels;
    auto masterRate = details.InputSampleRate;
    cLog::log (LOGINFO, "cWinAudio - audOpen mask:" + hex(masterChannelMask) +
                         " ch:" + hex(masterChannels) +
                         " rate:" + dec(masterRate));

    mChannels = channels;

    WAVEFORMATEX waveformatex;
    memset ((void*)&waveformatex, 0, sizeof (WAVEFORMATEX));
    waveformatex.wFormatTag      = WAVE_FORMAT_PCM;
    waveformatex.wBitsPerSample  = bitsPerSample;
    waveformatex.nChannels       = channels;
    waveformatex.nSamplesPerSec  = (unsigned long)sampleFreq;
    waveformatex.nBlockAlign     = waveformatex.nChannels * waveformatex.wBitsPerSample/8;
    waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * waveformatex.nChannels * waveformatex.wBitsPerSample/8;

    hr = mxAudio2->CreateSourceVoice (&mxAudio2SourceVoice, &waveformatex,
                                      0, XAUDIO2_DEFAULT_FREQ_RATIO, &mAudio2VoiceCallback, nullptr, nullptr);
    if (hr != S_OK) {
      cLog::log (LOGERROR, "CreateSourceVoice failed");
      return;
      }

    mxAudio2SourceVoice->Start();
    }
  //}}}
  //{{{
  void audPlay (int16_t* src, int len, float pitch) {

    if (!src)
      src = mSilence;

    // copy data, it can be reused before we play it
    // - can reverse if needed
    mBuffers[mBufferIndex] = (uint8_t*)realloc (mBuffers[mBufferIndex], len);
    if (mVolume == 1.0f)
      memcpy (mBuffers[mBufferIndex], src, len);
    else {
      auto dst = (int16_t*)mBuffers[mBufferIndex];
      for (auto i = 0; i < len/ 2; i++)
        *dst++ = (int16_t)(*src++ * mVolume);
      }

    // queue buffer
    XAUDIO2_BUFFER xAudio2_buffer;
    memset ((void*)&xAudio2_buffer, 0, sizeof (XAUDIO2_BUFFER));
    xAudio2_buffer.AudioBytes = len;
    xAudio2_buffer.pAudioData = mBuffers[mBufferIndex];
    HRESULT hr = mxAudio2SourceVoice->SubmitSourceBuffer (&xAudio2_buffer);
    if (hr != S_OK) {
      cLog::log (LOGERROR, "XAudio2 - SubmitSourceBufferCreate failed");
      return;
      }

    // printf ("winAudioPlay %3.1f\n", pitch);
    if ((pitch > 0.005f) && (pitch < 4.0f))
      mxAudio2SourceVoice->SetFrequencyRatio (pitch, XAUDIO2_COMMIT_NOW);

    // cycle round buffers
    mBufferIndex = (mBufferIndex+1) % NUM_BUFFERS;

    // wait for buffer free if none left
    XAUDIO2_VOICE_STATE xAudio_voice_state;
    mxAudio2SourceVoice->GetState (&xAudio_voice_state);
    if (xAudio_voice_state.BuffersQueued >= NUM_BUFFERS)
      WaitForSingleObject (mAudio2VoiceCallback.hBufferEndEvent, INFINITE);
    }
  //}}}
  //{{{
  void audSilence() {
    audPlay (mSilence, mChannels * 1024*2, 1.0f);
    }
  //}}}
  //{{{
  void audClose() {

    HRESULT hr = mxAudio2SourceVoice->Stop();
    hr = mxAudio2->Release();
    }
  //}}}

  float mVolume = 0.8f;

private:
  int mChannels = 0;
  int16_t* mSilence = nullptr;

  IXAudio2* mxAudio2;
  IXAudio2MasteringVoice* mxAudio2MasteringVoice;
  IXAudio2SourceVoice* mxAudio2SourceVoice;

  int mBufferIndex = 0;
  BYTE* mBuffers [NUM_BUFFERS] = { NULL, NULL, NULL, NULL, NULL, NULL };

  cAudio2VoiceCallback mAudio2VoiceCallback;
  };
