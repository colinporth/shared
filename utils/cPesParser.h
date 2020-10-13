// cPesparser.h
#pragma once
//{{{  includes
#include <string>
#include <thread>

#include "utils.h"
#include "cLog.h"
#include "readerWriterQueue.h"
//}}}

class cPesParser {
// extract pes from ts, possible queue, decode
public:
  //{{{
  class cPesItem {
  public:
    //{{{
    cPesItem (uint8_t* pes, int size, int num, uint64_t pts) : mPesSize(size), mNum(num), mPts(pts) {
      mPes = (uint8_t*)malloc (size);
      memcpy (mPes, pes, size);
      }
    //}}}
    //{{{
    ~cPesItem() {
      free (mPes);
      }
    //}}}

    uint8_t* mPes;
    const int mPesSize;
    const int mNum;
    const uint64_t mPts;
    };
  //}}}
  //{{{
  cPesParser (int pid, bool useQueue, const std::string& name,
              std::function<int (uint8_t* pes, int size, int num, uint64_t pts, bool useQueue,
                             readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*>& mQueue)> process,
              std::function<void (uint8_t* pes, int size, int num, uint64_t pts)> decode)
      : mPid(pid), mName(name), mProcess(process), mDecode(decode), mUseQueue(useQueue) {

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
  float getFrac() { return mUseQueue ? (float)mQueue.size_approx() / mQueue.max_capacity() : 0.f; }
  //{{{
  void parseTs (bool payStart, uint8_t* ts, int tsBodySize) {

    // could check pes type ts[0,1,2,3] as well
    if (payStart) {
      processLastPes();

      if (ts[7] & 0x80)
        mPts = getPts (ts+9);

      int headerSize = 9 + ts[8];
      ts += headerSize;
      tsBodySize -= headerSize;
      }

    if (mPesSize + tsBodySize > mAllocSize) {
      mAllocSize *= 2;
      mPes = (uint8_t*)realloc (mPes, mAllocSize);
      cLog::log (LOGINFO, mName + " pes allocSize doubled to " + dec(mAllocSize));
      }

    memcpy (mPes + mPesSize, ts, tsBodySize);
    mPesSize += tsBodySize;
    }
  //}}}

  //{{{
  void processLastPes() {
    if (mPesSize) {
      mNum = mProcess (mPes, mPesSize, mNum, mPts, mUseQueue, mQueue);
      mPesSize = 0;
      }
    }
  //}}}

private:
  static constexpr int kInitPesSize = 4096;

  //{{{
  static uint64_t getPts (const uint8_t* ts) {
  // return 33 bits of pts,dts

    if ((ts[0] & 0x01) && (ts[2] & 0x01) && (ts[4] & 0x01)) {
      // valid marker bits
      uint64_t pts = ts[0] & 0x0E;
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
      cPesItem* queueItem;
      mQueue.wait_dequeue (queueItem);
      mDecode (queueItem->mPes, queueItem->mPesSize, queueItem->mNum, queueItem->mPts);
      delete queueItem;
      }
    }
  //}}}

  // vars
  int mPid = 0;
  std::string mName;

  std::function<int (uint8_t* pes, int size, int num, uint64_t pts, bool useQueue,
                     readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*>& mQueue)> mProcess;
  std::function<void (uint8_t* pes, int size, int num, uint64_t pts)> mDecode;

  bool mUseQueue = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;

  int mAllocSize = kInitPesSize;
  uint8_t* mPes = nullptr;
  int mPesSize = 0;
  int mNum = 0;
  uint64_t mPts = 0;
  };
