// cLoaderPlayer.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <thread>

class cSong;
class cPidParser;
class iVideoDecoder;
class cFileList;
//}}}

enum eLoaderFlags { eFFmpeg = 0x01, eQueueAudio = 0x02,  eQueueVideo = 0x04 };
class cLoaderPlayer {
public:
  cLoaderPlayer();
  virtual ~cLoaderPlayer();

  int64_t getPlayPts() { return mPlayPts; }
  cSong* getSong() { return mSong; }
  iVideoDecoder* getVideoDecoder() { return mVideoDecoder; }

  void getFrac (float& loadFrac, float& videoFrac, float& audioFrac);
  void getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize);

  void videoFollowAudio();

  void hlsLoaderThread (bool radio, const std::string& channelName, int
                        audioBitrate, int videoBitrate, eLoaderFlags loaderFlags);
  void icyLoaderThread (const std::string& url);
  void fileLoaderThread (const std::string& filename, eLoaderFlags loaderFlags);

  void startPlayer (bool streaming);

  // vars
  bool mExit = false;
  bool mRunning = false;
  bool mPlaying = true;
  std::string mLastTitleStr;
  cFileList* mFileList = nullptr;

private:
  void clear();
  std::string getHlsPathName (bool radio, int vidBitrate);
  static std::string getTagValue (uint8_t* buffer, const char* tag);
  void addIcyInfo (int frame, const std::string& icyInfo);

  // vars
  std::map <int, cPidParser*> mPidParsers;

  cSong* mSong = nullptr;
  std::thread mPlayer;
  int64_t mPlayPts;

  // only really global for info graphics
  // !!! could be owned or refrenced by cSong or passed to player directly !!!
  iVideoDecoder* mVideoDecoder = nullptr;

  // for info graphics
  int mLoadSize = 0;
  float mLoadFrac = 0.f;
  int mAudioPid = -1;
  int mVideoPid = -1;
  };
