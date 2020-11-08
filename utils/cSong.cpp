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
bool cSong::cSelect::inRange (int frame) {

  for (auto &item : mItems)
    if (item.inRange (frame))
      return true;

  return false;
  }
//}}}
//{{{
int cSong::cSelect::constrainToRange (int frame, int constrainedFrame) {
// if frame in a select range return frame constrained to it

  for (auto &item : mItems) {
    if (item.inRange (frame)) {
      if (constrainedFrame > item.getLastFrame())
        return item.getFirstFrame();
      else if (constrainedFrame < item.getFirstFrame())
        return item.getFirstFrame();
      else
        return constrainedFrame;
      }
    }

  return constrainedFrame;
  }
//}}}

//{{{
void cSong::cSelect::clearAll() {

  mItems.clear();

  mEdit = eEditNone;
  mEditFrame = 0;
  }
//}}}
//{{{
void cSong::cSelect::addMark (int frame, const std::string& title) {
  mItems.push_back (cSelectItem (cSelectItem::eLoop, frame, frame, title));
  mEdit = eEditLast;
  mEditFrame = frame;
  }
//}}}
//{{{
void cSong::cSelect::start (int frame) {

  mEditFrame = frame;

  mItemNum = 0;
  for (auto &item : mItems) {
    // pick from select range
    if (abs(frame - item.getLastFrame()) < 2) {
      mEdit = eEditLast;
      return;
      }
    else if (abs(frame - item.getFirstFrame()) < 2) {
      mEdit = eEditFirst;
      return;
      }
    else if (item.inRange (frame)) {
      mEdit = eEditRange;
      return;
      }
    mItemNum++;
    }

  // add new select item
  mItems.push_back (cSelectItem (cSelectItem::eLoop, frame, frame, ""));
  mEdit = eEditLast;
  }
//}}}
//{{{
void cSong::cSelect::move (int frame) {

  if (mItemNum < (int)mItems.size()) {
    switch (mEdit) {
      case eEditFirst:
        mItems[mItemNum].setFirstFrame (frame);
        if (mItems[mItemNum].getFirstFrame() > mItems[mItemNum].getLastFrame()) {
          mItems[mItemNum].setLastFrame (frame);
          mItems[mItemNum].setFirstFrame (mItems[mItemNum].getLastFrame());
          }
        break;

      case eEditLast:
        mItems[mItemNum].setLastFrame (frame);
        if (mItems[mItemNum].getLastFrame() < mItems[mItemNum].getFirstFrame()) {
          mItems[mItemNum].setFirstFrame (frame);
          mItems[mItemNum].setLastFrame (mItems[mItemNum].getFirstFrame());
          }
        break;

      case eEditRange:
        mItems[mItemNum].setFirstFrame (mItems[mItemNum].getFirstFrame() + frame - mEditFrame);
        mItems[mItemNum].setLastFrame (mItems[mItemNum].getLastFrame() + frame - mEditFrame);
        mEditFrame = frame;
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
  mEditFrame = 0;
  }
//}}}

// cSong
//{{{
cSong::cSong (eAudioFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame, 
              int64_t(ptsDuration), int maxMapSize)
  : mSampleRate(sampleRate), mSamplesPerFrame(samplesPerFrame),
    mFrameType(frameType), mNumChannels(numChannels),
    mPtsDuration(ptsDuration), mMaxMapSize(maxMapSize) {

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
int64_t cSong::getPlayPts() {
  auto framePtr = getFramePtr (mPlayFrame);
  return framePtr ? framePtr->getPts() : -1;
  }
//}}}

//{{{
void cSong::addFrame (bool reuseFront, int frameNum, float* samples, bool ownSamples, int totalFrames, int64_t pts) {

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
  mFrameMap.insert (map<int,cFrame*>::value_type (frameNum, frame));
  // totalFrames can be a changing estimate for file, or increasing value for streaming
  mTotalFrames = totalFrames;
  } // end of locked mutex
  //}}}

  checkSilenceWindow (frameNum);
  }
//}}}

// playFrame
//{{{
void cSong::setPlayFrame (int frame) {
  mPlayFrame = min (max (frame, 0), getLastFrame()+1);
  }
//}}}
//{{{
void cSong::incPlayFrame (int frames, bool constrainToRange) {

  int playFrame = mPlayFrame + frames;
  if (constrainToRange)
    playFrame = mSelect.constrainToRange (mPlayFrame, playFrame);

  setPlayFrame (playFrame);
  }
//}}}
//{{{
void cSong::incPlaySec (int secs, bool useSelectRange) {
  incPlayFrame ((secs * mSampleRate) / mSamplesPerFrame, useSelectRange);
  }
//}}}

// actions
//{{{
void cSong::prevSilencePlayFrame() {
  mPlayFrame = skipPrev (mPlayFrame, false);
  mPlayFrame = skipPrev (mPlayFrame, true);
  mPlayFrame = skipPrev (mPlayFrame, false);
  }
//}}}
//{{{
void cSong::nextSilencePlayFrame() {
  mPlayFrame = skipNext (mPlayFrame, true);
  mPlayFrame = skipNext (mPlayFrame, false);
  mPlayFrame = skipNext (mPlayFrame, true);
  }
//}}}

// private
//{{{
int cSong::skipPrev (int fromFrame, bool silence) {

  for (int frame = fromFrame-1; frame >= getFirstFrame(); frame--) {
    auto framePtr = getFramePtr (frame);
    if (framePtr && (framePtr->isSilence() ^ silence))
      return frame;
    }

  return fromFrame;
  }
//}}}
//{{{
int cSong::skipNext (int fromFrame, bool silence) {

  for (int frame = fromFrame; frame <= getLastFrame(); frame++) {
    auto framePtr = getFramePtr (frame);
    if (framePtr && (framePtr->isSilence() ^ silence))
      return frame;
    }

  return fromFrame;
  }
//}}}
//{{{
void cSong::checkSilenceWindow (int frameNum) {

  unique_lock<shared_mutex> lock (mSharedMutex);

  // walk backwards looking for continuous loaded quiet frames
  auto windowSize = 0;
  while (true) {
    auto framePtr = getFramePtr (frameNum);
    if (framePtr && framePtr->isQuiet()) {
      windowSize++;
      frameNum--;
      }
    else
      break;
    };

  if (windowSize > kSilenceWindowFrames) {
    // walk forward setting silence for continuous loaded quiet frames
    while (true) {
      auto framePtr = getFramePtr (++frameNum);
      if (framePtr && framePtr->isQuiet())
        framePtr->setSilence (true);
      else
        break;
      }
    }
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
bool cHlsSong::getLoadChunk (int& chunkNum, int& frameNum, int preloadChunks) {
// return true if a chunk load needed to play mPlayFrame
// - update chunkNum and frameNum
// !!!! dodgy hard coding of chunk duration 6400ms !!!!

  system_clock::time_point now = system_clock::now();

  // get offsets of playFrame from baseFrame, handle -v offsets correctly
  int frameOffset = mPlayFrame - mBaseFrameNum;
  int chunkNumOffset = (frameOffset >= 0)  ? (frameOffset / mFramesPerChunk) :
                                             -((mFramesPerChunk - 1 - frameOffset) / mFramesPerChunk);

  // loop until chunkNum with unloaded frame, chunkNum not available yet, or preload ahead of playFrame loaded
  int loadedChunks = 0;
  int secs = (preloadChunks * mFramesPerChunk * mSamplesPerFrame) / mSampleRate;
  while ((loadedChunks < preloadChunks) &&
         ((now - (mBaseTimePoint + (chunkNumOffset * 6400ms))).count() > secs))
    // chunkNum chunk should be available
    if (!getFramePtr (mBaseFrameNum + (chunkNumOffset * mFramesPerChunk))) {
      // not loaded, return chunkNum to load
      chunkNum = mBaseChunkNum + chunkNumOffset;
      frameNum = mBaseFrameNum + (chunkNum - mBaseChunkNum) * mFramesPerChunk;
      return true;
      }
    else {
      // already loaded, next
      loadedChunks++;
      chunkNumOffset++;
      }

  // return false, no chunkNum available to load
  chunkNum = 0;
  frameNum = 0;
  return false;
  }
//}}}
//{{{
void cHlsSong::setBase (int chunkNum, int64_t pts, system_clock::time_point timePoint, seconds offset) {
// set baseChunkNum, baseTimePoint and baseFrame (sinceMidnight)

  unique_lock<shared_mutex> lock (mSharedMutex);

  mBaseChunkNum = chunkNum;
  mBasePts = pts;

  timePoint += offset;
  mBaseTimePoint = timePoint;

  // calc hlsBaseFrame
  auto midnightTimePoint = date::floor<date::days>(timePoint);
  uint64_t msSinceMidnight = duration_cast<milliseconds>(timePoint - midnightTimePoint).count();
  mBaseFrameNum = int((msSinceMidnight * mSampleRate) / mSamplesPerFrame / 1000);
  mPlayFrame = mBaseFrameNum;
  }
//}}}
//{{{
void cHlsSong::setPlayFrame (int frame) {
  mPlayFrame = min (frame, getLastFrame()+1);
  }
//}}}
