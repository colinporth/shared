// cVideoDecode.h
#pragma once
//{{{  includes
extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

#ifdef _WIN32
  #include <emmintrin.h>
  #include "../mfx/include/mfxvideo++.h"
#endif

using namespace std;
using namespace chrono;
//}}}

class cVideoDecode {
public:
  //{{{
  class cFrame {
  public:
    enum eState { eFree, eAllocated, eLoaded };
    cFrame (uint64_t pts) : mPts(pts) {}
    //{{{
    virtual ~cFrame() {

      #ifdef __linux__
        free (m32);
      #else
        _aligned_free (m32);
      #endif
      }
    //}}}

    //{{{
    void clear() {
      mState = eFree;
      mPts = 0;
      }
    //}}}

    // gets
    eState getState() { return mState; }
    uint64_t getPts() { return mPts; }
    uint32_t* get32() { return m32; }

    // sets
    //{{{
    void set (uint64_t pts) {

      mState = eAllocated;
      mPts = pts;
      }
    //}}}
    #ifdef _WIN32
      //{{{
      void setNv12mfx (int width, int height, uint8_t* buffer, int stride) {

        int uvStride = stride/2;
        int argbStride = width;
        //{{{  allocate buffers, unpack nv12 interleaved uv to planar uv
        #ifdef __linux__

          uint8_t* uBuffer = (uint8_t*)aligned_alloc (128, uvStride * (height/2));
          uint8_t* vBuffer = (uint8_t*)aligned_alloc (128, uvStride * (height/2));
          if (!m32)
            m32 = (uint32_t*)aligned_alloc (128, width * height * 4);

        #else

          uint8_t* uBuffer = (uint8_t*)_aligned_malloc (uvStride * (height/2), 128);
          uint8_t* vBuffer = (uint8_t*)_aligned_malloc (uvStride * (height/2), 128);
          if (!m32)
            m32 = (uint32_t*)_aligned_malloc (width * height * 4, 128);

        #endif

        uint8_t* uv = buffer + (height * stride);
        uint8_t* u = uBuffer;
        uint8_t* v = vBuffer;
        for (int i = 0; i < uvStride * (height/2); i++) {
          *u++ = *uv++;
          *v++ = *uv++;
          }
        //}}}

        __m128i ysub  = _mm_set1_epi32 (0x00100010);
        __m128i uvsub = _mm_set1_epi32 (0x00800080);
        __m128i facy  = _mm_set1_epi32 (0x004a004a);
        __m128i facrv = _mm_set1_epi32 (0x00660066);
        __m128i facgu = _mm_set1_epi32 (0x00190019);
        __m128i facgv = _mm_set1_epi32 (0x00340034);
        __m128i facbu = _mm_set1_epi32 (0x00810081);
        __m128i zero  = _mm_set1_epi32 (0x00000000);

        for (int y = 0; y < height; y += 2) {
          __m128i* srcy128r0 = (__m128i*)(buffer + stride*y);
          __m128i* srcy128r1 = (__m128i*)(buffer + stride*y + stride);
          __m64* srcu64 = (__m64*)(uBuffer + uvStride * (y/2));
          __m64* srcv64 = (__m64*)(vBuffer + uvStride * (y/2));
          __m128i* dstrgb128r0 = (__m128i*)(m32 + argbStride * y);
          __m128i* dstrgb128r1 = (__m128i*)(m32 + argbStride * y + argbStride);

          for (int x = 0; x < width; x += 16) {
            __m128i u0 = _mm_loadl_epi64 ((__m128i*)srcu64);
            srcu64++;
            __m128i v0 = _mm_loadl_epi64 ((__m128i*)srcv64);
            srcv64++;
            __m128i y0r0 = _mm_load_si128 (srcy128r0++);
            __m128i y0r1 = _mm_load_si128 (srcy128r1++);
            //{{{  constant y factors
            __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r0, zero), ysub), facy);
            __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r0, zero), ysub), facy);
            __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r1, zero), ysub), facy);
            __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r1, zero), ysub), facy);
            //}}}
            //{{{  expand u and v so they're aligned with y values
            u0 = _mm_unpacklo_epi8 (u0, zero);
            __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub);
            __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub);

            v0 = _mm_unpacklo_epi8( v0,  zero );
            __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub);
            __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub);
            //}}}
            //{{{  common factors on both rows.
            __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
            __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
            __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
            __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
            __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
            __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
            __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
            __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
            //}}}
            //{{{  row 0
            __m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6);
            __m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6);
            __m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6);
            __m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6);
            __m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6);
            __m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6);

            r00 = _mm_packus_epi16 (r00, r01);                // rrrr.. saturated
            g00 = _mm_packus_epi16 (g00, g01);                // gggg.. saturated
            b00 = _mm_packus_epi16 (b00, b01);                // bbbb.. saturated

            r01 = _mm_unpacklo_epi8 (r00, zero);              // 0r0r..
            __m128i gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
            __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // 0rgb0rgb..
            __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // 0rgb0rgb..

            r01 = _mm_unpackhi_epi8 (r00, zero);
            gbgb = _mm_unpackhi_epi8 (b00, g00);
            __m128i rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
            __m128i rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

            _mm_stream_si128 (dstrgb128r0++, rgb0123);
            _mm_stream_si128 (dstrgb128r0++, rgb4567);
            _mm_stream_si128 (dstrgb128r0++, rgb89ab);
            _mm_stream_si128 (dstrgb128r0++, rgbcdef);
            //}}}
            //{{{  row 1
            r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6);
            r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6);
            g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6);
            g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6);
            b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6);
            b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6);

            r00 = _mm_packus_epi16 (r00, r01);        // rrrr.. saturated
            g00 = _mm_packus_epi16 (g00, g01);        // gggg.. saturated
            b00 = _mm_packus_epi16 (b00, b01);        // bbbb.. saturated

            r01     = _mm_unpacklo_epi8 (r00, zero);  // 0r0r..
            gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
            rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // 0rgb0rgb..
            rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // 0rgb0rgb..

            r01     = _mm_unpackhi_epi8 (r00, zero);
            gbgb    = _mm_unpackhi_epi8 (b00, g00);
            rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
            rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

            _mm_stream_si128 (dstrgb128r1++, rgb0123);
            _mm_stream_si128 (dstrgb128r1++, rgb4567);
            _mm_stream_si128 (dstrgb128r1++, rgb89ab);
            _mm_stream_si128 (dstrgb128r1++, rgbcdef);
            //}}}
            }
          }

        //!!! alpha fixup !!!
        //for (auto ptr = m32; ptr < m32 + (height*width); ptr++)
        //  *ptr |= 0xFF000000;

        //{{{  free temp planar buffers
        #ifdef __linux__

          free (uBuffer);
          free (vBuffer);

        #else

          _aligned_free (uBuffer);
          _aligned_free (vBuffer);

        #endif
        //}}}

        mState = eLoaded;
        }
      //}}}
      //{{{
      void setNv12ffmpeg (int width, int height, uint8_t** data, int* linesize) {

        uint8_t* yBuffer = data[0];
        uint8_t* uBuffer = data[1];
        uint8_t* vBuffer = data[2];

        int yStride = linesize[0];
        int uvStride = linesize[1];

        if (!m32) {
          #ifdef __linux__
            m32 = (uint32_t*)aligned_alloc (128, width * height * 4);
          #else
            m32 = (uint32_t*)_aligned_malloc (width * height * 4, 128);
          #endif
          }

        __m128i ysub  = _mm_set1_epi32 (0x00100010);
        __m128i uvsub = _mm_set1_epi32 (0x00800080);
        __m128i facy  = _mm_set1_epi32 (0x004a004a);
        __m128i facrv = _mm_set1_epi32 (0x00660066);
        __m128i facgu = _mm_set1_epi32 (0x00190019);
        __m128i facgv = _mm_set1_epi32 (0x00340034);
        __m128i facbu = _mm_set1_epi32 (0x00810081);
        __m128i zero  = _mm_set1_epi32 (0x00000000);

        for (int y = 0; y < height; y += 2) {
          __m128i* srcy128r0 = (__m128i*)(yBuffer + yStride*y);
          __m128i* srcy128r1 = (__m128i*)(yBuffer + yStride*y + yStride);
          __m64* srcu64 = (__m64*)(uBuffer + uvStride * (y/2));
          __m64* srcv64 = (__m64*)(vBuffer + uvStride * (y/2));
          __m128i* dstrgb128r0 = (__m128i*)(m32 + width * y);
          __m128i* dstrgb128r1 = (__m128i*)(m32 + width * y + width);

          for (int x = 0; x < width; x += 16) {
            __m128i u0 = _mm_loadl_epi64 ((__m128i*)srcu64);
            srcu64++;
            __m128i v0 = _mm_loadl_epi64 ((__m128i*)srcv64);
            srcv64++;
            __m128i y0r0 = _mm_load_si128 (srcy128r0++);
            __m128i y0r1 = _mm_load_si128 (srcy128r1++);
            //{{{  constant y factors
            __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r0, zero), ysub), facy);
            __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r0, zero), ysub), facy);
            __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r1, zero), ysub), facy);
            __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r1, zero), ysub), facy);
            //}}}
            //{{{  expand u and v so they're aligned with y values
            u0 = _mm_unpacklo_epi8 (u0, zero);
            __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub);
            __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub);

            v0 = _mm_unpacklo_epi8( v0,  zero );
            __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub);
            __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub);
            //}}}
            //{{{  common factors on both rows.
            __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
            __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
            __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
            __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
            __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
            __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
            __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
            __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
            //}}}
            //{{{  row 0
            __m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6);
            __m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6);
            __m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6);
            __m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6);
            __m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6);
            __m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6);

            r00 = _mm_packus_epi16 (r00, r01);                // rrrr.. saturated
            g00 = _mm_packus_epi16 (g00, g01);                // gggg.. saturated
            b00 = _mm_packus_epi16 (b00, b01);                // bbbb.. saturated

            r01 = _mm_unpacklo_epi8 (r00, zero);              // 0r0r..
            __m128i gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
            __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // 0rgb0rgb..
            __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // 0rgb0rgb..

            r01 = _mm_unpackhi_epi8 (r00, zero);
            gbgb = _mm_unpackhi_epi8 (b00, g00);
            __m128i rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
            __m128i rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

            _mm_stream_si128 (dstrgb128r0++, rgb0123);
            _mm_stream_si128 (dstrgb128r0++, rgb4567);
            _mm_stream_si128 (dstrgb128r0++, rgb89ab);
            _mm_stream_si128 (dstrgb128r0++, rgbcdef);
            //}}}
            //{{{  row 1
            r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6);
            r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6);
            g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6);
            g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6);
            b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6);
            b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6);

            r00 = _mm_packus_epi16 (r00, r01);        // rrrr.. saturated
            g00 = _mm_packus_epi16 (g00, g01);        // gggg.. saturated
            b00 = _mm_packus_epi16 (b00, b01);        // bbbb.. saturated

            r01     = _mm_unpacklo_epi8 (r00, zero);  // 0r0r..
            gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
            rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // 0rgb0rgb..
            rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // 0rgb0rgb..

            r01     = _mm_unpackhi_epi8 (r00, zero);
            gbgb    = _mm_unpackhi_epi8 (b00, g00);
            rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
            rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

            _mm_stream_si128 (dstrgb128r1++, rgb0123);
            _mm_stream_si128 (dstrgb128r1++, rgb4567);
            _mm_stream_si128 (dstrgb128r1++, rgb89ab);
            _mm_stream_si128 (dstrgb128r1++, rgbcdef);
            //}}}
            }
          }

        //!!! alpha fixup !!!
        for (auto ptr = m32; ptr < m32 + (height*width); ptr++)
          *ptr |= 0xFF000000;

        mState = eLoaded;
        }
      //}}}
    #endif
    //{{{
    void setNv12ffmpegSws (int width, int height, uint8_t** data, int* linesize) {

      if (!m32) {
        #ifdef __linux__
          m32 = (uint32_t*)aligned_alloc (128, width * height * 4);
        #else
          m32 = (uint32_t*)_aligned_malloc (width * height * 4, 128);
        #endif
        }

      // convert yuv to rgba
      uint8_t* dstData[1] = { (uint8_t*)m32 };
      int dstStride[1] = { width * 4 };
      SwsContext* swsContext = sws_getContext (width, height, AV_PIX_FMT_YUV420P,
                                               width, height, AV_PIX_FMT_RGBA,
                                               SWS_BILINEAR, NULL, NULL, NULL);
      sws_scale (swsContext, data, linesize, 0, height, dstData, dstStride);

      mState = eLoaded;
      }
    //}}}

  private:
    eState mState = eFree;
    uint64_t mPts = 0;
    uint32_t* m32 = nullptr;
    };
  //}}}

  cVideoDecode() {}
  //{{{
  virtual ~cVideoDecode() {

    for (auto frame : mFramePool)
      delete frame;
    }
  //}}}

  //{{{
  void clear (uint64_t pts) {

    for (auto frame : mFramePool)
      if ((frame->getState() != cFrame::eLoaded) || (frame->getPts() < pts))
        frame->clear();
    }
  //}}}

  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }
  int getFramePoolSize() { return (int)mFramePool.size(); }
  float getDecodeFrac() { return mFrac; }

  void setDecodeFrac (float frac) { mFrac = frac; }
  void setPlayPts (uint64_t playPts) { mPlayPts = playPts; }
  //{{{
  cFrame* findPlayFrame() {
  // returns nearest frame within a 25fps frame of mPlayPts, nullptr if none

    uint64_t nearDist = 90000 / 25;

    cFrame* nearFrame = nullptr;
    for (auto frame : mFramePool) {
      if (frame->getState() == cFrame::eLoaded) {
        uint64_t dist = frame->getPts() > mPlayPts ? frame->getPts() - mPlayPts : mPlayPts - frame->getPts();
        if (dist < nearDist) {
          nearDist = dist;
          nearFrame = frame;
          }
        }
      }

    return nearFrame;
    }
  //}}}

  virtual void decode (uint8_t* pesBuffer, unsigned int pesBufferLen, bool validPts, uint64_t pts) = 0;

protected:
  static constexpr int kMaxVideoFramePoolSize = 200;
  //{{{
  cFrame* getFreeFrame (uint64_t pts) {
  // return first frame older than mPlayPts - 2 frames just in case, otherwise add new frame

    uint64_t playFramePts = mPlayPts - 2 * (90000 / 25);

    for (auto frame : mFramePool) {
      if ((frame->getState() == cFrame::eFree) ||
          ((frame->getState() == cFrame::eLoaded) && (frame->getPts() < playFramePts))) {
        // reuse frame before playPts minus a couple of frames
        frame->set (pts);
        return frame;
        }
      }

    //cLog::log (LOGINFO, "allocate newFrame %d for %u at play:%u", mFramePool.size(), pts, mPlayPts);

    auto frame = new cFrame (pts);
    mFramePool.push_back (frame);
    return frame;
    }
  //}}}

  int mWidth = 0;
  int mHeight = 0;
  float mFrac = 0.f;
  uint64_t mPlayPts = 0;
  vector <cFrame*> mFramePool;
  };

#ifdef _WIN32
  //{{{
  class cMfxVideoDecode : public cVideoDecode {
  public:
    //{{{
    cMfxVideoDecode() : cVideoDecode() {

      mfxVersion kMfxVersion = { 0,1 };
      mSession.Init (MFX_IMPL_AUTO, &kMfxVersion);
      }
    //}}}
    //{{{
    virtual ~cMfxVideoDecode() {

      MFXVideoDECODE_Close (mSession);

      for (auto surface : mSurfacePool)
        delete surface;
      }
    //}}}

    int getSurfacePoolSize() { return (int)mSurfacePool.size(); }
    //{{{
    void decode (uint8_t* pesBuffer, unsigned int pesBufferLen, bool validPts, uint64_t pts) {

      mBitstream.Data = pesBuffer;
      mBitstream.DataOffset = 0;
      mBitstream.DataLength = pesBufferLen;
      mBitstream.MaxLength = pesBufferLen;
      mBitstream.TimeStamp = pts;

      if (!mWidth) {
        // firstTime, decode header, init decoder
        memset (&mVideoParams, 0, sizeof(mVideoParams));
        mVideoParams.mfx.CodecId = MFX_CODEC_AVC;
        mVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        if (MFXVideoDECODE_DecodeHeader (mSession, &mBitstream, &mVideoParams) != MFX_ERR_NONE) {
          cLog::log (LOGERROR, "MFXVideoDECODE_DecodeHeader failed");
          return;
          }

        //  query surface for mWidth,mHeight
        mfxFrameAllocRequest frameAllocRequest;
        memset (&frameAllocRequest, 0, sizeof(frameAllocRequest));
        if (MFXVideoDECODE_QueryIOSurf (mSession, &mVideoParams, &frameAllocRequest) != MFX_ERR_NONE) {
          cLog::log (LOGERROR, "MFXVideoDECODE_QueryIOSurf failed");
          return;
          }
        mWidth = ((mfxU32)((frameAllocRequest.Info.Width)+31)) & (~(mfxU32)31);
        mHeight = frameAllocRequest.Info.Height;
        // unsure why this was done ??? trace it back for height as well as width ???
        //mHeight = ((mfxU32)((frameAllocRequest.Info.Height)+31)) & (~(mfxU32)31);

        if (MFXVideoDECODE_Init (mSession, &mVideoParams) != MFX_ERR_NONE) {
          cLog::log (LOGERROR, "MFXVideoDECODE_Init failed");
          return;
          }
        }

      // reset decoder on skip
      //mfxStatus status = MFXVideoDECODE_Reset (mSession, &mVideoParams);

      // decode video pes
      // - could be none or multiple frames
      // - returned by decode order, not presentation order
      mfxStatus status = MFX_ERR_NONE;
      while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
        mfxFrameSurface1* surface = nullptr;
        mfxSyncPoint syncDecode = nullptr;
        status = MFXVideoDECODE_DecodeFrameAsync (mSession, &mBitstream, getFreeSurface(), &surface, &syncDecode);
        if (status == MFX_ERR_NONE) {
          status = mSession.SyncOperation (syncDecode, 60000);
          if (status == MFX_ERR_NONE) {
            cLog::log (LOGINFO1, "decoded pts:%u %dx%d:%d",
                                 surface->Data.TimeStamp, surface->Info.Width, surface->Info.Height, surface->Data.Pitch);
            auto frame = getFreeFrame (surface->Data.TimeStamp);
            frame->setNv12mfx (surface->Info.Width, surface->Info.Height, surface->Data.Y, surface->Data.Pitch);
            }
          }
        }
      }
    //}}}

  private:
    //{{{
    mfxFrameSurface1* getFreeSurface() {
    // return first unlocked surface, allocate new if none

      // reuse any unlocked surface
      for (auto surface : mSurfacePool)
        if (!surface->Data.Locked)
          return surface;

      // allocate new surface
      auto surface = new mfxFrameSurface1;
      memset (surface, 0, sizeof (mfxFrameSurface1));
      memcpy (&surface->Info, &mVideoParams.mfx.FrameInfo, sizeof(mfxFrameInfo));
      surface->Data.Y = new mfxU8[mWidth * mHeight * 12 / 8];
      surface->Data.U = surface->Data.Y + mWidth * mHeight;
      surface->Data.V = nullptr; // NV12 ignores V pointer
      surface->Data.Pitch = mWidth;
      mSurfacePool.push_back (surface);

      cLog::log (LOGINFO1, "allocating new mfxFrameSurface1");

      return nullptr;
      }
    //}}}

    MFXVideoSession mSession;
    mfxVideoParam mVideoParams;
    mfxBitstream mBitstream;
    vector <mfxFrameSurface1*> mSurfacePool;
    };
  //}}}
#endif

//{{{
class cFFmpegVideoDecode : public cVideoDecode {
public:
  //{{{
  cFFmpegVideoDecode() : cVideoDecode() {

    mAvParser = av_parser_init (AV_CODEC_ID_H264);
    mAvCodec = avcodec_find_decoder (AV_CODEC_ID_H264);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    avcodec_open2 (mAvContext, mAvCodec, NULL);
    }
  //}}}
  //{{{
  virtual ~cFFmpegVideoDecode() {

    if (mAvContext)
      avcodec_close (mAvContext);
    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  //{{{
  void decode (uint8_t* pesBuffer, unsigned int pesBufferLen, bool validPts, uint64_t pts) {

    // ffmpeg doesn't maintain correct avFrame.pts, but does decode frames in presentation order
    if (validPts)
      mDecodePts = pts;

    //cLog::log (LOGINFO, "cFFmpegVideoDecode PES " + getPtsString (pts));
    auto pesPtr = pesBuffer;
    auto pesSize = pesBufferLen;
    while (pesSize) {
      AVPacket avPacket;
      av_init_packet (&avPacket);
      auto bytesUsed = av_parser_parse2 (mAvParser, mAvContext,
                                         &avPacket.data, &avPacket.size,
                                         pesPtr, (int)pesSize, pts, AV_NOPTS_VALUE, 0);
      avPacket.pts = pts;

      pesPtr += bytesUsed;
      pesSize -= bytesUsed;
      if (avPacket.size) {
        //cLog::log (LOGINFO, "decode  start");
        auto ret = avcodec_send_packet (mAvContext, &avPacket);
        while (ret >= 0) {
          auto avFrame = av_frame_alloc();
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
            break;

          mWidth = avFrame->width;
          mHeight = avFrame->height;
          cVideoDecode::cFrame* frame = getFreeFrame (mDecodePts);
          //cLog::log (LOGINFO, "decode done");
          frame->setNv12ffmpegSws (mWidth, mHeight, avFrame->data, avFrame->linesize);
          //cLog::log (LOGINFO, "yuv2rgb done");

          // fake pts from avContext framerate
          mDecodePts += (90000 * mAvContext->framerate.den) / mAvContext->framerate.num;
          av_frame_free (&avFrame);
          }
        }
      }
    }
  //}}}

private:
  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;

  uint64_t mDecodePts = 0;
  };

//}}}
