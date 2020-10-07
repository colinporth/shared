// cHlsPlayer.h - video, audio loader,player
#pragma once
//{{{  includes
// c++
#include <cstdint>
#include <string>
//}}}

class cSong;
class cVideoDecode;

class cHlsPlayer {
public:
  cHlsPlayer();
  virtual ~cHlsPlayer();

  void initPlayer (const std::string& host, const std::string& channel, int audBitrate, int vidBitrate,
                  bool streaming = true, bool useFFmpeg = true);

protected:
  void videoFollowAudio();
  void hlsThread();
  void playThread();

  // vars
  std::string mHost;
  std::string mChannel;
  int mAudBitrate = 0;
  int mVidBitrate = 0;
  bool mStreaming = false;
  bool mUseFFmpeg = false;

  bool mExit = false;

  cSong* mSong;
  bool mPlaying = true;
  cVideoDecode* mVideoDecode = nullptr;
  };
