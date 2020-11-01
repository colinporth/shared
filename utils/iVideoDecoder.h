// iVideoDecoder.h
#pragma once
//{{{  includes
#include <cstdint>
#include <vector>
#include <map>
//}}}

// iVideoFrame
class iVideoFrame {
public:
  virtual ~iVideoFrame() {}

  // gets
  virtual int64_t getPts() = 0;
  virtual int64_t getPtsDuration() = 0;
  virtual int64_t getPtsEnd() = 0;
  virtual bool isPtsWithinFrame (int64_t pts) = 0;

  virtual int getPesSize() = 0;
  virtual char getFrameType() = 0;
  virtual uint32_t* getBuffer8888() = 0;

  // sets
  virtual void set (int64_t pts, int64_t ptsDuration, int pesSize, int width, int height, char frameType) = 0;
  virtual void setYuv420 (void* context, uint8_t** data, int* linesize) = 0;
  };

// iVideoDecoder
class iVideoDecoder {
public:
  static iVideoDecoder* create (bool ffmpeg, int poolSize, int64_t& playPts);
  virtual ~iVideoDecoder() {}

  // gets
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;
  virtual std::map <int64_t,iVideoFrame*>& getFramePool() = 0;

  // actions
  virtual iVideoFrame* findFrame (int64_t pts) = 0;
  virtual void decodeFrame (bool afterPlay, uint8_t* pes, unsigned int pesSize, int64_t pts) = 0;
  };
