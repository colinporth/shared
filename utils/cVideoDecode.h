// cVideoDecode.h
#pragma once

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

using namespace std;
using namespace chrono;

// cVideoDecode
class cVideoDecode {
public:
  //{{{
  class cFrame {
  public:
    cFrame (uint64_t pts) : mPts(pts), mOk(false) {}
    //{{{
    virtual ~cFrame() {

      #ifdef __linux__
        free (mYbuf);
        free (mUbuf);
        free (mVbuf);
        free (m32);
      #else
        _aligned_free (mYbuf);
        _aligned_free (mUbuf);
        _aligned_free (mVbuf);
        _aligned_free (m32);
      #endif
      }
    //}}}

    bool ok() { return mOk; }
    uint64_t getPts() { return mPts; }

    int getWidth() { return mWidth; }
    int getHeight() { return mHeight; }
    uint32_t* get32() { return m32; }

    //{{{
    void set (uint64_t pts) {
      mOk = false;
      mPts = pts;
      }
    //}}}
    //{{{
    void setNv12mfx (uint8_t* buffer, int width, int height, int stride) {

      mOk = false;
      mYStride = stride;
      mUVStride = stride/2;

      mWidth = width;
      mHeight = height;

      #ifdef __linux__
        if (!mYbuf)
          mYbuf = (uint8_t*)aligned_alloc (height * mYStride * 3 / 2, 128);
        if (!mUbuf)
          mUbuf = (uint8_t*)aligned_alloc ((mHeight/2) * mUVStride, 128);
        if (!mVbuf)
          mVbuf = (uint8_t*)aligned_alloc ((mHeight/2) * mUVStride, 128);
        if (!m32)
          m32 = (uint32_t*)aligned_alloc (mWidth * 4 * mHeight, 128);
      #else
        mYbuf = (uint8_t*)_aligned_realloc (mYbuf, height * mYStride * 3 / 2, 128);
        mUbuf = (uint8_t*)_aligned_realloc (mUbuf, (mHeight/2) * mUVStride, 128);
        mVbuf = (uint8_t*)_aligned_realloc (mVbuf, (mHeight/2) * mUVStride, 128);
        m32 = (uint32_t*)_aligned_realloc (m32, mWidth * 4 * mHeight, 128);
      #endif

      // copy all of nv12 to yBuf
      memcpy (mYbuf, buffer, height * mYStride * 3 / 2);

      // unpack nv12 to planar uv
      uint8_t* uv = mYbuf + (mHeight * mYStride);
      uint8_t* u = mUbuf;
      uint8_t* v = mVbuf;
      for (int i = 0; i < mHeight/2 * mUVStride; i++) {
        *u++ = *uv++;
        *v++ = *uv++;
        }

      int argbStride = mWidth;

      __m128i ysub  = _mm_set1_epi32 (0x00100010);
      __m128i uvsub = _mm_set1_epi32 (0x00800080);
      __m128i facy  = _mm_set1_epi32 (0x004a004a);
      __m128i facrv = _mm_set1_epi32 (0x00660066);
      __m128i facgu = _mm_set1_epi32 (0x00190019);
      __m128i facgv = _mm_set1_epi32 (0x00340034);
      __m128i facbu = _mm_set1_epi32 (0x00810081);
      __m128i zero  = _mm_set1_epi32 (0x00000000);

      for (int y = 0; y < mHeight; y += 2) {
        __m128i* srcy128r0 = (__m128i*)(mYbuf + mYStride*y);
        __m128i* srcy128r1 = (__m128i*)(mYbuf + mYStride*y + mYStride);
        __m64* srcu64 = (__m64*)(mUbuf + mUVStride * (y/2));
        __m64* srcv64 = (__m64*)(mVbuf + mUVStride * (y/2));
        __m128i* dstrgb128r0 = (__m128i*)(m32 + argbStride * y);
        __m128i* dstrgb128r1 = (__m128i*)(m32 + argbStride * y + argbStride);

        for (int x = 0; x < mWidth; x += 16) {
          __m128i u0 = _mm_loadl_epi64 ((__m128i *)srcu64 );
          srcu64++;
          __m128i v0 = _mm_loadl_epi64 ((__m128i *)srcv64 );
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
      mOk = true;
      }
    //}}}
    //{{{
    void setNv12ffmpeg (uint8_t** data, int* linesize, int width, int height) {

      mOk = false;

      mWidth = width;
      mHeight = height;

      #ifdef __linux__
        if (!m32)
          m32 = (uint32_t*)aligned_alloc (width * 4 * height, 128);
      #else
        m32 = (uint32_t*)_aligned_realloc (m32, width * 4 * height, 128);
      #endif

      // Convert from YUV to RGB
      uint8_t* rgbaData[1] = { (uint8_t*)m32 };
      int rgbaStride[1] = { width * 4 };
      SwsContext* swsContext = sws_getContext (width, height, AV_PIX_FMT_YUV420P,
                                               width, height, AV_PIX_FMT_RGBA,
                                               SWS_BILINEAR, NULL, NULL, NULL);
      sws_scale (swsContext, data, linesize, 0, height, rgbaData, rgbaStride);

      mOk = true;
      }
    //}}}

    //{{{
    void clear() {
      mOk = false;
      mPts = 0;
      }
    //}}}

  private:
    bool mOk = false;
    uint64_t mPts = 0;

    int mWidth = 0;
    int mHeight = 0;
    int mYStride = 0;
    int mUVStride = 0;

    uint8_t* mYbuf = nullptr;
    uint8_t* mUbuf = nullptr;
    uint8_t* mVbuf = nullptr;

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

  int getWidth() { return mWidth; }
  int getHeight() { return mHeight; }
  int getFramePoolSize() { return (int)mFramePool.size(); }
  virtual int getSurfacePoolSize() = 0;

  void setPlayPts (uint64_t playPts) { mPlayPts = playPts; }
  //{{{
  cFrame* findPlayFrame() {
  // returns nearest frame within a 25fps frame of mPlayPts, nullptr if none

    uint64_t nearDist = 90000 / 25;

    cFrame* nearFrame = nullptr;
    for (auto frame : mFramePool) {
      if (frame->ok()) {
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
  //{{{
  void clear() {
  // returns nearest frame within a 25fps frame of mPlayPts, nullptr if none

    for (auto frame : mFramePool)
      frame->clear();
    }
  //}}}

  virtual void decode (bool firstPts, uint64_t pts, uint8_t* pesBuffer, unsigned int pesBufferLen) = 0;

protected:
  static constexpr int kMaxVideoFramePoolSize = 200;
  //{{{
  cFrame* getFreeFrame (uint64_t pts) {
  // return first frame older than mPlayPts, otherwise add new frame

    while (true) {
      for (auto frame : mFramePool) {
        if (frame->ok() && (frame->getPts() < mPlayPts)) {
          // reuse frame
          frame->set (pts);
          return frame;
          }
        }

      if (mFramePool.size() < kMaxVideoFramePoolSize) {
        // allocate new frame
        mFramePool.push_back (new cFrame (pts));
        cLog::log (LOGINFO1, "allocated newFrame %d for %u at play:%u", mFramePool.size(), pts, mPlayPts);
        return mFramePool.back();
        }
      else
        std::this_thread::sleep_for (40ms);
      }

    // cannot reach here
    return nullptr;
    }
  //}}}

  int mWidth = 0;
  int mHeight = 0;

  uint64_t mPlayPts = 0;
  vector <cFrame*> mFramePool;
  };


// cFFmpegVideoDecode
class cFFmpegVideoDecode : public cVideoDecode {
public:
  cFFmpegVideoDecode() : cVideoDecode() { }
  //{{{
  virtual ~cFFmpegVideoDecode() {

    if (mAvContext)
      avcodec_close (mAvContext);
    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  int getSurfacePoolSize() { return 0; }

  //{{{
  void decode (bool firstPts, uint64_t pts, uint8_t* pesBuffer, unsigned int pesBufferLen) {

    if (!mAvParser) {
      mAvParser = av_parser_init (AV_CODEC_ID_H264);
      mAvCodec = avcodec_find_decoder (AV_CODEC_ID_H264);
      mAvContext = avcodec_alloc_context3 (mAvCodec);
      avcodec_open2 (mAvContext, mAvCodec, NULL);
      }

    // ffmpeg doesn't maintain correct frame.pts, but decodes frames in presentation order
    if (firstPts)
      mDecodePts = pts;

    cLog::log (LOGINFO, "decode PES " + getPtsString (pts));

    // init avPacket
    AVPacket avPacket;
    av_init_packet (&avPacket);

    auto avFrame = av_frame_alloc();

    auto pesPtr = pesBuffer;
    auto pesSize = pesBufferLen;
    while (pesSize) {
      auto bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket.data, &avPacket.size,
                                         pesPtr, (int)pesSize, pts, AV_NOPTS_VALUE, 0);
      avPacket.pts = pts;

      pesPtr += bytesUsed;
      pesSize -= bytesUsed;
      if (avPacket.size) {
        auto ret = avcodec_send_packet (mAvContext, &avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
            break;

          auto framePts = avFrame->pts;
          auto frame = getFreeFrame (mDecodePts);
          frame->setNv12ffmpeg (avFrame->data, avFrame->linesize, avFrame->width, avFrame->height);

          // fake pts from avContext framerate
          mDecodePts += (90000 * mAvContext->framerate.den) / mAvContext->framerate.num;
          }
        }
      }

    av_frame_free (&avFrame);
    }
  //}}}

private:
  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;

  uint64_t mDecodePts = 0;
  };
