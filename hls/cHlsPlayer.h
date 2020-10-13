// cHlsPlayer.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
//}}}

class cSong;
class cPesParser;
class cAudioDecode;
class cVideoDecode;

class cHlsPlayer {
public:
  cHlsPlayer();
  virtual ~cHlsPlayer();

  void init (const std::string& hostName, const std::string& channelName, int audBitrate, int vidBitrate,
             bool useFFmpeg = true, bool videoQueue = true, bool audioQueue = true, bool streaming = true);

  cSong* getSong() { return mSong; }
  cVideoDecode* getVideoDecode() { return mVideoDecode; }
  cAudioDecode* getAudioDecode() { return mAudioDecode; }

  float getLoadFrac() { return mLoadFrac; }
  float getVideoFrac();
  float getAudioFrac();

  std::string getChannelName() { return mChannelName; }
  int getVidBitrate() { return mVidBitrate; }
  int getAudBitrate() { return mAudBitrate; }

  void videoFollowAudio();
  void loaderThread();

  cSong* mSong;
  int mChunkNum = 0;
  bool mPlaying = true;
  bool mExit = false;

private:
  void startPlayer();

  std::string mHostName;
  std::string mChannelName;
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
