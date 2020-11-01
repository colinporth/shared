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
class iVideoDecoder;
class cFileList;
//}}}

enum eLoaderFlags { eFFmpeg = 0x01, eQueueAudio = 0x02,  eQueueVideo = 0x04 };
class cLoader {
public:
  cLoader();
  virtual ~cLoader();

  cSong* getSong() { return mSong; }
  cSongPlayer* getPlayer() { return mSongPlayer; }
  iVideoDecoder* getVideoDecoder() { return mVideoDecoder; }
  int64_t getPlayerPts() { return mPlayPts; }
  bool getRunning() { return mRunning; }

  void getFrac (float& loadFrac, float& videoFrac, float& audioFrac);
  void getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize);

  void hls (bool radio, const std::string& channelName, int audioBitrate, int videoBitrate, eLoaderFlags loaderFlags);
  void file (const std::string& filename, eLoaderFlags loaderFlags);
  void icy (const std::string& url);

  void skipped();
  void stopAndWait();

  // vars
  bool mExit = false;
  std::string mLastTitleStr;
  cFileList* mFileList = nullptr;

private:
  void clear();
  std::string getHlsPathName (bool radio, int vidBitrate);
  static std::string getTagValue (uint8_t* buffer, const char* tag);
  void addIcyInfo (int frame, const std::string& icyInfo);

  // vars
  cSong* mSong = nullptr;

  int64_t mPlayPts;
  cSongPlayer* mSongPlayer = nullptr;

  iVideoDecoder* mVideoDecoder = nullptr;
  bool mRunning = false;

  int mAudioPid = -1;
  int mVideoPid = -1;
  std::map <int, cPidParser*> mPidParsers;

  // for info graphics
  int mLoadSize = 0;
  float mLoadFrac = 0.f;
  };