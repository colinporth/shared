// cBitStream.h
#pragma once
#include "assembly.h"

class cBitStream {
public:
  cBitStream (int numBytes, uint8_t* buf) : bytePtr(buf), nBytes(numBytes) {}

  //{{{
  void AdvanceBitstream (int nBits) {
  // move bitstream pointer ahead by nBits

    nBits &= 0x1f;
    if (nBits > cachedBits) {
      nBits -= cachedBits;
      RefillBitstreamCache();
      }

    iCache <<= nBits;
    cachedBits -= nBits;
    }
  //}}}
  //{{{
  void ByteAlignBitstream() {
  // bump bitstream pointer to start of next byte

    AdvanceBitstream (cachedBits & 0x07);
    }
  //}}}
  //{{{
  unsigned int GetBits (int nBits) {
  // get bits from bitstream, advance bitstream pointer
  // nBits must be in range [0, 31], nBits outside this range masked by 0x1f
  // for speed, does not indicate error if you overrun bit buffer
  // if nBits == 0, returns 0

    nBits &= 0x1f;                  // nBits mod 32 to avoid unpredictable results like >> by negative amount
    unsigned int data = iCache >> (31 - nBits);   // unsigned >> so zero-extend
    data >>= 1;                     // do as >> 31, >> 1 so that nBits = 0 works okay (returns 0)

    iCache <<= nBits;          // left-justify cache
    cachedBits -= nBits;       // how many bits have we drawn from the cache so far

    // if we cross an int boundary, refill the cache
    if (cachedBits < 0) {
      unsigned int lowBits = -cachedBits;
      RefillBitstreamCache();
      data |= iCache >> (32 - lowBits);    // get the low-order bits
      cachedBits -= lowBits;   // how many bits have we drawn from the cache so far
      iCache <<= lowBits;      // left-justify cache
      }

    return data;
    }
  //}}}
  //{{{
  unsigned int GetBitsNoAdvance (int nBits) {

    nBits &= 0x1f;              // nBits mod 32 to avoid unpredictable results like >> by negative amount
    unsigned int data = iCache >> (31 - nBits);   // unsigned >> so zero-extend
    data >>= 1;                 // do as >> 31, >> 1 so that nBits = 0 works okay (returns 0)
    signed int lowBits = nBits - cachedBits;      // how many bits do we have left to read

    // if we cross an int boundary, read next bytes in buffer
    if (lowBits > 0) {
      unsigned int iCache = 0;
      uint8_t* buf = bytePtr;
      while (lowBits > 0) {
        iCache <<= 8;
        if (buf < bytePtr + nBytes)
          iCache |= (unsigned int)*buf++;
        lowBits -= 8;
        }
      lowBits = -lowBits;
      data |= iCache >> lowBits;
      }

    return data;
    }
  //}}}

  //{{{
  int CalcBitsUsed (uint8_t* startBuf, int startOffset) {
    return ((int)(bytePtr - startBuf) * 8) - cachedBits - startOffset;
    }
  //}}}

private:
  //{{{
  // Description: read new data from bitstream buffer into 32-bit cache
  // Inputs:      pointer to initialized bitStream struct
  // Outputs:     updated bitstream info struct
  // Notes:       only call when iCache is completely drained (resets bitOffset to 0)
  //              always loads 4 new bytes except when nBytes < 4 (end of buffer)
  //              stores data as big-endian in cache, regardless of machine endian-ness
  void RefillBitstreamCache() {

    if (nBytes >= 4) {
      unsigned int* Ptr32 = (unsigned int*)bytePtr;
      iCache = REV32 (*Ptr32);
      bytePtr += 4;
      cachedBits = 32;
      nBytes -= 4;
      }
    else if (nBytes == 2) {
      unsigned short* Ptr16 = (unsigned short*)bytePtr;
      iCache = REV16 (*Ptr16);
      bytePtr += 2;
      cachedBits = 16;
      nBytes -= 2;
      }
    else { // = 1
      iCache = (*bytePtr++) << 24;
      cachedBits = 8;
      nBytes = 0;
      }
    }
  //}}}

  uint8_t* bytePtr;
  unsigned int iCache = 0;
  int cachedBits = 0;
  int nBytes;
  };
