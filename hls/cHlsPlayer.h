// cHlsPlayer.h - video, audio loader,player
#pragma once
//{{{  includes
// c++
#include <cstdint>
#include <string>
#include <thread>
//}}}

class cSong;
class cAudioDecode;
class cVideoDecode;

class cHlsPlayer {
public:
  cHlsPlayer();
  virtual ~cHlsPlayer();

  void init (const std::string& host, const std::string& channel, int audBitrate, int vidBitrate,
             bool useFFmpeg = true, bool videoQueue = true, bool audioQueue = true, bool streaming = true);

protected:
  void videoFollowAudio();
  void hlsThread();

  cSong* mSong;
  bool mPlaying = true;

  cVideoDecode* mVideoDecode = nullptr;
  cAudioDecode* mAudioDecode = nullptr;
  std::thread mPlayer;

  bool mExit = false;

private:
  int processVideoPes (uint8_t* pes, int size, int num, uint64_t pts);
  void dequeVideoPesThread();

  int processAudioPes (uint8_t* pes, int size, int num, uint64_t pts);
  void dequeAudioPesThread();

  void playThread();

  // vars
  std::string mHost;
  std::string mChannel;
  int mVidBitrate = 0;
  int mAudBitrate = 0;

  bool mUseFFmpeg = false;
  bool mQueueVideo = true;
  bool mQueueAudio= true;
  bool mStreaming = false;
  };
