// cSongPlayer.cpp - audio,video loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cSongPlayer.h"

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// container
#include "../utils/cSong.h"

// audio device
#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

using namespace std;
using namespace chrono;
//}}}

//{{{
void cSongPlayer::start (bool streaming) {

  if (!mStarted) {
    mPlayerThread = thread ([=]() {
      // player lambda
      cLog::setThreadName ("play");

      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      #ifdef _WIN32
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        //{{{  WSAPI player thread, video just follows play pts
        auto device = getDefaultAudioOutputDevice();
        if (device) {
          cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", mSong->getSampleRate());
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

              if (framePtr) {
                mPlayPts = framePtr->getPts();
                if (mPlaying)
                  mSong->incPlayFrame (1, true);
                }
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

          if (framePtr) {
            mPlayPts = framePtr->getPts();
            if (mPlaying)
              mSong->incPlayFrame (1, true);
            }

          if (!streaming && (mSong->getPlayFrame() > mSong->getLastFrame()))
            break;
          }
        //}}}
      #endif

      cLog::log (LOGINFO, "exit");
      });

    mStarted = true;
    }
  }
//}}}
//{{{
void cSongPlayer::wait() {

  mPlayerThread.join();
  }
//}}}
//{{{
void cSongPlayer::stopAndWait() {

  mExit = true;
  if (mStarted)
    mPlayerThread.join();
  }
//}}}
