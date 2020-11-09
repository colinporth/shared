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

class cSong {
public:
  //{{{
  class cFrame {
  public:
    cFrame (int numChannels, int numFreqBytes, float* samples, bool ownSamples, int64_t pts);
    virtual ~cFrame();

    // gets
    float* getSamples() { return mSamples; }
    int64_t getPts() { return mPts; }

    float* getPowerValues() { return mPowerValues;  }
    float* getPeakValues() { return mPeakValues;  }
    uint8_t* getFreqValues() { return mFreqValues; }
    uint8_t* getFreqLuma() { return mFreqLuma; }

    bool isQuiet() { return mPeakValues[0] + mPeakValues[1] < kQuietThreshold; }

    bool isMuted() { return mMuted; }
    bool isSilence() { return mSilence; }
    void setSilence (bool silence) { mSilence = silence; }

    bool hasTitle() { return !mTitle.empty(); }
    std::string getTitle() { return mTitle; }

    // vars
    float* mSamples;
    bool mOwnSamples;
    int64_t mPts;

    float* mPowerValues;
    float* mPeakValues;
    uint8_t* mFreqValues;
    uint8_t* mFreqLuma;

  private:
    static constexpr float kQuietThreshold = 0.01f;

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

      cSelectItem(eType type, int64_t firstFramePts, int64_t lastFramePts, const std::string& title) :
        mType(type), mFirstFramePts(firstFramePts), mLastFramePts(lastFramePts), mTitle(title) {}

      eType getType() { return mType; }
      int64_t getFirstFramePts() { return mFirstFramePts; }
      int64_t getLastFramePts() { return mLastFramePts; }
      bool getMark() { return getFirstFramePts() == getLastFramePts(); }
      std::string getTitle() { return mTitle; }
      //{{{
      bool inRange (int64_t FramePts) {
      // ignore 1 FramePts select range
        return (mFirstFramePts != mLastFramePts) && (FramePts >= mFirstFramePts) && (FramePts <= mLastFramePts);
        }
      //}}}

      void setType (eType type) { mType = type; }
      void setFirstFramePts (int64_t framePts) { mFirstFramePts = framePts; }
      void setLastFramePts (int64_t framePts) { mLastFramePts = framePts; }
      void setTitle (const std::string& title) { mTitle = title; }

    private:
      eType mType;
      int64_t mFirstFramePts = 0;
      int64_t mLastFramePts = 0;
      std::string mTitle;
      };
    //}}}

    // gets
    bool empty() { return mItems.empty(); }
    int64_t getFirstFramePts() { return empty() ? 0 : mItems.front().getFirstFramePts(); }
    int64_t getLastFramePts() { return empty() ? 0 : mItems.back().getLastFramePts(); }
    std::vector<cSelectItem>& getItems() { return mItems; }

    bool inRange (int64_t framePts);
    int64_t constrainToRange (int64_t framePts, int64_t constrainedFramePts);

    // actions
    void clearAll();
    void addMark (int64_t framePts, const std::string& title = "");
    void start (int64_t framePts);
    void move (int64_t framePts);
    void end();

  private:
    enum eEdit { eEditNone, eEditFirst, eEditLast, eEditRange };

    eEdit mEdit = eEditNone;
    int64_t mEditFramePts = 0;
    std::vector<cSelectItem> mItems;
    int mItemNum = 0;
    };
  //}}}
  cSong (eAudioFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame,
         int64_t ptsDuration, int maxMapSize);
  virtual ~cSong();

  //{{{  gets
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  bool getChanged() { return mChanged; }

  eAudioFrameType getFrameType() { return mFrameType; }
  int getNumChannels() { return mNumChannels; }
  int getNumSampleBytes() { return mNumChannels * sizeof(float); }
  int getSampleRate() { return mSampleRate; }
  int getSamplesPerFrame() { return mSamplesPerFrame; }

  int64_t getPtsDuration() { return mPtsDuration; }
  int getFrameNumFromPts (int64_t pts) { return int (pts / mPtsDuration); }
  int64_t getPtsFromFrameNum (int frameNum) { return frameNum * mPtsDuration; }

  // pts
  int64_t getPlayPts() { return mPlayPts; }
  int64_t getFirstPts() { return mFrameMap.empty() ? 0 : mFrameMap.begin()->second->getPts(); }
  int64_t getLastPts() { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->second->getPts();  }

  // frameNum - useful for graphics
  int getPlayFrameNum() { return getFrameNumFromPts (mPlayPts); }
  int getFirstFrameNum() { return mFrameMap.empty() ? 0 : mFrameMap.begin()->first; }
  int getLastFrameNum() { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->first;  }
  int getNumFrames() { return mFrameMap.empty() ? 0 : (mFrameMap.rbegin()->first - mFrameMap.begin()->first + 1); }
  int getTotalFrames() { return mTotalFrames; }

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
  void setChanged (bool changed) { mChanged = changed; }

  //{{{
  cFrame* findFrameByFrameNum (int frameNum) {
    auto it = mFrameMap.find (frameNum);
    return (it == mFrameMap.end()) ? nullptr : it->second;
    }
  //}}}
  cFrame* findFrameByPts (int64_t pts) { return findFrameByFrameNum (getFrameNumFromPts (pts)); }
  cFrame* findPlayFrame() { return findFrameByPts (mPlayPts); }

  void addFrame (bool reuseFront, int64_t pts, float* samples, bool ownSamples, int totalFrames);

  // playFrame
  virtual void setPlayPts (int64_t pts);
  void nextPlayFrame (bool useSelectRange);
  void incPlaySec (int secs, bool useSelectRange);

  // actions
  void prevSilencePlayFrame();
  void nextSilencePlayFrame();

protected:
  //{{{  vars
  const int mSampleRate = 0;
  const int mSamplesPerFrame = 0;
  int64_t mPlayPts = 0;
  int64_t mPtsDuration = 1;
  std::shared_mutex mSharedMutex;
  //}}}

private:
  //{{{  static constexpr
  static constexpr int kMaxNumChannels = 2;           // arbitrary chan max
  static constexpr int kMaxNumSamplesPerFrame = 2048; // arbitrary frame max
  static constexpr int kMaxFreq = (kMaxNumSamplesPerFrame / 2) + 1; // fft max
  static constexpr int kMaxFreqBytes = 512; // arbitrary graphics max
  //}}}
  //{{{  members
  int64_t skipPrev (int64_t fromPts, bool silence);
  int64_t skipNext (int64_t fromPts, bool silence);
  void checkSilenceWindow (int64_t pts);
  //}}}
  //{{{  vars
  const eAudioFrameType mFrameType;
  const int mNumChannels;

  // map of frames keyed by frame pts/ptsDuration
  // - frameNum offset by firstFrame pts/ptsDuration
  std::map <int, cFrame*> mFrameMap;
  int mMaxMapSize = 0;
  int mTotalFrames = 0;

  bool mOwnSamples = false;
  bool mChanged = false;

  cSelect mSelect;

  // fft vars
  kiss_fftr_cfg mFftrConfig;
  kiss_fft_scalar mTimeBuf[kMaxNumSamplesPerFrame];
  kiss_fft_cpx mFreqBuf[kMaxFreq];

  // max stuff for ui
  float mMaxPowerValue = 0.f;
  float mMaxPeakValue = 0.f;
  float mMaxFreqValue = 0.f;
  //}}}
  };

// cHlsSong - cSong with added HLS
class cHlsSong : public cSong {
public:
  cHlsSong (eAudioFrameType frameType, int numChannels,
            int sampleRate, int samplesPerFrame, int framesPerChunk,
            int64_t ptsDuration, int maxMapSize);
  virtual ~cHlsSong();

  // gets
  int64_t getBasePlayPts() { return mPlayPts - mBasePts; }
  bool getLoadChunk (int& chunkNum, int64_t& pts, int preloadChunks);
  int64_t getLengthPts() { return getLastPts(); }

  // sets
  void setBase (int chunkNum, int64_t pts, std::chrono::system_clock::time_point timePoint, std::chrono::seconds offset);
  virtual void setPlayPts (int64_t pts);

private:
  //{{{  vars
  const int mFramesPerChunk = 0;

  int mBaseChunkNum = 0;
  int64_t mBasePts = -1;
  std::chrono::system_clock::time_point mBaseTimePoint;
  //}}}
  };
