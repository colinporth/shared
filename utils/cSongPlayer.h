// cSongPlayer.h
#pragma once
#include <thread>
class cSong;

class cSongPlayer {
public:
  cSongPlayer (cSong* song, int64_t& playPts) : mSong(song), mPlayPts(playPts) {}

  // gets
  bool getPlaying() { return mPlaying; }
  int64_t getPlayPts() { return mPlayPts; }

  // toggle
  void togglePlaying() { mPlaying = !mPlaying; }

  // actions
  void start (bool streaming);
  void wait();
  void stopAndWait();

private:
  cSong* mSong;
  int64_t& mPlayPts;

  bool mExit = false;
  bool mPlaying = true;

  bool mStarted = false;
  std::thread mPlayerThread;
  };
