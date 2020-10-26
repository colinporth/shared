// cLoaderPlayer.cpp - audio,video loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cLoaderPlayer.h"

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
class cPesParser {
// extract pes from ts, queue?, decode
public:
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
  //{{{
  cPesParser (int pid, bool useQueue, const string& name,
              function <int (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* parserPes)> process,
              function <void (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)> decode)
      : mPid(pid), mName(name), mProcessCallback(process), mDecodeCallback(decode), mUseQueue(useQueue) {

    mPes = (uint8_t*)malloc (kInitPesSize);

    if (useQueue)
      thread ([=](){ dequeThread(); }).detach();
    }
  //}}}
  virtual ~cPesParser() {}

  //{{{
  void clear (int num) {
    mNum = num;
    mPesSize = 0;
    mPts = 0;
    }
  //}}}

  int getPid() { return mPid; }
  int getQueueSize() { return mUseQueue ? (int)mQueue.size_approx() : 0; }
  float getQueueFrac() { return mUseQueue ? (float)mQueue.size_approx() / mQueue.max_capacity() : 0.f; }

  //{{{
  bool parseTs (uint8_t* ts, bool afterPlay) {

    int pid = ((ts[1] & 0x1F) << 8) | ts[2];
    if (pid == mPid) {
      bool payloadStart = ts[1] & 0x40;
      auto headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
      ts += headerSize;
      int tsLeft = 188 - headerSize;

      if (payloadStart) {
        // could check pes type as well

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
      return true;
      }
    else
      return false;
    }
  //}}}
  //{{{
  void process (bool afterPlay) {
    if (mPesSize) {
      mNum = mProcessCallback (afterPlay, mPes, mPesSize, mNum, mPts, this);
      mPesSize = 0;
      }
    }
  //}}}
  //{{{
  void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {

    if (mUseQueue)
      mQueue.enqueue (new cPesParser::cPesItem (afterPlay, pes, size, num, pts));
    else
      mDecodeCallback (afterPlay, pes, size, num, pts);
    }
  //}}}

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
  //{{{
  void dequeThread() {

    cLog::setThreadName (mName + "Q");

    while (true) {
      cPesItem* pesItem;
      mQueue.wait_dequeue (pesItem);
      mDecodeCallback (pesItem->mAfterPlay, pesItem->mPes, pesItem->mPesSize, pesItem->mNum, pesItem->mPts);
      delete pesItem;
      }
    }
  //}}}

  // vars
  int mPid = 0;
  string mName;

  function <int (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* parserPes)> mProcessCallback;
  function <void (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)> mDecodeCallback;

  bool mUseQueue = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;

  int mAllocSize = kInitPesSize;

  uint8_t* mPes = nullptr;
  int mPesSize = 0;
  int mNum = 0;
  int64_t mPts = 0;
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
  audioFrac = mPesParsers.size() > 0 ? mPesParsers[0]->getQueueFrac() : 0.f;
  videoFrac = mPesParsers.size() > 1 ? mPesParsers[1]->getQueueFrac() : 0.f;
  }
//}}}
//{{{
void cLoaderPlayer::getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize) {
// return sizes

  loadSize = mLoadSize;
  audioQueueSize = mPesParsers.size() > 0 ? mPesParsers[0]->getQueueSize() : 0;
  videoQueueSize = mPesParsers.size() > 1 ? mPesParsers[1]->getQueueSize() : 0;
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

  constexpr int kVideoPoolSize = 128;
  const string kM3u8 = ".norewind.m3u8";
  const string hostName = radio ? "as-hls-uk-live.bbcfmt.s.llnwi.net" : "vs-hls-uk-live.akamaized.net";

  cLog::setThreadName ("hls ");
  mRunning = true;
  mExit = false;

  mAudioDecode = new cAudioDecode (cAudioDecode::eAac);
  //{{{  init parsers
  // audio pesParser
  mPesParsers.push_back (
    new cPesParser (0x22, loaderFlags & eQueueAudio, "aud",
      [&](bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
        //{{{  audio pes process callback lambda
        uint8_t* framePes = pes;
        while (getAudioDecode()->parseFrame (framePes, pes + size)) {
          // decode a single frame from pes
          int framePesSize = getAudioDecode()->getNextFrameOffset();
          pesParser->decode (afterPlay, framePes, framePesSize, num, pts);

          // pts of next frame in pes, assumes 48000 sample rate
          pts += (getAudioDecode()->getNumSamplesPerFrame() * 90) / 48;
          num++;

          // point to next frame in pes
          framePes += framePesSize;
          }

        return num;
        },
        //}}}
      [&](bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) noexcept {
        //{{{  audio pes decode callback lambda
        float* samples = mAudioDecode->decodeFrame (pes, size, num, pts);
        if (samples) {
          mSong->addFrame (afterPlay, num, samples, true, mSong->getNumFrames(), pts);
          startPlayer (true);
          }
        else
          cLog::log (LOGERROR, "aud pesParser failed to decode %d %d", size, num);
        }
        //}}}
      )
    );

  if (vidBitrate) {
    mVideoDecode = cVideoDecode::create (loaderFlags & eFFmpeg, loaderFlags & eBgra, kVideoPoolSize);

    // video pesParser
    mPesParsers.push_back (
      new cPesParser (0x21, loaderFlags & eQueueVideo, "vid",
        [&](bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
          //{{{  video pes process callback lambda
          pesParser->decode (afterPlay, pes, size, num, pts);
          num++;
          return num;
          },
          //}}}
        [&](bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) noexcept {
          //{{{  video pes decode callback lambda
          mVideoDecode->decodeFrame (afterPlay, pes, size, num, pts);
          }
          //}}}
        )
      );
    }

  //{{{  PAT pesParser
  mPesParsers.push_back (
    new cPesParser (0x00, false, "pat",
      [&] (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
        // pat callback
        string info;
        for (int i = 0; i < size; i++) {
          int value = pes[i];
          info += hex (value, 2) + " ";
          }
        cLog::log (LOGINFO, "PAT process " + dec (size) + ":" + info);
        return num;
        },
      [&] (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) noexcept {}
      )
    );
  //}}}
  //{{{  PMT pesParser 0x20
  mPesParsers.push_back (
    new cPesParser (0x20, false, "PMT",
      [&] (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* pesParser) noexcept {
        // PMT callback
        string info;
        for (int i = 0; i < size; i++) {
          int value = pes[i];
          info += hex (value, 2) + " ";
          }
        cLog::log (LOGINFO, "PMT process " + dec (size) + ":" + info);
        return num;
        },
      [&] (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) noexcept {}
      )
    );
  //}}}
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
          //{{{  pesParser init
          // always audio
          mPesParsers[0]->clear (frameNum);

          // video
          if (mPesParsers.size() > 1)
            mPesParsers[1]->clear (0);
          //}}}
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
                             // pesParse as we go
                             while (http.getContentSize() - contentParsed >= 188) {
                               uint8_t* ts = http.getContent() + contentParsed;
                               if (*ts == 0x47) {
                                 for (auto pesParser : mPesParsers)
                                   if (pesParser->parseTs (ts, afterPlay))
                                     break;
                                 ts += 188;
                                 }
                               else {
                                 cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                                 return false;
                                 }
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
                //mPesParsers[0]->parseTs (ts, afterPlay);
                //ts += 188;
                //}

              //mPesParsers[0]->process (afterPlay);
              //}
              //}}}
            else {
              //{{{  pesParser finish
              for (auto pesParser : mPesParsers)
                pesParser->process (afterPlay);
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

  cLog::setThreadName ("icy ");

  while (!mExit) {
    int icySkipCount = 0;
    int icySkipLen = 0;
    int icyInfoCount = 0;
    int icyInfoLen = 0;
    char icyInfo[255] = { 0 };

    uint8_t bufferFirst[4096];
    uint8_t* bufferEnd = bufferFirst;
    uint8_t* buffer = bufferFirst;

    int frameNum = -1;
    cAudioDecode decode (cAudioDecode::eAac);

    mSong = new cSong();
    mSong->setChanged (false);

    cPlatformHttp http;
    cUrl parsedUrl;
    parsedUrl.parse (url);
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
          mSong->initialise(frameType, 2, 44100, (frameType == cAudioDecode::eMp3) ? 1152 : 2048, 3000);
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
void cLoaderPlayer::fileLoaderThread() {

  cLog::setThreadName ("file");

  while (!mExit) {
    if (cAudioDecode::mJpegPtr) {
      //{{{  delete any jpegImage
      //mJpegImageView->setImage (nullptr);

      //delete (cAudioDecode::mJpegPtr);
      //cAudioDecode::mJpegPtr = nullptr;
      }
      //}}}

    #ifdef _WIN32
      HANDLE fileHandle = CreateFile (mFileList->getCurFileItem().getFullName().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      HANDLE mapping = CreateFileMapping (fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
      auto fileMapFirst = (uint8_t*)MapViewOfFile (mapping, FILE_MAP_READ, 0, 0, 0);
      auto fileMapSize = GetFileSize (fileHandle, NULL);
      auto fileMapEnd = fileMapFirst + fileMapSize;

      // sampleRate for aac-sbr wrong in header, fixup later, use a maxValue for samples alloc
      int sampleRate;
      auto frameType = cAudioDecode::parseSomeFrames (fileMapFirst, fileMapEnd, sampleRate);
      //if (cAudioDecode::mJpegPtr) // should delete old jpegImage, but we have memory to waste
      //  mJpegImageView->setImage (new cJpegImage (cAudioDecode::mJpegPtr, cAudioDecode::mJpegLen));

      int frameNum = 0;
      bool songDone = false;
      auto fileMapPtr = fileMapFirst;
      cAudioDecode decode (frameType);

      mSong = new cSong();
      if (frameType == cAudioDecode::eWav) {
        //{{{  parse wav
        auto frameSamples = 1024;
        mSong->initialise (frameType, 2, sampleRate, frameSamples);
        decode.parseFrame (fileMapPtr, fileMapEnd);
        auto samples = decode.getFramePtr();
        while (!mExit && !mSong->getChanged() && ((samples + (frameSamples * 2 * sizeof(float))) <= fileMapEnd)) {
          mSong->addFrame (true, frameNum++, (float*)samples, false, fileMapSize / (frameSamples * 2 * sizeof(float)), 0);
          samples += frameSamples * 2 * sizeof(float);
          startPlayer (false);
          }
        }
        //}}}
      else {
        //{{{  parse coded
        mSong->initialise (frameType, 2, sampleRate, (frameType == cAudioDecode::eMp3) ? 1152 : 2048);
        while (!mExit && !mSong->getChanged() && decode.parseFrame (fileMapPtr, fileMapEnd)) {
          if (decode.getFrameType() == mSong->getFrameType()) {
            auto samples = decode.decodeFrame (frameNum);
            if (samples) {
              int numFrames = mSong->getNumFrames();
              int totalFrames = (numFrames > 0) ? int(fileMapEnd - fileMapFirst) / (int(decode.getFramePtr() - fileMapFirst) / numFrames) : 0;
              mSong->setFixups (decode.getNumChannels(), decode.getSampleRate(), decode.getNumSamplesPerFrame());
              mSong->addFrame (true, frameNum++, samples, true, totalFrames+1, 0);
              startPlayer (false);
              }
            }
          fileMapPtr += decode.getNextFrameOffset();
          }
        }
        //}}}
      cLog::log (LOGINFO, "loaded");

      // wait for play to end or abort
      mPlayer.join();
      //{{{  next file
      UnmapViewOfFile (fileMapFirst);
      CloseHandle (fileHandle);

      if (mSong->getChanged()) // use changed fileIndex
        mSong->setChanged (false);
      else if (!mFileList->nextIndex())
        mExit = true;;
      //}}}
    #else
      this_thread::sleep_for (100ms);
    #endif
    }

  clear();
  cLog::log (LOGINFO, "exit");
  }
//}}}

// private
//{{{
void cLoaderPlayer::clear() {

  for (auto pesParser : mPesParsers)
    delete pesParser;
  mPesParsers.clear();

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
string cLoaderPlayer:: getTagValue (uint8_t* buffer, const char* tag) {

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
    mPlayer = std::thread ([=]() {
      // player thread lambda
      cLog::setThreadName ("play");

      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      cAudioDecode decode (mSong->getFrameType());

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
