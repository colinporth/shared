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
//}}}

class cLoader {
public:
  enum eFlags { eFFmpeg = 0x01, eQueueAudio = 0x02,  eQueueVideo = 0x04 };

  // gets
  cSong* getSong() { return mSong; }
  cSongPlayer* getSongPlayer() { return mSongPlayer; }
  iVideoPool* getVideoPool() { return mVideoPool; }
  void getFracs (float& loadFrac, float& audioFrac, float& videoFrac);
  void getSizes (int& loadSize, int& audioQueueSize, int& videoQueueSize);
  bool getRunning() { return mRunning; }
  bool getShowGraphics() { return mShowGraphics; }

  // loaders
  void hls (bool radio, const std::string& channelName, int audioRate, int videoRate, eFlags flags);
  void file (const std::string& filename, eFlags flags);
  void icycast (const std::string& url);

  // actions
  void skipped();
  void stopAndWait();
  void toggleShowGraphics() { mShowGraphics = !mShowGraphics; }

  // vars
  bool mExit = false;
  std::string mLastTitleStr;
  cFileList* mFileList = nullptr;

private:
  void addIcyInfo (int64_t pts, const std::string& icyInfo);

  void loadTs (uint8_t* first, int size, eFlags flags);
  void loadAudio (uint8_t* first, int size, eFlags flags);

  // vars
  cSong* mSong = nullptr;
  cSongPlayer* mSongPlayer = nullptr;
  iVideoPool* mVideoPool = nullptr;

  // transport stream
  std::map <int, cPidParser*> mPidParsers;
  std::map <int, cService*> mServices;
  int mCurSid = -1;

  bool mRunning = false;

  // for info graphics
  int mLoadSize = 0;
  float mLoadFrac = 0.f;

  bool mShowGraphics = true;
  };
