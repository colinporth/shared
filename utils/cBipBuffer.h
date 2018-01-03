// cBipBuffer.h
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class cBipBuffer {
public:
  cBipBuffer() : mBuffer(NULL), mBufferLen(0), mIxa(0), mSza(0), mIxb(0), mSzb(0), mIxResrv(0), mSzResrv(0) {}
  virtual ~cBipBuffer() { freeBuffer(); }

  //{{{
  bool allocateBuffer (int bufferLen) {
  // Allocates a buffer in virtual memory, to the nearest page size (rounded up)
  //   int buffersize size of buffer to allocate, in bytes
  //   return bool true if successful, false if buffer cannot be allocated

    if (bufferLen <= 0)
      return false;
    if (mBuffer != NULL)
      freeBuffer();

    mBuffer = (uint8_t*)malloc (bufferLen);
    if (mBuffer == NULL)
      return false;

    mBufferLen = bufferLen;
    return true;
    }
  //}}}
  //{{{
  void clear() {
  // Clears the buffer of any allocations or reservations. Note; it
  // does not wipe the buffer memory; it merely resets all pointers,
  // returning the buffer to a completely empty state ready for new
  // allocations.

    mIxa = 0;
    mSza = 0;
    mIxb = 0;
    mSzb = 0;
    mIxResrv = 0;
    mSzResrv = 0;
    }
  //}}}
  //{{{
  void freeBuffer() {

    free (mBuffer);
    mBuffer = NULL;
    mBufferLen = 0;
    clear();
    }
  //}}}

  //{{{
  int getCommittedSize() const {
  // Queries how much data (in total) has been committed in the buffer
  // - returns total amount of committed data in the buffer

    return mSza + mSzb;
    }
  //}}}

  // write to
  //{{{
  uint8_t* reserve (int size, int& reserved) {
  // Reserves space in the buffer for a memory write operation
  //   int size                 amount of space to reserve
  //   OUT int& reserved        size of space actually reserved
  // Returns:
  //   BYTE*                    pointer to the reserved block
  //   Will return NULL for the pointer if no space can be allocated.
  //   Can return any value from 1 to size in reserved.
  //   Will return NULL if a previous reservation has not been committed.

    // We always allocate on B if B exists; this means we have two blocks and our buffer is filling.
    if (mSzb) {
      int freespace = getBFreeSpace();
      if (size < freespace)
        freespace = size;
      if (freespace == 0)
        return NULL;

      mSzResrv = freespace;
      reserved = freespace;
      mIxResrv = mIxb + mSzb;
      return mBuffer + mIxResrv;
      }

    else {
      // Block b does not exist, so we can check if the space AFTER a is bigger than the space
      // before A, and allocate the bigger one.
      int freespace = getSpaceAfterA();
      if (freespace >= mIxa) {
        if (freespace == 0)
          return NULL;
        if (size < freespace)
          freespace = size;

        mSzResrv = freespace;
        reserved = freespace;
        mIxResrv = mIxa + mSza;
        return mBuffer + mIxResrv;
        }

      else {
        if (mIxa == 0)
          return NULL;
        if (mIxa < size)
          size = mIxa;
        mSzResrv = size;
        reserved = size;
        mIxResrv = 0;
        return mBuffer;
        }
      }
    }
  //}}}
  //{{{
  void commit (int size) {
  // Commits space that has been written to in the buffer
  // Parameters:
  //   int size                number of bytes to commit
  //   Committing a size > than the reserved size will cause an assert in a debug build;
  //   in a release build, the actual reserved size will be used.
  //   Committing a size < than the reserved size will commit that amount of data, and release
  //   the rest of the space.
  //   Committing a size of 0 will release the reservation.

    if (size == 0) {
      // decommit any reservation
      mSzResrv = mIxResrv = 0;
      return;
      }

    // If we try to commit more space than we asked for, clip to the size we asked for.

    if (size > mSzResrv)
      size = mSzResrv;

    // If we have no blocks being used currently, we create one in A.
    if (mSza == 0 && mSzb == 0) {
      mIxa = mIxResrv;
      mSza = size;

      mIxResrv = 0;
      mSzResrv = 0;
      return;
      }

    if (mIxResrv == mSza + mIxa)
      mSza += size;
    else
      mSzb += size;

    mIxResrv = 0;
    mSzResrv = 0;
    }
  //}}}

  // read from
  //{{{
  uint8_t* getContiguousBlock (int& size) {
  // return pointer to first contiguous block in buffer, size returns blockSize

    if (mSza == 0) {
      size = 0;
      return NULL;
      }

    size = mSza;
    return mBuffer + mIxa;
    }
  //}}}
  //{{{
  void decommitBlock (int size) {
  // Decommits space from the first contiguous block,  size amount of memory to decommit

    if (size >= mSza) {
      mIxa = mIxb;
      mSza = mSzb;
      mIxb = 0;
      mSzb = 0;
      }
    else {
      mSza -= size;
      mIxa += size;
      }
    }
  //}}}

private:
  //{{{
  int getSpaceAfterA() const {
    return mBufferLen - mIxa - mSza;
    }
  //}}}
  //{{{
  int getBFreeSpace() const {
    return mIxa - mIxb - mSzb;
    }
  //}}}

  uint8_t* mBuffer;
  int mBufferLen;

  int mIxa;
  int mSza;

  int mIxb;
  int mSzb;

  int mIxResrv;
  int mSzResrv;
  };
