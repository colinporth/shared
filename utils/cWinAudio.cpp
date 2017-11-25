// cWinAudio.cpp
//{{{  includes
#include "cWinAudio.h"

#include <stdio.h>
#include "utils.h"
#include "cLog.h"
#pragma comment(lib,"Xaudio2.lib")
//}}}
//{{{  const
const int kMaxChannels = 6;
const int kBitsPerSample = 16;
const int kBytesPerChannel = 2;
const int kMaxSilenceSamples = 1152;

const float kDefaultVolume = 0.8f;
const float kMaxVolume = 4.f;
//}}}

//{{{
cWinAudio::cWinAudio() :
    mVolume(kDefaultVolume) {

  // init XAUDIO2_BUFFER, most fields unused

  // allloc and clear mSilence
  mSilence = (int16_t*)malloc (kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);
  memset (mSilence, 0, kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);

  // guess initial buffer alloc
  for (auto i = 0; i < kMaxBuffers; i++) {
    memset (&mBuffers[i], 0, sizeof (XAUDIO2_BUFFER));
    mBuffers[i].AudioBytes = kMaxChannels * kMaxSilenceSamples * kBytesPerChannel;
    mBuffers[i].pAudioData = (const BYTE*)malloc (kMaxChannels * kMaxSilenceSamples * kBytesPerChannel);
    }
  }
//}}}
//{{{
cWinAudio::~cWinAudio() {

  audClose();

  free (mSilence);
  mSilence = nullptr;
  }
//}}}

//{{{
void cWinAudio::audOpen (int srcChannels, int srcSampleRate) {

  mSrcChannels = srcChannels;
  mSrcSampleRate = srcSampleRate;

  // create XAudio2 engine.
  if (XAudio2Create (&mXAudio2) != S_OK) {
    cLog::log (LOGERROR, "cWinAudio - XAudio2Create failed");
    return;
    }

  // create masteringVoice
  if (mXAudio2->CreateMasteringVoice (&mMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, srcSampleRate) != S_OK) {
    cLog::log (LOGERROR, "cWinAudio - CreateMasteringVoice failed");
    return;
    }

  // get masteringVoiceChannelMask
  DWORD masteringVoiceChannelMask;
  mMasteringVoice->GetChannelMask (&masteringVoiceChannelMask);
  mDstChannelMask = masteringVoiceChannelMask;

  // get masteringVoice outChannels, sampleRate
  XAUDIO2_VOICE_DETAILS masteringVoiceDetails;
  mMasteringVoice->GetVoiceDetails (&masteringVoiceDetails);
  mDstChannels = masteringVoiceDetails.InputChannels;
  mDstSampleRate = masteringVoiceDetails.InputSampleRate;
  cLog::log (LOGINFO, "cWinAudio - audOpen mask:" + hex(mDstChannelMask) +
                       " ch:" + dec(mDstChannels) +
                       " rate:" + dec(mDstSampleRate));

  // create sourceVoice
  WAVEFORMATEX waveformatex;
  memset (&waveformatex, 0, sizeof (WAVEFORMATEX));
  waveformatex.wFormatTag      = WAVE_FORMAT_PCM;
  waveformatex.wBitsPerSample  = kBitsPerSample;
  waveformatex.nChannels       = srcChannels;
  waveformatex.nSamplesPerSec  = (unsigned long)srcSampleRate;
  waveformatex.nBlockAlign     = srcChannels * kBitsPerSample / 8;
  waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * srcChannels * kBitsPerSample/8;

  if (mXAudio2->CreateSourceVoice (&mSourceVoice, &waveformatex,
                                   0, XAUDIO2_DEFAULT_FREQ_RATIO, &mVoiceCallback, nullptr, nullptr) != S_OK) {
    cLog::log (LOGERROR, "CreateSourceVoice failed");
    return;
    }

  mSourceVoice->Start();
  }
//}}}
//{{{
void cWinAudio::audPlay (int srcChannels, int16_t* src, int srcSamples, float pitch) {

  if (srcChannels != mSrcChannels) {
    cLog::log (LOGNOTICE, "audPlay - srcChannels:" + dec (mSrcChannels) + " changedTo:" + dec(srcChannels));
    audClose();
    audOpen (srcChannels, mSrcSampleRate);
    }

  int len = srcChannels * srcSamples * 2;
  memcpy ((void*)mBuffers[mBufferIndex].pAudioData, src ? src : mSilence, len);

  // queue buffer
  mBuffers[mBufferIndex].AudioBytes = len;
  if (mSourceVoice->SubmitSourceBuffer (&mBuffers[mBufferIndex]) != S_OK) {
    cLog::log (LOGERROR, "XAudio2 - SubmitSourceBuffer failed");
    return;
    }
  mBufferIndex = (mBufferIndex + 1) % kMaxBuffers;

  mMasteringVoice->SetVolume (mVolume, XAUDIO2_COMMIT_NOW);

  if ((pitch > 0.005f) && (pitch < 4.0f))
    mSourceVoice->SetFrequencyRatio (pitch, XAUDIO2_COMMIT_NOW);

  // block waiting for free buffer if all used
  XAUDIO2_VOICE_STATE voiceState;
  mSourceVoice->GetState (&voiceState);
  if (voiceState.BuffersQueued >= kMaxBuffers)
    mVoiceCallback.wait();
  }
//}}}
//{{{
void cWinAudio::audClose() {

  if (mXAudio2) {
    mSourceVoice->Stop();
    mXAudio2->Release();
    mXAudio2 = nullptr;
    }
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
