// cVideoDecode.h
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

#ifdef _WIN32
  #include "../../libmfx/include/mfxvideo++.h"
  //class mfxFrameSurface1;
#endif

struct AVCodecParserContext;
struct AVCodec;
struct AVCodecContext;
struct SwsContext;
//}}}

class cVideoDecode {
public:
  //{{{
  class cFrame {
  public:
    enum eState { eFree, eAllocated, eLoaded };
    //{{{
    cFrame (int64_t pts, int64_t ptsDuration, int pesSize, int num)
      : mPts(pts), mPtsDuration(ptsDuration), mPesSize(pesSize), mNum(num) {}
    //}}}
    virtual ~cFrame();

    void clear();

    // gets
    eState getState() { return mState; }
    int64_t getPts() { return mPts; }
    int64_t getPtsDuration() { return mPtsDuration; }
    int64_t getPtsEnd() { return mPts + mPtsDuration; }
    int getPesSize() { return mPesSize; }
    int getNum() { return mNum; }
    uint32_t* getBuffer() { return mBuffer; }
    bool isPtsWithinFrame (int64_t pts);

    // sets
    void set (int64_t pts, int64_t ptsDuration, int pesSize, int num);

    void setYuv420RgbaInterleaved (int width, int height, uint8_t* buffer, int stride);
    void setYuv420BgraInterleaved (int width, int height, uint8_t* buffer, int stride);
    void setYuv420RgbaPlanar (int width, int height, uint8_t** data, int* linesize);
    void setYuv420BgraPlanar (int width, int height, uint8_t** data, int* linesize);
    void setYuv420RgbaPlanarSws (SwsContext* swsContext, int width, int height, uint8_t** data, int* linesize);
    void setYuv420RgbaPlanarTable (int width, int height, uint8_t** data, int* linesize);
    void setYuv420RgbaPlanarSimple (int width, int height, uint8_t** data, int* linesize);

  private:
    void allocateBuffer (int width, int height);

    eState mState = eFree;
    int64_t mPts = 0;
    int64_t mPtsDuration = 0;
    int mPesSize = 0;
    int mNum = 0;
    uint32_t* mBuffer = nullptr;
    };
  //}}}

  cVideoDecode (bool bgra, int poolSize) : mBgra(bgra), mPoolSize(poolSize) {}
  virtual ~cVideoDecode();

  void clear (int64_t pts);

  // gets
  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }
  int getFramePoolSize() { return (int)mFramePool.size(); }
  std::vector <cFrame*>& getFramePool() { return mFramePool; }

  // sets
  void setPlayPts (int64_t playPts) { mPlayPts = playPts; }

  cFrame* findPlayFrame();
  virtual void decodeFrame (uint8_t* pes, unsigned int pesSize, int num, int64_t pts) = 0;

protected:
  cFrame* getFreeFrame (int64_t pts, int pesSize, int num);

  const bool mBgra;
  const int mPoolSize;

  int mWidth = 0;
  int mHeight = 0;
  int64_t mPlayPts = 0;
  int64_t mPtsDuration = 0;

  std::vector <cFrame*> mFramePool;
  };

#ifdef _WIN32
  //{{{
  class cMfxVideoDecode : public cVideoDecode {
  public:
    cMfxVideoDecode (bool bgra, int poolSize);
    virtual ~cMfxVideoDecode();

    int getSurfacePoolSize() { return (int)mSurfacePool.size(); }
    void decodeFrame (uint8_t* pes, unsigned int pesSize, int num, int64_t pts);

  private:
    mfxFrameSurface1* getFreeSurface();

    // vars
    MFXVideoSession mSession;
    mfxVideoParam mVideoParams;
    mfxBitstream mBitstream;
    std::vector <mfxFrameSurface1*> mSurfacePool;
    };
  //}}}
#endif

//{{{
class cFFmpegVideoDecode : public cVideoDecode {
public:
  cFFmpegVideoDecode (bool bgra, int poolSize);
  virtual ~cFFmpegVideoDecode();

  void decodeFrame (uint8_t* pes, unsigned int pesSize, int num, int64_t pts);

private:
  // vars
  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;
  SwsContext* mSwsContext = nullptr;

  int64_t mDecodePts = 0;
  };
//}}}
