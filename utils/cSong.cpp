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
int64_t cSong::cSelect::constrainToRange (int64_t frameNum, int64_t constrainedFrameNum) {
// if FrameNum in a select range return FrameNum constrained to it

  for (auto &item : mItems) {
    if (item.inRange (frameNum)) {
      if (constrainedFrameNum > item.getLastFrameNum())
        return item.getFirstFrameNum();
      else if (constrainedFrameNum < item.getFirstFrameNum())
        return item.getFirstFrameNum();
      else
        return constrainedFrameNum;
      }
    }

  return constrainedFrameNum;
  }
//}}}

//{{{
void cSong::cSelect::clearAll() {

  mItems.clear();

  mEdit = eEditNone;
  mEditFrameNum = 0;
  }
//}}}
//{{{
void cSong::cSelect::addMark (int64_t frameNum, const std::string& title) {
  mItems.push_back (cSelectItem (cSelectItem::eLoop, frameNum, frameNum, title));
  mEdit = eEditLast;
  mEditFrameNum = frameNum;
  }
//}}}
//{{{
void cSong::cSelect::start (int64_t frameNum) {

  mEditFrameNum = frameNum;

  mItemNum = 0;
  for (auto &item : mItems) {
    // pick from select range
    if (abs(frameNum - item.getLastFrameNum()) < 2) {
      mEdit = eEditLast;
      return;
      }
    else if (abs(frameNum - item.getFirstFrameNum()) < 2) {
      mEdit = eEditFirst;
      return;
      }
    else if (item.inRange (frameNum)) {
      mEdit = eEditRange;
      return;
      }
    mItemNum++;
    }

  // add new select item
  mItems.push_back (cSelectItem (cSelectItem::eLoop, frameNum, frameNum, ""));
  mEdit = eEditLast;
  }
//}}}
//{{{
void cSong::cSelect::move (int64_t frameNum) {

  if (mItemNum < (int)mItems.size()) {
    switch (mEdit) {
      case eEditFirst:
        mItems[mItemNum].setFirstFrameNum (frameNum);
        if (mItems[mItemNum].getFirstFrameNum() > mItems[mItemNum].getLastFrameNum()) {
          mItems[mItemNum].setLastFrameNum (frameNum);
          mItems[mItemNum].setFirstFrameNum (mItems[mItemNum].getLastFrameNum());
          }
        break;

      case eEditLast:
        mItems[mItemNum].setLastFrameNum (frameNum);
        if (mItems[mItemNum].getLastFrameNum() < mItems[mItemNum].getFirstFrameNum()) {
          mItems[mItemNum].setFirstFrameNum (frameNum);
          mItems[mItemNum].setLastFrameNum (mItems[mItemNum].getFirstFrameNum());
          }
        break;

      case eEditRange:
        mItems[mItemNum].setFirstFrameNum (mItems[mItemNum].getFirstFrameNum() + frameNum - mEditFrameNum);
        mItems[mItemNum].setLastFrameNum (mItems[mItemNum].getLastFrameNum() + frameNum - mEditFrameNum);
        mEditFrameNum = frameNum;
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
  mEditFrameNum = 0;
  }
//}}}

// cSong
//{{{
cSong::cSong (eAudioFrameType frameType, int numChannels,
              int sampleRate, int samplesPerFrame, int maxMapSize)
    : mSampleRate(sampleRate), mSamplesPerFrame(samplesPerFrame),
      mFrameType(frameType), mNumChannels(numChannels),
      mMaxMapSize(maxMapSize) {

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
string cSong::getTimeString (int64_t pts, int daylightSeconds) {
// pts = frameNum

  // turn pts as frameNum into seconds * 100
  int64_t value = (pts * mSamplesPerFrame) / mSampleRate / 100;

  int subSeconds = value % 100;
  value /= 100;

  int seconds = value % 60;
  value /= 60;

  int minutes = value % 60;
  value /= 60;

  int hours = (int)value;

  // !!! must be a better formatter lib !!!
  return (hours > 0) ? (dec (hours) + ':' + dec (minutes, 2, '0') + ':' + dec(seconds, 2, '0')) :
           ((minutes > 0) ? (dec (minutes) + ':' + dec(seconds, 2, '0') + ':' + dec(subSeconds, 2, '0')) :
             (dec(seconds) + ':' + dec(subSeconds, 2, '0')));
  }
//}}}
//{{{
void cSong::addFrame (bool reuseFront, int64_t pts, float* samples, bool ownSamples, int64_t totalFrames) {

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

  { //  insert with locked mutex
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFrameMap.insert (map<int64_t,cFrame*>::value_type (pts/getFramePtsDuration(), frame));
  mTotalFrames = totalFrames;
  }

  checkSilenceWindow (pts);
  }
//}}}

// playFrame
//{{{
void cSong::setPlayPts (int64_t pts) {
  mPlayPts = min (pts, getLastPts() + getFramePtsDuration());
  }
//}}}
//{{{
void cSong::setPlayFirstFrame() {
  setPlayPts (mSelect.empty() ? getFirstPts() : getPtsFromFrameNum (mSelect.getFirstFrameNum()));
  }
//}}}
//{{{
void cSong::setPlayLastFrame() {
  setPlayPts (mSelect.empty() ? getLastPts() : getPtsFromFrameNum (mSelect.getLastFrameNum()));
  }
//}}}
//{{{
void cSong::nextPlayFrame (bool constrainToRange) {

  int64_t playPts = mPlayPts + getFramePtsDuration();
  //if (constrainToRange)
  //  mPlayPts = mSelect.constrainToRange (mPlayPts, mPlayPts);

  setPlayPts (playPts);
  }
//}}}
//{{{
void cSong::incPlaySec (int secs, bool useSelectRange) {

  int64_t frames = (secs * mSampleRate) / mSamplesPerFrame;
  mPlayPts += frames * getFramePtsDuration();
  }
//}}}
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

  int64_t fromFrame = getFrameNumFromPts (fromPts);

  for (int64_t frameNum = fromFrame-1; frameNum >= getFirstFrameNum(); frameNum--) {
    auto framePtr = findFrameByFrameNum (frameNum);
    if (framePtr && (framePtr->isSilence() ^ silence))
      return getPtsFromFrameNum (frameNum);
   }

  return fromPts;
  }
//}}}
//{{{
int64_t cSong::skipNext (int64_t fromPts, bool silence) {

  int64_t fromFrame = getFrameNumFromPts (fromPts);

  for (int64_t frameNum = fromFrame; frameNum <= getLastFrameNum(); frameNum++) {
    auto framePtr = findFrameByFrameNum (frameNum);
    if (framePtr && (framePtr->isSilence() ^ silence))
      return getPtsFromFrameNum (frameNum);
    }

  return fromPts;
  }
//}}}
//{{{
void cSong::checkSilenceWindow (int64_t pts) {

  unique_lock<shared_mutex> lock (mSharedMutex);

  // walk backwards looking for continuous loaded quiet frames
  int64_t frameNum = getFrameNumFromPts (pts);

  auto windowSize = 0;
  while (true) {
    auto frame = findFrameByFrameNum (frameNum);
    if (frame && frame->isQuiet()) {
      windowSize++;
      frameNum--;
      }
    else
      break;
    };

  if (windowSize > kSilenceWindowFrames) {
    // walk forward setting silence for continuous loaded quiet frames
    while (true) {
      auto frame = findFrameByFrameNum (++frameNum);
      if (frame && frame->isQuiet())
        frame->setSilence (true);
      else
        break;
      }
    }
  }
//}}}

// cPtsSong
//{{{
string cPtsSong::getTimeString (int64_t pts, int daylightSeconds) {
// 90khz int64_t pts - display as time, daylightSaving adjusted

  auto midnightBaseTimePoint = date::floor<date::days>(mBaseTimePoint);
  uint64_t msSinceMidnightBaseTimePoint = duration_cast<milliseconds>(mBaseTimePoint - midnightBaseTimePoint).count();

  int64_t ms = msSinceMidnightBaseTimePoint + ((pts - mBasePts) / 90);

  int64_t value = ms / 10;

  int subSeconds = value % 100;
  value /= 100;

  int seconds = value % 60;
  value /= 60;

  int minutes = value % 60;
  value /= 60;

  int hours = (int)value;

  // !!! use a better formatter !!!
  return (hours > 0) ? (dec (hours) + ':' + dec (minutes, 2, '0') + ':' + dec(seconds, 2, '0')) :
           ((minutes > 0) ? (dec (minutes) + ':' + dec(seconds, 2, '0') + ':' + dec(subSeconds, 2, '0')) :
             (dec(seconds) + ':' + dec(subSeconds, 2, '0')));
  }
//}}}
//{{{
void cPtsSong::setBasePts (int64_t pts) {

  mBasePts = pts;
  mPlayPts = pts;
  }
//}}}

// cHlsSong
//{{{
cHlsSong::cHlsSong (eAudioFrameType frameType, int numChannels,
                    int sampleRate, int samplesPerFrame,
                    int64_t framePtsDuration, int maxMapSize, int framesPerChunk)
  : cPtsSong(frameType, numChannels, sampleRate, samplesPerFrame, framePtsDuration, maxMapSize),
    mFramesPerChunk(framesPerChunk) {}
//}}}
cHlsSong::~cHlsSong() {}

//{{{
int cHlsSong::getLoadChunkNum (int64_t& loadPts, bool& reuseFromFront) {
// return chunkNum needed to play or preload playPts

  // get frameNumOffset of playPts from basePts
  int64_t frameNumOffset = getFrameNumFromPts (mPlayPts) - getFrameNumFromPts (mBasePts);

  // chunkNumOffset, handle -v offsets correctly
  int64_t chunkNumOffset = frameNumOffset / mFramesPerChunk;
  if (frameNumOffset < 0)
    chunkNumOffset -= mFramesPerChunk - 1;

  int chunkNum = mBaseChunkNum + (int)chunkNumOffset;
  loadPts = mBasePts + ((chunkNum - mBaseChunkNum) * mFramesPerChunk) * mFramePtsDuration;

  // test for chunkNum available now

  // check playPts chunkNum for firstFrame of chunk loaded
  if (!findFrameByPts (loadPts)) {
    cLog::log (LOGINFO, "getLoadChunk - load frameNumOffset:%d chunkNumOffset:%d chunkNum:%d",
                         frameNumOffset, chunkNumOffset, chunkNum);
    reuseFromFront = loadPts >= mPlayPts;
    return chunkNum;
    }

  // check preload chunkNum+1
  chunkNum++;
  loadPts += mFramesPerChunk * mFramePtsDuration;
  if (!findFrameByPts (loadPts))  {
    cLog::log (LOGINFO, "getLoadChunk - preload+1 chunkNum:%d", chunkNum);
    reuseFromFront = loadPts >= mPlayPts;
    return chunkNum;
    }

  // return no chunkNum to load
  loadPts = -1;
  reuseFromFront = false;
  return 0;
  }
//}}}
//{{{
void cHlsSong::setBaseHls (int64_t pts, system_clock::time_point timePoint, seconds offset, int chunkNum) {
// set baseChunkNum, baseTimePoint and baseFrame (sinceMidnight)

  cLog::log (LOGINFO, "setBase chunk:" + dec(chunkNum) + " pts:" + getPtsString (pts));

  unique_lock<shared_mutex> lock (mSharedMutex);
  setBasePts (pts);
  timePoint += offset;
  mBaseTimePoint = timePoint;
  mBaseChunkNum = chunkNum;
  }
//}}}
