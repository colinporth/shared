// cTsBlockPool.cpp
//{{{  includes
#include "cTsBlockPool.h"

#include "../../shared/fmt/core.h"
#include "../../shared/utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}

// public
//{{{
cTsBlock* cTsBlockPool::newBlock() {

  cTsBlock* block;
  if (mFreeBlockCount) {
    block = mBlockPool;
    mBlockPool = block->mNextBlock;
    mFreeBlockCount--;
    }
  else {
    block = new cTsBlock();
    mAllocatedBlockCount++;
    mMaxBlockCount = max (mMaxBlockCount, mAllocatedBlockCount);
    }

  block->mNextBlock = NULL;
  block->mRefCount = 1;

  return block;
  }
//}}}
//{{{
cTsBlockPool::~cTsBlockPool() {

  // free blocks
  while (mFreeBlockCount) {
    cTsBlock* block = mBlockPool;
    mBlockPool = block->mNextBlock;
    delete block;
    mAllocatedBlockCount--;
    mFreeBlockCount--;
    }
  }
//}}}

//{{{
void cTsBlockPool::freeBlock (cTsBlock* block) {
// delte if too many blocks allocated else return to pool

  if (mFreeBlockCount >= mMaxBlocks) {
    delete block;
    mAllocatedBlockCount--;
    return;
    }

  block->mNextBlock = mBlockPool;
  mBlockPool = block;
  mFreeBlockCount++;
  }
//}}}
//{{{
void cTsBlockPool::unRefBlock (cTsBlock* block) {

  block->mRefCount--;
  if (!block->mRefCount)
    cTsBlockPool::freeBlock (block);
  }
//}}}
