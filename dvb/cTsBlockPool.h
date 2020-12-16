// cBlockPool.h - dvb transport stream block pool, produced by cDvb, consumed by cDvbRtp
#pragma once
//{{{  includes
#include <cstdlib>
#include <cstdint>
class cTsBlockPool;
//}}}

class cTsBlock {
friend cTsBlockPool;
public:
  void incRefCount() { mRefCount++; }
  void decRefCount() { mRefCount--; }

  cTsBlock* mNextBlock = nullptr;
  int64_t mDts = 0;
  uint8_t mTs[188];

private:
  int mRefCount = 0;
  };


class cTsBlockPool {
public:
  cTsBlockPool (int maxBlocks) : mMaxBlocks(maxBlocks) {}
  ~cTsBlockPool();

  cTsBlock* newBlock();
  void freeBlock (cTsBlock* block);
  void unRefBlock (cTsBlock* block);

private:
  const int mMaxBlocks = 0;

  int mFreeBlockCount = 0;
  int mAllocatedBlockCount = 0;
  int mMaxBlockCount = 0;
  cTsBlock* mBlockPool = NULL;
  };
