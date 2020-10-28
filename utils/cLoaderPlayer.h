// cLoaderPlayer.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <thread>

class cSong;
class cPesParser;
class cAudioDecode;
class cVideoDecode;
class cFileList;
//}}}

enum eLoaderFlags { eFFmpeg = 0x01, eBgra = 0x02, eQueueAudio = 0x04,  eQueueVideo = 0x08 };
class cLoaderPlayer {
public:
  cLoaderPlayer();
  virtual ~cLoaderPlayer();

  cSong* getSong() { return mSong; }
  cVideoDecode* getVideoDecode() { return mVideoDecode; }
  cAudioDecode* getAudioDecode() { return mAudioDecode; }

  void getFrac (float& loadFrac, float& videoFrac, float& audioFrac);
  void getSizes (int& loadSize, int& videoQueueSize, int& audioQueueSize);

  void videoFollowAudio();

  void hlsLoaderThread (bool radio, const std::string& channelName,
                        int audBitrate, int vidBitrate, eLoaderFlags loaderFlags);
  void icyLoaderThread (const std::string& url);
  void fileLoaderThread (const std::string& filename);

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
  void startPlayer (bool streaming);

  // vars
  cSong* mSong = nullptr;

  std::vector<cPesParser*> mPesParsers;
  cAudioDecode* mAudioDecode = nullptr;
  cVideoDecode* mVideoDecode = nullptr;

  std::thread mPlayer;

  int mLoadSize = 0;
  float mLoadFrac = 0.f;
  };
