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

cSongPlayer::cSongPlayer (cSong* song, bool streaming) {

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
        cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", song->getSampleRate());
        device->setSampleRate (song->getSampleRate());
        device->start();

        cSong::cFrame* framePtr;
        while (!mExit && !song->getChanged()) {
          device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
            // lambda callback - load srcSamples
            shared_lock<shared_mutex> lock (song->getSharedMutex());

            framePtr = song->findFrame (song->getPlayFrame());
            if (mPlaying && framePtr && framePtr->getSamples()) {
              if (song->getNumChannels() == 1) {
                //{{{  mono to stereo
                auto src = framePtr->getSamples();
                auto dst = samples;
                for (int i = 0; i < song->getSamplesPerFrame(); i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                }
                //}}}
              else
                memcpy (samples, framePtr->getSamples(), song->getSamplesPerFrame() * song->getNumChannels() * sizeof(float));
              srcSamples = samples;
              }
            else
              srcSamples = silence;
            numSrcSamples = song->getSamplesPerFrame();

            if (framePtr && mPlaying)
              song->incPlayFrame (1, true);
            });

          if (!streaming && (song->getPlayFrame() > song->getLastFrame()))
            break;
          }

        device->stop();
        }
      //}}}
    #else
      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mPlayerThread.native_handle(), SCHED_RR, &sch_params);
      //{{{  audio16 player thread, video just follows play pts
      cAudio audio (2, song->getSampleRate(), 40000, false);

      cSong::cFrame* framePtr;
      while (!mExit && !song->getChanged()) {
        float* playSamples = silence;
          {
          // scoped song mutex
          shared_lock<shared_mutex> lock (song->getSharedMutex());
          framePtr = song->getFramePtr (song->getPlayFrame());
          bool gotSamples = mPlaying && framePtr && framePtr->getSamples();
          if (gotSamples) {
            memcpy (samples, framePtr->getSamples(), song->getSamplesPerFrame() * 8);
            playSamples = samples;
            }
          }
        audio.play (2, playSamples, song->getSamplesPerFrame(), 1.f);

        if (framePtr && mPlaying)
          song->incPlayFrame (1, true);

        if (!streaming && (song->getPlayFrame() > song->getLastFrame()))
          break;
        }
      //}}}
    #endif

    cLog::log (LOGINFO, "exit");
    });
  }

//{{{
void cSongPlayer::wait() {

  mPlayerThread.join();
  }
//}}}
//{{{
void cSongPlayer::stopAndWait() {

  mExit = true;
  wait();
  }
//}}}
