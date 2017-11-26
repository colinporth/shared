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
    mDstVolume(kDefaultVolume) {

  // alloc and clear mSilence
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
    //{{{  error, return
    cLog::log (LOGERROR, "cWinAudio - XAudio2Create failed");
    return;
    }
    //}}}

  // create masteringVoice
  if (mXAudio2->CreateMasteringVoice (&mMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, srcSampleRate) != S_OK) {
    //{{{  error, return
    cLog::log (LOGERROR, "cWinAudio - CreateMasteringVoice failed");
    return;
    }
    //}}}

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
    //{{{  error, return
    cLog::log (LOGERROR, "CreateSourceVoice failed");
    return;
    }
    //}}}

  mSourceVoice->Start();
  }
//}}}
//{{{
void cWinAudio::audPlay (int srcChannels, int16_t* src, int srcSamples, float pitch) {

  if (srcChannels != mSrcChannels) {
    //{{{  recreate sourceVoice with new num of channels
    cLog::log (LOGNOTICE, "audPlay - srcChannels:" + dec (mSrcChannels) + " changedTo:" + dec(srcChannels));
    audClose();
    audOpen (srcChannels, mSrcSampleRate);
    }
    //}}}

  int len = srcChannels * srcSamples * 2;
  memcpy ((void*)mBuffers[mBufferIndex].pAudioData, src ? src : mSilence, len);

  // queue buffer
  mBuffers[mBufferIndex].AudioBytes = len;
  if (mSourceVoice->SubmitSourceBuffer (&mBuffers[mBufferIndex]) != S_OK) {
    //{{{  error, return
    cLog::log (LOGERROR, "XAudio2 - SubmitSourceBuffer failed");
    return;
    }
    //}}}
  mBufferIndex = (mBufferIndex + 1) % kMaxBuffers;

  mMasteringVoice->SetVolume (mDstVolume, XAUDIO2_COMMIT_NOW);

  if (mMixDown != mLastMixDown) {
    if (mSrcChannels == 6) {
      // 5.1 src
      if (mDstChannels == 2) {
        //{{{  6 to 2 mixDown
        switch (mMixDown) {
          case eFLFR: {
            //{{{  front only
            float kLevelMatrix[12] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dstL
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f}; // dstR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eBLBR: {
            //{{{  back only
            float kLevelMatrix[12] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dstL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f}; // dstR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eCentre: {
            //{{{  centre only
            float kLevelMatrix[12] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dstL
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f}; // dstR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eWoofer: {
            //{{{  woofer only
            float kLevelMatrix[12] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dstL
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f}; // dstR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eAllMix:
          case eBestMix: {
            //{{{  eBestMix
            float kLevelMatrix[12] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 1.f, 1.f, 1.f, 0.f,  // dstL
                                        0.f, 1.f, 1.f, 1.f, 0.f, 1.f}; // dstR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          }

        mLastMixDown = mMixDown;

        cLog::log (LOGNOTICE, "6 to 2 mixdown changed to " + dec(mMixDown));
        }
        //}}}
      else if (mDstChannels == 4) {
        //{{{  6 to 4 mixDown
        cLog::log (LOGNOTICE, "6 to 4 mixdown missing");
        }
        //}}}
      else {
        //{{{  6 to 6 mixDown
        switch (mMixDown) {
          case eFLFR: {
            //{{{  front only
            float kLevelMatrix[36] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dst FL
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f,  // dst FR
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dst C
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f,  // dst W
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dst BL
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eBLBR: {
            //{{{  back only
            float kLevelMatrix[36] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dst FL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f,  // dst FR
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dst C
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f,  // dst W
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dst BL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eCentre: {
            //{{{  centre only
            float kLevelMatrix[36] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dst FL
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dst FR
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dst C
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dst W
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dst BL
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eWoofer: {
            //{{{  woofer only
            float kLevelMatrix[36] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dst FL
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dst FR
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dst C
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dst W
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dst BL
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eAllMix: {
            //{{{  1:1
            float kLevelMatrix[36] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dst FL
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f,  // dst FR
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dst C
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dst W
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dst BL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eBestMix: {
            //{{{  5.1 to 4
            float kLevelMatrix[36] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 1.f, 1.f, 0.f, 0.f,  // dst FL + C + W
                                        0.f, 1.f, 1.f, 1.f, 0.f, 0.f,  // dst FR + C + W
                                        1.f, 0.f, 1.f, 1.f, 1.f, 0.f,  // dst C    full stereo mix
                                        0.f, 1.f, 1.f, 1.f, 0.f, 1.f,  // dst W    full stereo mix
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dst BL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          }

        mLastMixDown = mMixDown;

        cLog::log (LOGNOTICE, "6 to 6 mixdown changed to " + dec(mMixDown));
        }
        //}}}
      }
    else { 
      // steroe src
      if (mDstChannels == 2) {
        //{{{  2 to 2 mixDown
        float kLevelMatrix[4] = {// L   R
                                  1.f, 0.f,  // dst L
                                  0.f, 1.f}; // dst R
        mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
        mLastMixDown = mMixDown;

        cLog::log (LOGNOTICE, "2 to 2 mixdown changed to " + dec(mMixDown) = " nothing changed");
        }
        //}}}
      else if (mDstChannels == 4) {
        //{{{  2 to 4 mixDown
        cLog::log (LOGNOTICE, "2 to 4 mixdown missing");
        }
        //}}}
      else {
        //{{{  2 to 6 duplicate src LR to dst pairs
        float kLevelMatrix[12] = {// L   R
                                   1.f, 0.f,  // dst FL
                                   0.f, 1.f,  // dst FR
                                   1.f, 0.f,  // dst C
                                   0.f, 1.f,  // dst W
                                   1.f, 0.f,  // dst BL
                                   0.f, 1.f}; // dst BR
        mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
        mLastMixDown = mMixDown;

        cLog::log (LOGNOTICE, "2 to 6 mixdown changed to " + dec(mMixDown) + " nothing changed");
        }
        //}}}
      }
    }
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
  mDstVolume = min (max (volume, 0.f), kMaxVolume);
  }
//}}}
