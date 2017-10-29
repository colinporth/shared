// hls.h
#pragma once
//{{{  includes
#include "../utils/utils.h"
#include "../utils/cLog.h"
#include "../utils/date.h"

#include "../teensyAac/cAacDecoder.h"
//}}}
//{{{  const
const float kDefaultVolume = 0.8f;
const uint32_t kDefaultChan = 4;
const uint32_t kDefaultBitrate = 128000;

const int kPool [] = { 0, 7, 7, 7, 6, 6, 6 };

const char* kPathNames[] = { "none", "bbc_radio_one",    "bbc_radio_two",       "bbc_radio_three",
                                     "bbc_radio_fourfm", "bbc_radio_five_live", "bbc_6music" };

const uint16_t kFramesPerChunk = 300;
const uint16_t kSamplesPerFrame = 1024;
const uint16_t kSamplesPerSec = 48000;

const float kSamplesPerSecF = kSamplesPerSec;
const double kSamplesPerSecD = kSamplesPerSec;
const float kFramesPerSecond = kSamplesPerSecF / kSamplesPerFrame;
const float kSecondsPerFrame = kSamplesPerFrame / kSamplesPerSecF;

const int kMaxZoom = 4;
const float kNormalZoom = 1;
const int kPeakDecimate = 4;

const int kMaxChunkRange = 1;
const int kMaxChunks = 1 + (2*kMaxChunkRange);
//}}}

class cHls {
public:
  //{{{
  cHls (int chan, int bitrate, int daylightSecs) :
      mHlsChan (chan), mHlsBitrate(bitrate), mDaylightSecs(daylightSecs) {
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
  void getChunkLoad (int chunk, bool& loaded, bool& loading, int& offset) {
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
    int chunk;
    int chunkFrame;
    uint32_t subFrameSamples;

    return findFrame (sample, seqNum, chunk, chunkFrame, subFrameSamples) ?
      mChunks[chunk].getPeakSamples (chunkFrame, subFrameSamples, numSamples, zoom) : nullptr;
    }
  //}}}
  //{{{
  int16_t* getPlaySamples (double sample, uint32_t& seqNum, uint32_t& numSamples) {

    numSamples = 0;

    int chunk;
    int chunkFrame;
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
  uint32_t getPlayTzSec() {
    return uint32_t (mPlaySample / kSamplesPerSec) + mDaylightSecs;
    }
  //}}}
  //{{{
  string getPlayTimeTzStr () {

    auto secsSinceMidnight = getPlayTzSec();

    return dec(secsSinceMidnight / (60*60)) + ':' +
           dec((secsSinceMidnight / 60) % 60, 2, '0') + ':' +
           dec(secsSinceMidnight % 60, 2, '0');
    }
  //}}}

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

  //{{{
  void incPlaySample (double samples) {
    setPlaySample (mPlaySample + samples);
    }
  //}}}
  //{{{
  void incPlayFrame (int incFrames) {
    setPlaySample (mPlaySample + (incFrames * kSamplesPerFrame));
    }
  //}}}
  //{{{
  void incPlaySec (int secs) {
    setPlaySample (mPlaySample + (secs * kSamplesPerSec));
    }
  //}}}

  //{{{
  void setChan (cHttp& http, int chan, int bitrate) {

    clearChunks();
    loadChan (http, chan, bitrate);

    mPlaying = true;
    mScrubbing = false;

    mChanChanged = false;
    }
  //}}}
  //{{{
  void clearChunks() {
    for (auto chunk = 0; chunk < kMaxChunks; chunk++)
      mChunks[chunk].clear();
    }
  //}}}

  //{{{
  bool loadAtPlayFrame (cHttp& http) {
  // load around playFrame, return false if any load failed, no point retrying too soon

    bool ok = true;

    auto seqNum = getSeqNumFromFrame (getPlayFrame());

    int chunk;
    if (!findSeqNumChunk (seqNum, 0, chunk)) {
      cLog::log (LOGINFO, "loading " + dec(seqNum) + " at " + getPlayTimeTzStr());
      ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum), seqNum, mBitrate);
      cLog::log (LOGINFO, "loaded");
      }
    if (mChanChanged)
      return true;

    for (auto range = 1; range <= kMaxChunkRange; range++) {
      if (!findSeqNumChunk (seqNum, range, chunk)) {
        cLog::log (LOGINFO, "loading " + dec(seqNum+range) + " at " + getPlayTimeTzStr());
        ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum+range), seqNum+range, mBitrate);
        cLog::log (LOGINFO, "loaded");
        }
      if (mChanChanged)
        return true;

      if (!findSeqNumChunk (seqNum, -range, chunk)) {
        cLog::log (LOGINFO, "loading " + dec(seqNum-range) + " at " + getPlayTimeTzStr());
        ok &= mChunks[chunk].load (http, mDecoder, mHost, getTsPath (seqNum-range), seqNum-range, mBitrate);
        cLog::log (LOGINFO, "loaded");
        }
      if (mChanChanged)
        return true;
      }

    return ok;
    }
  //}}}

  //{{{  vars
  bool mChanChanged = true;
  int mHlsChan = kDefaultChan;
  int mHlsBitrate = kDefaultBitrate;

  time_t mBaseTime;
  chrono::time_point<chrono::system_clock> mBaseTimePoint;
  chrono::time_point<chrono::system_clock> mPlayTimePoint;

  bool mVolumeChanged = true;
  float mVolume = kDefaultVolume;
  //}}}

private:
  const int kBaseTimeSecsOffset = 17;
  const string kSrc = "as-hls-uk-live.akamaized.net";

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
      mLoaded = false;
      mLoading = false;
      mSrcFramesLoaded = 0;
      }
    //}}}
    //{{{
    bool load (cHttp& http, cAacDecoder* decoder, string host, string path, uint32_t seqNum, uint32_t bitrate) {

      auto ok = true;

      mLoaded = false;
      mLoading = true;
      mSrcPeakFrames = 0;
      mSrcFramesLoaded = 0;
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
    uint8_t* packTsBuffer (uint8_t* buf, uint8_t* bufEnd) {
    // pack transportStream buf to aacFrames back into same buf
    // return new end, shorter than bufEnd

      auto packedBufEnd = buf;
      while ((buf < bufEnd) && (*buf++ == 0x47)) {
        auto payStart = *buf & 0x40;
        auto pid = ((*buf & 0x1F) << 8) | *(buf+1);
        auto headerBytes = (*(buf+2) & 0x20) ? (4 + *(buf+3)) : 3;
        buf += headerBytes;
        auto tsFrameBytesLeft = 187 - headerBytes;
        if (pid == 34) {
          if (payStart && !(*buf) && !(*(buf+1)) && (*(buf+2) == 1) && (*(buf+3) == 0xC0)) {
            int pesHeaderBytes = 9 + *(buf+8);
            buf += pesHeaderBytes;
            tsFrameBytesLeft -= pesHeaderBytes;
            }
          memcpy (packedBufEnd, buf, tsFrameBytesLeft);
          packedBufEnd += tsFrameBytesLeft;
          }
        buf += tsFrameBytesLeft;
        }

      return packedBufEnd;
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
  string getPathRoot() {

    // "pool_x/live/chanPathName/chanPathName.isml/chanPathName-audio=bitrate"
    return "pool_" + dec(kPool[mChan]) +
           "/live/" + kPathNames[mChan] + '/' + kPathNames[mChan] +
           ".isml/" + kPathNames[mChan] + "-audio=" + dec(mBitrate);
    }
  //}}}
  //{{{
  string getM3u8path() {
    return getPathRoot() + ".norewind.m3u8";
    }
  //}}}
  //{{{
  string getTsPath (uint32_t seqNum) {
    return getPathRoot() + '-' + dec(seqNum) + ".ts";
    }
  //}}}

  //{{{
  int getSeqNumOffset (uint32_t seqNum) {
    auto frame = uint32_t (getPlaySample() / kSamplesPerFrame);
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
  bool findSeqNumChunk (uint32_t seqNum, int seqNumOffset, int& chunk) {
  // return true if seqNum+seqNumOffset chunk loaded
  // - else return first stale chunk outside seqNum +- range

    // look for matching chunk
    for (chunk = 0; chunk < kMaxChunks; chunk++)
      if (seqNum + seqNumOffset == mChunks[chunk].getSeqNum())
        return true;

    // look for stale chunk
    for (chunk = 0; chunk < kMaxChunks; chunk++)
      if ((mChunks[chunk].getSeqNum() < seqNum - kMaxChunkRange) ||
          (mChunks[chunk].getSeqNum() > seqNum + kMaxChunkRange))
        return false;

    chunk = 0;
    return false;
    }
  //}}}
  //{{{
  bool findFrame (double sample, uint32_t& seqNum, int& chunk, int& chunkFrame, uint32_t& subFrameSamples) {
  // return true, seqNum, chunk and chunkFrame of loadedChunk from frame
  // - return false if not found

    auto frame = uint32_t(sample / kSamplesPerFrame);
    subFrameSamples = uint32_t(fmod (sample, kSamplesPerFrame));

    seqNum = getSeqNumFromFrame (frame);
    for (chunk = 0; chunk < kMaxChunks; chunk++)
      if (mChunks[chunk].getSeqNum() && (seqNum == mChunks[chunk].getSeqNum())) {
        int r = (frame - mBaseFrame) % kFramesPerChunk;
        chunkFrame = r < 0 ? r + kFramesPerChunk : r;
        return true;
        }

    seqNum = 0;
    chunk = 0;
    chunkFrame = 0;
    return false;
    }
  //}}}

  //{{{
  void setPlaySample (double sample) {
  // update mPlaySample and wall clock time

    mPlaySample = sample;

    // convert mPlaySample to wall clock time
    mPlayTimePoint = floor<date::days>(mBaseTimePoint) + chrono::milliseconds (int(mPlaySample * 1000/ kSamplesPerSec));
    }
  //}}}
  //{{{
  void loadChan (cHttp& http, int chan, int bitrate) {
  // load chan

    mChan = chan;
    mBitrate = bitrate;
    mHost = http.getRedirectable (kSrc, getM3u8path());

    // point to #EXT-X-MEDIA-SEQUENCE: sequence num str
    const auto kExtSeq = "#EXT-X-MEDIA-SEQUENCE:";
    const auto extSeq = strstr ((char*)http.getContent(), kExtSeq) + strlen (kExtSeq);
    const auto extSeqEnd = strchr (extSeq, '\n');
    *extSeqEnd = '\0';
    mBaseSeqNum = atoi (extSeq) + 3;

    // point to #EXT-X-PROGRAM-DATE-TIME dateTime str
    const auto kExtDateTime = "#EXT-X-PROGRAM-DATE-TIME:";
    const auto extDateTime = strstr (extSeqEnd + 1, kExtDateTime) + strlen (kExtDateTime);
    const auto extDateTimeEnd =  strstr (extDateTime + 1, "\n");
    const auto extDateTimeString = string(extDateTime, size_t(extDateTimeEnd - extDateTime));
    cLog::log (LOGNOTICE, kExtDateTime + extDateTimeString);

    // parse ISO time format from string
    istringstream inputStream (extDateTimeString);
    inputStream >> date::parse ("%FT%T", mBaseTimePoint);

    const auto datePoint = floor<date::days>(mBaseTimePoint);
    const auto secsSinceMidnight = chrono::duration_cast<chrono::seconds>(mBaseTimePoint - datePoint);
    mBaseFrame = uint32_t((uint32_t(secsSinceMidnight.count()) - kBaseTimeSecsOffset) * kFramesPerSecond);

    http.freeContent();

    mPlaySample = double(mBaseFrame * kSamplesPerFrame);
    }
  //}}}

  //{{{  vars
  string mHost;
  cChunk mChunks[kMaxChunks];

  uint16_t mChan = 0;
  uint32_t mBitrate = kDefaultBitrate;

  cAacDecoder* mDecoder = 0;

  int mDaylightSecs;
  string mDateTime;
  uint32_t mBaseFrame = 0;
  uint32_t mBaseSeqNum = 0;

  double mPlaySample = 0;
  bool mPlaying = true;
  bool mScrubbing = false;
  //}}}
  };
