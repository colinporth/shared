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

class cLoaderPlayer {
public:
  cLoaderPlayer();
  virtual ~cLoaderPlayer();

  void initialise (bool radio, const std::string& hostName, const std::string& channelName,
                   int audBitrate, int vidBitrate,
                   bool useFFmpeg, bool videoQueue, bool audioQueue, bool streaming);

  cSong* getSong() { return mSong; }
  cVideoDecode* getVideoDecode() { return mVideoDecode; }
  cAudioDecode* getAudioDecode() { return mAudioDecode; }

  std::string getChannelName() { return mChannelName; }
  int getVidBitrate() { return mVidBitrate; }
  int getAudBitrate() { return mAudBitrate; }
  bool getFrac (float& loadFrac, float& videoFrac, float& audioFrac);

  void videoFollowAudio();
  void loaderThread();
  void icyThread (const std::string& url);
  void fileThread();

  cSong* mSong;
  bool mPlaying = true;
  bool mExit = false;

  std::string mUrl;
  std::string mLastTitleStr;

  cFileList* mFileList = nullptr;

private:
  void addIcyInfo (int frame, const std::string& icyInfo);
  void startPlayer();

  std::string mHostName;
  std::string mChannelName;
  std::string mPoolName;
  bool mRadio = false;

  int mVidBitrate = 0;
  int mAudBitrate = 0;

  bool mUseFFmpeg = false;
  bool mStreaming = false;

  float mLoadFrac = 0.f;

  std::vector<cPesParser*> mPesParsers;
  cAudioDecode* mAudioDecode = nullptr;
  cVideoDecode* mVideoDecode = nullptr;

  std::thread mPlayer;
  };
