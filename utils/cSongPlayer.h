// cSongPlayer.h
#pragma once
#include <thread>
class cSong;

class cSongPlayer {
public:
  cSongPlayer (int64_t& playPts) : mPlayPts(playPts) {}

  // gets
  int64_t getPlayPts() { return mPlayPts; }

  // toggle
  void togglePlaying() { mPlaying = !mPlaying; }

  // actions
  void wait();
  void stopAndWait();
  void start (cSong* song, bool streaming);

private:
  int64_t& mPlayPts;

  bool mExit = false;
  bool mPlaying = true;

  bool mStarted = false;
  std::thread mPlayerThread;
  };
