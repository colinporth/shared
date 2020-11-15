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

  // load
  void stopAndWait();
  void load (const std::vector<std::string>& params);

  // actions
  bool togglePlaying();
  bool skipBegin();
  bool skipEnd();
  bool skipBack (bool shift, bool control);
  bool skipForward (bool shift, bool control);


private:
  cLoadSource* mLoadSource = nullptr;
  std::vector <cLoadSource*> mLoadSources;
  };
