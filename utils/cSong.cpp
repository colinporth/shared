// cSong.cpp - audioFrames container
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "cSong.h"

#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}

constexpr static float kMinPowerValue = 0.25f;
constexpr static float kMinPeakValue = 0.25f;
constexpr static float kMinFreqValue = 256.f;
constexpr static int kSilenceWindowFrames = 4;

// cSong::cFrame
//{{{
cSong::cFrame::cFrame (int numChannels, int numFreqBytes, float* samples, bool ourSamples, uint64_t pts)
   : mSamples(samples), mOurSamples(ourSamples), mPts (pts), mMuted(false), mSilence(false) {

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

  if (mOurSamples)
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
cSong::~cSong() {
  clearFrames();
  }
//}}}

//{{{
void cSong::init (cAudioDecode::eFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame,
                  int maxMapSize) {

  unique_lock<shared_mutex> lock (mSharedMutex);

  mMaxMapSize = maxMapSize;

  // reset frame type
  mFrameType = frameType;
  mNumChannels = numChannels;
  mSampleRate = sampleRate;
  mSamplesPerFrame = samplesPerFrame;
  mHlsBaseValid = false;

  clearFrames();

  // ??? should deallocate ???
  mFftrConfig = kiss_fftr_alloc (mSamplesPerFrame, 0, 0, 0);
  }
//}}}
//{{{
void cSong::addFrame (int frameNum, float* samples, bool ourSamples, int totalFrames, uint64_t pts) {

  cFrame* frame;
  if (mMaxMapSize && (int(mFrameMap.size()) > mMaxMapSize)) {
    // remove from front of map, reuse frame
    unique_lock<shared_mutex> lock (mSharedMutex);
    auto it = mFrameMap.begin();
    frame = (*it).second;
    mFrameMap.erase (it);

    // reuse power,peak,fft buffers, but free samples if we own them
    if (frame->mOurSamples)
      free (frame->mSamples);

    frame->mSamples = samples;
    frame->mOurSamples = ourSamples;
    frame->mPts = pts;
    }
  else
    frame = new cFrame (mNumChannels, getNumFreqBytes(), samples,ourSamples, pts);

  // totalFrames can be a changing estimate for file, or increasing value for streaming
  mTotalFrames = totalFrames;

  // peak
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

  for (auto channel = 0; channel < mNumChannels; channel++) {
    frame->mPowerValues[channel] = sqrtf (frame->mPowerValues[channel] / mSamplesPerFrame);
    mMaxPowerValue = max (mMaxPowerValue, frame->mPowerValues[channel]);
    mMaxPeakValue = max (mMaxPeakValue, frame->mPeakValues[channel]);
    }

  // ??? lock against init fftrConfig recalc???
  kiss_fftr (mFftrConfig, mTimeBuf, mFreqBuf);

  float freqScale = 255.f / mMaxFreqValue;

  auto freqBufPtr = mFreqBuf;
  auto freqValuesPtr = frame->mFreqValues;
  auto lumaValuesPtr = frame->mFreqLuma + getNumFreqBytes() - 1;
  for (auto i = 0; i < getNumFreqBytes(); i++) {
    float value = sqrtf (((*freqBufPtr).r * (*freqBufPtr).r) + ((*freqBufPtr).i * (*freqBufPtr).i));
    mMaxFreqValue = std::max (mMaxFreqValue, value);

    // freq scaled to byte, only used for display
    value *= freqScale;
    *freqValuesPtr++ = value > 255 ? 255 : uint8_t(value);

    // luma crushed, reversed index, for quicker copyMem to bitmap later
    value *= 4.f;
    *lumaValuesPtr-- = value > 255 ? 255 : uint8_t(value);

    freqBufPtr++;
    }

  unique_lock<shared_mutex> lock (mSharedMutex);

  mFrameMap.insert (map<int,cFrame*>::value_type (frameNum, frame));
  mTotalFrames = totalFrames;

  checkSilenceWindow (frameNum);
  }
//}}}
//{{{
void cSong::clear() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  mFrameType = cAudioDecode::eUnknown;
  mNumChannels = 0;
  mSampleRate = 0;
  mSamplesPerFrame = 0;
  mHlsBaseValid = false;

  clearFrames();

  // ??? should deallocate ???
  //fftrConfig = kiss_fftr_alloc (mSamplesPerFrame, 0, 0, 0);
  }
//}}}

// playFrame
//{{{
void cSong::setPlayFrame (int frame) {

  if (hasHlsBase())
    mPlayFrame = min (frame, getLastFrame()+1);
  else
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

// hls
//{{{
void cSong::setHlsBase (int chunkNum, system_clock::time_point timePoint, seconds offset, int startSecs) {
// set baseChunkNum, baseTimePoint and baseFrame (sinceMidnight)

  unique_lock<shared_mutex> lock (mSharedMutex);

  mHlsBaseChunkNum = chunkNum;

  timePoint += offset;
  mHlsBaseTimePoint = timePoint;

  // calc hlsBaseFrame
  auto midnightTimePoint = date::floor<date::days>(timePoint);
  uint64_t msSinceMidnight = duration_cast<milliseconds>(timePoint - midnightTimePoint).count();
  mHlsBaseFrame = int((msSinceMidnight * mSampleRate) / mSamplesPerFrame / 1000);

  // add inn startSecs offset to playFrame for curious case of tv reporting 2 hours late in .m3u8
  mPlayFrame = mHlsBaseFrame + ((startSecs * mSampleRate) / mSamplesPerFrame);

  mHlsBaseValid = true;
  }
//}}}
//{{{
bool cSong::loadChunk (system_clock::time_point now, chrono::seconds secs, int preload, int& chunkNum, int& frameNum) {
// return true if a chunk load needed
// - update chunkNum and frameNum

  // get offsets of playFrame from baseFrame, handle -v offsets correctly
  int frameOffset = mPlayFrame - mHlsBaseFrame;
  int chunkNumOffset = (frameOffset >= 0)  ? (frameOffset / mHlsFramesPerChunk) :
                                             -((mHlsFramesPerChunk - 1 - frameOffset) / mHlsFramesPerChunk);

  // loop until chunkNum with unloaded frame, chunkNum not available yet, or preload ahead of playFrame loaded
  int loaded = 0;
  while ((loaded < preload) && ((now - (mHlsBaseTimePoint + (chunkNumOffset * 6400ms))) > secs))
    // chunkNum chunk should be available
    if (!getAudioFramePtr (mHlsBaseFrame + (chunkNumOffset * mHlsFramesPerChunk))) {
      // not loaded, return chunkNum to load
      chunkNum = mHlsBaseChunkNum + chunkNumOffset;
      frameNum = mHlsBaseFrame + (chunkNum - mHlsBaseChunkNum) * mHlsFramesPerChunk;
      mLastChunkNum = chunkNum;
      mLastFrameNum = frameNum;;
      return true;
      }
    else {
      // already loaded, next
      loaded++;
      chunkNumOffset++;
      }

  // return false, no chunkNum available to load
  chunkNum = 0;
  frameNum = 0;
  mLastChunkNum = chunkNum;
  mLastFrameNum = frameNum;
  return false;
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
void cSong::clearFrames() {

  // new id for any cache
  mId++;

  // reset frames
  mPlayFrame = 0;
  mTotalFrames = 0;
  mSelect.clearAll();

  for (auto frame : mFrameMap)
    delete (frame.second);
  mFrameMap.clear();

  // reset maxValues
  mMaxPowerValue = kMinPowerValue;
  mMaxPeakValue = kMinPeakValue;
  mMaxFreqValue = kMinFreqValue;
  }
//}}}
//{{{
void cSong::checkSilenceWindow (int frameNum) {

  // walk backwards looking for continuous loaded quiet frames
  auto windowSize = 0;
  while (true) {
    auto framePtr = getAudioFramePtr (frameNum);
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
      auto framePtr = getAudioFramePtr (++frameNum);
      if (framePtr && framePtr->isQuiet())
        framePtr->setSilence (true);
      else
        break;
      }
    }
  }
//}}}

//{{{
int cSong::skipPrev (int fromFrame, bool silence) {

  for (int frame = fromFrame-1; frame >= getFirstFrame(); frame--) {
    auto framePtr = getAudioFramePtr (frame);
    if (framePtr && (framePtr->isSilence() ^ silence))
      return frame;
    }

  return fromFrame;
  }
//}}}
//{{{
int cSong::skipNext (int fromFrame, bool silence) {

  for (int frame = fromFrame; frame <= getLastFrame(); frame++) {
    auto framePtr = getAudioFramePtr (frame);
    if (framePtr && (framePtr->isSilence() ^ silence))
      return frame;
    }

  return fromFrame;
  }
//}}}
