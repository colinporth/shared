// cSong.h - set of audioFrames, videoFrames added with own timing
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
  class cAudioFrame {
  public:
    static constexpr float kQuietThreshold = 0.01f;
    //{{{
    cAudioFrame (float* samples, bool owned, uint8_t* framePtr,
                 float* powerValues, float* peakValues, uint8_t* freqValues, uint8_t* lumaValues) :
        mSamples(samples), mOwned(owned), mPtr(framePtr),
        mPowerValues(powerValues), mPeakValues(peakValues),
        mFreqValues(freqValues), mFreqLuma(lumaValues), mMuted(false), mSilence(false) {}
    //}}}
    //{{{
    ~cAudioFrame() {

      if (mOwned)
        free (mSamples);

      free (mPowerValues);
      free (mPeakValues);
      free (mFreqValues);
      free (mFreqLuma);
      }
    //}}}

    // gets
    float* getSamples() { return mSamples; }
    uint8_t* getPtr() { return mPtr; }

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

  private:
    // vars
    float* mSamples;
    bool mOwned;
    bool mMuted;
    bool mSilence;

    // hold onto mapped file framePtr
    uint8_t* mPtr;

    float* mPowerValues;
    float* mPeakValues;
    uint8_t* mFreqValues;
    uint8_t* mFreqLuma;

    std::string mTitle;
    };
  //}}}
  //{{{
  class cVideoFrame {
  public:
    cVideoFrame(uint8_t* pes, int pesLen) : mPes(pes), mPesLen(pesLen) {}
    ~cVideoFrame() {
      free (mPes);
      }

    // gets
    uint8_t* getPes() { return mPes; }
    int getPesLen() { return mPesLen; }

  private:
    uint8_t* mPes;
    int mPesLen;
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

  void init (cAudioDecode::eFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame);
  void addAudioFrame (int frameNum, float* samples, bool owned, int totalFrames, uint8_t* framePtr = nullptr);
  void addVideoFrame (int frameNum, uint8_t* pes, int pesLen);
  void clear();

  enum eHlsLoad { eHlsIdle, eHlsLoading, eHlsFailed };
  //{{{  gets
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  int getId() { return mId; }

  cAudioDecode::eFrameType getFrameType() { return mFrameType; }
  int getNumChannels() { return mNumChannels; }
  int getNumSampleBytes() { return mNumChannels * sizeof(float); }
  int getSampleRate() { return mSampleRate; }
  int getSamplesPerFrame() { return mSamplesPerFrame; }

  int getFirstFrame() { return mAudioFrameMap.empty() ? 0 : mAudioFrameMap.begin()->first; }
  int getLastFrame() { return mAudioFrameMap.empty() ? 0 : mAudioFrameMap.rbegin()->first;  }
  int getNumFrames() { return mAudioFrameMap.empty() ? 0 : (mAudioFrameMap.rbegin()->first - mAudioFrameMap.begin()->first + 1); }
  int getTotalFrames() { return mTotalFrames; }
  int getPlayFrame() { return mPlayFrame; }
  int getBasePlayFrame() { return mPlayFrame - mHlsBaseFrame; }

  //{{{
  cAudioFrame* getAudioFramePtr (int frame) {
    auto it = mAudioFrameMap.find (frame);
    return (it == mAudioFrameMap.end()) ? nullptr : it->second;
    }
  //}}}
  //{{{
  cVideoFrame* getVideoFramePtr (int frame) {
    auto it = mVideoFrameMap.find (frame);
    return (it == mVideoFrameMap.end()) ? nullptr : it->second;
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
  std::string getChan() { return mChan; }

  // hls
  bool hasHlsBase() { return mHlsBaseValid; }
  eHlsLoad getHlsLoad() { return mHlsLoad; }
  int getHlsLoadChunkNum (std::chrono::system_clock::time_point now, std::chrono::seconds secs, int preload);
  int getHlsFrameFromChunkNum (int chunkNum) { return mHlsBaseFrame + (chunkNum - mHlsBaseChunkNum) * mHlsFramesPerChunk; }
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


  // playFrame
  void setPlayFrame (int frame);
  void incPlaySec (int secs, bool useSelectRange);
  void incPlayFrame (int frames, bool useSelectRange);

  // stream
  void setChan (const std::string& chan) { mChan = chan; }
  //{{{
  void setBitrate (int bitrate, int framesPerChunk) {
    mBitrate = bitrate;
    mHlsFramesPerChunk = framesPerChunk;
    }
  //}}}

  // hls
  void setHlsBase (int chunkNum, std::chrono::system_clock::time_point timePoint, std::chrono::seconds offset);
  void setHlsLoad (eHlsLoad hlsLoad, int chunkNum);
  //}}}

  // actions
  void prevSilencePlayFrame();
  void nextSilencePlayFrame();

private:
  void clearFrames();

  void checkSilenceWindow (int frameNum);
  int skipPrev (int fromFrame, bool silence);
  int skipNext (int fromFrame, bool silence);

  constexpr static int kMaxNumChannels = 2;           // arbitrary chan max
  constexpr static int kMaxNumSamplesPerFrame = 2048; // arbitrary frame max
  constexpr static int kMaxFreq = (kMaxNumSamplesPerFrame / 2) + 1; // fft max
  constexpr static int kMaxFreqBytes = 512; // arbitrary graphics max

  // vars
  std::shared_mutex mSharedMutex;
  std::map <int, cAudioFrame*> mAudioFrameMap;
  std::map <int, cVideoFrame*> mVideoFrameMap;

  cAudioDecode::eFrameType mFrameType = cAudioDecode::eUnknown;
  bool owned = false;

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
  std::string mChan;
  int mBitrate = 0;

  bool mHlsBaseValid = false;
  int mHlsBaseChunkNum = 0;
  int mHlsBaseFrame = 0;
  std::chrono::system_clock::time_point mHlsBaseTimePoint;

  int mHlsFramesPerChunk = 0;
  int eHlsFailedChunkNum = 0;
  eHlsLoad mHlsLoad = eHlsIdle;
  //}}}
  //{{{  fft vars
  kiss_fftr_cfg fftrConfig;
  kiss_fft_scalar timeBuf[kMaxNumSamplesPerFrame];
  kiss_fft_cpx freqBuf[kMaxFreq];
  //}}}
  };
