// cSongPlayer.h
#pragma once
class cSong;

class cSongPlayer {
public:
  cSongPlayer (cSong* song, bool streaming);
  ~cSongPlayer() {}

  bool getRunning() { return mRunning; }
  void stop() { mExit = true; }

private:
  bool mExit = false;
  bool mRunning = true;
  };
