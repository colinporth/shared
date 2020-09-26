//{{{  includes
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "cWinAudio32.h"

#include <stdio.h>
#include "../utils/utils.h"
#include "../utils/cLog.h"

#pragma comment(lib,"Xaudio2.lib")
//}}}
const int kMaxBuffers = 2;
const int kMaxChannels = 6;
const int kBitsPerSample = 32;
const int kMaxSamples = 2048;

// public
//{{{
cAudio32::cAudio32 (int srcChannels, int srcSampleRate) : mDstVolume(kDefaultVolume) {

  // alloc and clear mSilence
  mSilence = (float*)calloc (kMaxChannels * kMaxSamples, kBitsPerSample/8);

  // guess initial buffer alloc
  for (auto i = 0; i < kMaxBuffers; i++) {
    XAUDIO2_BUFFER buffer;
    memset (&buffer, 0, sizeof (XAUDIO2_BUFFER));
    buffer.AudioBytes = kMaxChannels * kMaxSamples * kBitsPerSample/4;
    buffer.pAudioData = (const BYTE*)malloc (buffer.AudioBytes);
    mBuffers.push_back (buffer);
    }
  open (srcChannels, srcSampleRate);
  }
//}}}
//{{{
cAudio32::~cAudio32() {

  close();

  mLastMixDown = eNoMix;

  free (mSilence);
  mSilence = nullptr;
  }
//}}}

//{{{
void cAudio32::setVolume (float volume) {
  mDstVolume = min (max (volume, 0.f), getMaxVolume());
  }
//}}}
//{{{
void cAudio32::play (int srcChannels, void* srcSamples, int srcNumSamples, float pitch) {
// play silence if src == nullptr, maintains timing

  if (srcChannels != mSrcChannels) {
    //{{{  recreate sourceVoice with new num of channels
    cLog::log (LOGNOTICE, "audPlay - srcChannels:" + dec (mSrcChannels) + " changedTo:" + dec(srcChannels));
    close();

    open (srcChannels, mSrcSampleRate);
    }
    //}}}

  int len = srcChannels * srcNumSamples * kBitsPerSample/8;
  if (srcNumSamples > kMaxSamples) {
    //{{{  error, return
    cLog::log (LOGERROR, "audPlay - too many samples " + dec(srcNumSamples));
    return;
    }
    //}}}
  memcpy ((void*)mBuffers[mBufferIndex].pAudioData, srcSamples ? srcSamples : mSilence, len);

  // queue buffer
  mBuffers[mBufferIndex].AudioBytes = len;
  if (mSourceVoice->SubmitSourceBuffer (&mBuffers[mBufferIndex]) != S_OK) {
    //{{{  error, return
    cLog::log (LOGERROR, "XAudio2 - SubmitSourceBuffer failed");
    return;
    }
    //}}}
  mBufferIndex = (mBufferIndex + 1) % mBuffers.size();

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
        cLog::log (LOGNOTICE, "6 to 2 mixdown changed to " + dec(mMixDown));
        }
        //}}}
      else if (mDstChannels == 4) {
        //{{{  6 to 4 mixDown
        switch (mMixDown) {
          case eFLFR: {
            //{{{  front only
            float kLevelMatrix[24] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dstFL
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f,  // dstFR
                                        1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  // dstBL
                                        0.f, 1.f, 0.f, 0.f, 0.f, 0.f}; // dstBR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eBLBR: {
            //{{{  back only
            float kLevelMatrix[24] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dstFL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f,  // dstFR
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dstBL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f}; // dstBR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eCentre: {
            //{{{  centre only
            float kLevelMatrix[24] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dstFL
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dstFR
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f,  // dstBL
                                        0.f, 0.f, 1.f, 0.f, 0.f, 0.f}; // dstBR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eWoofer: {
            //{{{  woofer only
            float kLevelMatrix[24] = {// FL   FR    C    W    BL  BR
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dstFL
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dstFR
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f,  // dstBL
                                        0.f, 0.f, 0.f, 1.f, 0.f, 0.f}; // dstBR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          case eAllMix:
          case eBestMix: {
            //{{{  eBestMix
            float kLevelMatrix[24] = {// FL   FR    C    W    BL  BR
                                        1.f, 0.f, 1.f, 1.f, 0.f, 0.f,  // dst FL + C + W
                                        0.f, 1.f, 1.f, 1.f, 0.f, 0.f,  // dst FR + C + W
                                        0.f, 0.f, 0.f, 0.f, 1.f, 0.f,  // dst BL
                                        0.f, 0.f, 0.f, 0.f, 0.f, 1.f}; // dst BR
            mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
            break;
            }
            //}}}
          }
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
        cLog::log (LOGNOTICE, "6 to 6 mixdown changed to " + dec(mMixDown));
        }
        //}}}
      }
    else {
      // stereo src
      if (mDstChannels == 2) {
        //{{{  2 to 2 mixDown
        float kLevelMatrix[4] = {// L   R
                                  1.f, 0.f,  // dst L
                                  0.f, 1.f}; // dst R
        mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
        cLog::log (LOGNOTICE, "2 to 2 mixdown changed to " + dec(mMixDown) = " nothing changed");
        }
        //}}}
      else if (mDstChannels == 4) {
        //{{{  2 to 4 mixDown
        float kLevelMatrix[8] = {// L   R
                                 1.f, 0.f,  // dst FL
                                 0.f, 1.f,  // dst FR
                                 1.f, 0.f,  // dst BL
                                 0.f, 1.f}; // dst BR
        mSourceVoice->SetOutputMatrix (mMasteringVoice, mSrcChannels, mDstChannels, kLevelMatrix, XAUDIO2_COMMIT_NOW);
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
        cLog::log (LOGNOTICE, "2 to 6 mixdown changed to " + dec(mMixDown) + " nothing changed");
        }
        //}}}
      }
    mLastMixDown = mMixDown;
    }
  if ((pitch > 0.005f) && (pitch < 4.0f))
    mSourceVoice->SetFrequencyRatio (pitch, XAUDIO2_COMMIT_NOW);

  // block waiting for free buffer if all used
  XAUDIO2_VOICE_STATE voiceState;
  mSourceVoice->GetState (&voiceState);
  if (voiceState.BuffersQueued >= mBuffers.size())
    mVoiceCallback.wait();
  }
//}}}

// private
//{{{
void cAudio32::open (int srcChannels, int srcSampleRate) {

  mSrcChannels = srcChannels;
  mSrcSampleRate = srcSampleRate;

  // create XAudio2 engine.
  if (XAudio2Create (&mXAudio2) != S_OK) {
    //{{{  error, return
    cLog::log (LOGERROR, "cWinAudio32 - XAudio2Create failed");
    return;
    }
    //}}}

  // create masteringVoice
  if (mXAudio2->CreateMasteringVoice (&mMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, srcSampleRate) != S_OK) {
    //{{{  error, return
    cLog::log (LOGERROR, "cWinAudio32 - CreateMasteringVoice failed");
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
  cLog::log (LOGINFO, "cWinAudio32 - audOpen mask:" + hex(mDstChannelMask) +
                       " channels:" + dec(mDstChannels) +
                       " sampleRate:" + dec(mDstSampleRate));

  // create sourceVoice
  WAVEFORMATEX waveformatex;
  memset (&waveformatex, 0, sizeof (WAVEFORMATEX));
  waveformatex.wFormatTag      = WAVE_FORMAT_IEEE_FLOAT;
  waveformatex.wBitsPerSample  = kBitsPerSample;
  waveformatex.nChannels       = srcChannels;
  waveformatex.nSamplesPerSec  = (unsigned long)srcSampleRate;
  waveformatex.nBlockAlign     = srcChannels * kBitsPerSample / 8;
  waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * srcChannels * kBitsPerSample / 8;

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
void cAudio32::close() {

  if (mXAudio2) {
    mSourceVoice->Stop();
    mXAudio2->Release();
    mXAudio2 = nullptr;
    }
  }
//}}}
