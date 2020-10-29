// cVideoDecode.h
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
//}}}

class cVideoDecode {
public:
  //{{{
  class cFrame {
  public:
    enum eState { eFree, eAllocated, eLoaded };
    virtual ~cFrame();

    void clear();

    // gets
    eState getState() { return mState; }

    bool isPtsWithinFrame (int64_t pts);
    int64_t getPts() { return mPts; }
    int64_t getPtsDuration() { return mPtsDuration; }
    int64_t getPtsEnd() { return mPts + mPtsDuration; }

    int getPesSize() { return mPesSize; }
    int getNum() { return mNum; }
    uint32_t* getBuffer8888() { return mBuffer8888; }

    // sets
    void set (int64_t pts, int64_t ptsDuration, int pesSize, int num, int width, int height);
    virtual void setYuv420 (void* context, uint8_t** data, int* linesize);

  protected:
    void allocateBuffer8888 (int width, int height);

    eState mState = eFree;
    uint32_t* mBuffer8888 = nullptr;
    int mWidth = 0;
    int mHeight = 0;

  private:
    int64_t mPts = 0;
    int64_t mPtsDuration = 0;
    int mPesSize = 0;
    int mNum = 0;
    };
  //}}}

  virtual ~cVideoDecode();

  // dodgy static factory create
  static cVideoDecode* create (bool ffmpeg, int poolSize);

  void clear (int64_t pts);

  // gets
  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }

  int getFramePoolSize() { return (int)mFramePool.size(); }
  std::vector <cFrame*>& getFramePool() { return mFramePool; }

  // sets
  void setPlayPts (int64_t playPts) { mPlayPts = playPts; }

  cFrame* findPlayFrame();
  virtual void decodeFrame (bool afterPlay, uint8_t* pes, unsigned int pesSize, int num, int64_t pts) = 0;

protected:
  cVideoDecode (bool planar, int poolSize);

  cFrame* getFreeFrame (bool afterPlay, int64_t pts);

  int mWidth = 0;
  int mHeight = 0;
  int64_t mPlayPts = 0;
  int64_t mPtsDuration = 0;

  std::vector <cFrame*> mFramePool;
  };
