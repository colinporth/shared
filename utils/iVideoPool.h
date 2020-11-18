// iVideoPool.h - pool of decoded iVideoFrame
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>
#include <map>

class cSong;
//}}}

// iVideoFrame
class iVideoFrame {
public:
  virtual ~iVideoFrame() {}

  // gets
  virtual bool isFree() = 0;
  virtual int64_t getPts() = 0;
  virtual int getPesSize() = 0;
  virtual char getFrameType() = 0;
  virtual uint32_t* getBuffer8888() = 0;

  virtual void setFree (bool free, int64_t pts) = 0;

  // sets
  virtual void set (int64_t pts, int pesSize, int width, int height, char frameType) = 0;
  virtual void setYuv420 (void* context, uint8_t** data, int* linesize) = 0;
  };

// iVideoPool
class iVideoPool {
public:
  static iVideoPool* create (bool ffmpeg, int maxPoolSize, cSong* song);
  virtual ~iVideoPool() {}

  // gets
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;
  virtual std::string getInfoString() = 0;
  virtual std::map <int64_t,iVideoFrame*>& getFramePool() = 0;

  //
  virtual void flush (int64_t pts) = 0;

  // actions
  virtual iVideoFrame* findFrame (int64_t pts) = 0;
  virtual void decodeFrame (bool afterPlay, uint8_t* pes, unsigned int pesSize, int64_t pts, int64_t dts) = 0;
  };
