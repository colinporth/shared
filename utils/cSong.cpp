// cSong.cpp - audioFrames container
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cSong.h"

#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}
//{{{  constexpr
constexpr static float kMinPowerValue = 0.25f;
constexpr static float kMinPeakValue = 0.25f;
constexpr static float kMinFreqValue = 256.f;
constexpr static int kSilenceWindowFrames = 4;
//}}}

// cSong::cFrame
//{{{
cSong::cFrame::cFrame (int numChannels, int numFreqBytes, float* samples, bool ownSamples, int64_t pts)
   : mSamples(samples), mOwnSamples(ownSamples), mPts(pts), mMuted(false), mSilence(false) {

  mPowerValues = (float*)malloc (numChannels * 4);
  memset (mPowerValues, 0, numChannels * 4);

  mPeakValues = (float*)malloc (numChannels * 4);
  memset (mPeakValues, 0, numChannels * 4);

  mFreqValues = (uint8_t*)malloc (numFreqBytes);
  mFreqLuma = (uint8_t*)malloc (numFreqBytes);
  }
//}}}
//{{{
cSong::cFrame::~cFrame() {

  if (mOwnSamples)
    free (mSamples);

  free (mPowerValues);
  free (mPeakValues);
  free (mFreqValues);
  free (mFreqLuma);
  }
//}}}

// cSong::cSelect
//{{{
bool cSong::cSelect::inRange (int64_t framePts) {

  for (auto &item : mItems)
    if (item.inRange (framePts))
      return true;

  return false;
  }
//}}}
//{{{
int64_t cSong::cSelect::constrainToRange (int64_t framePts, int64_t constrainedFramePts) {
// if FramePts in a select range return FramePts constrained to it

  for (auto &item : mItems) {
    if (item.inRange (framePts)) {
      if (constrainedFramePts > item.getLastFramePts())
        return item.getFirstFramePts();
      else if (constrainedFramePts < item.getFirstFramePts())
        return item.getFirstFramePts();
      else
        return constrainedFramePts;
      }
    }

  return constrainedFramePts;
  }
//}}}

//{{{
void cSong::cSelect::clearAll() {

  mItems.clear();

  mEdit = eEditNone;
  mEditFramePts = 0;
  }
//}}}
//{{{
void cSong::cSelect::addMark (int64_t pts, const std::string& title) {
  mItems.push_back (cSelectItem (cSelectItem::eLoop, pts, pts, title));
  mEdit = eEditLast;
  mEditFramePts = pts;
  }
//}}}
//{{{
void cSong::cSelect::start (int64_t framePts) {

  mEditFramePts = framePts;

  mItemNum = 0;
  for (auto &item : mItems) {
    // pick from select range
    if (abs(framePts - item.getLastFramePts()) < 2) {
      mEdit = eEditLast;
      return;
      }
    else if (abs(framePts - item.getFirstFramePts()) < 2) {
      mEdit = eEditFirst;
      return;
      }
    else if (item.inRange (framePts)) {
      mEdit = eEditRange;
      return;
      }
    mItemNum++;
    }

  // add new select item
  mItems.push_back (cSelectItem (cSelectItem::eLoop, framePts, framePts, ""));
  mEdit = eEditLast;
  }
//}}}
//{{{
void cSong::cSelect::move (int64_t framePts) {

  if (mItemNum < (int)mItems.size()) {
    switch (mEdit) {
      case eEditFirst:
        mItems[mItemNum].setFirstFramePts (framePts);
        if (mItems[mItemNum].getFirstFramePts() > mItems[mItemNum].getLastFramePts()) {
          mItems[mItemNum].setLastFramePts (framePts);
          mItems[mItemNum].setFirstFramePts (mItems[mItemNum].getLastFramePts());
          }
        break;

      case eEditLast:
        mItems[mItemNum].setLastFramePts (framePts);
        if (mItems[mItemNum].getLastFramePts() < mItems[mItemNum].getFirstFramePts()) {
          mItems[mItemNum].setFirstFramePts (framePts);
          mItems[mItemNum].setLastFramePts (mItems[mItemNum].getFirstFramePts());
          }
        break;

      case eEditRange:
        mItems[mItemNum].setFirstFramePts (mItems[mItemNum].getFirstFramePts() + framePts - mEditFramePts);
        mItems[mItemNum].setLastFramePts (mItems[mItemNum].getLastFramePts() + framePts - mEditFramePts);
        mEditFramePts = framePts;
        break;

      default:
        cLog::log (LOGERROR, "moving invalid select");
      }
    }
  }
//}}}
//{{{
void cSong::cSelect::end() {
  mEdit = eEditNone;
  mEditFramePts = 0;
  }
//}}}

// cSong
//{{{
cSong::cSong (eAudioFrameType frameType, int numChannels,
              int sampleRate, int samplesPerFrame,
              int64_t ptsDuration, int maxMapSize)
    : mSampleRate(sampleRate), mSamplesPerFrame(samplesPerFrame),
      mFrameType(frameType), mNumChannels(numChannels),
      mMaxMapSize(maxMapSize), mPtsDuration(ptsDuration) {

  mFftrConfig = kiss_fftr_alloc (mSamplesPerFrame, 0, 0, 0);
  }
//}}}
//{{{
cSong::~cSong() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  // reset frames
  mSelect.clearAll();

  for (auto frame : mFrameMap)
    delete (frame.second);
  mFrameMap.clear();

  // delloc this???
  // kiss_fftr_alloc (mSamplesPerFrame, 0, 0, 0);
  }
//}}}

//{{{
cSong::cFrame* cSong::findFrameByFrame (int frame) {
  auto it = mFrameMap.find (frame);
  return (it == mFrameMap.end()) ? nullptr : it->second;
  }
//}}}
//{{{
cSong::cFrame* cSong::findFrameByPts (int64_t pts) {
  auto it = mFrameMap.find (int(pts / mPtsDuration));
  return (it == mFrameMap.end()) ? nullptr : it->second;
  }
//}}}
//{{{
void cSong::addFrame (bool reuseFront, int64_t pts, float* samples, bool ownSamples, int totalFrames) {

  cFrame* frame;
  if (mMaxMapSize && (int(mFrameMap.size()) > mMaxMapSize)) { // reuse a cFrame
    //{{{  remove with locked mutex
    {
    unique_lock<shared_mutex> lock (mSharedMutex);
    if (reuseFront) {
      //{{{  remove frame from map begin, reuse it
      auto it = mFrameMap.begin();
      frame = (*it).second;
      mFrameMap.erase (it);
      }
      //}}}
    else {
      //{{{  remove frame from map end, reuse it
      auto it = prev (mFrameMap.end());
      frame = (*it).second;
      mFrameMap.erase (it);
      }
      //}}}
    } // end of locked mutex
    //}}}
    //{{{  reuse power,peak,fft buffers, but free samples if we own them
    if (frame->mOwnSamples)
      free (frame->mSamples);
    frame->mSamples = samples;
    frame->mOwnSamples = ownSamples;
    frame->mPts = pts;
    //}}}
    }
  else // allocate new cFrame
    frame = new cFrame (mNumChannels, getNumFreqBytes(), samples, ownSamples, pts);

  //{{{  calc power,peak
  for (auto channel = 0; channel < mNumChannels; channel++) {
    frame->mPowerValues[channel] = 0.f;
    frame->mPeakValues[channel] = 0.f;
    }

  auto samplePtr = samples;
  for (int sample = 0; sample < mSamplesPerFrame; sample++) {
    mTimeBuf[sample] = 0;
    for (auto channel = 0; channel < mNumChannels; channel++) {
      auto value = *samplePtr++;
      mTimeBuf[sample] += value;
      frame->mPowerValues[channel] += value * value;
      frame->mPeakValues[channel] = max (abs(frame->mPeakValues[channel]), value);
      }
    }

  // max
  for (auto channel = 0; channel < mNumChannels; channel++) {
    frame->mPowerValues[channel] = sqrtf (frame->mPowerValues[channel] / mSamplesPerFrame);
    mMaxPowerValue = max (mMaxPowerValue, frame->mPowerValues[channel]);
    mMaxPeakValue = max (mMaxPeakValue, frame->mPeakValues[channel]);
    }
  //}}}

  // ??? lock against init fftrConfig recalc???
  kiss_fftr (mFftrConfig, mTimeBuf, mFreqBuf);
  //{{{  calc frequency values,luma
  float freqScale = 255.f / mMaxFreqValue;
  auto freqBufPtr = mFreqBuf;
  auto freqValuesPtr = frame->mFreqValues;
  auto lumaValuesPtr = frame->mFreqLuma + getNumFreqBytes() - 1;
  for (auto i = 0; i < getNumFreqBytes(); i++) {
    float value = sqrtf (((*freqBufPtr).r * (*freqBufPtr).r) + ((*freqBufPtr).i * (*freqBufPtr).i));
    mMaxFreqValue = std::max (mMaxFreqValue, value);

    // freq scaled to byte, used by gui
    value *= freqScale;
    *freqValuesPtr++ = value > 255 ? 255 : uint8_t(value);

    // crush luma, reverse index, used by gui copy to bitmap
    value *= 4.f;
    *lumaValuesPtr-- = value > 255 ? 255 : uint8_t(value);

    freqBufPtr++;
    }
  //}}}

  //{{{  insert with locked mutex
  {
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFrameMap.insert (map<int,cFrame*>::value_type (int(pts / mPtsDuration), frame));
  // totalFrames can be a changing estimate for file, or increasing value for streaming
  mTotalFrames = totalFrames;
  } // end of locked mutex
  //}}}

  checkSilenceWindow (pts);
  }
//}}}

// playFrame
//{{{
void cSong::setPlayPts (int64_t pts) {
  mPlayPts = min (max (pts, 0LL), getLastPts()); // +1); ??
  }
//}}}
//{{{
void cSong::nextPlayFrame (bool constrainToRange) {

  int64_t playPts = mPlayPts + mPtsDuration;
  //if (constrainToRange)
  //  int64_t = mSelect.constrainToRange (mPlayFrame, int64_t);

  setPlayPts (playPts);
  }
//}}}
//{{{
void cSong::incPlaySec (int secs, bool useSelectRange) {

  int64_t frames = (secs * mSampleRate) / mSamplesPerFrame;
  mPlayPts += frames * mPtsDuration;
  }
//}}}

// actions
//{{{
void cSong::prevSilencePlayFrame() {
  mPlayPts = skipPrev (mPlayPts, false);
  mPlayPts = skipPrev (mPlayPts, true);
  mPlayPts = skipPrev (mPlayPts, false);
  }
//}}}
//{{{
void cSong::nextSilencePlayFrame() {
  mPlayPts = skipNext (mPlayPts, true);
  mPlayPts = skipNext (mPlayPts, false);
  mPlayPts = skipNext (mPlayPts, true);
  }
//}}}

// private
//{{{
int64_t cSong::skipPrev (int64_t fromPts, bool silence) {

  //for (int frame = fromFrame-1; frame >= getFirstFrame(); frame--) {
  //  auto framePtr = findFrame (frame);
  //  if (framePtr && (framePtr->isSilence() ^ silence))
  //    return frame;
  // }

  return fromPts;
  }
//}}}
//{{{
int64_t cSong::skipNext (int64_t fromPts, bool silence) {

  //for (int frame = fromFrame; frame <= getLastFrame(); frame++) {
  //  auto framePtr = findFrame (frame);
  //  if (framePtr && (framePtr->isSilence() ^ silence))
  //    return frame;
  //  }

  return fromPts;
  }
//}}}
//{{{
void cSong::checkSilenceWindow (int64_t pts) {

  //unique_lock<shared_mutex> lock (mSharedMutex);

  // walk backwards looking for continuous loaded quiet frames
  //auto windowSize = 0;
  //while (true) {
  //  auto framePtr = findFrame (frameNum);
  //  if (framePtr && framePtr->isQuiet()) {
  //    windowSize++;
  //    frameNum--;
  //    }
  //  else
  //    break;
  //  };

  //if (windowSize > kSilenceWindowFrames) {
  //  // walk forward setting silence for continuous loaded quiet frames
  //  while (true) {
  //    auto framePtr = findFrame (++frameNum);
  //    if (framePtr && framePtr->isQuiet())
  //      framePtr->setSilence (true);
  //    else
  //      break;
  //    }
  //  }
  }
//}}}

// cHlsSong
//{{{
cHlsSong::cHlsSong (eAudioFrameType frameType, int numChannels,
                   int sampleRate, int samplesPerFrame, int framesPerChunk,
                   int64_t ptsDuration, int maxMapSize)
  : cSong(frameType, numChannels, sampleRate, samplesPerFrame, ptsDuration, maxMapSize),
    mFramesPerChunk(framesPerChunk) {}
//}}}
cHlsSong::~cHlsSong() {}

//{{{
bool cHlsSong::getLoadChunk (int& chunkNum, int64_t& pts, int preloadChunks) {
// return true if a chunk load needed to play mPlayFrame
// - update chunkNum and frameNum
// !!!! dodgy hard coding of chunk duration 6400ms !!!!

  system_clock::time_point now = system_clock::now();

  // get offsets of playFrame from baseFrame, handle -v offsets correctly
  int64_t ptsOffset = mPlayPts - mBasePts;
  int chunkNumOffset = (ptsOffset >= 0)  ? int(((ptsOffset/mPtsDuration) / mFramesPerChunk)) :
                                           -int((mFramesPerChunk - 1 - (ptsOffset/mPtsDuration)) / mFramesPerChunk);

  // loop until chunkNum with unloaded frame, chunkNum not available yet, or preload ahead of playFrame loaded
  int loadedChunks = 0;
  int secs = (preloadChunks * mFramesPerChunk * mSamplesPerFrame) / mSampleRate;
  while ((loadedChunks < preloadChunks) &&
         ((now - (mBaseTimePoint + (chunkNumOffset * 6400ms))).count() > secs))
    // chunkNum chunk should be available
    if (!findFrameByPts (mBasePts + ((chunkNumOffset * mFramesPerChunk) * mPtsDuration))) {
      // not loaded, return chunkNum to load
      chunkNum = mBaseChunkNum + chunkNumOffset;
      pts = mBasePts + ((chunkNum - mBaseChunkNum) * mFramesPerChunk) * mPtsDuration;
      return true;
      }
    else {
      // already loaded, next
      loadedChunks++;
      chunkNumOffset++;
      }

  // return false, no chunkNum available to load
  chunkNum = 0;
  pts = -1;
  return false;
  }
//}}}
//{{{
void cHlsSong::setBase (int chunkNum, int64_t pts, system_clock::time_point timePoint, seconds offset) {
// set baseChunkNum, baseTimePoint and baseFrame (sinceMidnight)

  unique_lock<shared_mutex> lock (mSharedMutex);

  mBaseChunkNum = chunkNum;
  mBasePts = pts;

  // calc hlsBaseFrame
  //timePoint += offset;
  //mBaseTimePoint = timePoint;
  //auto midnightTimePoint = date::floor<date::days>(timePoint);
  //uint64_t msSinceMidnight = duration_cast<milliseconds>(timePoint - midnightTimePoint).count();
  //mBaseFrameNum = int((msSinceMidnight * mSampleRate) / mSamplesPerFrame / 1000);

  mPlayPts = mBasePts;
  }
//}}}
//{{{
void cHlsSong::setPlayPts (int64_t pts) {
  mPlayPts = min (pts, getLastPts() + mPtsDuration);
  }
//}}}
