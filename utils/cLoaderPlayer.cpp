// cLoaderPlayer.cpp - audio,video loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cLoaderPlayer.h"

#include <functional>

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// container
#include "../utils/cSong.h"

// audio decode
#include "../decoders/cAudioDecode.h"

// audio device
#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

// video decode
#include "../utils/cVideoDecode.h"

// net
#ifdef _WIN32
  #include "../net/cWinSockHttp.h"
#else
  #include "../net/cLinuxHttp.h"
#endif

#ifdef _WIN32
  //#include "../common/cJpegImage.h"
  #include "../../shared/utils/cFileList.h"
#endif

#include "readerWriterQueue.h"

using namespace std;
using namespace chrono;
//}}}

//{{{
class cTsParser {
// extract pes from ts
public:
  cTsParser (const string& name) : mName(name) {}
  virtual ~cTsParser() {}

  virtual int getQueueSize() { return 0; }
  virtual float getQueueFrac() { return 0.f; }

  virtual void clear (int num) {}
  virtual void stopAndWait() {}

  //{{{
  void parseTs (uint8_t* ts, bool afterPlay) {

    bool payloadStart = ts[1] & 0x40;
    auto headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
    ts += headerSize;
    int tsLeft = 188 - headerSize;

    processBody (ts, tsLeft, payloadStart, afterPlay);
    }
  //}}}
  virtual void process (bool afterPlay) {}

protected:
  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, bool afterPlay) = 0;

  // vars
  const string mName;
  };
//}}}
//{{{
class cPatTsParser : public cTsParser {
public:
  cPatTsParser() : cTsParser ("pat") {}
  virtual ~cPatTsParser() {}

  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, bool afterPlay) {
    // pat callback
    string info;
    for (int i = 0; i < tsLeft; i++) {
      int value = ts[i];
      info += hex (value, 2) + " ";
      }
    cLog::log (LOGINFO, "PAT ts " + dec (tsLeft) + ":" + info);
    }
  };
//}}}
//{{{
class cPmtTsParser : public cTsParser {
public:
  cPmtTsParser() : cTsParser ("pmt") {}
  virtual ~cPmtTsParser() {}

  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, bool afterPlay) {
    string info;
    for (int i = 0; i < tsLeft; i++) {
      int value = ts[i];
      info += hex (value, 2) + " ";
      }
    cLog::log (LOGINFO, "PMT ts " + dec (tsLeft) + ":" + info);
    }
  };
//}}}
//{{{
class cPesTsParser : public cTsParser {
// extract pes from ts, queue it, decode
//{{{
class cPesItem {
public:
  //{{{
  cPesItem (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)
      : mAfterPlay(afterPlay), mPesSize(size), mNum(num), mPts(pts) {
    mPes = (uint8_t*)malloc (size);
    memcpy (mPes, pes, size);
    }
  //}}}
  //{{{
  ~cPesItem() {
    free (mPes);
    }
  //}}}

  bool mAfterPlay;
  uint8_t* mPes;
  const int mPesSize;
  const int mNum;
  const int64_t mPts;
  };
//}}}
public:
  cPesTsParser (const string& name, bool useQueue = true) : cTsParser(name), mUseQueue(useQueue) {
    if (useQueue)
      thread ([=](){ dequeThread(); }).detach();

    mPes = (uint8_t*)malloc (kInitPesSize);
    }

  virtual ~cPesTsParser() {}

  virtual int getQueueSize() { return (int)mQueue.size_approx(); }
  virtual float getQueueFrac() { return (float)mQueue.size_approx() / mQueue.max_capacity(); }

  virtual void clear (int num) = 0;
  //{{{
  virtual void stopAndWait() {

    if (mUseQueue) {
      mQueueExit = true;
      //while (mQueueRunning)
      this_thread::sleep_for (100ms);
      }
    }
  //}}}
  //{{{
  virtual void processBody (uint8_t* ts, int tsLeft, bool payloadStart, bool afterPlay) {

    if (payloadStart) {
      // end of last pes, if any, process it
      process (afterPlay);

      if (ts[7] & 0x80)
        mPts = getPts (ts+9);

      int headerSize = 9 + ts[8];
      ts += headerSize;
      tsLeft -= headerSize;
      }

    if (mPesSize + tsLeft > mAllocSize) {
      mAllocSize *= 2;
      mPes = (uint8_t*)realloc (mPes, mAllocSize);
      cLog::log (LOGINFO, mName + " pes allocSize doubled to " + dec(mAllocSize));
      }

    memcpy (mPes + mPesSize, ts, tsLeft);
    mPesSize += tsLeft;
    }
  //}}}
  //{{{
  virtual void process (bool afterPlay) {
  // simple process, subclasses may split pes into smaller elements

    if (mPesSize) {
      dispatchDecode (afterPlay, mPes, mPesSize, mNum, mPts);
      mNum++;
      mPesSize = 0;
      }

    }
  //}}}
  //{{{
  virtual void dispatchDecode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
    if (mUseQueue)
      mQueue.enqueue (new cPesTsParser::cPesItem (afterPlay, pes, size, num, pts));
    else
      decode (afterPlay, pes, size, num, pts);
    }
  //}}}
  virtual void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) = 0;

protected:
  //{{{
  void dequeThread() {

    cLog::setThreadName (mName + "Q");

    mQueueRunning = true;

    while (!mQueueExit) {
      cPesItem* pesItem;
      if (mQueue.wait_dequeue_timed (pesItem, 40000)) {
        decode (pesItem->mAfterPlay, pesItem->mPes, pesItem->mPesSize, pesItem->mNum, pesItem->mPts);
        delete pesItem;
        }
      }

    mQueueRunning = false;
    }
  //}}}

  int mAllocSize = kInitPesSize;

  uint8_t* mPes = nullptr;
  int mPesSize = 0;
  int mNum = 0;
  int64_t mPts = 0;

private:
  static constexpr int kInitPesSize = 4096;
  //{{{
  static int64_t getPts (const uint8_t* ts) {
  // return 33 bits of pts,dts

    if ((ts[0] & 0x01) && (ts[2] & 0x01) && (ts[4] & 0x01)) {
      // valid marker bits
      int64_t pts = ts[0] & 0x0E;
      pts = (pts << 7) | ts[1];
      pts = (pts << 8) | (ts[2] & 0xFE);
      pts = (pts << 7) | ts[3];
      pts = (pts << 7) | (ts[4] >> 1);
      return pts;
      }
    else {
      cLog::log (LOGERROR, "getPts marker bits - %02x %02x %02x %02x 0x02", ts[0], ts[1], ts[2], ts[3], ts[4]);
      return 0;
      }
    }
  //}}}

  bool mUseQueue = true;
  bool mQueueExit = false;
  bool mQueueRunning = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;
  };
//}}}
//{{{
class cAudioPesTsParser : public cPesTsParser {
public:
  cAudioPesTsParser (cLoaderPlayer* loaderPlayer) : cPesTsParser("aud"), mLoaderPlayer(loaderPlayer) {}
  virtual ~cAudioPesTsParser() {}

  virtual void clear (int num) {
  // use num as frameNum

    mNum = num;
    mPesSize = 0;
    mPts = 0;
    }

  virtual void process (bool afterPlay) {
  // split pes into several frames
    if (mPesSize) {
      uint8_t* framePes = mPes;
      while (mLoaderPlayer->getAudioDecode()->parseFrame (framePes, mPes + mPesSize)) {
        // decode a single frame from pes
        int framePesSize = mLoaderPlayer->getAudioDecode()->getNextFrameOffset();

        dispatchDecode (afterPlay, framePes, framePesSize, mNum, mPts);

        // pts of next frame in pes, assumes 48000 sample rate
        mPts += (mLoaderPlayer->getAudioDecode()->getNumSamplesPerFrame() * 90) / 48;
        mNum++;

        // point to next frame in pes
        framePes += framePesSize;
        }
      mPesSize = 0;
      }
    }

  virtual void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
  // decode, add to song, and startPlayer
    float* samples = mLoaderPlayer->getAudioDecode()->decodeFrame (pes, size, num, pts);
    if (samples) {
      auto song = mLoaderPlayer->getSong();
      song->addFrame (afterPlay, num, samples, true, song->getNumFrames(), pts);
      mLoaderPlayer->startPlayer (true);
      }
    else
      cLog::log (LOGERROR, "aud pesParser failed to decode %d %d", size, num);
    }

private:
  cLoaderPlayer* mLoaderPlayer;
  };
//}}}
//{{{
class cVideoPesTsParser : public cPesTsParser {
public:
  cVideoPesTsParser (cVideoDecode* videoDecode) : cPesTsParser ("vid"), mVideoDecode(videoDecode) {}
  virtual ~cVideoPesTsParser() {}

  virtual void clear (int num) {
  // use num as frame in chunk for ffmpeg pts synthesis

    mNum = 0;
    mPesSize = 0;
    mPts = 0;
    }

  void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {
    mVideoDecode->decodeFrame (afterPlay, pes, size, num, pts);
    }

private:
  cVideoDecode* mVideoDecode;
  };
//}}}

// public
cLoaderPlayer::cLoaderPlayer() {}
//{{{
cLoaderPlayer::~cLoaderPlayer() {
  clear();
  }
//}}}

//{{{
void cLoaderPlayer::getFrac (float& loadFrac, float& videoFrac, float& audioFrac) {
// return fracs for spinner graphic, true if ok to display

  loadFrac = mLoadFrac;
  //audioFrac = mTsParsers.size() > 0 ? mTsParsers[0]->getQueueFrac() : 0.f;
  //videoFrac = mTsParsers.size() > 1 ? mTsParsers[1]->getQueueFrac() : 0.f;
  audioFrac = 0.f;
  videoFrac = 0.f;
  }
//}}}
//{{{
void cLoaderPlayer::getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize) {
// return sizes

  loadSize = mLoadSize;
  audioQueueSize = 0;
  videoQueueSize = 0;
  //audioQueueSize = mTsParsers.size() > 0 ? mTsParsers[0]->getQueueSize() : 0;
  //videoQueueSize = mTsParsers.size() > 1 ? mTsParsers[1]->getQueueSize() : 0;
  }
//}}}

//{{{
void cLoaderPlayer::videoFollowAudio() {

  if (mVideoDecode) {
    // get play frame
    auto framePtr = mSong->getFramePtr (mSong->getPlayFrame());
    if (framePtr) {
      auto pts = framePtr->getPts();
      mVideoDecode->setPlayPts (pts);
      mVideoDecode->clear (pts);
      }
    }
  }
//}}}

//{{{
void cLoaderPlayer::hlsLoaderThread (bool radio, const string& channelName,
                                     int audBitrate, int vidBitrate, eLoaderFlags loaderFlags) {
// hls http chunk load and possible decode thread

  const string hostName = radio ? "as-hls-uk-live.bbcfmt.s.llnwi.net" : "vs-hls-uk-live.akamaized.net";
  const string kM3u8 = ".norewind.m3u8";
  constexpr int kVideoPoolSize = 128;

  cLog::setThreadName ("hlsL");
  mRunning = true;
  mExit = false;

  mAudioDecode = new cAudioDecode (cAudioDecode::eAac);
  //{{{  init parsers
  // audio pesParser
  //loaderFlags & eQueueAudio,
  mTsParsers.insert (map<int, cTsParser*>::value_type (0x22, new cAudioPesTsParser (this)));

  if (vidBitrate) {
    mVideoDecode = cVideoDecode::create (loaderFlags & eFFmpeg, loaderFlags & eBgra, kVideoPoolSize);
    //loaderFlags & eQueueVideo
    mTsParsers.insert (map<int, cTsParser*>::value_type (0x21, new cVideoPesTsParser (mVideoDecode)));
    }

  // PAT pesParser
  mTsParsers.insert (map<int, cTsParser*>::value_type (0x00, new cPatTsParser()));

  // PMT pesParser 0x20
  mTsParsers.insert (map<int, cTsParser*>::value_type (0x20, new cPmtTsParser()));
  //}}}

  // audBitrate < 128000 use aacHE, more samplesPerframe, less framesPerChunk
  mSong = new cSong();
  mSong->initialise (cAudioDecode::eAac, 2, 48000, audBitrate < 128000 ? 2048 : 1024, 1000);
  mSong->setBitrateFramesPerChunk (audBitrate, audBitrate < 128000 ? (radio ? 150 : 180) : (radio ? 300 : 360));
  mSong->setChannel (channelName);

  while (!mExit && !mSong->getChanged()) {
    mSong->setChanged (false);
    cPlatformHttp http;
    string pathName = getHlsPathName (radio, vidBitrate);
    string redirectedHostName = http.getRedirect (hostName, pathName + kM3u8);
    if (http.getContent()) {
      //{{{  parse m3u8 for mediaSequence,programDateTimePoint
      int extXMediaSequence = stoi (getTagValue (http.getContent(), "#EXT-X-MEDIA-SEQUENCE:"));
      istringstream inputStream (getTagValue (http.getContent(), "#EXT-X-PROGRAM-DATE-TIME:"));

      system_clock::time_point extXProgramDateTimePoint;
      inputStream >> date::parse ("%FT%T", extXProgramDateTimePoint);

      http.freeContent();

      mSong->setHlsBase (extXMediaSequence, extXProgramDateTimePoint, -37s);
      //}}}
      while (!mExit && !mSong->getChanged()) {
        int chunkNum;
        int frameNum;
        if (mSong->loadChunk (system_clock::now(), 2, chunkNum, frameNum)) {
          bool afterPlay = frameNum >= mSong->getPlayFrame();
          for (auto tsParser : mTsParsers)
            tsParser.second->clear (frameNum);
          int contentParsed = 0;
          if (http.get (redirectedHostName, pathName + '-' + dec(chunkNum) + ".ts", "",
                        [&] (const string& key, const string& value) noexcept {
                          //{{{  header lambda
                          if (key == "content-length")
                            cLog::log (LOGINFO, "chunk:" + dec(chunkNum) +
                                                " frame:" + dec(frameNum) +
                                                " size:" + dec(http.getHeaderContentSize()/1000) + "k");
                          },
                          //}}}
                        [&] (const uint8_t* data, int length) noexcept {
                          //{{{  data lambda
                          mLoadSize = http.getContentSize();
                          mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();

                          if (!radio) {
                            // tsParse as we go
                            while (http.getContentSize() - contentParsed >= 188) {
                              uint8_t* ts = http.getContent() + contentParsed;
                              if (*ts == 0x47) {
                                int pid = ((ts[1] & 0x1F) << 8) | ts[2];
                                auto it = mTsParsers.find (pid);
                                if (it != mTsParsers.end())
                                  it->second->parseTs (ts, afterPlay);
                                else
                                  cLog::log (LOGERROR, "pid parser not found %d", pid);
                                ts += 188;
                                }
                              else
                                cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                              contentParsed += 188;
                              }
                            }
                          return true;
                          }
                          //}}}
                        ) == 200) {
            // ??? why does pesParser fail for radio audio pes ???
            if (radio) {
              //{{{  parse chunk of ts
              // extract audio pes from chunk of ts packets, write it back crunched into ts, always gets smaller as ts stripped
              uint8_t* tsPtr = http.getContent();
              uint8_t* tsEndPtr = tsPtr + http.getContentSize();
              uint8_t* pesPtr = tsPtr;
              while ((tsPtr < tsEndPtr) && (*tsPtr++ == 0x47)) {
                // ts packet start
                auto pid = ((tsPtr[0] & 0x1F) << 8) | tsPtr[1];
                if (pid == 34) {
                  // audio pid
                  bool payStart = tsPtr[0] & 0x40;
                  int headerBytes = (tsPtr[2] & 0x20) ? 4 + tsPtr[3] : 3;
                  tsPtr += headerBytes;
                  int tsBodyBytes = 187 - headerBytes;
                  if (payStart) {
                    int pesHeaderBytes = 9 + tsPtr[8];
                    tsPtr += pesHeaderBytes;
                    tsBodyBytes -= pesHeaderBytes;
                    }

                  // copy ts payload back into same buffer, always copying to lower address in same buffer
                  memcpy (pesPtr, tsPtr, tsBodyBytes);
                  pesPtr += tsBodyBytes;
                  tsPtr += tsBodyBytes;
                  }
                else // PAT and PMT 0x20 pids expected
                  tsPtr += 187;
                }

              // parse audio pes for audio frames
              uint8_t* pesEnd = pesPtr;
              pesPtr = http.getContent();
              while (getAudioDecode()->parseFrame (pesPtr, pesEnd)) {
                float* samples = getAudioDecode()->decodeFrame (frameNum);
                if (samples) {
                  mSong->addFrame (afterPlay, frameNum++, samples, true, mSong->getNumFrames(), 0);
                  startPlayer (true);
                  }
                else
                  cLog::log (LOGERROR, "aud parser failed to decode %d", frameNum);

                pesPtr += getAudioDecode()->getNextFrameOffset();
                }
              }
              //}}}
              //{{{  pesParse chunk of ts - should work but fails !!! find out why !!!
              //uint8_t* ts = http.getContent();
              //uint8_t* tsEnd = ts + http.getContentSize();
              //while ((ts < tsEnd) && (*ts == 0x47)) {
                //mTsParsers[0]->parseTs (ts, afterPlay);
                //ts += 188;
                //}

              //mTsParsers[0]->process (afterPlay);
              //}
              //}}}
            else {
              //{{{  pesParser finish
              for (auto tsParser : mTsParsers)
                tsParser.second->process (afterPlay);
              }
              //}}}
            http.freeContent();
            }
          else {
            //{{{  failed to load expected to be available chunk, backoff for 250ms
            cLog::log (LOGERROR, "late " + dec(chunkNum));
            this_thread::sleep_for (250ms);
            }
            //}}}
          }
        else // no chunk available, back off for 100ms
          this_thread::sleep_for (100ms);
        }

      // wait for playerThread to end
      mPlayer.join();
      }
    }

  clear();
  cLog::log (LOGINFO, "exit");
  mRunning = false;
  }
//}}}
//{{{
void cLoaderPlayer::icyLoaderThread (const string& url) {

  cLog::setThreadName ("icyL");

  while (!mExit) {
    int icySkipCount = 0;
    int icySkipLen = 0;
    int icyInfoCount = 0;
    int icyInfoLen = 0;
    char icyInfo[255] = { 0 };

    uint8_t bufferFirst[4096];
    uint8_t* bufferEnd = bufferFirst;
    uint8_t* buffer = bufferFirst;

    // init container and audDecode
    mSong = new cSong();
    cAudioDecode decode (cAudioDecode::eAac);

    // init http
    cPlatformHttp http;
    cUrl parsedUrl;
    parsedUrl.parse (url);

    int frameNum = -1;
    http.get (parsedUrl.getHost(), parsedUrl.getPath(), "Icy-MetaData: 1",
      //{{{  headerCallback lambda
      [&](const string& key, const string& value) noexcept {
        if (key == "icy-metaint")
          icySkipLen = stoi (value);
        },
      //}}}
      //{{{  dataCallback lambda
      [&] (const uint8_t* data, int length) noexcept {
        // return false to exit

        // cLog::log (LOGINFO, "callback %d", length);
        if ((icyInfoCount >= icyInfoLen) && (icySkipCount + length <= icySkipLen)) {
          //{{{  simple copy of whole body, no metaInfo
          //cLog::log (LOGINFO1, "body simple copy len:%d", length);

          memcpy (bufferEnd, data, length);

          bufferEnd += length;
          icySkipCount += length;
          }
          //}}}
        else {
          //{{{  dumb copy for metaInfo straddling body, could be much better
          //cLog::log (LOGINFO1, "body split copy length:%d info:%d:%d skip:%d:%d ",
                                //length, icyInfoCount, icyInfoLen, icySkipCount, icySkipLen);
          for (int i = 0; i < length; i++) {
            if (icyInfoCount < icyInfoLen) {
              icyInfo [icyInfoCount] = data[i];
              icyInfoCount++;
              if (icyInfoCount >= icyInfoLen)
                addIcyInfo (frameNum, icyInfo);
              }
            else if (icySkipCount >= icySkipLen) {
              icyInfoLen = data[i] * 16;
              icyInfoCount = 0;
              icySkipCount = 0;
              //cLog::log (LOGINFO1, "body icyInfo len:", data[i] * 16);
              }
            else {
              icySkipCount++;
              *bufferEnd = data[i];
              bufferEnd++;
              }
            }

          return !mExit && !mSong->getChanged();
          }
          //}}}

        if (frameNum == -1) {
          // enough data to determine frameType and sampleRate (wrong for aac sbr)
          frameNum = 0;
          int sampleRate;
          auto frameType = cAudioDecode::parseSomeFrames (bufferFirst, bufferEnd, sampleRate);
          mSong->initialise (frameType, 2, 44100, (frameType == cAudioDecode::eMp3) ? 1152 : 2048, 3000);
          }

        while (decode.parseFrame (buffer, bufferEnd)) {
          if (decode.getFrameType() == mSong->getFrameType()) {
            auto samples = decode.decodeFrame (frameNum);
            if (samples) {
              mSong->setFixups (decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame());
              mSong->addFrame (true, frameNum++, samples, true, mSong->getNumFrames()+1, 0);
              startPlayer (true);
              }
            }
          buffer += decode.getNextFrameOffset();
          }

        if ((buffer > bufferFirst) && (buffer < bufferEnd)) {
          // shuffle down last partial frame
          auto bufferLeft = int(bufferEnd - buffer);
          memcpy (bufferFirst, buffer, bufferLeft);
          bufferEnd = bufferFirst + bufferLeft;
          buffer = bufferFirst;
          }

        return !mExit && !mSong->getChanged();
        }
      //}}}
      );

    cLog::log (LOGINFO, "icyThread songChanged");
    mPlayer.join();
    }

  clear();
  cLog::log (LOGINFO, "exit");
  }
//}}}
//{{{
void cLoaderPlayer::fileLoaderThread (const string& filename) {

  cLog::setThreadName ("file");

  while (!mExit) {
    #ifdef _WIN32
      //{{{  windows file map
      HANDLE fileHandle = CreateFile (filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      HANDLE fileMapping = CreateFileMapping (fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
      uint8_t* fileFirst = (uint8_t*)MapViewOfFile (fileMapping, FILE_MAP_READ, 0, 0, 0);
      auto fileSize = GetFileSize (fileHandle, NULL);
      uint8_t* fileEnd = fileFirst + fileSize;
      //}}}
    #else
      //{{{  linux mmap to do
      uint8_t* fileFirst = nullptr;
      auto fileSize = 0;
      uint8_t* fileEnd = fileFirst + fileSize;
      //}}}
    #endif

    if ((fileFirst[0] == 0x47) && (fileFirst[188] == 0x47)) {
      //{{{  ts
      cLog::log (LOGINFO, "Ts file %d", fileSize);

      // PAT pesParser
      mTsParsers.insert (map<int, cTsParser*>::value_type (0x00, new cPatTsParser()));

      mAudioDecode = new cAudioDecode (cAudioDecode::eAac);
      mVideoDecode = cVideoDecode::create (true, false, 128);

      mSong = new cSong();
      mSong->initialise (cAudioDecode::eAac, 2, 48000, 1024, 1000);

      // mTsParsers init
      for (auto tsParser : mTsParsers)
        tsParser.second->clear (0);

      uint8_t* ts = fileFirst;
      while ((ts + 188) <= fileEnd) {
        if (*ts == 0x47) {
          int pid = ((ts[1] & 0x1F) << 8) | ts[2];
          auto it = mTsParsers.find (pid);
          if (it != mTsParsers.end())
            it->second->parseTs (ts, true);
          else
            cLog::log (LOGERROR, "pid parser not found %d", pid);
          ts += 188;
          }
        else {
          cLog::log (LOGERROR, "ts packet sync");
          ts++;
          }

        mLoadFrac = float(ts - fileFirst) / fileSize;
        }
      mLoadFrac = float(ts - fileFirst) / fileSize;

      // mTsParsers finish
      for (auto tsParser : mTsParsers)
        tsParser.second->process (true);

      cLog::log (LOGINFO, "loaded");
      }
      //}}}
    else {
      //{{{  aac or mp3
      int sampleRate;
      auto fileFrameType = cAudioDecode::parseSomeFrames (fileFirst, fileEnd, sampleRate);
      cAudioDecode decode (fileFrameType);

      if (cAudioDecode::mJpegPtr) {
        //{{{  found jpegImage
        cLog::log (LOGINFO, "found jpeg piccy in file, load it");
        delete (cAudioDecode::mJpegPtr);
        cAudioDecode::mJpegPtr = nullptr;
        }
        //}}}

      mSong = new cSong();
      int frameNum = 0;
      if (fileFrameType == cAudioDecode::eWav) {
        // parse wav
        constexpr int kFrameSamples = 1024;
        mSong->initialise (fileFrameType, 2, sampleRate, kFrameSamples, 0);

        uint8_t* filePtr = fileFirst;
        decode.parseFrame (filePtr, fileEnd);
        auto samples = decode.getFramePtr();
        while (!mExit && !mSong->getChanged() && ((samples + (kFrameSamples * 2 * sizeof(float))) <= fileEnd)) {
          mSong->addFrame (true, frameNum++, (float*)samples, false, fileSize / (kFrameSamples * 2 * sizeof(float)), 0);
          samples += kFrameSamples * 2 * sizeof(float);
          startPlayer (false);
          mLoadFrac = float(samples - fileFirst) / fileSize;
          }
        mLoadFrac = float(samples - fileFirst) / fileSize;
        }

      else {
        // parse coded
        bool firstSamples = true;
        uint8_t* filePtr = fileFirst;
        while (!mExit && !mSong->getChanged() && decode.parseFrame (filePtr, fileEnd)) {
          // parsed frame
          if (decode.getFrameType() == fileFrameType) {
            // frame is expected type, will get id3 which we ignore
            float* samples = decode.decodeFrame (frameNum);
            if (samples) {
              if (firstSamples) // need to decode a frame to set sampleRate for aacHE, its wrong in the header
                mSong->initialise (fileFrameType,
                                   decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame(), 0);

              int numFrames = mSong->getNumFrames();
              int totalFrames = (numFrames > 0) ? (fileSize / (int(decode.getFramePtr() - fileFirst) / numFrames)) : 0;
              mSong->addFrame (true, frameNum++, samples, true, totalFrames+1, 0);

              if (!firstSamples)
                startPlayer (false);
              firstSamples = false;
              }
            }
          filePtr += decode.getNextFrameOffset();
          mLoadFrac = float(filePtr - fileFirst) / fileSize;
          }
        mLoadFrac = float(filePtr - fileFirst) / fileSize;
        }

      cLog::log (LOGINFO, "loaded");
      // wait for play to end or abort, wav uses the file mapping pointers to play
      mPlayer.join();
      }
      //}}}

    #ifdef _WIN32
      //{{{  windows file map close
      UnmapViewOfFile (fileFirst);
      CloseHandle (fileHandle);
      //}}}
    #else
      //{{{  linux mmap close to do
      //}}}
    #endif

    //{{{  next file
    //if (mSong->getChanged()) // use changed fileIndex
    //  mSong->setChanged (false);
    //else if (!mFileList->nextIndex())
    //}}}
    mExit = true;;
    }

  clear();
  cLog::log (LOGINFO, "exit");
  }
//}}}

// private
//{{{
void cLoaderPlayer::clear() {

  for (auto tsParser : mTsParsers)
    tsParser.second->stopAndWait();
  for (auto tsParser : mTsParsers)
    delete tsParser.second;
  mTsParsers.clear();

  // remove main container
  auto tempSong = mSong;
  mSong = nullptr;
  delete tempSong;

  auto tempAudioDecode = mAudioDecode;
  mAudioDecode = nullptr;
  delete tempAudioDecode;

  auto tempVideoDecode = mVideoDecode;
  mVideoDecode = nullptr;
  delete tempVideoDecode;
  }
//}}}

//{{{
string cLoaderPlayer::getHlsPathName (bool radio, int vidBitrate) {

  const string mPoolName = radio ? "pool_904/live/uk/" : "pool_902/live/uk/";

  string pathName = mPoolName + mSong->getChannel() +
                    "/" + mSong->getChannel() +
                    ".isml/" + mSong->getChannel();
  if (radio)
    pathName += "-audio=" + dec(mSong->getBitrate());
  else {
    pathName += mSong->getBitrate() < 128000 ? "-pa3=" : "-pa4=" + dec(mSong->getBitrate());
    if (vidBitrate)
      pathName += "-video=" + dec(vidBitrate);
    }

  return pathName;
  }
//}}}
//{{{
string cLoaderPlayer::getTagValue (uint8_t* buffer, const char* tag) {

  const char* tagPtr = strstr ((const char*)buffer, tag);
  const char* valuePtr = tagPtr + strlen (tag);
  const char* endPtr = strchr (valuePtr, '\n');

  return string (valuePtr, endPtr - valuePtr);
  }
//}}}
//{{{
void cLoaderPlayer::addIcyInfo (int frame, const string& icyInfo) {

  cLog::log (LOGINFO, "addIcyInfo " + icyInfo);

  string icysearchStr = "StreamTitle=\'";
  string searchStr = "StreamTitle=\'";
  auto searchStrPos = icyInfo.find (searchStr);
  if (searchStrPos != string::npos) {
    auto searchEndPos = icyInfo.find ("\';", searchStrPos + searchStr.size());
    if (searchEndPos != string::npos) {
      string titleStr = icyInfo.substr (searchStrPos + searchStr.size(), searchEndPos - searchStrPos - searchStr.size());
      if (titleStr != mLastTitleStr) {
        cLog::log (LOGINFO1, "addIcyInfo found title = " + titleStr);
        mSong->getSelect().addMark (frame, titleStr);
        mLastTitleStr = titleStr;
        }
      }
    }

  string urlStr = "no url";
  searchStr = "StreamUrl=\'";
  searchStrPos = icyInfo.find (searchStr);
  if (searchStrPos != string::npos) {
    auto searchEndPos = icyInfo.find ('\'', searchStrPos + searchStr.size());
    if (searchEndPos != string::npos) {
      urlStr = icyInfo.substr (searchStrPos + searchStr.size(), searchEndPos - searchStrPos - searchStr.size());
      cLog::log (LOGINFO1, "addIcyInfo found url = " + urlStr);
      }
    }
  }
//}}}

//{{{
void cLoaderPlayer::startPlayer (bool streaming) {

  if (!mPlayer.joinable()) {
    mPlayer = thread ([=]() {
      // lambda - player
      cLog::setThreadName ("play");

      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      #ifdef _WIN32
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        //{{{  WSAPI player thread, video just follows play pts
        auto device = getDefaultAudioOutputDevice();

        if (device) {
          cLog::log (LOGINFO, "device @ %d", mSong->getSampleRate());
          device->setSampleRate (mSong->getSampleRate());
          device->start();

          cSong::cFrame* framePtr;
          while (!mExit && !mSong->getChanged()) {
            device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
              // lambda callback - load srcSamples
              shared_lock<shared_mutex> lock (mSong->getSharedMutex());

              framePtr = mSong->getFramePtr (mSong->getPlayFrame());
              if (mPlaying && framePtr && framePtr->getSamples()) {
                if (mSong->getNumChannels() == 1) {
                  //{{{  mono to stereo
                  auto src = framePtr->getSamples();
                  auto dst = samples;
                  for (int i = 0; i < mSong->getSamplesPerFrame(); i++) {
                    *dst++ = *src;
                    *dst++ = *src++;
                    }
                  }
                  //}}}
                else
                  memcpy (samples, framePtr->getSamples(), mSong->getSamplesPerFrame() * mSong->getNumChannels() * sizeof(float));
                srcSamples = samples;
                }
              else
                srcSamples = silence;
              numSrcSamples = mSong->getSamplesPerFrame();

              if (mVideoDecode && framePtr)
                mVideoDecode->setPlayPts (framePtr->getPts());
              if (mPlaying && framePtr)
                mSong->incPlayFrame (1, true);
              });

            if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
              break;
            }

          device->stop();
          }
        //}}}
      #else
        //SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        //{{{  audio16 player thread, video just follows play pts
        cAudio audio (2, mSong->getSampleRate(), 40000, false);

        cSong::cFrame* framePtr;
        while (!mExit && !mSong->getChanged()) {
          float* playSamples = silence;
            {
            // scoped song mutex
            shared_lock<shared_mutex> lock (mSong->getSharedMutex());
            framePtr = mSong->getFramePtr (mSong->getPlayFrame());
            bool gotSamples = mPlaying && framePtr && framePtr->getSamples();
            if (gotSamples) {
              memcpy (samples, framePtr->getSamples(), mSong->getSamplesPerFrame() * 8);
              playSamples = samples;
              }
            }
          audio.play (2, playSamples, mSong->getSamplesPerFrame(), 1.f);

          if (mVideoDecode && framePtr)
            mVideoDecode->setPlayPts (framePtr->getPts());
          if (mPlaying && framePtr)
            mSong->incPlayFrame (1, true);

          if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
            break;
          }
        //}}}
      #endif

      cLog::log (LOGINFO, "exit");
      });
    }
  }
//}}}
