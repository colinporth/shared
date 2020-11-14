// cSongPlayer.h
#pragma once
#include <thread>
class cSong;

class cSongPlayer {
public:
  cSongPlayer (cSong* song, bool streaming);
  ~cSongPlayer() {}

  // actions
  void wait();
  void stopAndWait();

private:
  bool mExit = false;
  std::thread mPlayerThread;
  };
