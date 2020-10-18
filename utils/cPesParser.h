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
    cPesItem (uint8_t* pes, int size, int sequenceNum, uint64_t pts)
        : mPesSize(size), mSequenceNum(sequenceNum), mPts(pts) {
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
    const int mSequenceNum;
    const uint64_t mPts;
    };
  //}}}
  //{{{
  cPesParser (int pid, bool useQueue, const std::string& name,
              std::function <int (uint8_t* pes, int size, int sequenceNum, uint64_t pts, cPesParser* parserPes)> process,
              std::function <void (uint8_t* pes, int size, int sequenceNum, uint64_t pts)> decode)
      : mPid(pid), mName(name), mProcessCallback(process), mDecodeCallback(decode), mUseQueue(useQueue) {

    mPes = (uint8_t*)malloc (kInitPesSize);

    if (useQueue)
      std::thread ([=](){ dequeThread(); }).detach();
    }
  //}}}
  virtual ~cPesParser() {}

  //{{{
  void clear (int sequenceNum) {
    mSequenceNum = sequenceNum;
    mPesSize = 0;
    mPts = 0;
    }
  //}}}

  int getPid() { return mPid; }
  float getQueueFrac() { return mUseQueue ? (float)mQueue.size_approx() / mQueue.max_capacity() : 0.f; }
  int getQueueSize() { return mUseQueue ? (int)mQueue.size_approx() : 0; }

  //{{{
  bool parseTs (uint8_t* ts) {

    int pid = ((ts[1] & 0x1F) << 8) | ts[2];
    if (pid == mPid) {
      bool payloadStart = ts[1] & 0x40;
      auto headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
      ts += headerSize;
      int tsLeft = 188 - headerSize;

      if (payloadStart) {
        // could check pes type as well

        // end of last pes, if any, process it
        process();

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
  void process() {
    if (mPesSize) {
      mSequenceNum = mProcessCallback (mPes, mPesSize, mSequenceNum, mPts, this);
      mPesSize = 0;
      }
    }
  //}}}
  //{{{
  void decode (uint8_t* pes, int size, int sequenceNum, uint64_t pts) {

    if (mUseQueue)
      mQueue.enqueue (new cPesParser::cPesItem (pes, size, sequenceNum, pts));
    else
      mDecodeCallback (pes, size, sequenceNum, pts);
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
      cPesItem* pesItem;
      mQueue.wait_dequeue (pesItem);
      mDecodeCallback (pesItem->mPes, pesItem->mPesSize, pesItem->mSequenceNum, pesItem->mPts);
      delete pesItem;
      }
    }
  //}}}

  // vars
  int mPid = 0;
  std::string mName;

  std::function <int (uint8_t* pes, int size, int sequenceNum, uint64_t pts, cPesParser* parserPes)> mProcessCallback;
  std::function <void (uint8_t* pes, int size, int sequenceNum, uint64_t pts)> mDecodeCallback;

  bool mUseQueue = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;

  int mAllocSize = kInitPesSize;

  uint8_t* mPes = nullptr;
  int mPesSize = 0;
  int mSequenceNum = 0;
  uint64_t mPts = 0;
  };
