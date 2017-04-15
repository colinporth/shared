// hls.h
#pragma once
//{{{  includes
#include "../rapidjson/document.h"
#include "../widgets/cDecodePicWidget.h"
#include "../teensyAac/cAacDecoder.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"

#include "r1x80.h"
#include "r2x80.h"
#include "r3x80.h"
#include "r4x80.h"
#include "r5x80.h"
#include "r6x80.h"
//}}}
//{{{  static const
static const int kMaxZoom = 4;
static const float kNormalZoom = 1;
static const int kScrubFrames = 3;
static const int kPeakDecimate = 4;
static const int kExtTimeOffset = 17;

static const uint16_t kFramesPerChunk = 300;
static const uint16_t kSamplesPerFrame = 1024;
static const uint16_t kSamplesPerSec = 48000;
static const float kSamplesPerSecF   = kSamplesPerSec;
static const double kSamplesPerSecD  = kSamplesPerSec;

static const uint32_t kDefaultChan = 4;
//static const uint32_t kDefaultBitrate = 48000;
//static const uint32_t kDefaultBitrate = 96000;
static const uint32_t kDefaultBitrate = 128000;
static const float kDefaultVolume = 0.8f;
//}}}
const bool kBST = true;

// utils
//{{{
static uint32_t getTimeInSecsFromDateTime (const char* dateTime) {

  uint16_t hour = ((dateTime[11] - '0') * 10) + (dateTime[12] - '0');
  uint16_t min =  ((dateTime[14] - '0') * 10) + (dateTime[15] - '0');
  uint16_t sec =  ((dateTime[17] - '0') * 10) + (dateTime[18] - '0');
  return (hour * 60 * 60) + (min * 60) + sec;
  }
//}}}
//{{{
static std::string getTimeStrFromSecs (uint32_t secsSinceMidnight) {

  uint32_t hours = (secsSinceMidnight / (60*60));
  uint32_t mins = (secsSinceMidnight / 60) % 60;
  uint32_t secs = secsSinceMidnight % 60;

  return dec (hours) + ':' + dec(mins, 2, '0') + ':' + dec(secs, 2, '0');
  }
//}}}

//{{{
class cScheduleItem {
public:
  void* operator new (size_t size) { return smallMalloc (size, "cScheduleItem"); }
  void operator delete (void *ptr) { smallFree (ptr); }

  uint32_t mStart;
  uint32_t mDuration;
  std::string mTitle;
  std::string mSynopsis;
  std::string mImagePid;
  };
//}}}
//{{{
class cSchedule {
public:
  void* operator new (size_t size) { return smallMalloc (size, "cSchedule"); }
  void operator delete (void *ptr) { smallFree (ptr); }

  //{{{
  void load (cHttp& http, uint16_t chan) {

    cLog::Log (LOGINFO1, "cSchedule::load");

    // get schedule json
    switch (chan) {
      case 1: http.get ("www.bbc.co.uk", "radio1/programmes/schedules/england/today.json"); break;
      case 2: http.get ("www.bbc.co.uk", "radio2/programmes/schedules/today.json"); break;
      case 3: http.get ("www.bbc.co.uk", "radio3/programmes/schedules/today.json"); break;
      case 4: http.get ("www.bbc.co.uk", "radio4/programmes/schedules/fm/today.json"); break;
      case 5: http.get ("www.bbc.co.uk", "5live/programmes/schedules/today.json"); break;
      case 6: http.get ("www.bbc.co.uk", "6music/programmes/schedules/today.json"); break;
      default:;
      }

    // parse it
    rapidjson::Document schedule;
    auto parseError = schedule.Parse ((const char*)http.getContent(), http.getContentSize()).HasParseError();
    http.freeContent();
    if (parseError) {
      cLog::Log (LOGERROR, "cSchedule::load error %d", schedule.GetParseError());
      return;
      }

    for (auto& element : schedule["schedule"]["day"]["broadcasts"].GetArray()) {
      auto item = new cScheduleItem();
      auto broadcast = element.GetObject();
      item->mStart    = getTimeInSecsFromDateTime (broadcast["start"].GetString());
      item->mDuration = broadcast["duration"].GetInt();
      if (broadcast["programme"]["display_titles"]["title"].IsString())
        item->mTitle    = broadcast["programme"]["display_titles"]["title"].GetString();
      item->mSynopsis = broadcast["programme"]["short_synopsis"].GetString();
      item->mImagePid = broadcast["programme"]["image"]["pid"].GetString();
      mSchedule.push_back (item);
      }

    cLog::Log (LOGINFO1, "cSchedule::load done size:%d %s %s",
                         http.getContentSize(), http.getLastHost().c_str(), http.getLastPath().c_str());
    list();
    }
  //}}}

  //{{{
  cScheduleItem* findItem (uint32_t secs) {
    if (kBST)
      secs += 3600;

    for (auto item : mSchedule)
      if ((secs >= item->mStart) && (secs < item->mStart + item->mDuration))
        return item;
    return nullptr;
    }
  //}}}
  //{{{
  void clear() {
    for (auto item : mSchedule) {
      delete (item);
      item = nullptr;
      }
    mSchedule.clear();
    }
  //}}}

private:
  //{{{
  void list() {

    for (auto item : mSchedule)
      cLog::Log (LOGINFO2, "start:%s dur:%s %s",
                 getTimeStrFromSecs (item->mStart).c_str(),
                 getTimeStrFromSecs (item->mDuration).c_str(),
                 item->mTitle.c_str());
    }
  //}}}

  std::vector<cScheduleItem*> mSchedule;
  };
//}}}
//{{{
class cHls {
public:
  //{{{
  cHls (int chan) : mHlsChan (chan) {
    mDecoder = new cAacDecoder();
    }
  //}}}
  //{{{
  ~cHls() {
    free (mDecoder);
    }
  //}}}
  void* operator new (size_t size) { return bigMalloc (size, "cHls"); }
  void operator delete (void *ptr) { bigFree (ptr); }

  //{{{
  cScheduleItem* findItem (uint32_t sec) {
    return mSchedule.findItem (sec);
    }
  //}}}
  //{{{
  void getChunkLoad (uint16_t chunk, bool& loaded, bool& loading, int& offset) {
    loaded = mChunks[chunk].getLoaded();
    loading = mChunks[chunk].getLoading();
    offset = getSeqNumOffset (mChunks[chunk].getSeqNum());
    }
  //}}}

  //{{{
  uint8_t* getPeakSamples (double sample, uint32_t& numSamples, float zoom, uint16_t scale) {

    if (zoom > kMaxZoom)
      zoom = kMaxZoom;

    numSamples = 0;

    uint32_t seqNum;
    uint16_t chunk;
    uint16_t chunkFrame;
    uint32_t subFrameSamples;

    return findFrame (sample, seqNum, chunk, chunkFrame, subFrameSamples) ?
      mChunks[chunk].getPeakSamples (chunkFrame, subFrameSamples, numSamples, zoom, scale) : nullptr;
    }
  //}}}
  //{{{
  int16_t* getPlaySamples (double sample, uint32_t& seqNum, uint32_t& numSamples) {

    numSamples = 0;

    uint16_t chunk;
    uint16_t chunkFrame;
    uint32_t subFrameSamples;

    return findFrame (sample, seqNum, chunk, chunkFrame, subFrameSamples) ?
      mChunks[chunk].getPlaySamples (chunkFrame, subFrameSamples, numSamples) : nullptr;
    }
  //}}}
  //{{{
  float getPlaySpeedSamples (double sample, uint32_t& seqNum, uint32_t& numSamples, int16_t* dstSamples, float speed) {
  // resample playSample at speed into dstSamples returing samples consumed

    float src = 0.0f;
    auto srcSample = getPlaySamples (sample, seqNum, numSamples);

    if (srcSample && (speed > 0) && (speed <= 2.0f)) {
      for (int i = 0; i < 1024; i++) {
        float subSrc = src - trunc(src);
        float invSubSrc = 1.0f - subSrc;
        *dstSamples++ = int16_t(((*(srcSample + int(src)*2) * invSubSrc) + (*(srcSample + int(src+1)*2) * subSrc)) / 2);
        *dstSamples++ = int16_t(((*(srcSample + int(src)*2 + 1) * invSubSrc) + (*(srcSample + int(src+1)*2+1) * subSrc)) / 2);
        src += speed;
        }
      }
    else if (srcSample && (speed < 0) && (speed > -2.0f)) {
      // reverse
      srcSample += 2048;
      for (int i = 0; i < 1024; i++) {
        *dstSamples++ = *(srcSample - int(src)*2 - 1);
        *dstSamples++ = *(srcSample - int(src)*2 - 2);
        src -= speed;
        }
      src = -src;
      }
    else
      memset (dstSamples, 0, 4096);

    return src;
    }
  //}}}

  //{{{
  double getPlaySample() {
    return mPlaySample;
    }
  //}}}
  //{{{
  uint32_t getPlayFrame() {
    return uint32_t (mPlaySample / kSamplesPerFrame);
    }
  //}}}
  //{{{
  uint32_t getPlaySec() {
    return uint32_t (mPlaySample / kSamplesPerSec);
    }
  //}}}
  //{{{  incPlay
  //{{{
  void incPlaySample (double samples) {
    mPlaySample += samples;
    }
  //}}}
  //{{{
  void incPlayFrame (int incFrames) {
    mPlaySample += incFrames * kSamplesPerFrame;
    }
  //}}}
  //{{{
  void incPlaySec (int secs) {
    mPlaySample += secs * kSamplesPerSec;
    }
  //}}}
  //}}}
  //{{{  playing
  //{{{
  bool getPlaying() {
    return mPlaying;
    }
  //}}}
  //{{{
  void setPlaying (bool playing) {
    mPlaying = playing;
    }
  //}}}
  //{{{
  void togglePlaying() {
    mPlaying = !mPlaying;
    }
  //}}}
  //{{{
  bool getScrubbing() {
    return mScrubbing;
    }
  //}}}
  //{{{
  void setScrubbing (bool scrubbing) {
    mScrubbing = scrubbing;
    }
  //}}}
  //}}}

  //{{{
  void setChan (cHttp& http, int chan, int bitrate) {

    clearChunks();
    mSchedule.clear();
    loadChan (http, chan, bitrate);
    mSchedule.load (http, chan);

    mPlaying = true;
    mScrubbing = false;

    mChanChanged = false;
    }
  //}}}
  //{{{
  void clearChunks() {
    for (auto i = 0; i < 3; i++)
      mChunks[i].clear();
    }
  //}}}

  //{{{
  bool loadAtPlayFrame (cHttp& http) {
  // load around playFrame, return false if any load failed, no point retrying too soon

    bool ok = true;

    uint32_t seqNum = getSeqNumFromFrame (getPlayFrame());

    uint16_t chunk;
    if (!findSeqNumChunk (seqNum, 0, chunk)) {
      cLog::Log (LOGINFO, "loadAtPlayFrame loading seqNum:%d at playFrame:%d", seqNum, getPlayFrame());
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum), seqNum, mBitrate);
      cLog::Log (LOGINFO, "loaded seqNum:%d", seqNum);
      }
    if (mChanChanged)
      return true;

    if (!findSeqNumChunk (seqNum, 1, chunk)) {
      cLog::Log (LOGINFO, "loadAtPlayFrame loading seqNum:%d at playFrame:%d", seqNum+1, getPlayFrame());
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum+1), seqNum+1, mBitrate);
      cLog::Log (LOGINFO, "loaded seqNum:%d", seqNum+1);
      }
    if (mChanChanged)
      return true;

    if (!findSeqNumChunk (seqNum, -1, chunk)) {
      cLog::Log (LOGINFO, "loadAtPlayFrame loading seqNum:%d at playFrame:%d", seqNum-1, getPlayFrame());
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum-1), seqNum-1, mBitrate);
      cLog::Log (LOGINFO, "loaded seqNum:%d", seqNum-1);
      }
    return ok;
    }
  //}}}
  //{{{
  void loadPicAtPlayFrame (cHttp& http) {

    auto item = findItem (getPlaySec());
    auto imagePid = item ? item->mImagePid : "";
    if (imagePid != mImagePid) {
      if (imagePid.empty())
        mContent = nullptr;
      else {
        http.get ("ichef.bbci.co.uk", "images/ic/160x90/" + imagePid + ".jpg");
        cLog::Log (LOGINFO1, "loadPicAtPlayFrame imagePid:%s size:%d", imagePid.c_str(), http.getContentSize());
        if (http.getContent()) {
          new cJpegPic (3, http.getContent());
          auto temp = (uint8_t*)bigMalloc (http.getContentSize(), "cHls::jpegPic");
          memcpy (temp, http.getContent(), http.getContentSize());
          mContentSize = http.getContentSize();
          mContent = temp;
          http.freeContent();
          cLog::Log (LOGINFO1, "- loaded imagePid:%s size:%d", imagePid.c_str(), http.getContentSize());
          }
        else
          mContent = nullptr;
        }
      mImagePid = imagePid;
      }
    }
  //}}}

  // vars
  bool mChanChanged = true;
  int mHlsChan = kDefaultChan;
  int mHlsBitrate = kDefaultBitrate;

  bool mPicChanged = false;
  int mPicValue = 0;

  bool mVolumeChanged = true;
  float mVolume = kDefaultVolume;

  std::string mImagePid;
  uint8_t* mContent = nullptr;
  int mContentSize = 0;

private:
  //{{{
  class cChunk {
  public:
    //{{{
    cChunk() {
      mSamples = (int16_t*)bigMalloc ((kFramesPerChunk+1) * 1024 * 2 * 2, "chunkSamples");     // 300 frames * 1024 samples * 2 chans * 2 bytes
      memset (mSamples, 0, (kFramesPerChunk+1) * 1024 * 2 * 2);
      mPeakSamples = (uint8_t*)bigMalloc (kMaxZoom * kFramesPerChunk * 2, "chunkPeak"); // 300 frames of L,R uint8_t peak
      memset (mPeakSamples, 0, kMaxZoom * kFramesPerChunk * 2);
      }
    //}}}
    //{{{
    ~cChunk() {
      bigFree (mSamples);
      bigFree (mPeakSamples);
      }
    //}}}

    // gets
    bool getLoaded() { return mLoaded; }
    bool getLoading() { return mLoading; }
    uint32_t getSeqNum() { return mSeqNum; }
    uint16_t getFramesLoaded() { return mSrcFramesLoaded; }

    //{{{
    uint8_t* getPeakSamples (uint16_t chunkFrame, uint32_t subFrameSamples, uint32_t& numSamples, float zoom, uint16_t scale) {

      if (zoom != mZoom)
        mSrcPeakFrames = 0;

      if (mSrcPeakFrames < kFramesPerChunk) {
        if (!mSrcPeakFrames) {
          mSample = mSamples + subFrameSamples * 2;
          mNumPeakSamples = 0;
          mPeakSample = mPeakSamples;
          }
        //else
        //  debug (dec(chunkFrame) + " " + dec(mSrcPeakFrames));

        auto maxSample = mSamples + mSrcFramesLoaded * kSamplesPerFrame * 2;
        while (mSample < maxSample) {
          auto endSample = mSamples + (subFrameSamples + int(((mNumPeakSamples+1) * kSamplesPerFrame) / zoom)) * 2;
          if (endSample > maxSample)
            endSample = maxSample;

          // find highest peak, +ve only get abs but not worth it
          int16_t valueL = 0;
          int16_t valueR = 0;
          while (mSample < endSample) {
            if (*mSample > valueL)
              valueL = *mSample;
            mSample++;
            if (*mSample > valueR)
              valueR = *mSample;
            mSample += kPeakDecimate*2 + 1;
            }

          *mPeakSample++ = uint8_t(valueL / scale);
          *mPeakSample++ = uint8_t(valueR / scale);
          mNumPeakSamples++;
          }

        mSrcPeakFrames = mSrcFramesLoaded;
        mZoom = zoom;
        }

      numSamples = uint32_t(kFramesPerChunk * zoom) - uint32_t((chunkFrame + ((float)subFrameSamples / kSamplesPerFrame)) * zoom);
      return mPeakSamples + int((chunkFrame + ((float)subFrameSamples / kSamplesPerFrame)) * zoom) * 2;
      }
    //}}}
    //{{{
    int16_t* getPlaySamples (uint16_t chunkFrame, uint32_t subFrameSamples, uint32_t& numSamples) {

      numSamples = (mSrcFramesLoaded - chunkFrame > 0) ? ((mSrcFramesLoaded - chunkFrame) * kSamplesPerFrame) - subFrameSamples : 0;
      return mSamples ? mSamples + ((chunkFrame * kSamplesPerFrame) + subFrameSamples) * 2 : nullptr;
      }
    //}}}

    //{{{
    void clear() {
      mSeqNum = 0;
      mSrcFramesLoaded = 0;
      mLoaded = false;
      }
    //}}}

    //{{{
    bool load (cHttp& http, cAacDecoder* decoder, std::string host, std::string path, uint32_t seqNum, uint32_t bitrate) {

      auto ok = true;
      mLoading = true;
      mSrcFramesLoaded = 0;
      mSrcPeakFrames = 0;
      mSeqNum = seqNum;

      auto response = http.get (host, path);
      if (response == 200) {
        // aacHE has double size frames, treat as two normal frames
        int framesPerAacFrame = bitrate <= 96000 ? 2 : 1;

        auto srcPtr = http.getContent();
        auto srcEnd = packTsBuffer (srcPtr, http.getContent() + http.getContentSize());

        auto samples = mSamples;

        int decode_res = 0;
        int bytesLeft = (int)(srcEnd-srcPtr);
        while (decode_res >= 0 && srcPtr < srcEnd) {
          decode_res = decoder->AACDecode (&srcPtr, &bytesLeft, samples);
          //AACFrameInfo aacFrameInfo;
          //AACGetLastFrameInfo (decoder, &aacFrameInfo);
          //printf ("aacdecode %d %d %d %d %x\n",
          //        decode_res, bytesLeft, aacFrameInfo.outputSamps, mSrcFramesLoaded, srcPtr);
          if (decode_res >= 0) {
            samples += kSamplesPerFrame * framesPerAacFrame * 2;
            mSrcFramesLoaded += framesPerAacFrame;
            }
          }

        http.freeContent();
        }
      else {
        mSeqNum = 0;
        ok = false;
        }

      mLoading = false;
      mLoaded = true;
      return ok;
      }
    //}}}

  private:
    //{{{
    uint8_t* packTsBuffer (uint8_t* ptr, uint8_t* endPtr) {
    // pack transportStream to aac frames in same buffer

      auto toPtr = ptr;
      while ((ptr < endPtr) && (*ptr++ == 0x47)) {
        auto payStart = *ptr & 0x40;
        auto pid = ((*ptr & 0x1F) << 8) | *(ptr+1);
        auto headerBytes = (*(ptr+2) & 0x20) ? (4 + *(ptr+3)) : 3;
        ptr += headerBytes;
        auto tsFrameBytesLeft = 187 - headerBytes;
        if (pid == 34) {
          if (payStart && !(*ptr) && !(*(ptr+1)) && (*(ptr+2) == 1) && (*(ptr+3) == 0xC0)) {
            int pesHeaderBytes = 9 + *(ptr+8);
            ptr += pesHeaderBytes;
            tsFrameBytesLeft -= pesHeaderBytes;
            }
          memcpy (toPtr, ptr, tsFrameBytesLeft);
          toPtr += tsFrameBytesLeft;
          }
        ptr += tsFrameBytesLeft;
        }

      return toPtr;
      }
    //}}}

    // vars
    uint32_t mSeqNum = 0;
    uint16_t mSrcFramesLoaded = 0;
    bool mLoaded = false;
    bool mLoading = false;

    int16_t* mSamples = nullptr;
    int16_t* mSample = nullptr;

    float mZoom = 0;
    uint8_t* mPeakSamples = nullptr;
    uint8_t* mPeakSample = nullptr;
    uint32_t mNumPeakSamples = 0;
    uint16_t mSrcPeakFrames = 0;
    };
  //}}}

  //{{{
  std::string getPathRoot() {

    const int kPool [] = { 0, 7, 7, 7, 6, 6, 6 };

    const char* kPathNames[] = {
      "none", "bbc_radio_one", "bbc_radio_two", "bbc_radio_three", "bbc_radio_fourfm", "bbc_radio_five_live", "bbc_6music" };

    // "pool_x/live/chanPathName/chanPathName.isml/chanPathName-audio=bitrate"
    return "pool_" + dec(kPool[mChan]) + "/live/" + kPathNames[mChan] + '/' + kPathNames[mChan] +
           ".isml/" + kPathNames[mChan] + "-audio=" + dec(mBitrate);
    }
  //}}}
  //{{{
  std::string getM3u8path() {
    return getPathRoot() + ".norewind.m3u8";
    }
  //}}}
  //{{{
  std::string getTsPath (uint32_t seqNum) {
    return getPathRoot() + '-' + dec(seqNum) + ".ts";
    }
  //}}}

  //{{{
  int getSeqNumOffset (uint32_t seqNum) {
    uint32_t frame = uint32_t (getPlaySample()/ kSamplesPerFrame);
    return seqNum - getSeqNumFromFrame (frame);
    }
  //}}}
  //{{{
  uint32_t getSeqNumFromFrame (uint32_t frame) {
  // works for -ve frame

    int r = frame - mBaseFrame;
    if (r >= 0)
      return mBaseSeqNum + (r / kFramesPerChunk);
    else
      return mBaseSeqNum - 1 - ((-r-1)/ kFramesPerChunk);
    }
  //}}}

  //{{{
  void loadChan (cHttp& http, int chan, int bitrate) {

    mChan = chan;
    mBitrate = bitrate;

    mHost = http.getRedirectable ("as-hls-uk-live.bbcfmt.vo.llnwd.net", getM3u8path());

    // find #EXT-X-MEDIA-SEQUENCE in .m3u8, point to seqNum string, extract seqNum from playListBuf
    auto extSeq = strstr ((char*)http.getContent(), "#EXT-X-MEDIA-SEQUENCE:") + strlen ("#EXT-X-MEDIA-SEQUENCE:");
    auto extSeqEnd = strchr (extSeq, '\n');
    *extSeqEnd = '\0';
    mBaseSeqNum = atoi (extSeq) + 3;

    // point to #EXT-X-PROGRAM-DATE-TIME dateTime str
    auto extDateTime = strstr (extSeqEnd + 1, "#EXT-X-PROGRAM-DATE-TIME:") + strlen ("#EXT-X-PROGRAM-DATE-TIME:");
    mBaseFrame = ((getTimeInSecsFromDateTime (extDateTime) - kExtTimeOffset) * kSamplesPerSec) / kSamplesPerFrame;

    http.freeContent();

    mPlaySample = double(mBaseFrame * kSamplesPerFrame);
    }
  //}}}

  //{{{
  bool findSeqNumChunk (uint32_t seqNum, int16_t offset, uint16_t& chunk) {
  // return true if match found
  // - if not, chunk = best reuse
  // - reuse same seqNum chunk if diff bitrate ?

    // look for matching chunk
    chunk = 0;
    while (chunk < 3) {
      if (seqNum + offset == mChunks[chunk].getSeqNum())
        return true;
      chunk++;
      }

    // look for stale chunk
    chunk = 0;
    while (chunk < 3) {
      if ((mChunks[chunk].getSeqNum() < seqNum-1) || (mChunks[chunk].getSeqNum() > seqNum+1))
        return false;
      chunk++;
      }

    chunk = 0;
    return false;
    }
  //}}}
  //{{{
  bool findFrame (double sample, uint32_t& seqNum, uint16_t& chunk, uint16_t& chunkFrame, uint32_t& subFrameSamples) {
  // return true, seqNum, chunk and chunkFrame of loadedChunk from frame
  // - return false if not found

    uint32_t frame = uint32_t(sample / kSamplesPerFrame);
    subFrameSamples = uint32_t(fmod (sample, kSamplesPerFrame));

    seqNum = getSeqNumFromFrame (frame);
    for (chunk = 0; chunk < 3; chunk++) {
      if (mChunks[chunk].getSeqNum() && (seqNum == mChunks[chunk].getSeqNum())) {
        int r = (frame - mBaseFrame) % kFramesPerChunk;
        chunkFrame = r < 0 ? r + kFramesPerChunk : r;
        return true;
        }
      }

    seqNum = 0;
    chunk = 0;
    chunkFrame = 0;
    return false;
    }
  //}}}

  //{{{  private vars
  std::string mHost;
  cChunk mChunks[3];

  uint16_t mChan = 0;
  uint32_t mBitrate = kDefaultBitrate;

  cAacDecoder* mDecoder = 0;

  std::string mDateTime;
  uint32_t mBaseFrame = 0;
  uint32_t mBaseSeqNum = 0;

  double mPlaySample = 0;
  bool mPlaying = true;
  bool mScrubbing = false;

  cSchedule mSchedule;
  //}}}
  };
//}}}

//{{{
class cHlsPicWidget : public cDecodePicWidget {
public:
  cHlsPicWidget(cHls* hls, float width, float height)
    : cDecodePicWidget(width, height, -1, mMyValue, mMyChanged), mHls(hls) {}
  virtual ~cHlsPicWidget() {}

  virtual void pressed (int16_t x, int16_t y, bool controlled) {
    cDecodePicWidget::pressed (x, y, controlled);
    mHls->togglePlaying();
    }

  virtual void render (iDraw* draw) {
    setOn (!mHls->getPlaying());

    if (mHls->mContent) {
      setPic (mHls->mContent, mHls->mContentSize);
      bigFree (mHls->mContent);
      mHls->mContent = nullptr;
      }

    cDecodePicWidget::render (draw);
    }

private:
  cHls* mHls;
  bool mMyChanged;
  int mMyValue;
  };
//}}}
//{{{
class cHlsDotsBox : public cWidget {
public:
  cHlsDotsBox (cHls* hls, float width, float height) : cWidget(width, height), mHls(hls) {}
  virtual ~cHlsDotsBox() {}

  virtual void pressed (int16_t x, int16_t y, bool controlled) {
    cWidget::pressed (x, y, controlled);
    mHls->clearChunks();
    }

  virtual void render (iDraw* draw) {
    for (auto i = 0; i < 3; i++)
      drawDot (draw, i);
    }

private:
  void drawDot (iDraw* draw, uint16_t chunk) {
    bool loaded;
    bool loading;
    int offset;
    mHls->getChunkLoad (chunk, loaded, loading, offset);

    draw->ellipseSolid (loading ? COL_DARKORANGE : loaded ? COL_DARKERGREEN : COL_DARKRED,
                   mX + 2 + ((mWidth-2) / 2), mY + (chunk * getBoxHeight()) + mHeight / 6, (mWidth-2) / 2, (mWidth-2) / 2);

    if (loaded || loading)
      draw->drawText(COL_LIGHTGREY, getSmallFontHeight(), dec(offset),
                  mX + mWidth / 3, mY + (chunk * getBoxHeight()) + mWidth/6, mWidth, mWidth);
    }

  cHls* mHls;
  };
//}}}
//{{{
class cHlsTextWidget : public cWidget {
public:
  cHlsTextWidget (cHls* hls, float width, float height) : cWidget(width, height), mHls(hls) {}
  virtual ~cHlsTextWidget() {}

  virtual void render (iDraw* draw) {
    auto item = mHls->findItem (mHls->getPlaySec());
    if (item)
      draw->drawText (COL_WHITE, getFontHeight(), item->mTitle + " - " + item->mSynopsis, mX, mY+1, mWidth-1, mHeight-1);
    }

private:
  cHls* mHls;
  };
//}}}
//{{{
class cHlsPeakWidget : public cWidget {
public:
  cHlsPeakWidget (cHls* hls, float width, float height) : cWidget (COL_BLUE, width, height), mHls(hls) {}
  virtual ~cHlsPeakWidget() {}

  //{{{
  virtual void pressed (int16_t x, int16_t y, bool controlled) {

    cWidget::pressed (x, y, controlled);

    mMove = 0;
    mPressInc = x - (mWidth/2);

    mHls->setScrubbing (true);

    setZoomAnim (1.0f + ((mWidth/2) - abs(x - (mWidth/2))) / (mWidth/6), 4);
    }
  //}}}
  //{{{
  virtual void moved (int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc, bool controlled) {

    cWidget::moved (x, y, z, xinc, yinc, controlled);
    mMove += abs(xinc) + abs(yinc);

    mHls->incPlaySample ((-xinc * kSamplesPerFrame) / mZoom);
    }
  //}}}
  //{{{
  virtual void released() {

    cWidget::released();

    if (mMove < getBoxHeight()/2) {
      mAnimation = mPressInc;
      mHls->incPlaySample (mPressInc * kSamplesPerFrame / kNormalZoom);
      mHls->setPlaying (true);
      }
    mHls->setScrubbing (false);

    setZoomAnim (kNormalZoom, 3);
    }
  //}}}

  //{{{
  virtual void render (iDraw* draw) {

    //{{{  animate
    mAnimation /= 2;

    if (mZoomInc > 0) {
      if (mZoom + mZoomInc > mTargetZoom)
        mZoom = mTargetZoom;
      else
        mZoom += mZoomInc;
      }
    else  {
      if (mZoom + mZoomInc < mTargetZoom)
        mZoom = mTargetZoom;
      else
        mZoom += mZoomInc;
      }
    //}}}

    uint16_t midx = mX + (mWidth/2);
    float samplesPerPixF = kSamplesPerFrame / mZoom;
    float pixPerSec = kSamplesPerSecF / samplesPerPixF;
    double sample = mHls->getPlaySample() - ((mWidth/2) + mAnimation) * samplesPerPixF;

    int16_t y = mY + mHeight - getBoxHeight()/2;
    int secs = int (sample / kSamplesPerSec);
    float subSecSamples = (float)fmod (sample, kSamplesPerSecD);
    float nextxF = (kSamplesPerSecF - subSecSamples) / samplesPerPixF;

    #ifdef USE_NANOVG
      //{{{  nanoVg rects waveform
      auto context = draw->getContext();

      context->fillColor (kDarkGrey);
      context->beginPath();
      for (float x = 0; x < mWidth; secs++, nextxF += pixPerSec) {
        if (secs & 1)
          context->rect (mX+x, y, nextxF - x, getBoxHeight()/2.0f);
        x = nextxF;
        }
      context->triangleFill();

      y = mY + mHeight - getBigFontHeight();

      context->fontSize ((float)getBigFontHeight());
      context->textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
      context->fillColor (kWhite);
      context->text (midx-60.0f+3.0f, y+1.0f, getTimeStrFromSecs (mHls->getPlaySec() + (kBST ? 3600 : 0)));

      float midy = (float)mY + (mHeight/2);
      uint16_t scale = 0x10000 / mHeight;
      uint16_t midWidth = midx + int(mHls->getScrubbing() ? kScrubFrames*mZoom : mZoom);

      context->beginPath();
      uint8_t* samples = nullptr;
      uint32_t numSamples = 0;
      for (float x = mX; x < mX+mWidth; x++) {
        if (!numSamples) {
          samples = mHls->getPeakSamples (sample, numSamples, mZoom, scale);
          if (samples)
            sample += numSamples * samplesPerPixF;
          }

        if (samples) {
          if (x == midx) {
            context->fillColor (nvgRGB32 (COL_BLUE));
            context->triangleFill();
            context->beginPath();
            }
          else if (x == midWidth) {
            context->fillColor (nvgRGB32 (COL_DARKGREEN));
            context->triangleFill();
            context->beginPath();
            }

          auto left = *samples++;
          auto right = *samples++;
          context->rect (x, midy - left, 1, (float)(left+right));
          numSamples--;
          }
        else
          sample += samplesPerPixF;
        }

      context->fillColor (nvgRGB32 (COL_DARKGREY));
      context->triangleFill();
      //}}}
    #else
      //{{{  iDraw waveform
      // iterate seconds
      for (int16_t x = 0; x < mWidth; secs++, nextxF += pixPerSec) {
        int16_t nextx = int16_t(nextxF);
        if (secs & 1)
          draw->drawRect (secs & 1 ? COL_DARKGREY : COL_DARKERGREY, mX+x, y, nextx - x, getBoxHeight()/2);
        x = nextx;
        }

      y = mY + mHeight - getBigFontHeight();
      draw->drawText (COL_LIGHTGREY, getBigFontHeight(), getTimeStrFromSecs (mHls->getPlaySec() + (kBST ? 3600 : 0)),
                  midx - 60, y, mWidth/2, getBigFontHeight());

      uint16_t scale = 0x10000 / mHeight;
      uint16_t midWidth = midx + int(mHls->getScrubbing() ? kScrubFrames*mZoom : mZoom);

      uint16_t midy = mY + (mHeight/2);

      uint32_t colour = COL_DARKBLUE;
      uint8_t* samples = nullptr;
      uint32_t numSamples = 0;
      for (auto x = mX; x < mX+mWidth; x++) {
        if (!numSamples) {
          samples = mHls->getPeakSamples (sample, numSamples, mZoom, scale);
          if (samples)
            sample += numSamples * samplesPerPixF;
          }
        if (samples) {
          if (x == midx)
            colour = COL_DARKGREEN;
          else if (x == midWidth)
            colour = COL_DARKERGREY;
          auto left = *samples++;
          draw->drawRect (colour, x, midy - left, 1, left + *samples++);
          numSamples--;
          }
        else
          sample += samplesPerPixF;
        }
      //}}}
    #endif
    }
  //}}}

private:
  //{{{
  void setZoomAnim (float zoom, int16_t frames) {

    mTargetZoom = zoom;
    mZoomInc = (mTargetZoom - mZoom) / frames;
    }
  //}}}

  cHls* mHls;

  float mMove = 0;
  bool mMoved = false;
  int16_t mPressInc = 0;
  int16_t mAnimation = 0;

  float mZoom = kNormalZoom;
  float mZoomInc = 0;
  float mTargetZoom = kNormalZoom;
  };
//}}}

//{{{
void hlsMenu (cRootContainer* root, cHls* hls) {

  root->addTopLeft (new cDecodePicWidget (r1x80, sizeof(r1x80), 3, 3, 1, hls->mHlsChan, hls->mChanChanged));
  root->add (new cDecodePicWidget (r2x80, sizeof(r2x80), 3, 3, 2, hls->mHlsChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r3x80, sizeof(r3x80), 3, 3, 3, hls->mHlsChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r4x80, sizeof(r4x80), 3, 3, 4, hls->mHlsChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r5x80, sizeof(r5x80), 3, 3, 5, hls->mHlsChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r6x80, sizeof(r6x80), 3, 3, 6, hls->mHlsChan,  hls->mChanChanged));

  root->add (new cHlsPicWidget (hls, 16/3.0f, 3));
  root->add (new cHlsDotsBox (hls, 1, 3));
  root->addAt (new cHlsPeakWidget (hls, 0, -4), 0, 4);
  root->addAt (new cHlsTextWidget (hls, 0, 3), 0, 3);

  root->addTopRight (new cValueBox (hls->mVolume, hls->mVolumeChanged, COL_YELLOW, 0.5, 0))->setOverPick (1.5);
  }
//}}}
