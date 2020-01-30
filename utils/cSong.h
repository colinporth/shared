// cSong.h
#pragma once
//{{{  includes
#include "../decoders/audioParser.h"

#include "../kissFft/kiss_fft.h"
#include "../kissFft/kiss_fftr.h"

#include "concurrent_vector.h"
//}}}

class cSong {
public:
  //{{{  static constexpr
  constexpr static int kMaxSamplesPerFrame = 2048;
  constexpr static int kMaxFreq = (kMaxSamplesPerFrame/2) + 1;
  constexpr static int kMaxSpectrum = kMaxFreq/2;
  //}}}
  //{{{
  class cFrame {
  public:
    //{{{
    cFrame (uint32_t streamIndex, uint32_t len, float* powerValues, float* freqValues, uint8_t* lumaValues)
        : mStreamIndex(streamIndex), mLen(len),
          mPowerValues(powerValues), mFreqValues(freqValues), mFreqLuma(lumaValues) {
      mSilent = isSilentThreshold();
      }
    //}}}
    //{{{
    ~cFrame() {
      free (mPowerValues);
      free (mFreqValues);
      free (mFreqLuma);
      }
    //}}}

    int getStreamIndex() { return mStreamIndex; }
    float* getPowerValues() { return mPowerValues;  }
    float* getFreqValues() { return mFreqValues; }
    uint8_t* getFreqLuma() { return mFreqLuma; }

    bool isSilent() { return mSilent; }
    bool isSilentThreshold() { return mPowerValues[0] + mPowerValues[1] < kSilentThreshold; }
    void setSilent (bool silent) { mSilent = silent; }

    bool hasTitle() { return !mTitle.empty(); }
    std::string getTitle() { return mTitle; }
    void setTitle (const std::string& title) { mTitle = title; }

    static constexpr float kSilentThreshold = 0.05f;

  private:
    uint32_t mStreamIndex;
    uint32_t mLen;

    float* mPowerValues;
    float* mFreqValues;
    uint8_t* mFreqLuma;

    bool mSilent;
    std::string mTitle;
    };
  //}}}

  //{{{
  virtual ~cSong() {

    mFrames.clear();

    auto temp = mImage;
    mImage = nullptr;
    delete temp;
    }
  //}}}

  //{{{  gets
  int getAudioFrameType() { return mAudioFrameType; }
  int getNumChannels() { return mNumChannels; }
  int getNumSampleBytes() { return mNumChannels * sizeof(float); }
  int getSampleRate() { return mSampleRate; }
  int getSamplesPerFrame() { return mSamplesPerFrame; }
  int getMaxSamplesPerFrame() { return kMaxSamplesPerFrame; }

  int getMinZoomIndex() { return -8; }
  int getMaxZoomIndex() { return 8; }
  float getMaxPowerValue() { return mMaxPowerValue; }
  int getMaxFreq() { return kMaxFreq; }

  int getNumFrames() { return (int)mFrames.size(); }
  int getLastFrame() { return getNumFrames() - 1;  }
  int getTotalFrames() { return mTotalFrames; }

  //{{{
  int getPlayFrame() {
    if (mPlayFrame < mFrames.size())
      return mPlayFrame;
    else if (!mFrames.empty())
      return (int)mFrames.size() - 1;
    else // startup case
      return 0;
    }
  //}}}
  //{{{
  uint32_t getPlayFrameStreamIndex() {

    if (mPlayFrame < mFrames.size())
      return mFrames[mPlayFrame]->getStreamIndex();
    else if (!mFrames.empty())
      return mFrames[mFrames.size()-1]->getStreamIndex();
    else
      return 0;
    }
  //}}}
  //}}}
  //{{{  sets
  void setSampleRate (int sampleRate) { mSampleRate = sampleRate; }
  void setSamplesPerFrame (int samplePerFrame) { mSamplesPerFrame = samplePerFrame; }

  //{{{
  void setTitle (std::string title) {

    if (!mFrames.empty())
      mFrames.back()->setTitle (title);
    }
  //}}}

  //{{{
  void setPlayFrame (int frame) {
    mPlayFrame = std::min (std::max (frame, 0), getLastFrame());
    }
  //}}}
  void incPlayFrame (int frames) { setPlayFrame (mPlayFrame + frames); }
  void incPlaySec (int secs) { incPlayFrame (secs * mSampleRate / mSamplesPerFrame); }
  //}}}

  //{{{
  void init (eAudioFrameType audioFrameType, int numChannels, int samplesPerFrame, int sampleRate) {

    mAudioFrameType = audioFrameType;
    mNumChannels = numChannels;
    mSampleRate = sampleRate;
    mSamplesPerFrame = samplesPerFrame;

    mFrames.clear();

    mPlayFrame = 0;
    mTotalFrames = 0;

    auto temp = mImage;
    mImage = nullptr;
    delete temp;

    mMaxPowerValue = kMinPowerValue;
    mMaxFreqValue = 0.f;
    for (int i = 0; i < kMaxFreq; i++)
      mMaxFreqValues[i] = 0.f;

    fftrConfig = kiss_fftr_alloc (samplesPerFrame, 0, 0, 0);
    }
  //}}}
  //{{{
  bool addFrame (int streamIndex, int frameLen, int streamLen, int samplesPerFrame, float* samples) {
  // return true if enough frames added to start playing, streamLen only used to estimate totalFrames

    mSamplesPerFrame = samplesPerFrame;

    // sum of squares channel power
    float* powerValues = (float*)malloc (mNumChannels * 4);
    memset (powerValues, 0, mNumChannels * 4);
    for (int sample = 0; sample < samplesPerFrame; sample++) {
      timeBuf[sample] = 0;
      for (auto chan = 0; chan < mNumChannels; chan++) {
        auto value = *samples++;
        timeBuf[sample] += value;
        powerValues[chan] += value * value;
        }
      }
    for (int chan = 0; chan < mNumChannels; chan++) {
      powerValues[chan] = sqrtf (powerValues[chan] / samplesPerFrame);
      mMaxPowerValue = std::max (mMaxPowerValue, powerValues[chan]);
      }

    kiss_fftr (fftrConfig, timeBuf, freqBuf);

    float* freqValues = (float*)malloc (kMaxFreq * 4);
    for (int freq = 0; freq < kMaxFreq; freq++) {
      freqValues[freq] = sqrt ((freqBuf[freq].r * freqBuf[freq].r) + (freqBuf[freq].i * freqBuf[freq].i));
      mMaxFreqValue = std::max (mMaxFreqValue, freqValues[freq]);
      mMaxFreqValues[freq] = std::max (mMaxFreqValues[freq], freqValues[freq]);
      }

    uint8_t* lumaValues = (uint8_t*)malloc (kMaxSpectrum);
    for (int freq = 0; freq < kMaxSpectrum; freq++) {
      auto value = uint8_t((freqValues[freq] / mMaxFreqValue) * 1024.f);
      lumaValues[freq] = value > 255 ? 255 : value;
      }

    mFrames.push_back (new cFrame (streamIndex, frameLen, powerValues, freqValues, lumaValues));

    // estimate totalFrames
    int averageFrameLen = (streamIndex + frameLen) / getNumFrames();
    mTotalFrames = streamLen / averageFrameLen;

    // calc silent window
    auto frameNum = getLastFrame();
    if (mFrames[frameNum]->isSilent()) {
      auto window = kSilentWindowFrames;
      auto windowFrame = frameNum - 1;
      while ((window >= 0) && (windowFrame >= 0)) {
        // walk backwards looking for no silence
        if (!mFrames[windowFrame]->isSilentThreshold()) {
          mFrames[frameNum]->setSilent (false);
          break;
          }
        windowFrame--;
        window--;
        }
      }

    // return true if first frame, used to launch player
    return frameNum == 0;
    }
  //}}}

  //{{{
  void prevSilence() {
    mPlayFrame = skipPrev (mPlayFrame, false);
    mPlayFrame = skipPrev (mPlayFrame, true);
    mPlayFrame = skipPrev (mPlayFrame, false);
    }
  //}}}
  //{{{
  void nextSilence() {
    mPlayFrame = skipNext (mPlayFrame, true);
    mPlayFrame = skipNext (mPlayFrame, false);
    mPlayFrame = skipNext (mPlayFrame, true);
    }
  //}}}

  // public vars
  concurrency::concurrent_vector<cFrame*> mFrames;
  int mPlayFrame = 0;
  float mMaxPowerValue = kMinPowerValue;
  float mMaxFreqValue = 0.f;
  float mMaxFreqValues[kMaxFreq];
  cJpegImage* mImage = nullptr;

private:
  //{{{
  int skipPrev (int fromFrame, bool silent) {

    for (auto frame = fromFrame-1; frame >= 0; frame--)
      if (mFrames[frame]->isSilent() ^ silent)
        return frame;

    return fromFrame;
    }
  //}}}
  //{{{
  int skipNext (int fromFrame, bool silent) {

    for (int frame = fromFrame; frame < getNumFrames(); frame++)
      if (mFrames[frame]->isSilent() ^ silent)
        return frame;

    return fromFrame;
    }
  //}}}

  constexpr static float kMinPowerValue = 0.25f;
  constexpr static int kSilentWindowFrames = 10;
  //{{{  vars
  eAudioFrameType mAudioFrameType = eUnknown;

  int mNumChannels = 0;
  int mSampleRate = 0;
  int mSamplesPerFrame = 0;

  int mTotalFrames = 0;

  kiss_fftr_cfg fftrConfig;
  kiss_fft_scalar timeBuf[kMaxSamplesPerFrame];
  kiss_fft_cpx freqBuf[kMaxFreq];
  //}}}
  };
