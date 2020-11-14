// cLoader.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <thread>

class cLoad;
class cSong;
class iVideoPool;
//}}}

class cLoader {
public:
  cLoader();

  // gets
  cSong* getSong();
  iVideoPool* getVideoPool();

  void getFracs (float& loadFrac, float& audioFrac, float& videoFrac);
  void getSizes (int& loadSize, int& audioQueueSize, int& videoQueueSize);
  bool getRunning() { return mRunning; }
  bool getShowGraphics() { return mShowGraphics; }

  // loaders
  void load (const std::vector<std::string>& strings);

  // actions
  void skipped();
  void stopAndWait();
  void toggleShowGraphics() { mShowGraphics = !mShowGraphics; }

private:
  bool mRunning = false;
  bool mShowGraphics = true;

  cLoad* mLoad = nullptr;
  std::vector <cLoad*> mLoads;
  };
