// hls.h
#pragma once
//{{{  includes
#include "../utils/utils.h"
#include "../utils/cLog.h"

#include "../teensyAac/cAacDecoder.h"
//}}}
//{{{  static const
const int kMaxZoom = 4;
const float kNormalZoom = 1;
const int kExtTimeOffset = 17;
const int kPeakDecimate = 4;

const uint16_t kFramesPerChunk = 300;
const uint16_t kSamplesPerFrame = 1024;
const uint16_t kSamplesPerSec = 48000;
const float kSamplesPerSecF   = kSamplesPerSec;
const double kSamplesPerSecD  = kSamplesPerSec;

const uint32_t kDefaultChan = 4;
//const uint32_t kDefaultBitrate = 48000;
//const uint32_t kDefaultBitrate = 96000;
const uint32_t kDefaultBitrate = 128000;
const float kDefaultVolume = 0.8f;

const int kBstSecs = 3600;
//}}}

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
  void getChunkLoad (uint16_t chunk, bool& loaded, bool& loading, int& offset) {
    loaded = mChunks[chunk].getLoaded();
    loading = mChunks[chunk].getLoading();
    offset = getSeqNumOffset (mChunks[chunk].getSeqNum());
    }
  //}}}
  //{{{
  uint8_t* getPeakSamples (double sample, uint32_t& numSamples, float zoom) {

    if (zoom > kMaxZoom)
      zoom = kMaxZoom;

    numSamples = 0;

    uint32_t seqNum;
    uint16_t chunk;
    uint16_t chunkFrame;
    uint32_t subFrameSamples;

    return findFrame (sample, seqNum, chunk, chunkFrame, subFrameSamples) ?
      mChunks[chunk].getPeakSamples (chunkFrame, subFrameSamples, numSamples, zoom) : nullptr;
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
    //mSchedule.clear();
    loadChan (http, chan, bitrate);
    //mSchedule.load (http, chan);

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
      cLog::log (LOGINFO, "loadAtPlayFrame seqNum " +
                          std::to_string (seqNum) + " at " + getTimeStrFromSecs (getPlaySec()+kBstSecs));
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum), seqNum, mBitrate);
      cLog::log (LOGINFO, "loaded seqNum:%d", seqNum);
      }
    if (mChanChanged)
      return true;

    if (!findSeqNumChunk (seqNum, 1, chunk)) {
      cLog::log (LOGINFO, "loadAtPlayFrame seqNum " +
                           std::to_string (seqNum+1) + " at " + getTimeStrFromSecs (getPlaySec()+kBstSecs));
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum+1), seqNum+1, mBitrate);
      cLog::log (LOGINFO, "loaded seqNum:%d", seqNum+1);
      }
    if (mChanChanged)
      return true;

    if (!findSeqNumChunk (seqNum, -1, chunk)) {
      cLog::log (LOGINFO, "loadAtPlayFrame seqNum " +
                           std::to_string (seqNum-1) + " at " + getTimeStrFromSecs (getPlaySec()+kBstSecs));
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum-1), seqNum-1, mBitrate);
      cLog::log (LOGINFO, "loaded seqNum:%d", seqNum-1);
      }
    return ok;
    }
  //}}}
  //{{{
  //cScheduleItem* findItem (uint32_t sec) {
    //return mSchedule.findItem (sec);
    //}
  //}}}
  //{{{
  //void loadPicAtPlayFrame (cHttp& http) {

    //auto item = findItem (getPlaySec());
    //auto imagePid = item ? item->mImagePid : "";
    //if (imagePid != mImagePid) {
      //if (imagePid.empty())
        //mContent = nullptr;
      //else {
        //http.get ("ichef.bbci.co.uk", "images/ic/160x90/" + imagePid + ".jpg");
        //cLog::log (LOGINFO1, "loadPicAtPlayFrame imagePid:%s size:%d", imagePid.c_str(), http.getContentSize());
        //if (http.getContent()) {
          //new cJpegPic (3, http.getContent());
          //auto temp = (uint8_t*)bigMalloc (http.getContentSize(), "cHls::jpegPic");
          //memcpy (temp, http.getContent(), http.getContentSize());
          //mContentSize = http.getContentSize();
          //mContent = temp;
          //http.freeContent();
          //cLog::log (LOGINFO1, "- loaded imagePid:%s size:%d", imagePid.c_str(), http.getContentSize());
          //}
        //else
          //mContent = nullptr;
        //}
      //mImagePid = imagePid;
      //}
    //}
  //}}}

  //{{{  vars
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
  //}}}

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
    uint8_t* getPeakSamples (uint16_t chunkFrame, uint32_t subFrameSamples, uint32_t& numSamples, float zoom) {

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

          // find highest peak, +ve only, could abs but not worth it
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

          // scale to uint8_t
          *mPeakSample++ = valueL >> 8;
          *mPeakSample++ = valueR >> 8;
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

    mHost = http.getRedirectable ("as-hls-uk-live.akamaized.net", getM3u8path());

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

  //cSchedule mSchedule;
  //}}}
  };
