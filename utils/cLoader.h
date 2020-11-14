// cLoader.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <thread>


class cSong;
class cSongPlayer;
class cPidParser;
class iVideoPool;
class cFileList;
class cService;
class cLoadType;
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
  //void icycast (const std::string& url);

  // actions
  void skipped();
  void stopAndWait();
  void toggleShowGraphics() { mShowGraphics = !mShowGraphics; }

  // vars
  std::string mLastTitleStr;
  cFileList* mFileList = nullptr;

private:
  //void addIcyInfo (int64_t pts, const std::string& icyInfo);

  bool mExit = false;
  bool mRunning = false;
  bool mShowGraphics = true;

  cLoadType* mLoadType = nullptr;
  std::vector <cLoadType*> mLoadTypes;
  };
