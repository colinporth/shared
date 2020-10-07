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

  void initPlayer (const std::string& name, bool useFFmpeg = true);

protected:
  void videoFollowAudio();
  void hlsThread (const std::string& host, const std::string& channel, int audBitrate, int vidBitrate);

  void playThread (bool streaming);

  // vars
  bool mExit = false;

  cSong* mSong;
  bool mPlaying = true;
  cVideoDecode* mVideoDecode = nullptr;
  };
