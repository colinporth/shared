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

#include "../decoders/cAudioDecode.h"

#include "../kissFft/kiss_fft.h"
#include "../kissFft/kiss_fftr.h"
//}}}

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
  virtual ~cSong();

  void initialise (cAudioDecode::eFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame,
                   int maxMapSize = 0);
  void clear();
  void addFrame (int frameNum, float* samples, bool ourSamples, int totalFrames, int64_t pts);

  //{{{  gets
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  int getId() { return mId; }
  bool getChanged() { return mChanged; }

  cAudioDecode::eFrameType getFrameType() { return mFrameType; }
  int getNumChannels() { return mNumChannels; }
  int getNumSampleBytes() { return mNumChannels * sizeof(float); }
  int getSampleRate() { return mSampleRate; }
  int getSamplesPerFrame() { return mSamplesPerFrame; }

  int getFirstFrame() { return mFrameMap.empty() ? 0 : mFrameMap.begin()->first; }
  int getLastFrame() { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->first;  }
  int getNumFrames() { return mFrameMap.empty() ? 0 : (mFrameMap.rbegin()->first - mFrameMap.begin()->first + 1); }
  int getTotalFrames() { return mTotalFrames; }
  int getPlayFrame() { return mPlayFrame; }
  int getBasePlayFrame() { return mPlayFrame - mHlsBaseFrame; }

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

  // info
  int getBitrate() { return mBitrate; }
  std::string getChannel() { return mChannel; }
  //}}}
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
  void setPlayFrame (int frame);
  void incPlaySec (int secs, bool useSelectRange);
  void incPlayFrame (int frames, bool useSelectRange);

  // stream
  void setChannel (const std::string& channel) { mChannel = channel; }
  //{{{
  void setBitrateFramesPerChunk (int bitrate, int framesPerChunk) {
    mBitrate = bitrate;
    mHlsFramesPerChunk = framesPerChunk;
    }
  //}}}

  // hls
  void setHlsBase (int chunkNum, std::chrono::system_clock::time_point timePoint, std::chrono::seconds offset);
  //}}}

  // hls
  int getLastChunkNum() { return mLastChunkNum; }
  int getLastFrameNum() { return mLastFrameNum; }
  bool hasHlsBase() { return mHlsBaseValid; }
  bool loadChunk (std::chrono::system_clock::time_point now, int preloadChunks, int& chunkNum, int& frameNum);

  // actions
  void prevSilencePlayFrame();
  void nextSilencePlayFrame();

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
  std::shared_mutex mSharedMutex;
  std::map <int, cFrame*> mFrameMap;

  cAudioDecode::eFrameType mFrameType = cAudioDecode::eUnknown;
  bool mOurSamples = false;
  bool mChanged = false;

  int mId = 0;
  int mNumChannels = kMaxNumChannels;
  int mSampleRate = 0;
  int mSamplesPerFrame = 0;

  int mPlayFrame = 0;
  int mTotalFrames = 0;
  cSelect mSelect;

  //{{{  max stuff for ui
  float mMaxPowerValue = 0.f;
  float mMaxPeakValue = 0.f;
  float mMaxFreqValue = 0.f;
  //}}}
  //{{{  hls vars
  std::string mChannel;
  int mBitrate = 0;

  bool mHlsBaseValid = false;
  int mHlsBaseChunkNum = 0;
  int mHlsBaseFrame = 0;
  std::chrono::system_clock::time_point mHlsBaseTimePoint;

  int mHlsFramesPerChunk = 0;

  int mLastChunkNum;
  int mLastFrameNum;
  //}}}
  //{{{  fft vars
  kiss_fftr_cfg mFftrConfig;
  kiss_fft_scalar mTimeBuf[kMaxNumSamplesPerFrame];
  kiss_fft_cpx mFreqBuf[kMaxFreq];
  //}}}
  };
