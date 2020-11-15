// cLoader.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class cLoadSource;
class cSong;
class iVideoPool;
//}}}

class cLoader {
public:
  cLoader();
  ~cLoader();

  // gets
  cSong* getSong();
  iVideoPool* getVideoPool();

  void getFracs (float& loadFrac, float& audioFrac, float& videoFrac);
  void getSizes (int& loadSize, int& audioQueueSize, int& videoQueueSize);

  bool getRunning() { return mRunning; }

  // loaders
  void load (const std::vector<std::string>& strings);

  // actions
  bool togglePlaying();
  bool skipBegin();
  bool skipEnd();
  bool skipBack (bool shift, bool control);
  bool skipForward (bool shift, bool control);

  void stopAndWait();

private:
  bool mRunning = false;

  cLoadSource* mLoadSource = nullptr;
  std::vector <cLoadSource*> mLoadSources;
  };
