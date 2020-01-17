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
  static const int kMaxChannels = 2;
  static const int kMaxSamples = 2048;
  static const int kMaxFreq = 1025;
  static const int kMaxSpectrum = 512;
  //{{{
  class cFrame {
  public:
    cFrame (uint32_t streamIndex, uint32_t len, float* powerValues, float* freqValues, uint8_t* lumaValues)
        : mStreamIndex(streamIndex), mLen(len) {

      memcpy (mPowerValues, powerValues, kMaxChannels * 4);
      memcpy (mFreqValues, freqValues, kMaxFreq * 4);
      memcpy (mFreqLuma, lumaValues, kMaxSpectrum);

      mSilent = isSilentThreshold();
      }

    bool isSilent() { return mSilent; }
    bool isSilentThreshold() { return mPowerValues[0] + mPowerValues[1] < kSilentThreshold; }

    bool hasTitle() { return !mTitle.empty(); }
    void setTitle (const std::string& title) { mTitle = title; }

    const float kSilentThreshold = 0.05f;

    // vars
    uint32_t mStreamIndex;
    uint32_t mLen;

    bool mSilent;
    float mPowerValues[kMaxChannels];
    float mFreqValues[kMaxFreq];
    uint8_t mFreqLuma[kMaxSpectrum];

    std::string mTitle;
    };
  //}}}

  //{{{
  cSong() {
    }
  //}}}
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
    mNumFrames = 0;

    auto temp = mImage;
    mImage = nullptr;
    delete temp;

    mMaxPowerValue = 0;
    mMaxFreqValue = 0.f;
    for (int i = 0; i < kMaxFreq; i++)
      mMaxFreqValues[i] = 0.f;

    fftrConfig = kiss_fftr_alloc (kMaxSamples, 0, 0, 0);
    }
  //}}}
  //{{{
  int addFrame (uint32_t streamIndex, uint32_t frameLen, int numSamples, float* samples, uint32_t streamLen) {
  // return true if enough frames added to start playing

    // sum of squares channel power
    float powerSum[kMaxChannels] = { 0.f };
    for (auto sample = 0; sample < numSamples; sample++) {
      timeBuf[sample] = 0;
      for (auto chan = 0; chan < mChannels; chan++) {
        auto value = samples[(sample * mChannels) + chan];
        timeBuf[sample] += value;
        powerSum[chan] += value * value;
        }
      }

    // channel powerValues
    float powerValues[kMaxChannels];
    for (auto chan = 0; chan < mChannels; chan++) {
      powerValues[chan] = sqrtf (powerSum[chan] / numSamples);
      mMaxPowerValue = std::max (mMaxPowerValue, powerValues[chan]);
      }

    kiss_fftr (fftrConfig, timeBuf, freqBuf);

    float freqValues[kMaxFreq];
    for (auto freq = 0; freq < kMaxFreq; freq++) {
      freqValues[freq] = sqrt ((freqBuf[freq].r * freqBuf[freq].r) + (freqBuf[freq].i * freqBuf[freq].i));
      mMaxFreqValue = std::max (mMaxFreqValue, freqValues[freq]);
      mMaxFreqValues[freq] = std::max (mMaxFreqValues[freq], freqValues[freq]);
      }

    uint8_t lumaValues[kMaxFreq];
    for (auto freq = 0; freq < kMaxSpectrum; freq++) {
      auto value = uint8_t((freqValues[freq] / mMaxFreqValue) * 1024.f);
      lumaValues[freq] = value > 255 ? 255 : value;
      }

    mFrames.push_back (cFrame (streamIndex, frameLen, powerValues, freqValues, lumaValues));

    // estimate numFrames
    mNumFrames = int (uint64_t(streamLen - mFrames[0].mStreamIndex) * (uint64_t)mFrames.size() /
                      uint64_t(streamIndex + frameLen - mFrames[0].mStreamIndex));

    // calc silent window
    auto frame = getNumParsedFrames()-1;
    if (mFrames[frame].isSilent()) {
      auto window = kSilentWindow;
      auto windowFrame = frame - 1;
      while ((window >= 0) && (windowFrame >= 0)) {
        // walk backwards looking for no silence
        if (!mFrames[windowFrame].isSilentThreshold()) {
          mFrames[frame].mSilent = false;
          break;
          }
        windowFrame--;
        window--;
        }
      }

    return frame;
    }
  //}}}
  //{{{
  void setTitle (std::string title) {

    if (!mFrames.empty())
      mFrames.back().setTitle (title);
    }
  //}}}

  // gets
  int getNumParsedFrames() { return (int)mFrames.size(); }
  //{{{
  uint32_t getPlayFrame() {

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
      return mFrames[mPlayFrame].mStreamIndex;
    else
      return mFrames[0].mStreamIndex;
    }
  //}}}
  int getSamplesSize() { return mMaxSamplesPerFrame * mChannels * (mBitsPerSample/8); }
  int getMaxFreq() { return kMaxFreq; }

  // sets
  //{{{
  void setPlayFrame (int frame) {
    mPlayFrame = std::min (std::max (frame, 0), mNumFrames-1);
    }
  //}}}
  void incPlayFrame (int frames) { setPlayFrame (mPlayFrame + frames); }
  void incPlaySec (int secs) { incPlayFrame (secs * mSampleRate / mSamplesPerFrame); }

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

  uint16_t mChannels = 2;
  int mSampleRate = 44100;
  uint16_t mSamplesPerFrame = 1152;
  eAudioFrameType mAudioFrameType = eUnknown;

  concurrency::concurrent_vector<cFrame> mFrames;

  int mPlayFrame = 0;
  int mNumFrames = 0;

  float mMaxPowerValue = 0.f;
  float mMaxFreqValue = 0.f;
  float mMaxFreqValues[kMaxFreq];

  cJpegImage* mImage = nullptr;
  //}}}

private:
  //{{{
  int skipPrev (int fromFrame, bool silent) {

    for (auto frame = fromFrame-1; frame >= 0; frame--)
      if (mFrames[frame].isSilent() ^ silent)
        return frame;

    return fromFrame;
    }
  //}}}
  //{{{
  int skipNext (int fromFrame, bool silent) {

    for (auto frame = fromFrame; frame < getNumParsedFrames(); frame++)
      if (mFrames[frame].isSilent() ^ silent)
        return frame;

    return fromFrame;
    }
  //}}}

  // private const
  const int kSilentWindow = 10;       // about a half second analyse silence in frames

  // private vars
  uint16_t mBitsPerSample = 32;
  uint16_t mMaxSamplesPerFrame = kMaxSamples;

  kiss_fftr_cfg fftrConfig;
  kiss_fft_scalar timeBuf[kMaxSamples];
  kiss_fft_cpx freqBuf[kMaxFreq];
  };
