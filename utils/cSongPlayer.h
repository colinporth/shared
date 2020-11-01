// cSongPlayer.h
#pragma once
#include <thread>
class cSong;

class cSongPlayer {
public:
  ~cSongPlayer() {}

  // toggle
  void togglePlaying() { mPlaying = !mPlaying; }

  // actions
  void wait();
  void stopAndWait();
  void start (cSong* song, int64_t* playPts, bool streaming);

private:
  bool mExit = false;
  bool mPlaying = true;

  bool mStarted = false;
  std::thread mPlayerThread;
  };
