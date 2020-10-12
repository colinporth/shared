// cHlsPlayer.cpp - video, audio loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cHlsPlayer.h"

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

using namespace std;
using namespace chrono;
//}}}

namespace {
  //{{{
  uint64_t getPts (const uint8_t* ts) {
  // return 33 bits of pts,dts

    if ((ts[0] & 0x01) && (ts[2] & 0x01) && (ts[4] & 0x01)) {
      // valid marker bits
      uint64_t pts = ts[0] & 0x0E;
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
  string getTagValue (uint8_t* buffer, const char* tag) {

    const char* tagPtr = strstr ((const char*)buffer, tag);
    const char* valuePtr = tagPtr + strlen (tag);
    const char* endPtr = strchr (valuePtr, '\n');

    return string (valuePtr, endPtr - valuePtr);
    }
  //}}}
  }

//{{{
class cPesParser {
public:
  //{{{
  class cPes {
  public:
    //{{{
    cPes (uint8_t* pes, int size, int num, uint64_t pts) : mSize(size), mNum(num), mPts(pts) {
      mPes = (uint8_t*)malloc (size);
      memcpy (mPes, pes, size);
      }
    //}}}
    //{{{
    ~cPes() {
      free (mPes);
      }
    //}}}

    uint8_t* mPes;
    const int mSize;
    const int mNum;
    const uint64_t mPts;
    };
  //}}}
  //{{{
  cPesParser (cHlsPlayer* hlsPlayer, bool useQueue) : mHlsPlayer(hlsPlayer), mUseQueue(useQueue) {
    mPes = (uint8_t*)malloc (kPesSize);
    }
  //}}}

  virtual ~cPesParser() {}

  //{{{
  void clear() {
    int mNum = 0;
    int mPesSize = 0;
    uint64_t mPesPts = 0;
    }
  //}}}
  int getNum() { return mNum; }
  void setNum (int num) { mNum = num; }

  //{{{
  void parsePes (bool payStart, uint8_t* ts, int tsBodySize) {

    if (payStart) {
      processLastPes();

      if (ts[7] & 0x80)
        mPesPts = getPts (ts+9);

      int headerSize = 9 + ts[8];
      ts += headerSize;
      tsBodySize -= headerSize;
      }

    memcpy (mPes + mPesSize, ts, tsBodySize);
    mPesSize += tsBodySize;
    }
  //}}}
  //{{{
  void processLastPes() {
    if (mPesSize) {
      processPes();
      mPesSize = 0;
      }
    }
  //}}}

protected:
  virtual void processPes() = 0;

  bool mUseQueue = false;
  cHlsPlayer* mHlsPlayer = nullptr;
  uint8_t* mPes = nullptr;
  int mNum = 0;
  int mPesSize = 0;
  uint64_t mPesPts = 0;
  readerWriterQueue::cBlockingReaderWriterQueue <cPes*> mPesQueue;

private:
  static constexpr int kPesSize = 1000000;

  virtual void dequePesThread() = 0;
  };
//}}}
//{{{
class cAudPesParser : public cPesParser {
public:
  //{{{
  cAudPesParser (cHlsPlayer* hlsPlayer, bool useQueue) : cPesParser(hlsPlayer, useQueue) {
    if (useQueue)
      thread ([=](){ dequePesThread(); }).detach();
    }
  //}}}
  virtual ~cAudPesParser() {}

protected:
  //{{{
  void processPes() {
    // split audio pes into frames, queue or decode now
    uint8_t* framePes = mPes;
    while (mHlsPlayer->getAudioDecode()->parseFrame (framePes, mPes + mPesSize)) {
      int framePesSize = mHlsPlayer->getAudioDecode()->getNextFrameOffset();

      // decode a single frame from pes
      if (mUseQueue)
        mPesQueue.enqueue (new cPes (framePes, framePesSize, mNum, mPesPts));
      else {
        float* samples = mHlsPlayer->getAudioDecode()->decodeFrame (framePes, framePesSize, mNum);
        mHlsPlayer->getSong()->addAudioFrame (mNum, samples, true, mHlsPlayer->getSong()->getNumFrames(), nullptr, mPesPts);
        }

      // point to next frame in pes, assumes 48000 sample rate
      mPesPts += (mHlsPlayer->getAudioDecode()->getNumSamples() * 90) / 48;
      mNum++;
      framePes += framePesSize;
      }

    mHlsPlayer->startPlayer();
    }
  //}}}

private:
  //{{{
  void dequePesThread() {

    cLog::setThreadName ("audQ");

    while (true) {
      cPes* pes;
      mPesQueue.wait_dequeue (pes);

      float* samples = mHlsPlayer->getAudioDecode()->decodeFrame (pes->mPes, pes->mSize, pes->mNum);
      mHlsPlayer->getSong()->addAudioFrame (pes->mNum, samples, true, mHlsPlayer->getSong()->getNumFrames(), nullptr, pes->mPts);

      delete pes;
      }
    }
  //}}}
  };
//}}}
//{{{
class cVidPesParser : public cPesParser {
public:
  //{{{
  cVidPesParser (cHlsPlayer* hlsPlayer, bool useQueue) : cPesParser(hlsPlayer, useQueue) {
    if (useQueue)
      thread ([=](){ dequePesThread(); }).detach();
    }
  //}}}
  virtual ~cVidPesParser() {}

protected:
  //{{{
  void processPes() {
    if (mUseQueue)
      mPesQueue.enqueue (new cPes (mPes, mPesSize, mNum, mPesPts));
    else
      mHlsPlayer->getVideoDecode()->decode (mPes, mPesSize, mNum, mPesPts);

    mNum++;
    }
  //}}}

private:
  //{{{
  void dequePesThread() {

    cLog::setThreadName ("vidQ");

    while (true) {
      cPes* pes;
      mPesQueue.wait_dequeue (pes);
      mHlsPlayer->getVideoDecode()->decode (pes->mPes, pes->mSize, pes->mNum, pes->mPts);
      delete pes;
      }
    }
  //}}}
  };
//}}}

// public
cHlsPlayer::cHlsPlayer() {}
//{{{
cHlsPlayer::~cHlsPlayer() {

  delete mSong;
  delete mVideoDecode;
  delete mAudioDecode;
  delete mVidPesParser;
  delete mAudPesParser;
  }
//}}}
//{{{
void cHlsPlayer::init (const string& host, const string& channel, int audBitrate, int vidBitrate,
                       bool useFFmpeg, bool queueVideo, bool queueAudio, bool streaming) {

  mHost = host;
  mChannel = channel;
  mAudBitrate = audBitrate;
  mVidBitrate = vidBitrate;

  mSong = new cSong();
  mAudioDecode = new cAudioDecode (cAudioDecode::eAac);

  mUseFFmpeg = useFFmpeg;
  if (useFFmpeg)
    mVideoDecode = new cFFmpegVideoDecode();
  #ifdef _WIN32
    else
      mVideoDecode = new cMfxVideoDecode();
  #endif

  mQueueVideo = queueVideo;
  mVidPesParser = new cVidPesParser (this, queueVideo);

  mQueueAudio = queueAudio;
  mAudPesParser = new cAudPesParser (this, queueAudio);

  mStreaming = streaming;
  }
//}}}

// protected
//{{{
void cHlsPlayer::videoFollowAudio() {

  auto framePtr = mSong->getAudioFramePtr (mSong->getPlayFrame());
  if (framePtr) {
    mVideoDecode->setPlayPts (framePtr->getPts());
    mVideoDecode->clear (framePtr->getPts());
    }
  }
//}}}
//{{{
void cHlsPlayer::loaderThread() {
// hls http chunk load and possible decode thread

  cLog::setThreadName ("hls ");

  mSong->setChannel (mChannel);
  mSong->setBitrate (mAudBitrate, mAudBitrate < 128000 ? 180 : 360); // audBitrate, audioFrames per chunk

  while (!mExit && !mSong->getChanged()) {
    mSong->setChanged (false);
    const string path = "pool_902/live/uk/" + mSong->getChannel() +
                        "/" + mSong->getChannel() +
                        ".isml/" + mSong->getChannel() +
                        (mSong->getBitrate() < 128000 ? "-pa3=" : "-pa4=") + dec(mSong->getBitrate()) +
                        "-video=" + dec(mVidBitrate);
    cPlatformHttp http;
    string redirectedHost = http.getRedirect (mHost, path + ".m3u8");
    if (http.getContent()) {
      //{{{  got .m3u8, parse for mediaSequence, programDateTimePoint
      int mediaSequence = stoi (getTagValue (http.getContent(), "#EXT-X-MEDIA-SEQUENCE:"));

      istringstream inputStream (getTagValue (http.getContent(), "#EXT-X-PROGRAM-DATE-TIME:"));
      system_clock::time_point programDateTimePoint;
      inputStream >> date::parse ("%FT%T", programDateTimePoint);

      http.freeContent();
      //}}}
      mSong->init (cAudioDecode::eAac, 2, 48000, mSong->getBitrate() < 128000 ? 2048 : 1024); // samplesPerFrame
      mSong->setHlsBase (mediaSequence, programDateTimePoint, -37s, (2*60*60) - 25);

      while (!mExit && !mSong->getChanged()) {
        int chunkNum = mSong->getHlsLoadChunkNum (system_clock::now(), 12s, 2);
        if (chunkNum) {
          //{{{  init pes states
          int contentUsed = 0;
          mAudPesParser->clear();
          mVidPesParser->clear();
          //}}}

          // get hls chunkNum chunk
          mAudPesParser->setNum (mSong->getFrameNumFromChunkNum (chunkNum));
          cLog::log (LOGINFO, "get chunkNum:" + dec(chunkNum) + " frameNum:" + dec(mAudPesParser->getNum()));
          if (http.get (redirectedHost, path + '-' + dec(chunkNum) + ".ts", "",
                        [&] (const string& key, const string& value) noexcept { /* headerCallback lambda */ },
                        [&] (const uint8_t* data, int length) noexcept {
                           //{{{  data callback lambda
                           mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();

                           while (http.getContentSize() - contentUsed >= 188) {
                             // whole ts packet left to parse
                             uint8_t* ts = http.getContent() + contentUsed;
                             if (*ts++ == 0x47) {
                               // is a ts packet
                               auto pid = ((ts[0] & 0x1F) << 8) | ts[1];
                               auto headerSize = (ts[2] & 0x20) ? 4 + ts[3] : 3;
                               if (pid == 33)
                                 mVidPesParser->parsePes (ts[0] & 0x40, ts + headerSize, 187 - headerSize);
                               else if (pid == 34)
                                 mAudPesParser->parsePes (ts[0] & 0x40, ts + headerSize, 187 - headerSize);
                               else if (pid && (pid != 32))
                                 cLog::log (LOGERROR, "other pid %d", pid);

                               ts += 187;
                               }
                             else
                               cLog::log (LOGERROR, "packet not ts %d", contentUsed);

                             contentUsed += 188;
                             }

                           return true;
                           }
                           //}}}
                        ) == 200) {
            mAudPesParser->processLastPes();
            mVidPesParser->processLastPes();
            mLoadFrac = 0.f;
            http.freeContent();
            }
          else {
            //{{{  failed to load expected available chunk, back off for 250ms
            cLog::log (LOGERROR, "late " + dec(chunkNum));
            this_thread::sleep_for (250ms);
            }
            //}}}
          }
        else // no chunk available, back off for 100ms
          this_thread::sleep_for (100ms);
        }
      mPlayer.join();
      }
    }

  cLog::log (LOGINFO, "exit");
  }
//}}}

#ifdef _WIN32
  //{{{
  void cHlsPlayer::playerThread() {
  // WSAPI player thread, video just follows play pts

    cLog::setThreadName ("play");
    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    float silence [2048*2] = { 0.f };
    float samples [2048*2] = { 0.f };

    auto device = getDefaultAudioOutputDevice();
    if (device) {
      cLog::log (LOGINFO, "device @ %d", mSong->getSampleRate());
      device->setSampleRate (mSong->getSampleRate());
      cAudioDecode decode (mSong->getFrameType());

      device->start();
      while (!mExit && !mSong->getChanged()) {
        device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
          // lambda callback - load srcSamples
          shared_lock<shared_mutex> lock (mSong->getSharedMutex());

          auto framePtr = mSong->getAudioFramePtr (mSong->getPlayFrame());
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

        if (!mStreaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
          break;
        }

      device->stop();
      }

    cLog::log (LOGINFO, "exit");
    }
  //}}}
#else
  //{{{
  void cHlsPlayer::playerThread() {
  // audio16 player thread, video just follows play pts

    cLog::setThreadName ("play");
    //SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    float samples [2048*2] = { 0 };
    float silence [2048*2] = { 0 };

    cSong::cFrame* framePtr;
    cAudio audio (2, mSong->getSampleRate(), 40000, false);
    cAudioDecode decode (mSong->getFrameType());

    while (!mExit && !mSong->getChanged()) {
      float* playSamples = silence;
        {
        // scoped song mutex
        shared_lock<shared_mutex> lock (mSong->getSharedMutex());
        framePtr = mSong->getAudioFramePtr (mSong->getPlayFrame());
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

      if (!mStreaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
        break;
      }

    cLog::log (LOGINFO, "exit");
    }
  //}}}
#endif
