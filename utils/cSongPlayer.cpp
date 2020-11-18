// cSongPlayer.cpp - audio,video loader,player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cSongPlayer.h"

#include <thread>

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// container
#include "../utils/cSong.h"

using namespace std;
using namespace chrono;
//}}}

#ifdef _WIN32
  cSongPlayer::cSongPlayer (cSong* song, bool streaming) {
    thread playerThread = thread ([=]() {
      // player lambda
      cLog::setThreadName ("play");
      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      song->togglePlaying();
      //{{{  WSAPI player thread, video just follows play pts
      auto device = getDefaultAudioOutputDevice();
      if (device) {
        cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", song->getSampleRate());
        device->setSampleRate (song->getSampleRate());
        device->start();

        cSong::cFrame* frame;
        while (!mExit) {
          device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
            // loadSrcSamples callback lambda
            shared_lock<shared_mutex> lock (song->getSharedMutex());

            frame = song->findPlayFrame();
            if (song->getPlaying() && frame && frame->getSamples()) {
              if (song->getNumChannels() == 1) {
                //{{{  mono to stereo
                auto src = frame->getSamples();
                auto dst = samples;
                for (int i = 0; i < song->getSamplesPerFrame(); i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                }
                //}}}
              else
                memcpy (samples, frame->getSamples(), song->getSamplesPerFrame() * song->getNumChannels() * sizeof(float));
              srcSamples = samples;
              }
            else
              srcSamples = silence;
            numSrcSamples = song->getSamplesPerFrame();

            if (frame && song->getPlaying())
              song->nextPlayFrame (true);
            });

          if (!streaming && song->getPlayFinished())
            break;
          }

        device->stop();
        }
      //}}}
      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });
    playerThread.detach();
    }
#else
  cSongPlayer::cSongPlayer (cSong* song, bool streaming) {
    thread playerThread = thread ([=]() {
      // player lambda
      cLog::setThreadName ("play");
      float silence [2048*2] = { 0.f };
      float samples [2048*2] = { 0.f };

      song->togglePlaying();
      //{{{  audio16 player thread, video just follows play pts
      cAudio audio (2, song->getSampleRate(), 40000, false);

      cSong::cFrame* frame;
      while (!mExit) {
        float* playSamples = silence;
          {
          // scoped song mutex
          shared_lock<shared_mutex> lock (song->getSharedMutex());
          frame = song->findPlayFrame();
          bool gotSamples = song->getPlaying() && frame && frame->getSamples();
          if (gotSamples) {
            memcpy (samples, frame->getSamples(), song->getSamplesPerFrame() * 8);
            playSamples = samples;
            }
          }
        audio.play (2, playSamples, song->getSamplesPerFrame(), 1.f);

        if (frame && song->getPlaying())
          song->nextPlayFrame (true);

        if (!streaming && (song->getPlayPts() > song->getLastPts()))
          break;
        }
      //}}}
      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    // raise to max prioritu
    sched_param sch_params;
    sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
    pthread_setschedparam (playerThread.native_handle(), SCHED_RR, &sch_params);
    playerThread.detach();
    }
#endif

void cSongPlayer::wait() {
  while (mRunning)
    this_thread::sleep_for (100ms);
  }
