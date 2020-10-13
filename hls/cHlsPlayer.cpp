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

#include "../utils/cPesParser.h"

using namespace std;
using namespace chrono;
//}}}

namespace {
  //{{{
  string getTagValue (uint8_t* buffer, const char* tag) {

    const char* tagPtr = strstr ((const char*)buffer, tag);
    const char* valuePtr = tagPtr + strlen (tag);
    const char* endPtr = strchr (valuePtr, '\n');

    return string (valuePtr, endPtr - valuePtr);
    }
  //}}}
  }

// public
cHlsPlayer::cHlsPlayer() {}
//{{{
cHlsPlayer::~cHlsPlayer() {

  delete mSong;
  delete mVideoDecode;
  delete mAudioDecode;
  delete mVideoPesParser;
  delete mAudioPesParser;
  }
//}}}
//{{{
void cHlsPlayer::init (const string& host, const string& channel, int audBitrate, int vidBitrate,
                       bool useFFmpeg, bool queueVideo, bool queueAudio, bool streaming) {
  mHost = host;
  mChannel = channel;

  // audio
  mAudBitrate = audBitrate;
  mSong = new cSong();
  mAudioDecode = new cAudioDecode (cAudioDecode::eAac);

  mAudioPesParser = new cPesParser (34, queueAudio, "aud",
    [&] (uint8_t* pes, int size, int num, uint64_t pts,
         bool useQueue, readerWriterQueue::cBlockingReaderWriterQueue <cPesParser::cPesItem*>& queue) noexcept {
      //{{{  audio pes process callback lambda
      uint8_t* framePes = pes;
      while (getAudioDecode()->parseFrame (framePes, pes + size)) {
        int framePesSize = getAudioDecode()->getNextFrameOffset();

        // decode a single frame from pes
        if (useQueue)
          queue.enqueue (new cPesParser::cPesItem (framePes, framePesSize, num, pts));
        else {
          mSong->addAudioFrame (num, mAudioDecode->decodeFrame (framePes, framePesSize, num, pts), true, mSong->getNumFrames(), nullptr, pts);
          startPlayer();
          }

        // point to next frame in pes, assumes 48000 sample rate
        pts += (getAudioDecode()->getNumSamples() * 90) / 48;
        num++;

        framePes += framePesSize;
        }

      return num;
      },
      //}}}
    [&] (uint8_t* pes, int size, int num, uint64_t pts) noexcept {
      //{{{  audio pes decode callback lambda
      mSong->addAudioFrame (num, mAudioDecode->decodeFrame (pes, size, num, pts), true, mSong->getNumFrames(), nullptr, pts);
      startPlayer();
      }
      //}}}
    );

  mUseFFmpeg = useFFmpeg;
  if (useFFmpeg)
    mVideoDecode = new cFFmpegVideoDecode();
  #ifdef _WIN32
    else
      mVideoDecode = new cMfxVideoDecode();
  #endif

  // video
  mVidBitrate = vidBitrate;
  mVideoPesParser = new cPesParser (33, queueVideo, "vid",
    [&] (uint8_t* pes, int size, int num, uint64_t pts,
         bool useQueue, readerWriterQueue::cBlockingReaderWriterQueue <cPesParser::cPesItem*>& queue) noexcept {
      //{{{  video pes process callback lambda
      if (useQueue)
        queue.enqueue (new cPesParser::cPesItem(pes, size, num, pts));
      else
        mVideoDecode->decodeFrame (pes, size, num, pts);

      num++;
      return num;
      },
      //}}}
    [&] (uint8_t* pes, int size, int num, uint64_t pts) noexcept {
      //{{{  video pes decode callback lambda
      mVideoDecode->decodeFrame (pes, size, num, pts);
      }
      //}}}
    );

  mStreaming = streaming;
  }
//}}}

float cHlsPlayer::getAudioFrac() { return mVideoPesParser->getFrac(); }
float cHlsPlayer::getVideoFrac() { return mVideoPesParser->getFrac(); }

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
          int contentUsed = 0;
          mVideoPesParser->clear(0);
          int audioFrameNum = mSong->getFrameNumFromChunkNum (chunkNum);
          mAudioPesParser->clear (audioFrameNum);
          cLog::log (LOGINFO, "get chunkNum:" + dec(chunkNum) + " audioFrameNum:" + dec(audioFrameNum));
          if (http.get (redirectedHost, path + '-' + dec(chunkNum) + ".ts", "",
                        [&] (const string& key, const string& value) noexcept { /* headerCallback lambda */ },
                        [&] (const uint8_t* data, int length) noexcept {
                           //{{{  data callback lambda
                           mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();

                           while (http.getContentSize() - contentUsed >= 188) { // have a whole ts packet
                             uint8_t* ts = http.getContent() + contentUsed;
                             if (*ts++ == 0x47) { // ts packet start
                               bool payStart = ts[0] & 0x40;
                               auto pid = ((ts[0] & 0x1F) << 8) | ts[1];
                               auto headerSize = (ts[2] & 0x20) ? 4 + ts[3] : 3;

                               if (pid == mVideoPesParser->getPid())
                                 mVideoPesParser->parseTs (payStart, ts + headerSize, 187 - headerSize);
                               else if (pid == mAudioPesParser->getPid())
                                 mAudioPesParser->parseTs (payStart, ts + headerSize, 187 - headerSize);
                               else if (pid && (pid != 32))
                                 cLog::log (LOGERROR, "other pid %d", pid);

                               ts += 187;
                               }
                             else {
                               cLog::log (LOGERROR, "packet not ts %d", contentUsed);
                               return false;
                               }
                             contentUsed += 188;
                             }

                           return true;
                           }
                           //}}}
                        ) == 200) {
            mAudioPesParser->processLastPes();
            mVideoPesParser->processLastPes();
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

      // wait for playerThread to end
      mPlayer.join();
      }
    }

  cLog::log (LOGINFO, "exit");
  }
//}}}

// private
//{{{
void cHlsPlayer::startPlayer() {

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

              framePtr = mSong->getAudioFramePtr (mSong->getPlayFrame());
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
        //}}}
      #endif

      cLog::log (LOGINFO, "exit");
      });
    }
  }
//}}}
