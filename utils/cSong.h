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
  static constexpr int kMaxChannels = 2;
  static constexpr int kMaxSamplesPerFrame = 2048;
  static constexpr int kMaxFreq = (kMaxSamplesPerFrame/2) + 1;
  static constexpr int kMaxSpectrum = kMaxFreq/2;
  //{{{
  class cFrame {
  public:
    //{{{
    cFrame (uint32_t streamIndex, uint32_t len, float* samples, float* powerValues, float* freqValues, uint8_t* lumaValues)
        : mStreamIndex(streamIndex), mLen(len),
          mSamples(samples), mPowerValues(powerValues), mFreqValues(freqValues), mFreqLuma(lumaValues) {
      mSilent = isSilentThreshold();
      }
    //}}}
    //{{{
    ~cFrame() {
      free (mSamples);
      free (mPowerValues);
      free (mFreqValues);
      free (mFreqLuma);
      }
    //}}}

    int getStreamIndex() { return mStreamIndex; }
    float* getSamples() { return mSamples; }
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

    float* mSamples;
    float* mPowerValues;
    float* mFreqValues;
    uint8_t* mFreqLuma;

    bool mSilent;
    std::string mTitle;
    };
  //}}}

  cSong() {}
  //{{{
  virtual ~cSong() {

    mFrames.clear();

    auto temp = mImage;
    mImage = nullptr;
    delete temp;
    }
  //}}}

  //{{{
  void init (std::string fileName, eAudioFrameType audioFrameType, uint16_t samplesPerFrame, int sampleRate) {

    mFileName = fileName;

    mSampleRate = sampleRate;
    mSamplesPerFrame = samplesPerFrame;
    mAudioFrameType = audioFrameType;

    mFrames.clear();

    mPlayFrame = 0;
    mTotalFrames = 0;

    auto temp = mImage;
    mImage = nullptr;
    delete temp;

    mMaxPowerValue = 0;
    mMaxFreqValue = 0.f;
    for (int i = 0; i < kMaxFreq; i++)
      mMaxFreqValues[i] = 0.f;

    fftrConfig = kiss_fftr_alloc (kMaxSamplesPerFrame, 0, 0, 0);
    }
  //}}}
  //{{{
  bool addFrame (int streamIndex, int frameLen, int samplesPerFrame, float* samples, int streamLen) {
  // return true if enough frames added to start playing

    mSamplesPerFrame = samplesPerFrame;

    // sum of squares channel power
    float powerSum[kMaxChannels] = { 0.f };
    for (int sample = 0; sample < samplesPerFrame; sample++) {
      timeBuf[sample] = 0;
      for (auto chan = 0; chan < mChannels; chan++) {
        auto value = samples[(sample * mChannels) + chan];
        timeBuf[sample] += value;
        powerSum[chan] += value * value;
        }
      }

    // channel powerValues
    float* powerValues = (float*)malloc (kMaxChannels * 4);
    for (int chan = 0; chan < mChannels; chan++) {
      powerValues[chan] = sqrtf (powerSum[chan] / samplesPerFrame);
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

    mFrames.push_back (new cFrame (streamIndex, frameLen, samples, powerValues, freqValues, lumaValues));

    // estimate totalFrames
    mTotalFrames = int (uint64_t(streamLen - mFrames[0]->getStreamIndex()) * (uint64_t)mFrames.size() /
                        uint64_t(streamIndex + frameLen - mFrames[0]->getStreamIndex()));

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

  // gets
  int getAudioFrameType() { return mAudioFrameType; }
  int getNumChannels() { return mChannels; }
  int getSampleRate() { return mSampleRate; }
  int getSamplesPerFrame() { return mSamplesPerFrame; }
  int getMaxSamplesPerFrame() { return kMaxSamplesPerFrame; }
  int getMaxFreq() { return kMaxFreq; }

  int getNumFrames() { return (int)mFrames.size(); }
  int getLastFrame() { return getNumFrames() - 1;  }
  int getTotalFrames() { return mTotalFrames; }
  //{{{
  int getPlayFrame() {

    if (mFrames.empty())
      return 0;
    else if (mPlayFrame < mFrames.size())
      return mPlayFrame;
    else
      return (uint32_t)mFrames.size() - 1;
    }
  //}}}
  //{{{
  uint32_t getPlayFrameStreamIndex() {

    if (mFrames.empty())
      return 0;
    else if (mPlayFrame < mFrames.size())
      return mFrames[mPlayFrame]->getStreamIndex();
    else
      return mFrames[0]->getStreamIndex();
    }
  //}}}
  //{{{
  float* getPlayFrameSamples() {

    if (mFrames.empty())
      return nullptr;
    else if (mPlayFrame < mFrames.size())
      return mFrames[mPlayFrame]->getSamples();
    else
      return mFrames[0]->getSamples();
    }
  //}}}

  // sets
  void setSampleRate (int sampleRate) { mSampleRate = sampleRate; }
  void setSamplesPerFrame (int samplePerFrame) { mSamplesPerFrame = samplePerFrame; }

  //{{{
  void setPlayFrame (int frame) {
    mPlayFrame = std::min (std::max (frame, 0), getLastFrame());
    }
  //}}}
  void incPlayFrame (int frames) { setPlayFrame (mPlayFrame + frames); }
  void incPlaySec (int secs) { incPlayFrame (secs * mSampleRate / mSamplesPerFrame); }

  //{{{
  void setTitle (std::string title) {

    if (!mFrames.empty())
      mFrames.back()->setTitle (title);
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

  //{{{  public vars, simpler access for gui
  std::string mFileName;

  concurrency::concurrent_vector<cFrame*> mFrames;

  int mPlayFrame = 0;

  float mMaxPowerValue = 0.f;
  float mMaxFreqValue = 0.f;
  float mMaxFreqValues[kMaxFreq];

  cJpegImage* mImage = nullptr;
  //}}}

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

  // private const
  static constexpr int kSilentWindowFrames = 10;

  int mChannels = kMaxChannels;
  int mSamplesPerFrame = 0;
  int mSampleRate = 0;
  eAudioFrameType mAudioFrameType = eUnknown;

  int mTotalFrames = 0;

  // private vars
  kiss_fftr_cfg fftrConfig;
  kiss_fft_scalar timeBuf[kMaxSamplesPerFrame];
  kiss_fft_cpx freqBuf[kMaxFreq];
  };
