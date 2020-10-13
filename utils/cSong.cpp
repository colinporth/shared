// cSong.cpp - set of audioFrames, videoFrames added with own timing
//{{{  includes
#include "cSong.h"

#include "../date/date.h"
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}

constexpr static float kMinPowerValue = 0.25f;
constexpr static float kMinPeakValue = 0.25f;
constexpr static float kMinFreqValue = 256.f;
constexpr static int kSilenceWindowFrames = 4;

//cSong::cSelect
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

//{{{
cSong::~cSong() {
  clearFrames();
  }
//}}}

//{{{
void cSong::init (cAudioDecode::eFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame) {

  unique_lock<shared_mutex> lock (mSharedMutex);

  // reset frame type
  mFrameType = frameType;
  mNumChannels = numChannels;
  mSampleRate = sampleRate;
  mSamplesPerFrame = samplesPerFrame;
  mHlsBaseValid = false;

  clearFrames();

  // ??? should deallocate ???
  fftrConfig = kiss_fftr_alloc (mSamplesPerFrame, 0, 0, 0);
  }
//}}}
//{{{
void cSong::addAudioFrame (int frameNum, float* samples, bool owned, int totalFrames, uint8_t* framePtr, uint64_t pts) {

  // sum of squares channel power
  auto powerValues = (float*)malloc (mNumChannels * 4);
  memset (powerValues, 0, mNumChannels * 4);

  // peak
  auto peakValues = (float*)malloc (mNumChannels * 4);
  memset (peakValues, 0, mNumChannels * 4);

  auto samplePtr = samples;
  for (int sample = 0; sample < mSamplesPerFrame; sample++) {
    timeBuf[sample] = 0;
    for (auto channel = 0; channel < mNumChannels; channel++) {
      auto value = *samplePtr++;
      timeBuf[sample] += value;
      powerValues[channel] += value * value;
      peakValues[channel] = max (abs(peakValues[channel]), value);
      }
    }

  for (auto channel = 0; channel < mNumChannels; channel++) {
    powerValues[channel] = sqrtf (powerValues[channel] / mSamplesPerFrame);
    mMaxPowerValue = max (mMaxPowerValue, powerValues[channel]);
    mMaxPeakValue = max (mMaxPeakValue, peakValues[channel]);
    }

  // ??? lock against init fftrConfig recalc???
  kiss_fftr (fftrConfig, timeBuf, freqBuf);

  auto freqValues = (uint8_t*)malloc (getNumFreqBytes());
  auto lumaValues = (uint8_t*)malloc (getNumFreqBytes());
  float freqScale = 255.f / mMaxFreqValue;

  auto freqBufPtr = freqBuf;
  auto freqValuesPtr = freqValues;
  auto lumaValuesPtr = lumaValues + getNumFreqBytes() - 1;
  for (auto i = 0; i < getNumFreqBytes(); i++) {
    float value = float(sqrt (((*freqBufPtr).r * (*freqBufPtr).r) + ((*freqBufPtr).i * (*freqBufPtr).i)));
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

  // totalFrames can be a changing estimate for file, or increasing value for streaming
  mFrameMap.insert (map<int,cFrame*>::value_type (frameNum,
    new cFrame (samples, owned, framePtr, powerValues, peakValues, freqValues, lumaValues, pts)));
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

// cSong gets
//{{{
int cSong::getLoadChunkNum (system_clock::time_point now, chrono::seconds secs, int preload, int& frameNum) {

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
      int chunkNum = mHlsBaseChunkNum + chunkNumOffset;
      frameNum = mHlsBaseFrame + (chunkNum - mHlsBaseChunkNum) * mHlsFramesPerChunk;
      return chunkNum;
      }
    else {
      // already loaded, next
      loaded++;
      chunkNumOffset++;
      }

  // return 0, no chunkNum available to load
  frameNum = 0;
  return 0;
  }
//}}}

// cSong playFrame
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

// cSong hls
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

// cSong actions
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

// cSong private
//{{{
void cSong::clearFrames() {

  // new id for any cache
  mId++;

  // reset frames
  mPlayFrame = 0;
  mTotalFrames = 0;
  mSelect.clearAll();

  for (auto frame : mFrameMap)
    if (frame.second)
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
