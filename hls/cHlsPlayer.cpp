// cHlsPlayer.cpp - video, audio loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

// c++
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>

#include "cHlsPlayer.h"

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// audio container
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

// net
#ifdef _WIN32
  #include "../net/cWinSockHttp.h"
#else
  #include "../net/cLinuxHttp.h"
#endif

// video decode
#include "../utils/cVideoDecode.h"

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

// public
//{{{
cHlsPlayer::~cHlsPlayer() {
  delete mSong;
  delete mVideoDecode;
  }
//}}}
//{{{
void cHlsPlayer::initPlayer (const std::string& name, bool useFFmpeg) {

  mSong = new cSong();
  if (useFFmpeg)
    mVideoDecode = new cFFmpegVideoDecode (name);
  #ifdef _WIN32
    else
      mVideoDecode = new cMfxVideoDecode (name);
  #endif
  }
//}}}

// private:
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
void cHlsPlayer::hlsThread (const string& host, const string& channel, int audBitrate, int vidBitrate) {
// hls http chunk load and decode thread

  cLog::setThreadName ("hls ");

  constexpr int kPesSize = 1000000;
  uint8_t* audPes = (uint8_t*)malloc (kPesSize);
  uint8_t* vidPes = (uint8_t*)malloc (kPesSize);

  mSong->setChannel (channel);
  mSong->setBitrate (audBitrate, audBitrate < 128000 ? 180 : 360); // audBitrate, audioFrames per chunk

  while (!mExit && !mSongChanged) {
    mSongChanged = false;
    const string path = "pool_902/live/uk/" + mSong->getChannel() +
                        "/" + mSong->getChannel() +
                        ".isml/" + mSong->getChannel() +
                        (mSong->getBitrate() < 128000 ? "-pa3=" : "-pa4=") + dec(mSong->getBitrate()) +
                        "-video=" + dec(vidBitrate);
    cPlatformHttp http;
    string redirectedHost = http.getRedirect (host, path + ".m3u8");
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
      cAudioDecode audioDecode (cAudioDecode::eAac);

      thread player;
      while (!mExit && !mSongChanged) {
        int chunkNum = mSong->getHlsLoadChunkNum (system_clock::now(), 12s, 2);
        if (chunkNum) {
          // get hls chunkNum chunk
          mSong->setHlsLoad (cSong::eHlsLoading, chunkNum);
          if (http.get (redirectedHost, path + '-' + dec(chunkNum) + ".ts", "",
                        [&] (const string& key, const string& value) noexcept { /* headerCallback lambda */ },
                        [&] (const uint8_t* data, int length) noexcept {
                          // data callback lambda
                          if (mVideoDecode)
                            mVideoDecode->setDecodeFrac (float(http.getContentSize()) / http.getHeaderContentSize());
                          return true; }
                        ) == 200) {
            // got content chunk
            mSong->setHlsLoad (cSong::eHlsIdle, chunkNum);
            int seqFrameNum = mSong->getHlsFrameFromChunkNum (chunkNum);
            if (mVideoDecode)
              mVideoDecode->setDecodeFrac (1.f);
            //{{{  parse content
            int audPesSize = 0;
            uint64_t audPesPts = 0;

            int vidPesNum = 0;
            int vidPesSize = 0;
            uint64_t vidPesPts = 0;

            // parse ts packets
            uint8_t* ts = http.getContent();
            int tsSize = http.getContentSize();
            while ((ts < ts + tsSize) && (*ts++ == 0x47)) {
              auto payStart = ts[0] & 0x40;
              auto pid = ((ts[0] & 0x1F) << 8) | ts[1];
              auto headerSize = (ts[2] & 0x20) ? 4 + ts[3] : 3;
              ts += headerSize;
              auto tsBodySize = 187 - headerSize;

              if (pid == 33) {
                //{{{  video pid
                if (payStart && !ts[0] && !ts[1] && (ts[2] == 1) && (ts[3] == 0xe0)) {
                  // new pes
                  if (vidPesSize) {
                    // process prev videoPes
                    mVideoDecode->decode (vidPes, vidPesSize, vidPesNum == 0, vidPesPts);
                    vidPesSize = 0;
                    vidPesNum++;
                    }

                  // header
                  if (ts[7] & 0x80)
                    vidPesPts = getPts (ts+9);

                  headerSize = 9 + ts[8];
                  ts += headerSize;
                  tsBodySize -= headerSize;
                  }

                // copy ts payload into Pes
                if (vidPesSize + tsBodySize > kPesSize)
                  cLog::log (LOGERROR, "Pes overflowed %d", vidPesSize + tsBodySize);
                else {
                  memcpy (vidPes + vidPesSize, ts, tsBodySize);
                  vidPesSize += tsBodySize;
                  }
                }
                //}}}
              else if (pid == 34) {
                //{{{  audio pid
                if (payStart && !ts[0] && !ts[1] && (ts[2] == 1) && (ts[3] == 0xC0)) {
                  // new pes
                  if (audPesSize) {
                    //  process prev audioPes
                    uint8_t* bufferPtr = audPes;
                    while (audioDecode.parseFrame (bufferPtr, audPes + audPesSize)) {
                      float* samples = audioDecode.decodeFrame (seqFrameNum);
                      if (samples) {
                        mSong->setFixups (audioDecode.getNumChannels(), audioDecode.getSampleRate(), audioDecode.getNumSamples());
                        mSong->addAudioFrame (seqFrameNum++, samples, true, mSong->getNumFrames(), nullptr, audPesPts);
                        audPesPts += (audioDecode.getNumSamples() * 90) / 48;
                        if (!player.joinable())
                          player = thread ([=](){ playThread (true); });  // playThread16 playThread32 playThreadWSAPI
                        }
                      bufferPtr += audioDecode.getNextFrameOffset();
                      }
                    audPesSize = 0;
                    }

                  // header
                  if (ts[7] & 0x80)
                    audPesPts = getPts (ts+9);
                  headerSize = 9 + ts[8];
                  ts += headerSize;
                  tsBodySize -= headerSize;
                  }

                // copy ts payload into Pes
                if (audPesSize + tsBodySize > kPesSize)
                  cLog::log (LOGERROR, "audPes overflowed %d", audPesSize + tsBodySize);
                else {
                  memcpy (audPes + audPesSize, ts, tsBodySize);
                  audPesSize += tsBodySize;
                  }
                }
                //}}}

              ts += tsBodySize;
              }

            if (audPesSize) {
              //{{{  process last audPes
              uint8_t* bufferPtr = audPes;
              while (audioDecode.parseFrame (bufferPtr, audPes + audPesSize)) {
                float* samples = audioDecode.decodeFrame (seqFrameNum);
                if (samples) {
                  mSong->addAudioFrame (seqFrameNum++, samples, true, mSong->getNumFrames(), nullptr, audPesPts);
                  audPesPts += (audioDecode.getNumSamples() * 90) / 48;
                  }
                bufferPtr += audioDecode.getNextFrameOffset();
                }
              }
              //}}}
            if (vidPesSize) {
              //{{{  process last vidPes
              mVideoDecode->decode (vidPes, vidPesSize, vidPesNum == 0, vidPesPts);
              vidPesNum++;
              }
              //}}}
            //}}}
            http.freeContent();
            }
          else {
            //{{{  failed to load expected available chunk, back off for 250ms
            mSong->setHlsLoad (cSong::eHlsFailed, chunkNum);
            cLog::log (LOGERROR, "late " + dec(chunkNum));
            this_thread::sleep_for (250ms);
            }
            //}}}
          }
        else // no chunk available, back off for 100ms
          this_thread::sleep_for (100ms);
        }
      player.join();
      }
    }

  free (audPes);
  free (vidPes);
  cLog::log (LOGINFO, "exit");
  }
//}}}

#ifdef _WIN32
  //{{{
  void cHlsPlayer::playThread (bool streaming) {
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
      while (!mExit && !mSongChanged) {
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

          if (mPlaying && framePtr) {
            if (mVideoDecode)
              mVideoDecode->setPlayPts (framePtr->getPts());
            mSong->incPlayFrame (1, true);
            }
          });

        if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
          break;
        }

      device->stop();
      }

    cLog::log (LOGINFO, "exit");
    }
  //}}}
#else
  //{{{
  void cHlsPlayer::playThread (bool streaming) {
  // audio16 player thread, video just follows play pts

    cLog::setThreadName ("play");
    //SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    float samples [2048*2] = { 0 };
    float silence [2048*2] = { 0 };

    cSong::cFrame* framePtr;
    cAudio audio (2, mSong->getSampleRate(), 40000, false);
    cAudioDecode decode (mSong->getFrameType());

    while (!mExit && !mSongChanged) {
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
      //cLog::log (LOGINFO, "audio.play");

      if (mPlaying && framePtr) {
        if (mVideoDecode)
          mVideoDecode->setPlayPts (framePtr->getPts());
        mSong->incPlayFrame (1, true);
        }

      if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
        break;
      }

    cLog::log (LOGINFO, "exit");
    }
  //}}}
#endif
