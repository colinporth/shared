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
    cFrame (uint64_t pts) : mPts(pts) {}
    virtual ~cFrame();

    void clear();

    // gets
    eState getState() { return mState; }
    uint64_t getPts() { return mPts; }
    uint32_t* getBuffer() { return mBuffer; }

    // sets
    void set (uint64_t pts);

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
    uint64_t mPts = 0;
    uint32_t* mBuffer = nullptr;
    };
  //}}}

  cVideoDecode (bool bgra) : mBgra(bgra) {}
  virtual ~cVideoDecode();

  void clear (uint64_t pts);

  // gets
  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }
  int getFramePoolSize() { return (int)mFramePool.size(); }

  // sets
  void setPlayPts (uint64_t playPts) { mPlayPts = playPts; }

  cFrame* findPlayFrame();
  virtual void decodeFrame (uint8_t* pes, unsigned int pesSize, int pesNumInChunk, uint64_t pts) = 0;

protected:
  cFrame* getFreeFrame (uint64_t pts);

  const bool mBgra;
  int mWidth = 0;
  int mHeight = 0;
  uint64_t mPlayPts = 0;
  std::vector <cFrame*> mFramePool;
  };

#ifdef _WIN32
  //{{{
  class cMfxVideoDecode : public cVideoDecode {
  public:
    cMfxVideoDecode (bool bgra);
    virtual ~cMfxVideoDecode();

    int getSurfacePoolSize() { return (int)mSurfacePool.size(); }
    void decodeFrame (uint8_t* pes, unsigned int pesSize, int pesNumInChunk, uint64_t pts);

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
  cFFmpegVideoDecode (bool bgra);
  virtual ~cFFmpegVideoDecode();

  void decodeFrame (uint8_t* pes, unsigned int pesSize, int pesNumInChunk, uint64_t pts);

private:
  // vars
  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;
  SwsContext* mSwsContext = nullptr;

  uint64_t mDecodePts = 0;
  };
//}}}
