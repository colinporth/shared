// cPesParser.h - parse Pes from tsBody packets, queue or call decode immediately
#pragma once
//{{{  includes
#include <string>
#include <thread>

#include "utils.h"
#include "cLog.h"
#include "readerWriterQueue.h"
//}}}

class cPesParser {
// extract pes from ts, queue?, decode
public:
  //{{{
  class cPesItem {
  public:
    //{{{
    cPesItem (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)
        : mAfterPlay(afterPlay), mPesSize(size), mNum(num), mPts(pts) {
      mPes = (uint8_t*)malloc (size);
      memcpy (mPes, pes, size);
      }
    //}}}
    //{{{
    ~cPesItem() {
      free (mPes);
      }
    //}}}

    bool mAfterPlay;
    uint8_t* mPes;
    const int mPesSize;
    const int mNum;
    const int64_t mPts;
    };
  //}}}
  //{{{
  cPesParser (int pid, bool useQueue, const std::string& name,
              std::function <int (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* parserPes)> process,
              std::function <void (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)> decode)
      : mPid(pid), mName(name), mProcessCallback(process), mDecodeCallback(decode), mUseQueue(useQueue) {

    mPes = (uint8_t*)malloc (kInitPesSize);

    if (useQueue)
      std::thread ([=](){ dequeThread(); }).detach();
    }
  //}}}
  virtual ~cPesParser() {}

  //{{{
  void clear (int num) {
    mNum = num;
    mPesSize = 0;
    mPts = 0;
    }
  //}}}

  int getPid() { return mPid; }
  int getQueueSize() { return mUseQueue ? (int)mQueue.size_approx() : 0; }
  float getQueueFrac() { return mUseQueue ? (float)mQueue.size_approx() / mQueue.max_capacity() : 0.f; }

  //{{{
  bool parseTs (uint8_t* ts, bool afterPlay) {

    int pid = ((ts[1] & 0x1F) << 8) | ts[2];
    if (pid == mPid) {
      bool payloadStart = ts[1] & 0x40;
      auto headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
      ts += headerSize;
      int tsLeft = 188 - headerSize;

      if (payloadStart) {
        // could check pes type as well

        // end of last pes, if any, process it
        process (afterPlay);

        if (ts[7] & 0x80)
          mPts = getPts (ts+9);

        int headerSize = 9 + ts[8];
        ts += headerSize;
        tsLeft -= headerSize;
        }

      if (mPesSize + tsLeft > mAllocSize) {
        mAllocSize *= 2;
        mPes = (uint8_t*)realloc (mPes, mAllocSize);
        cLog::log (LOGINFO, mName + " pes allocSize doubled to " + dec(mAllocSize));
        }

      memcpy (mPes + mPesSize, ts, tsLeft);
      mPesSize += tsLeft;
      return true;
      }
    else
      return false;
    }
  //}}}
  //{{{
  void process (bool afterPlay) {
    if (mPesSize) {
      mNum = mProcessCallback (afterPlay, mPes, mPesSize, mNum, mPts, this);
      mPesSize = 0;
      }
    }
  //}}}
  //{{{
  void decode (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts) {

    if (mUseQueue)
      mQueue.enqueue (new cPesParser::cPesItem (afterPlay, pes, size, num, pts));
    else
      mDecodeCallback (afterPlay, pes, size, num, pts);
    }
  //}}}

private:
  static constexpr int kInitPesSize = 4096;
  //{{{
  static int64_t getPts (const uint8_t* ts) {
  // return 33 bits of pts,dts

    if ((ts[0] & 0x01) && (ts[2] & 0x01) && (ts[4] & 0x01)) {
      // valid marker bits
      int64_t pts = ts[0] & 0x0E;
      pts = (pts << 7) | ts[1];
      pts = (pts << 8) | (ts[2] & 0xFE);
      pts = (pts << 7) | ts[3];
      pts = (pts << 7) | (ts[4] >> 1);
      return pts;
      }
    else {
      cLog::log (LOGERROR, "getPts marker bits - %02x %02x %02x %02x 0x02", ts[0], ts[1], ts[2], ts[3], ts[4]);
      return 0;
      }
    }
  //}}}
  //{{{
  void dequeThread() {

    cLog::setThreadName (mName + "Q");

    while (true) {
      cPesItem* pesItem;
      mQueue.wait_dequeue (pesItem);
      mDecodeCallback (pesItem->mAfterPlay, pesItem->mPes, pesItem->mPesSize, pesItem->mNum, pesItem->mPts);
      delete pesItem;
      }
    }
  //}}}

  // vars
  int mPid = 0;
  std::string mName;

  std::function <int (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts, cPesParser* parserPes)> mProcessCallback;
  std::function <void (bool afterPlay, uint8_t* pes, int size, int num, int64_t pts)> mDecodeCallback;

  bool mUseQueue = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;

  int mAllocSize = kInitPesSize;

  uint8_t* mPes = nullptr;
  int mPesSize = 0;
  int mNum = 0;
  int64_t mPts = 0;
  };
