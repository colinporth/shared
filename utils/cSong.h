// cSong.h - container of audio frames,power,peak,fft,range select, hls numbering
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <shared_mutex>

#include "../decoders/cAudioParser.h"

#include "../kissFft/kiss_fft.h"
#include "../kissFft/kiss_fftr.h"
//}}}

//{{{
class cSong {
public:
  //{{{
  class cFrame {
  public:
    static constexpr float kQuietThreshold = 0.01f;
    cFrame (int numChannels, int numFreqBytes, float* samples, bool ourSamples, int64_t pts);
    virtual ~cFrame();

    // gets
    float* getSamples() { return mSamples; }
    float* getPowerValues() { return mPowerValues;  }
    float* getPeakValues() { return mPeakValues;  }
    uint8_t* getFreqValues() { return mFreqValues; }
    uint8_t* getFreqLuma() { return mFreqLuma; }
    int64_t getPts() { return mPts; }

    bool isQuiet() { return mPeakValues[0] + mPeakValues[1] < kQuietThreshold; }

    bool isMuted() { return mMuted; }
    bool isSilence() { return mSilence; }
    void setSilence (bool silence) { mSilence = silence; }

    bool hasTitle() { return !mTitle.empty(); }
    std::string getTitle() { return mTitle; }

    float* mSamples;
    bool mOurSamples;

    float* mPowerValues;
    float* mPeakValues;
    uint8_t* mFreqValues;
    uint8_t* mFreqLuma;

    int64_t mPts;

  private:
    // vars
    bool mMuted;
    bool mSilence;

    std::string mTitle;
    };
  //}}}
  //{{{
  class cSelect {
  public:
    //{{{
    class cSelectItem {
    public:
      enum eType { eLoop, eMute };

      cSelectItem(eType type, int firstFrame, int lastFrame, const std::string& title) :
        mType(type), mFirstFrame(firstFrame), mLastFrame(lastFrame), mTitle(title) {}

      eType getType() { return mType; }
      int getFirstFrame() { return mFirstFrame; }
      int getLastFrame() { return mLastFrame; }
      bool getMark() { return getFirstFrame() == getLastFrame(); }
      std::string getTitle() { return mTitle; }
      //{{{
      bool inRange (int frame) {
      // ignore 1 frame select range
        return (mFirstFrame != mLastFrame) && (frame >= mFirstFrame) && (frame <= mLastFrame);
        }
      //}}}

      void setType (eType type) { mType = type; }
      void setFirstFrame (int frame) { mFirstFrame = frame; }
      void setLastFrame (int frame) { mLastFrame = frame; }
      void setTitle (const std::string& title) { mTitle = title; }

    private:
      eType mType;
      int mFirstFrame = 0;
      int mLastFrame = 0;
      std::string mTitle;
      };
    //}}}

    // gets
    bool empty() { return mItems.empty(); }
    int getFirstFrame() { return empty() ? 0 : mItems.front().getFirstFrame(); }
    int getLastFrame() { return empty() ? 0 : mItems.back().getLastFrame(); }
    std::vector<cSelectItem>& getItems() { return mItems; }

    bool inRange (int frame);
    int constrainToRange (int frame, int constrainedFrame);

    // actions
    void clearAll();
    void addMark (int frame, const std::string& title = "");
    void start (int frame);
    void move (int frame);
    void end();

  private:
    enum eEdit { eEditNone, eEditFirst, eEditLast, eEditRange };

    eEdit mEdit = eEditNone;
    int mEditFrame = 0;
    std::vector<cSelectItem> mItems;
    int mItemNum = 0;
    };
  //}}}

  cSong (eAudioFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame, int maxMapSize);
  virtual ~cSong();

  void clear();
  void addFrame (bool reuseFront, int frameNum, float* samples, bool ourSamples, int totalFrames, int64_t pts);

  //{{{  gets
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  bool getChanged() { return mChanged; }

  eAudioFrameType getFrameType() { return mFrameType; }
  int getNumChannels() { return mNumChannels; }
  int getNumSampleBytes() { return mNumChannels * sizeof(float); }
  int getSampleRate() { return mSampleRate; }
  int getSamplesPerFrame() { return mSamplesPerFrame; }

  int getFirstFrame() { return mFrameMap.empty() ? 0 : mFrameMap.begin()->first; }
  int getLastFrame() { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->first;  }
  int getNumFrames() { return mFrameMap.empty() ? 0 : (mFrameMap.rbegin()->first - mFrameMap.begin()->first + 1); }
  int getTotalFrames() { return mTotalFrames; }
  int getPlayFrame() { return mPlayFrame; }

  //{{{
  cFrame* getFramePtr (int frame) {
    auto it = mFrameMap.find (frame);
    return (it == mFrameMap.end()) ? nullptr : it->second;
    }
  //}}}
  cSelect& getSelect() { return mSelect; }

  // max nums for early allocations
  int getMaxNumSamplesPerFrame() { return kMaxNumSamplesPerFrame; }
  int getMaxNumSampleBytes() { return kMaxNumChannels * sizeof(float); }
  int getMaxNumFrameSamplesBytes() { return getMaxNumSamplesPerFrame() * getMaxNumSampleBytes(); }

  // max values for ui
  float getMaxPowerValue() { return mMaxPowerValue; }
  float getMaxPeakValue() { return mMaxPeakValue; }
  float getMaxFreqValue() { return mMaxFreqValue; }
  int getNumFreqBytes() { return kMaxFreqBytes; }
  //}}}
  virtual int getLengthFrame() { return mTotalFrames; }
  //{{{  sets
  void setNumChannels (int numChannels) { mNumChannels = numChannels; }
  void setSampleRate (int sampleRate) { mSampleRate = sampleRate; }
  void setSamplesPerFrame (int samplesPerFrame) { mSamplesPerFrame = samplesPerFrame; }
  //{{{
  void setFixups (int numChannels, int sampleRate, int samplesPerFrame) {
    mNumChannels = numChannels;
    mSampleRate = sampleRate;
    mSamplesPerFrame = samplesPerFrame;
    }
  //}}}

  void setChanged (bool changed) { mChanged = changed; }

  // playFrame
  virtual void setPlayFrame (int frame);
  void incPlaySec (int secs, bool useSelectRange);
  void incPlayFrame (int frames, bool useSelectRange);
  //}}}

  // actions
  void prevSilencePlayFrame();
  void nextSilencePlayFrame();

protected:
  int mPlayFrame = 0;
  int mSampleRate = 0;
  int mSamplesPerFrame = 0;
  std::shared_mutex mSharedMutex;

private:
  void clearFrames();

  void checkSilenceWindow (int frameNum);
  int skipPrev (int fromFrame, bool silence);
  int skipNext (int fromFrame, bool silence);

  static constexpr int kMaxNumChannels = 2;           // arbitrary chan max
  static constexpr int kMaxNumSamplesPerFrame = 2048; // arbitrary frame max
  static constexpr int kMaxFreq = (kMaxNumSamplesPerFrame / 2) + 1; // fft max
  static constexpr int kMaxFreqBytes = 512; // arbitrary graphics max

  // vars
  int mMaxMapSize = 0;
  std::map <int, cFrame*> mFrameMap;

  eAudioFrameType mFrameType = eAudioFrameType::eUnknown;
  bool mOurSamples = false;
  bool mChanged = false;

  int mNumChannels = kMaxNumChannels;

  int mTotalFrames = 0;
  cSelect mSelect;

  //{{{  fft vars
  kiss_fftr_cfg mFftrConfig;
  kiss_fft_scalar mTimeBuf[kMaxNumSamplesPerFrame];
  kiss_fft_cpx mFreqBuf[kMaxFreq];
  //}}}
  //{{{  max stuff for ui
  float mMaxPowerValue = 0.f;
  float mMaxPeakValue = 0.f;
  float mMaxFreqValue = 0.f;
  //}}}
  };
//}}}

// song with added HLS
//{{{
class cHlsSong : public cSong {
public:
  cHlsSong (eAudioFrameType frameType, int numChannels,
            int sampleRate, int samplesPerFrame, int maxMapSize, int framesPerChunk);
  virtual ~cHlsSong();

  // gets
  int getBasePlayFrame() { return mPlayFrame - mBaseFrameNum; }
  bool getLoadChunk (int& chunkNum, int& frameNum, int preloadChunks);
  int getLengthFrame() { return getLastFrame(); }

  // sets
  void setBase (int chunkNum, int64_t pts,
                std::chrono::system_clock::time_point timePoint, std::chrono::seconds offset);

  virtual void setPlayFrame (int frame);

private:
  int mFramesPerChunk = 0;

  int mBaseChunkNum = 0;
  int mBaseFrameNum = 0;
  int64_t mBasePts = -1;
  std::chrono::system_clock::time_point mBaseTimePoint;
  };
//}}}
