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
class cAudioDecode;
class cVideoDecode;

class cHlsPlayer {
public:
  cHlsPlayer();
  virtual ~cHlsPlayer();

  void init (const std::string& host, const std::string& channel, int audBitrate, int vidBitrate,
             bool useFFmpeg = true, bool videoQueue = true, bool audioQueue = true, bool streaming = true);

  cVideoDecode* getVideoDecode() { return mVideoDecode; }

  float getLoadFrac() { return mLoadFrac; }
  //{{{
  float getVideoFrac() {
    return (float)vidPesQueue.size_approx() / vidPesQueue.max_capacity();
    }
  //}}}
  //{{{
  float getAudioFrac() {
    return (float)audPesQueue.size_approx() / audPesQueue.max_capacity();
    }
  //}}}
  std::string getChannel() { return mChannel; }
  int getVidBitrate() { return mVidBitrate; }
  int getAudBitrate() { return mAudBitrate; }

protected:
  void videoFollowAudio();
  void loaderThread();

  cSong* mSong;
  bool mPlaying = true;

  cVideoDecode* mVideoDecode = nullptr;
  cAudioDecode* mAudioDecode = nullptr;

  //{{{
  class cPes {
  public:
    //{{{
    cPes (uint8_t* pes, int size, int num, uint64_t pts) : mSize(size), mNum(num), mPts(pts) {
      mPes = (uint8_t*)malloc (size);
      memcpy (mPes, pes, size);
      }
    //}}}
    //{{{
    ~cPes() {
      free (mPes);
      }
    //}}}

    uint8_t* mPes;
    const int mSize;
    const int mNum;
    const uint64_t mPts;
    };
  //}}}
  readerWriterQueue::cBlockingReaderWriterQueue <cPes*> vidPesQueue;
  readerWriterQueue::cBlockingReaderWriterQueue <cPes*> audPesQueue;

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

  float mLoadFrac = 0.f;
  };
