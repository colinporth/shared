// cHlsPlayer.h - video, audio - loader,player
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

#include "../utils/readerWriterQueue.h"
//}}}

class cSong;
class cPesParser;
class cAudioDecode;
class cVideoDecode;

class cHlsPlayer {
public:
  cHlsPlayer();
  virtual ~cHlsPlayer();

  void init (const std::string& host, const std::string& channel, int audBitrate, int vidBitrate,
             bool useFFmpeg = true, bool videoQueue = true, bool audioQueue = true, bool streaming = true);

  cSong* getSong() { return mSong; }
  cVideoDecode* getVideoDecode() { return mVideoDecode; }
  cAudioDecode* getAudioDecode() { return mAudioDecode; }

  float getLoadFrac() { return mLoadFrac; }
  float getVideoFrac();
  float getAudioFrac();

  std::string getChannel() { return mChannel; }
  int getVidBitrate() { return mVidBitrate; }
  int getAudBitrate() { return mAudBitrate; }

  int processVideoPes (uint8_t* pes, int size, int num, uint64_t pts);
  int processAudioPes (uint8_t* pes, int size, int num, uint64_t pts);
  void dequeVideoPesThread();
  void dequeAudioPesThread();

  //{{{
  void startPlayer() {
    if (!mPlayer.joinable())
      mPlayer = std::thread ([=](){ playerThread(); });
    }
  //}}}

protected:
  void videoFollowAudio();
  void loaderThread();

  cSong* mSong;

  cPesParser* mVideoPesParser = nullptr;
  cPesParser* mAudioPesParser = nullptr;

  cVideoDecode* mVideoDecode = nullptr;
  cAudioDecode* mAudioDecode = nullptr;

  bool mExit = false;
  bool mPlaying = true;
  std::thread mPlayer;

private:
  void playerThread();

  //{{{  private vars
  std::string mHost;
  std::string mChannel;
  int mVidBitrate = 0;
  int mAudBitrate = 0;

  bool mUseFFmpeg = false;
  bool mQueueVideo = true;
  bool mQueueAudio= true;
  bool mStreaming = false;

  float mLoadFrac = 0.f;
  //}}}
  };
