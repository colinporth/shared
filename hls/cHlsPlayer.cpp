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

  // !!!!! should clear down mPesParsers !!!!
  }
//}}}
//{{{
void cHlsPlayer::init (const string& hostName, const string& channelName, int audBitrate, int vidBitrate,
                       bool useFFmpeg, bool queueVideo, bool queueAudio, bool streaming) {
  mHostName = hostName;
  mChannelName = channelName;

  // audio
  mAudBitrate = audBitrate;
  mSong = new cSong();
  mAudioDecode = new cAudioDecode (cAudioDecode::eAac);

  // video
  mVidBitrate = vidBitrate;
  mUseFFmpeg = useFFmpeg;
  if (useFFmpeg)
    mVideoDecode = new cFFmpegVideoDecode();
  #ifdef _WIN32
    else
      mVideoDecode = new cMfxVideoDecode();
  #endif

  // audio pesParser
  mPesParsers.push_back (
    new cPesParser (0x22, queueAudio, "aud",
      [&](uint8_t* pes, int size, int sequenceNum, uint64_t pts, cPesParser* pesParser) noexcept {
        //{{{  audio pes process callback lambda
        uint8_t* framePes = pes;
        while (getAudioDecode()->parseFrame (framePes, pes + size)) {
          // decode a single frame from pes
          int framePesSize = getAudioDecode()->getNextFrameOffset();
          pesParser->decode (framePes, framePesSize, sequenceNum, pts);

          // pts of next frame in pes, assumes 48000 sample rate
          pts += (getAudioDecode()->getNumSamples() * 90) / 48;
          sequenceNum++;

          // point to next frame in pes
          framePes += framePesSize;
          }

        return sequenceNum;
        },
        //}}}
      [&](uint8_t* pes, int size, int sequenceNum, uint64_t pts) noexcept {
        //{{{  audio pes decode callback lambda
        mSong->addFrame (sequenceNum,
                         mAudioDecode->decodeFrame (pes, size, sequenceNum, pts),
                         true, mSong->getNumFrames(), nullptr, pts);
        startPlayer();
        }
        //}}}
      )
    );

  // video pesParser
  mPesParsers.push_back (
    new cPesParser (0x21, queueVideo, "vid",
      [&](uint8_t* pes, int size, int sequenceNum, uint64_t pts, cPesParser* pesParser) noexcept {
        //{{{  video pes process callback lambda
        pesParser->decode (pes, size, sequenceNum, pts);
        sequenceNum++;
        return sequenceNum;
        },
        //}}}
      [&](uint8_t* pes, int size, int sequenceNum, uint64_t pts) noexcept {
        //{{{  video pes decode callback lambda
        mVideoDecode->decodeFrame (pes, size, sequenceNum, pts);
        }
        //}}}
      )
    );

  //{{{  parse PAT
  //mPesParsers.push_back (
    //new cPesParser (0x00, false, "pat",
      //[&] (uint8_t* pes, int size, int num, uint64_t pts, cPesParser* pesParser) noexcept {
        //{{{  pat callback
        //string info;
        //for (int i = 0; i < size; i++) {
          //int value = pes[i];
          //info += hex (value, 2) + " ";
          //}
        //cLog::log (LOGINFO, "PAT process " + dec (size) + ":" + info);
        //return num;
        //},
        //}}}
      //[&] (uint8_t* pes, int size, int num, uint64_t pts) noexcept {}
      //)
    //);
  //}}}
  //{{{  parse pgm 0x20
  //mPesParsers.push_back (
    //new cPesParser (0x20, false, "pgm",
      //[&] (uint8_t* pes, int size, int num, uint64_t pts, cPesParser* pesParser) noexcept {
        //{{{  pgm callback
        //string info;
        //for (int i = 0; i < size; i++) {
          //int value = pes[i];
          //info += hex (value, 2) + " ";
          //}
        //cLog::log (LOGINFO, "pgm process " + dec (size) + ":" + info);
        //return num;
        //},
        //}}}
      //[&] (uint8_t* pes, int size, int num, uint64_t pts) noexcept {}
      //)
    //);
  //}}}

  mStreaming = streaming;
  }
//}}}

//{{{
bool cHlsPlayer::getFrac (float& loadFrac, float& videoFrac, float& audioFrac) {
// return fracs for spinner graphic, true if ok to display

  loadFrac = mLoadFrac;
  audioFrac = mPesParsers.size() > 0 ? mPesParsers[0]->getFrac() : 0.f;
  videoFrac = mPesParsers.size() > 1 ? mPesParsers[1]->getFrac() : 0.f;

  return (loadFrac > 0.f) && ((loadFrac < 1.f) || (audioFrac > 0.f) || (videoFrac > 0.f));
  }
//}}}

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

  mSong->setChannel (mChannelName);
  mSong->setBitrate (mAudBitrate, mAudBitrate < 128000 ? 180 : 360);
  mSong->init (cAudioDecode::eAac, 2, 48000, mSong->getBitrate() < 128000 ? 2048 : 1024, 1000);

  while (!mExit && !mSong->getChanged()) {
    mSong->setChanged (false);
    const string pathName = "pool_902/live/uk/" + mSong->getChannel() +
                            "/" + mSong->getChannel() +
                            ".isml/" + mSong->getChannel() +
                            (mSong->getBitrate() < 128000 ? "-pa3=" : "-pa4=") + dec(mSong->getBitrate()) +
                            "-video=" + dec(mVidBitrate);
    cPlatformHttp http;
    string redirectedHostName = http.getRedirect (mHostName, pathName + ".m3u8");
    if (http.getContent()) {
      //{{{  parse m3u8 for mediaSequence,programDateTimePoint
      int extXMediaSequence = stoi (getTagValue (http.getContent(), "#EXT-X-MEDIA-SEQUENCE:"));
      istringstream inputStream (getTagValue (http.getContent(), "#EXT-X-PROGRAM-DATE-TIME:"));

      system_clock::time_point extXProgramDateTimePoint;
      inputStream >> date::parse ("%FT%T", extXProgramDateTimePoint);

      http.freeContent();

      mSong->setHlsBase (extXMediaSequence, extXProgramDateTimePoint, -37s, (2*60*60) - 25);
      //}}}
      while (!mExit && !mSong->getChanged()) {
        int chunkNum;
        int frameNum;
        if (mSong->loadChunk (system_clock::now(), 12s, 2, chunkNum, frameNum)) {
          cLog::log (LOGINFO, "get chunkNum:" + dec(chunkNum) + " frameNum:" + dec(frameNum));
          mPesParsers[0]->clear (frameNum);
          mPesParsers[1]->clear (0);
          int contentParsed = 0;
          if (http.get (redirectedHostName, pathName + '-' + dec(chunkNum) + ".ts", "",
                        [&] (const string& key, const string& value) noexcept {}, // header lambda
                        [&] (const uint8_t* data, int length) noexcept {
                           //{{{  data lambda
                           mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();
                           while (http.getContentSize() - contentParsed >= 188) {
                             uint8_t* ts = http.getContent() + contentParsed;
                             if (*ts == 0x47) {
                               for (auto pesParser : mPesParsers)
                                 if (pesParser->parseTs (ts))
                                   break;
                               ts += 188;
                               }
                             else {
                               cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                               return false;
                               }
                             contentParsed += 188;
                             }
                           return true;
                           }
                           //}}}
                        ) == 200) {
            for (auto pesParser : mPesParsers)
              pesParser->process();
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
