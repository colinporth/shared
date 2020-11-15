// cSongPlayer.h
#pragma once
class cSong;

class cSongPlayer {
public:
  cSongPlayer (cSong* song, bool streaming);
  ~cSongPlayer() {}

  void exit() { mExit = true; }
  void wait();

private:
  bool mExit = false;
  bool mRunning = true;
  };
