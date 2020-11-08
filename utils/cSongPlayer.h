// cSongPlayer.h
#pragma once
#include <thread>
class cSong;

class cSongPlayer {
public:
  cSongPlayer (cSong* song, bool streaming);
  ~cSongPlayer() {}

  // toggle
  void togglePlaying() { mPlaying = !mPlaying; }

  // actions
  void wait();
  void stopAndWait();

private:
  bool mExit = false;
  bool mPlaying = true;
  std::thread mPlayerThread;
  };
