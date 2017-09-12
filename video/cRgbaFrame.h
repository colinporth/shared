// cYuvFrame.h
#pragma once

class cRgbaFrame  {
public:
  //{{{
  void set (uint64_t pts, bool ok, uint8_t* rgba, int stride, int width, int height, int len, int info) {

    // invalidate
    mPts = 0;
    mOk = ok;

    mWidth = width;
    mHeight = height;
    mLen = len;
    mInfo = info;

    mStride = stride;

    // copy rgba to mRgbaBuf
    mRgbaBuf = (uint8_t*)_aligned_realloc (mRgbaBuf, height * mStride, 128);
    memcpy (mRgbaBuf, rgba, height * mStride);

    // flag valid pts
    mPts = pts;
    }
  //}}}
  //{{{
  uint32_t* bgra() {
    return (uint32_t*)mRgbaBuf;
    }
  //}}}
  //{{{
  void freeResources() {

    mPts = 0;
    mWidth = 0;
    mHeight = 0;
    mLen = 0;

    mStride = 0;

    _aligned_free (mRgbaBuf);
    mRgbaBuf = nullptr;
    }
  //}}}
  //{{{
  void invalidate() {

    mOk = false;
    mPts = 0;
    mLen = 0;
    }
  //}}}

  // vars
  bool mOk = false;
  uint64_t mPts = 0;

  int mWidth = 0;
  int mHeight = 0;
  int mLen = 0;
  int mInfo = 0;

  int mStride = 0;

  uint8_t* mRgbaBuf = nullptr;
  };
