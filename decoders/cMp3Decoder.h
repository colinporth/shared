// cMp3Decoder.h
#pragma once
#include "iAudioDecoder.h"

//{{{  defines
#define MAX_FREE_FORMAT_FRAME_SIZE  2304  // more than ISO spec's
#define MAX_L3_FRAME_PAYLOAD_BYTES  MAX_FREE_FORMAT_FRAME_SIZE // MUST be >= 320000/8/32000*1152 = 1440
#define MAX_BITRESERVOIR_BYTES      511
#define MINIMP3_MAX_SAMPLES_PER_FRAME (1152*2)
//}}}
class cMp3Decoder : public iAudioDecoder {
public:
  //{{{
  struct sGranule {
    const uint8_t* sfbtab;
    uint16_t part23length;
    uint16_t bigValues;
    uint16_t scalefac_compress;
    uint8_t  globalGain;
    uint8_t  blockType;
    uint8_t  mixedBlockFlag;
    uint8_t  numLongSfb;
    uint8_t  numShortSfb;
    uint8_t  tableSelect [3];
    uint8_t  regionCount [3];
    uint8_t  subBlockGain [3];
    uint8_t  preflag;
    uint8_t  scaleFactorScale;
    uint8_t  count1table;
    uint8_t  scfsi;
    };
  //}}}
  //{{{
  class cBitStream {
  public:
     cBitStream() {}
     cBitStream (const uint8_t* buffer, int32_t bytes) : mBuffer(buffer), mPosition(0), mLimit(bytes * 8) {}

    //{{{
    void bitStreamInit (const uint8_t* buffer, int32_t bytes) {
      mBuffer = buffer;
      mPosition = 0;
      mLimit = bytes * 8;
      }
    //}}}

    // gets
    //{{{
    uint32_t getBits (int32_t n) {

      uint32_t s = mPosition & 7;
      int32_t shl = n + s;

      const uint8_t* p = mBuffer + (mPosition >> 3);
      if ((mPosition += n) > mLimit)
        return 0;

      uint32_t cache = 0;
      uint32_t next = *p++ & (255 >> s);
      while ((shl -= 8) > 0) {
        cache |= next << shl;
        next = *p++;
        }

      return cache | (next >> -shl);
      }
    //}}}
    const uint8_t* getBitStreamBuffer() { return mBuffer; }
    int32_t getBitStreamPosition() { return mPosition; }
    int32_t getBitStreamLimit() { return mLimit; }

    uint32_t getBitStreamCache() { return mCache; }
    uint32_t peekBits (int32_t n) { return mCache >> (32 - n); }
    int32_t getBitPosition() { return int32_t(mPtr - mBuffer) * 8 - 24 + mShift; }

    // actions
    void setBitStreamPos (int32_t position) { mPosition = position; }

    void fillCache() {
      mPtr = mBuffer + mPosition / 8;
      mCache = ((((((mPtr[0] << 8) + mPtr[1]) << 8) + mPtr[2]) << 8) + mPtr[3]) << (mPosition & 7);
      mPtr += 4;
      mShift = (mPosition & 7) - 8;
      }

    void flushBits (int32_t n) {
      mCache <<= n;
      mShift += n;
      }

    void checkBits() {
      while (mShift >= 0) {
        mCache |= (uint32_t)*mPtr++ << mShift;
        mShift -= 8;
        }
      }

  private:
    const uint8_t* mBuffer = nullptr;
    int32_t mPosition = 0;
    int32_t mLimit = 0;

    const uint8_t* mPtr = nullptr;
    uint32_t mCache = 0;
    int32_t mShift = 0;
    };
  //}}}

  cMp3Decoder();
  ~cMp3Decoder();

  int32_t getNumChannels() { return mNumChannels; }
  int32_t getSampleRate() { return mSampleRate; }
  int32_t decodeFrame (uint8_t* inBuffer, int32_t bytesLeft, float* outBuffer);

private:
  // private members
  void clear();
  void saveReservoir();
  bool restoreReservoir (class cBitStream* bitStream, int32_t mainDataBegin);

  // private vars
  uint8_t mHeader[4] = { 0 };
  int32_t mNumChannels = 0;
  int32_t mSampleRate = 0;

  float mMdctOverlap [2][9*32] = { 0.f };
  float mQmfState [15*2*32] = { 0.f };

  struct sGranule mGranules [4];
  float mGranuleBuffer [2][576];

  float mScf [40] = { 0.f };
  float mSyn [18+15][2*32] = { 0.f };
  uint8_t mIstPos [2][39] = { 0 };

  int32_t mReservoir = 0;
  uint8_t mReservoirBuffer [511] = { 0 };

  cBitStream mBitStream;
  uint8_t mBitStreamData [MAX_BITRESERVOIR_BYTES + MAX_L3_FRAME_PAYLOAD_BYTES] = { 0 };
  };
