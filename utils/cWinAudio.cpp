// cWinAudio.cpp
#include "cWinAudio.h"

#include <stdio.h>
#include "utils.h"
#include "cLog.h"
#pragma comment(lib,"Xaudio2.lib")

const float kDefaultVolume = 0.8f;
const float kMaxVolume = 4.f;

  //{{{
  cWinAudio::cWinAudio() :
      mVolume(kDefaultVolume) {

    // init XAUDIO2_BUFFER, most fields unused
    memset (&mBuffer, 0, sizeof (XAUDIO2_BUFFER));

    // allloc and clear mSilence
    mSilence = (int16_t*)malloc (kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);
    memset (mSilence, 0, kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);

    // guess initial buffer alloc
    for (auto i = 0; i < kMaxBuffers; i++)
      mBuffers [i] = (uint8_t*)malloc (kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);
    }
  //}}}
  //{{{
  cWinAudio::~cWinAudio() {
    audClose();
    }
  //}}}

  //{{{
  void cWinAudio::audOpen (int sampleFreq, int bitsPerSample, int channels) {

    mInChannels = channels;

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
    mOutChannelMask = dwChannelMask;
    mOutChannels = details.InputChannels;
    mOutSampleRate = details.InputSampleRate;
    cLog::log (LOGINFO, "cWinAudio - audOpen mask:" + hex(mOutChannelMask) +
                         " ch:" + dec(mOutChannels) +
                         " rate:" + dec(mOutSampleRate));

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
  void cWinAudio::audPlay (int16_t* src, int len, float pitch) {

    memcpy (mBuffers[mBufferIndex], src ? src : mSilence, len);

    mMasteringVoice->SetVolume (mVolume, XAUDIO2_COMMIT_NOW);

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
  void cWinAudio::audSilence (int samples) {
    audPlay (mSilence, mInChannels * samples * kBytesPerChannel, 1.0f);
    }
  //}}}
  //{{{
  void cWinAudio::audClose() {

    free (mSilence);

    for (auto i = 0; i < kMaxBuffers; i++)
      free (mBuffers[i]);

    mSourceVoice->Stop();
    mXAudio2->Release();
    }
  //}}}

  //{{{
  void cWinAudio::setVolume (float volume) {

    if (volume < 0.f)
      mVolume = 0.f;
    else if (volume > kMaxVolume)
      mVolume = kMaxVolume;
    else
      mVolume = volume;
    }
  //}}}
