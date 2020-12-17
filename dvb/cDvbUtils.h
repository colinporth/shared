// cDvbUtils.h
#pragma once
//{{{  includes
#include <cstdlib>
#include <cstdint>
#include <string>
//}}}

class cTsBlockPool;
//{{{
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
//}}}
//{{{
class cTsBlockPool {
public:
  cTsBlockPool (int maxBlocks) : mMaxBlocks(maxBlocks) {}
  ~cTsBlockPool();

  int getNumAllocatedBlocks() { return mAllocatedBlockCount; }
  int getNumFreeBlocks() { return mFreeBlockCount; }
  int getNumMaxBlocks() { return mMaxBlockCount; }
  std::string getInfoString();

  cTsBlock* newBlock();
  void freeBlock (cTsBlock* block);
  void unRefBlock (cTsBlock* block);

private:
  const int mMaxBlocks = 0;

  int mAllocatedBlockCount = 0;
  int mFreeBlockCount = 0;
  int mMaxBlockCount = 0;

  cTsBlock* mBlockPool = NULL;
  };
//}}}

class cDvbUtils {
public:
  static int getSectionLength (uint8_t* buf) { return ((buf[0] & 0x0F) << 8) + buf[1] + 3; }

  static uint32_t getCrc32 (uint8_t* buf, uint32_t len);

  static uint32_t getEpochTime (uint8_t* buf);
  static uint32_t getBcdTime (uint8_t* buf);
  static int64_t getPts (const uint8_t* buf);

  static bool isHuff (uint8_t* buf);
  static std::string getString (uint8_t* buf);
  };
