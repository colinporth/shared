// cYuvFrame.h
#pragma once

class cAudFrame  {
public:
  cAudFrame() {}
  //{{{
  ~cAudFrame() {
    if (mSamples)
      free (mSamples);
    }
  //}}}

  //{{{
  void set (uint64_t pts, int channels, int sampleRate, int numSamples) {

    mPts = pts;
    mChannels = channels;
    mNumSamples = numSamples;
    mSampleRate = sampleRate;

  #ifdef WIN32
    mNumSampleBytes = channels * numSamples * 2;
    mSamples = (int16_t*)realloc (mSamples, mNumSampleBytes);
  #else
    if ((channels * numSamples * 2 != mNumSampleBytes) && mSamples) {
      free (mSamples);
      mSamples = nullptr;
      mNumSampleBytes = 0;
      }

    if (!mSamples) {
      mNumSampleBytes = channels * numSamples * 2;
      mSamples = (int16_t*)malloc (mNumSampleBytes);
      }
  #endif
    }
  //}}}
  //{{{
  void invalidate() {

    mPts = 0;
    mChannels = 0;
    mNumSamples = 0;
    mSampleRate = 0;
    }
  //}}}
  //{{{
  void freeResources() {

    invalidate();

    mNumSampleBytes = 0;
    free (mSamples);
    mSamples = nullptr;
    }
  //}}}

  uint64_t mPts = 0;

  int mChannels = 0;
  int mNumSamples = 0;
  int mSampleRate = 48000;

  int mNumSampleBytes = 0;
  int16_t* mSamples = nullptr;
  float mPower[6] = { 0,0,0,0,0,0 };
  };
