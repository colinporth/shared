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
//}}}

class cLoader {
public:
  enum eFlags { eFFmpeg = 0x01, eQueueAudio = 0x02,  eQueueVideo = 0x04 };

  // gets
  cSong* getSong() { return mSong; }
  cSongPlayer* getSongPlayer() { return mSongPlayer; }
  int64_t getPlayerPts() { return mPlayPts; }
  iVideoPool* getVideoPool() { return mVideoPool; }
  void getFracs (float& loadFrac, float& audioFrac, float& videoFrac);
  void getSizes (int& loadSize, int& audioQueueSize, int& videoQueueSize);
  bool getRunning() { return mRunning; }

  // loaders
  void hls (bool radio, const std::string& channelName, int audioBitrate, int videoBitrate, eFlags loaderFlags);
  void file (const std::string& filename, eFlags loaderFlags);
  void icycast (const std::string& url);

  // actions
  void skipped();
  void stopAndWait();

  // vars
  bool mExit = false;
  std::string mLastTitleStr;
  cFileList* mFileList = nullptr;

private:
  void clear();
  std::string getHlsPathName (bool radio, int vidBitrate);
  static std::string getTagValue (uint8_t* buffer, const char* tag, char terminator);
  void addIcyInfo (int frame, const std::string& icyInfo);

  // vars
  cSong* mSong = nullptr;
  cSongPlayer* mSongPlayer = nullptr;
  int64_t mPlayPts = -1;

  iVideoPool* mVideoPool = nullptr;
  int mAudioPid = -1;
  int mVideoPid = -1;
  std::map <int, cPidParser*> mPidParsers;

  bool mRunning = false;

  // for info graphics
  int mLoadSize = 0;
  float mLoadFrac = 0.f;
  };
