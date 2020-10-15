// cVideoDecode.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cVideoDecode.h"

// utils
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

#ifdef _WIN32
  #include <emmintrin.h>
  #include <tmmintrin.h>
  #include "../../libmfx/include/mfxvideo++.h"
#endif

using namespace std;
using namespace chrono;
//}}}

// cVideoDecode::cFrame
//{{{
cVideoDecode::cFrame::~cFrame() {

  #ifdef _DEBUG
    cLog::log (LOGINFO, "cVideoDecode::~cFrame %u", mPts);
  #endif

  #ifdef __linux__
    free (m32);
  #else
    _aligned_free (m32);
  #endif
  }
//}}}
//{{{
void cVideoDecode::cFrame::clear() {
  mState = eFree;
  mPts = 0;
  }
//}}}

//{{{
void cVideoDecode::cFrame:: set (uint64_t pts) {

  mState = eAllocated;
  mPts = pts;
  }
//}}}

#ifdef _WIN32
  //{{{
  void cVideoDecode::cFrame::setYuv420Bgra (int width, int height, uint8_t* buffer, int stride) {

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
    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

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

        r00 = _mm_packus_epi16 (r00, r01); // rrrr.. saturated
        g00 = _mm_packus_epi16 (g00, g01); // gggg.. saturated
        b00 = _mm_packus_epi16 (b00, b01); // bbbb.. saturated

        r01 = _mm_unpacklo_epi8 (r00, alpha);             // arar..
        __m128i gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
        __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        r01 = _mm_unpackhi_epi8 (r00, alpha);
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

        r01     = _mm_unpacklo_epi8 (r00, alpha); // arar..
        gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
        rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        r01     = _mm_unpackhi_epi8 (r00, alpha);
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

    // free temp planar buffers
    #ifdef __linux__
      free (uBuffer);
      free (vBuffer);
    #else
      _aligned_free (uBuffer);
      _aligned_free (vBuffer);
    #endif

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420Rgba (int width, int height, uint8_t* buffer, int stride) {

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
    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    for (int y = 0; y < height; y += 2) {
      __m128i* srcy128r0 = (__m128i*)(buffer + stride*y);
      __m128i* srcy128r1 = (__m128i*)(buffer + stride*y + stride);
      __m64* srcu64 = (__m64*)(uBuffer + uvStride * (y/2));
      __m64* srcv64 = (__m64*)(vBuffer + uvStride * (y/2));
      __m128i* dstrgb128r0 = (__m128i*)(m32 + argbStride * y);
      __m128i* dstrgb128r1 = (__m128i*)(m32 + argbStride * y + argbStride);

      for (int x = 0; x < width; x += 16) {
        __m128i y0r0 = _mm_load_si128 (srcy128r0++); // load yyyyyyyy yyyyyyyy for row 0
        __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r0, zero), ysub), facy);
        __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r0, zero), ysub), facy);

        __m128i y0r1 = _mm_load_si128 (srcy128r1++); // load yyyyyyyy yyyyyyyy for row 1
        __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r1, zero), ysub), facy);
        __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r1, zero), ysub), facy);

        // load,expand u to align with y
        __m128i u0 = _mm_loadl_epi64 ((__m128i*)srcu64++); // load vvvvvvvv for row 0,1
        u0 = _mm_unpacklo_epi8 (u0, zero);
        __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub); // uuuuuuuu uuuuuuuu row 0,1 ?
        __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub); // uuuuuuuu uuuuuuuu row 0,1 ?

        // load,expand v to align with y
        __m128i v0 = _mm_loadl_epi64 ((__m128i*)srcv64++); // load vvvvvvvv for row 0,1
        v0 = _mm_unpacklo_epi8 (v0,  zero );
        __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub); // vvvvvvvv vvvvvvvv row 0,1 ?
        __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub); // vvvvvvvv vvvvvvvv row 0,1 ?

        __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
        __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
        //{{{  row 0
        __m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6);
        __m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6);
        __m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6);
        __m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6);
        __m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6);
        __m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6);

        r00 = _mm_packus_epi16 (r00, r01); // rrrr.. saturated
        g00 = _mm_packus_epi16 (g00, g01); // gggg.. saturated
        b00 = _mm_packus_epi16 (b00, b01); // bbbb.. saturated

        r01 = _mm_unpacklo_epi8 (b00, alpha);             // abab..
        __m128i gbgb    = _mm_unpacklo_epi8 (r00, g00);   // grgr..
        __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // abgrabgr..
        __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // abgrabgr..

        r01 = _mm_unpackhi_epi8 (b00, alpha);
        gbgb = _mm_unpackhi_epi8 (r00, g00);
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

        r01     = _mm_unpacklo_epi8 (b00, alpha); // abab..
        gbgb    = _mm_unpacklo_epi8 (r00, g00);   // grgr..
        rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // abgrabgr..
        rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // abgrabgr..

        r01     = _mm_unpackhi_epi8 (b00, alpha);
        gbgb    = _mm_unpackhi_epi8 (r00, g00);
        rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        _mm_stream_si128 (dstrgb128r1++, rgb0123);
        _mm_stream_si128 (dstrgb128r1++, rgb4567);
        _mm_stream_si128 (dstrgb128r1++, rgb89ab);
        _mm_stream_si128 (dstrgb128r1++, rgbcdef);
        //}}}
        }
      }

    // free temp planar buffers
    #ifdef __linux__
      free (uBuffer);
      free (vBuffer);
    #else
      _aligned_free (uBuffer);
      _aligned_free (vBuffer);
    #endif

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420PlanarRgba (int width, int height, uint8_t** data, int* linesize) {

    uint8_t* yBuffer = data[0];
    uint8_t* uBuffer = data[1];
    uint8_t* vBuffer = data[2];

    int stride = linesize[0];
    int uvStride = linesize[1];
    int argbStride = width;

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
    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    for (int y = 0; y < height; y += 2) {
      __m128i* srcy128r0 = (__m128i*)(yBuffer + stride*y);
      __m128i* srcy128r1 = (__m128i*)(yBuffer + stride*y + stride);
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

        r00 = _mm_packus_epi16 (r00, r01); // rrrr.. saturated
        g00 = _mm_packus_epi16 (g00, g01); // gggg.. saturated
        b00 = _mm_packus_epi16 (b00, b01); // bbbb.. saturated

        r01 = _mm_unpacklo_epi8 (r00, alpha);             // arar..
        __m128i gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
        __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        r01 = _mm_unpackhi_epi8 (r00, alpha);
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

        r01     = _mm_unpacklo_epi8 (r00, alpha); // arar..
        gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
        rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        r01     = _mm_unpackhi_epi8 (r00, alpha);
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

    mState = eLoaded;
    }
  //}}}
  //{{{
  //void cVideoDecode::cFrame::setYuv420Rgba (int width, int height, uint8_t* buffer, int stride) {

    //int uvStride = stride/2;
    //uint8_t* uvBuffer = buffer + (height * stride);

    //if (!m32) // allocate aligned buffer
      //m32 = (uint32_t*)
      //#ifdef __linux__
        //aligned_alloc (128, width * height * 4);
      //#else
        //_aligned_malloc (width * height * 4, 128);
      //#endif

    //__m128i ysub  = _mm_set1_epi32 (0x00100010);
    //__m128i uvsub = _mm_set1_epi32 (0x00800080);
    //__m128i facy  = _mm_set1_epi32 (0x004a004a);
    //__m128i facrv = _mm_set1_epi32 (0x00660066);
    //__m128i facgu = _mm_set1_epi32 (0x00190019);
    //__m128i facgv = _mm_set1_epi32 (0x00340034);
    //__m128i facbu = _mm_set1_epi32 (0x00810081);
    //__m128i zero  = _mm_set1_epi32 (0x00000000);
    //__m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);
    //__m128i masku = _mm_set_epi8 (-1,14, -1,12, -1,10, -1,8, -1,6, -1,4, -1,2, -1,0);
    //__m128i maskv = _mm_set_epi8 (-1,15, -1,13, -1,11, -1,9, -1,7, -1,5, -1,3, -1,1);

    //for (int y = 0; y < height; y += 2) {
      //// calc row pointers
      //__m128i* srcY128r0 = (__m128i*)(buffer + (stride*y));
      //__m128i* srcY128r1 = srcY128r0 + stride;
      //__m128* srcUv128 = (__m128*)(uvBuffer + (uvStride*y));
      //__m128i* dstrgb128r0 = (__m128i*)(m32 + width * y);
      //__m128i* dstrgb128r1 = dstrgb128r0 + width;

      //for (int x = 0; x < width; x += 16) {
        //// row0 y, could shuffle
        //__m128i r0y0 = _mm_load_si128 (srcY128r0++); // y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        //__m128i r0y00 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (r0y0, zero), ysub), facy); // ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        //__m128i r0y01 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (r0y0, zero), ysub), facy); // ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy

        //// row1 y, could shuffle
        //__m128i r1y0 = _mm_load_si128 (srcY128r1++); // y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        //__m128i r1y00 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (r1y0, zero), ysub), facy); // ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        //__m128i r1y01 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (r1y0, zero), ysub), facy); // ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy

        //// row01 u,v - align with y
        //__m128i uv = _mm_load_si128 ((__m128i*)srcUv128++);               //  u0 v0 u1 v1 u2 v2 u3 v3 u4 v4 u5 v5 u6 v6 u7 v7
        //__m128i u0 = _mm_shuffle_epi8 (uv, masku);                        //  0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
        //__m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub); // (0.u0 0.u0 0.u1 0.u1 0.u2 0.u2 0.u3 0.u3) - uvsub
        //__m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub); // (0.u4 0.u4 0.u5 0.u5 0.u6 0.u6 0.u7 0.u7) - uvsub
        //__m128i v0 = _mm_shuffle_epi8 (uv, maskv);                        //  0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
        //__m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub); // (0.v0 0.v0 0.v1 0.v1 0.v2 0.v2 0.v3 0.v3) - uvsub
        //__m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub); // (0.v4 0.v4 0.v5 0.v5 0.v6 0.v6 0.v7 0.v7) - uvsub

        //{{{  common factors on both rows
        //__m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        //__m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        //__m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        //__m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        //__m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        //__m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        //__m128i bu00 = _mm_mullo_epi16 (facbu, u00);
        //__m128i bu01 = _mm_mullo_epi16 (facbu, u01);
        //}}}
        //{{{  row 0
        //__m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (r0y00, rv00), 6);
        //__m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (r0y01, rv01), 6);

        //__m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r0y00, gu00), gv00), 6);
        //__m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r0y01, gu01), gv01), 6);

        //__m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (r0y00, bu00), 6);
        //__m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (r0y01, bu01), 6);

        //r00 = _mm_packus_epi16 (r00, r01); // rrrr.. saturated
        //g00 = _mm_packus_epi16 (g00, g01); // gggg.. saturated
        //b00 = _mm_packus_epi16 (b00, b01); // bbbb.. saturated

        //r01 = _mm_unpacklo_epi8 (b00, alpha);             // abab..
        //__m128i gbgb    = _mm_unpacklo_epi8 (r00, g00);   // grgr..
        //__m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // abgrabgr..
        //__m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // abgrabgr..

        //r01 = _mm_unpackhi_epi8 (b00, alpha);
        //gbgb = _mm_unpackhi_epi8 (r00, g00);
        //__m128i rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        //__m128i rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        //_mm_stream_si128 (dstrgb128r0++, rgb0123);
        //_mm_stream_si128 (dstrgb128r0++, rgb4567);

        //_mm_stream_si128 (dstrgb128r0++, rgb89ab);
        //_mm_stream_si128 (dstrgb128r0++, rgbcdef);
        //}}}
        //{{{  row 1
        //r00 = _mm_srai_epi16 (_mm_add_epi16 (r1y00, rv00), 6);
        //r01 = _mm_srai_epi16 (_mm_add_epi16 (r1y01, rv01), 6);
        //g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r1y00, gu00), gv00), 6);
        //g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r1y01, gu01), gv01), 6);
        //b00 = _mm_srai_epi16 (_mm_add_epi16 (r1y00, bu00), 6);
        //b01 = _mm_srai_epi16 (_mm_add_epi16 (r1y01, bu01), 6);

        //r00 = _mm_packus_epi16 (r00, r01);        // rrrr.. saturated
        //g00 = _mm_packus_epi16 (g00, g01);        // gggg.. saturated
        //b00 = _mm_packus_epi16 (b00, b01);        // bbbb.. saturated

        //r01     = _mm_unpacklo_epi8 (b00, alpha); // abab..
        //gbgb    = _mm_unpacklo_epi8 (r00, g00);   // grgr..
        //rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // abgrabgr..
        //rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // abgrabgr..

        //r01     = _mm_unpackhi_epi8 (b00, alpha);
        //gbgb    = _mm_unpackhi_epi8 (r00, g00);
        //rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        //rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        //_mm_stream_si128 (dstrgb128r1++, rgb0123);
        //_mm_stream_si128 (dstrgb128r1++, rgb4567);
        //_mm_stream_si128 (dstrgb128r1++, rgb89ab);
        //_mm_stream_si128 (dstrgb128r1++, rgbcdef);
        //}}}
        //}
      //}

    //mState = eLoaded;
    //}
  //}}}
  //{{{
  //void cVideoDecode::cFrame::setYuv420Bgra (int width, int height, uint8_t* buffer, int stride) {

    //int uvStride = stride/2;
    //uint8_t* uvBuffer = buffer + (height * stride);

    //if (!m32) //  allocate aligned buffer
      //m32 = (uint32_t*)
      //#ifdef __linux__
        //aligned_alloc (128, width * height * 4);
      //#else
        //_aligned_malloc (width * height * 4, 128);
      //#endif

    //__m128i ysub  = _mm_set1_epi32 (0x00100010);
    //__m128i uvsub = _mm_set1_epi32 (0x00800080);
    //__m128i facy  = _mm_set1_epi32 (0x004a004a);
    //__m128i facrv = _mm_set1_epi32 (0x00660066);
    //__m128i facgu = _mm_set1_epi32 (0x00190019);
    //__m128i facgv = _mm_set1_epi32 (0x00340034);
    //__m128i facbu = _mm_set1_epi32 (0x00810081);
    //__m128i zero  = _mm_set1_epi32 (0x00000000);
    //__m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);
    //__m128i masku = _mm_set_epi8 (-1,14, -1,12, -1,10, -1,8, -1,6, -1,4, -1,2, -1,0);
    //__m128i maskv = _mm_set_epi8 (-1,15, -1,13, -1,11, -1,9, -1,7, -1,5, -1,3, -1,1);

    //for (int y = 0; y < height; y += 2) {
      //// calc row pointers
      //__m128i* srcY128r0 = (__m128i*)(buffer + (stride*y));
      //__m128i* srcY128r1 = srcY128r0 + stride;
      //__m128* srcUv128 = (__m128*)(uvBuffer + (uvStride*y));
      //__m128i* dstrgb128r0 = (__m128i*)(m32 + width * y);
      //__m128i* dstrgb128r1 = dstrgb128r0 + width;

      //for (int x = 0; x < width; x += 16) {
        //// row0 y, could shuffle
        //__m128i r0y0 = _mm_load_si128 (srcY128r0++); // y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        //__m128i r0y00 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (r0y0, zero), ysub), facy); // ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        //__m128i r0y01 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (r0y0, zero), ysub), facy); // ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy

        //// row1 y, could shuffle
        //__m128i r1y0 = _mm_load_si128 (srcY128r1++); // y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        //__m128i r1y00 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (r1y0, zero), ysub), facy); // ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        //__m128i r1y01 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (r1y0, zero), ysub), facy); // ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy

        //// row01 u,v - align with y, could shuffle
        //__m128i uv = _mm_load_si128 ((__m128i*)srcUv128++);               //  u0 v0 u1 v1 u2 v2 u3 v3 u4 v4 u5 v5 u6 v6 u7 v7
        //__m128i u0 = _mm_shuffle_epi8 (uv, masku);                        //  0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
        //__m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub); // (0.u0 0.u0 0.u1 0.u1 0.u2 0.u2 0.u3 0.u3) - uvsub
        //__m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub); // (0.u4 0.u4 0.u5 0.u5 0.u6 0.u6 0.u7 0.u7) - uvsub
        //__m128i v0 = _mm_shuffle_epi8 (uv, maskv);                        //  0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
        //__m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub); // (0.v0 0.v0 0.v1 0.v1 0.v2 0.v2 0.v3 0.v3) - uvsub
        //__m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub); // (0.v4 0.v4 0.v5 0.v5 0.v6 0.v6 0.v7 0.v7) - uvsub

        //{{{  common factors on both rows
        //__m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        //__m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        //__m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        //__m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        //__m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        //__m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        //__m128i bu00 = _mm_mullo_epi16 (facbu, u00);
        //__m128i bu01 = _mm_mullo_epi16 (facbu, u01);
        //}}}
        //{{{  row 0
        //__m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (r0y00, rv00), 6);
        //__m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (r0y01, rv01), 6);
        //__m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r0y00, gu00), gv00), 6);
        //__m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r0y01, gu01), gv01), 6);
        //__m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (r0y00, bu00), 6);
        //__m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (r0y01, bu01), 6);

        //r00 = _mm_packus_epi16 (r00, r01); // rrrr.. saturated
        //g00 = _mm_packus_epi16 (g00, g01); // gggg.. saturated
        //b00 = _mm_packus_epi16 (b00, b01); // bbbb.. saturated

        //r01 = _mm_unpacklo_epi8 (r00, alpha);             // arar..
        //__m128i gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
        //__m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        //__m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        //r01 = _mm_unpackhi_epi8 (r00, alpha);
        //gbgb = _mm_unpackhi_epi8 (b00, g00);
        //__m128i rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        //__m128i rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        //_mm_stream_si128 (dstrgb128r0++, rgb0123);
        //_mm_stream_si128 (dstrgb128r0++, rgb4567);
        //_mm_stream_si128 (dstrgb128r0++, rgb89ab);
        //_mm_stream_si128 (dstrgb128r0++, rgbcdef);
        //}}}
        //{{{  row 1
        //r00 = _mm_srai_epi16 (_mm_add_epi16 (r1y00, rv00), 6);
        //r01 = _mm_srai_epi16 (_mm_add_epi16 (r1y01, rv01), 6);
        //g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r1y00, gu00), gv00), 6);
        //g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (r1y01, gu01), gv01), 6);
        //b00 = _mm_srai_epi16 (_mm_add_epi16 (r1y00, bu00), 6);
        //b01 = _mm_srai_epi16 (_mm_add_epi16 (r1y01, bu01), 6);

        //r00 = _mm_packus_epi16 (r00, r01);        // rrrr.. saturated
        //g00 = _mm_packus_epi16 (g00, g01);        // gggg.. saturated
        //b00 = _mm_packus_epi16 (b00, b01);        // bbbb.. saturated

        //r01     = _mm_unpacklo_epi8 (r00, alpha); // arar..
        //gbgb    = _mm_unpacklo_epi8 (b00, g00);   // gbgb..
        //rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        //rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        //r01     = _mm_unpackhi_epi8 (r00, alpha);
        //gbgb    = _mm_unpackhi_epi8 (b00, g00);
        //rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        //rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        //_mm_stream_si128 (dstrgb128r1++, rgb0123);
        //_mm_stream_si128 (dstrgb128r1++, rgb4567);
        //_mm_stream_si128 (dstrgb128r1++, rgb89ab);
        //_mm_stream_si128 (dstrgb128r1++, rgbcdef);
        //}}}
        //}
      //}

    //mState = eLoaded;
    //}
  //}}}
  //{{{
  //void cVideoDecode::cFrame::setYuv420PlanarRgba (int width, int height, uint8_t** data, int* linesize) {

    //uint8_t* yBuffer = data[0];
    //uint8_t* uBuffer = data[1];
    //uint8_t* vBuffer = data[2];

    //int yStride = linesize[0];
    //int uvStride = linesize[1];

    //if (!m32) // allocate aligned buffer
      //m32 = (uint32_t*)
      //#ifdef __linux__
        //aligned_alloc (128, width * height * 4);
      //#else
        //_aligned_malloc (width * height * 4, 128);
      //#endif

    //__m128i ysub  = _mm_set1_epi32 (0x00100010);
    //__m128i uvsub = _mm_set1_epi32 (0x00800080);
    //__m128i facy  = _mm_set1_epi32 (0x004a004a);
    //__m128i facrv = _mm_set1_epi32 (0x00660066);
    //__m128i facgu = _mm_set1_epi32 (0x00190019);
    //__m128i facgv = _mm_set1_epi32 (0x00340034);
    //__m128i facbu = _mm_set1_epi32 (0x00810081);
    //__m128i zero  = _mm_set1_epi32 (0x00000000);
    //__m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    //for (int y = 0; y < height; y += 2) {
      //// calc row pointers
      //__m128i* srcY128r0 = (__m128i*)(yBuffer + (yStride * y));
      //__m128i* srcY128r1 = srcY128r0 + yStride;
      //__m64* srcU64 = (__m64*)(uBuffer + uvStride * (y / 2));
      //__m64* srcV64 = (__m64*)(vBuffer + uvStride * (y / 2));
      //__m128i* dstrgb128r0 = (__m128i*)(m32 + (width * y));
      //__m128i* dstrgb128r1 = dstrgb128r0 + width;

      //for (int x = 0; x < width; x += 16) {
        //{{{  read src into y00r0,y01r0, y00r1,y01r1, u00,u01, v00,v01
        //// row0 y
        //__m128i temp = _mm_load_si128 (srcY128r0++);
        //__m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
        //__m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

        //// row1 y
        //temp = _mm_load_si128 (srcY128r1++);
        //__m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
        //__m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

        //// row01 u, could shuffle
        //temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcU64++), zero); // 0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
        //__m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
        //__m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

        //// row01 v, could shuffle
        //temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcV64++), zero); // 0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
        //__m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
        //__m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);
        //}}}
        //{{{  write convert to rgba and write to dst
        //__m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        //__m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        //__m128i r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6),
                                        //_mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6)); // rrrr.. saturated

        //__m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        //__m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        //__m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        //__m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        //__m128i g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6),
                                        //_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6)); // gggg.. saturated

        //__m128i bu00 = _mm_mullo_epi16 (facbu, u00);
        //__m128i bu01 = _mm_mullo_epi16 (facbu, u01);
        //__m128i b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6),
                                        //_mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6)); // bbbb.. saturated
        //// row0
        //temp = _mm_unpacklo_epi8 (r00, alpha); // arar..
        //__m128i temp1 = _mm_unpacklo_epi8 (b00, g00);  // gbgb..
        //_mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (temp1, temp)); // lo argbargb lo
        //_mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (temp1, temp)); // lo argbargb hi

        //temp = _mm_unpackhi_epi8 (r00, alpha);
        //temp1 = _mm_unpackhi_epi8 (b00, g00);
        //_mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (temp1, temp)); // hi argbargb lo
        //_mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (temp1, temp)); // hi argbargb hi

        //// row1
        //r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6),
                                //_mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6)); // rrrr.. saturated
        //g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6),
                                //_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6)); // gggg.. saturated
        //b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6),
                                //_mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6)); // bbbb.. saturated

        //temp = _mm_unpacklo_epi8 (r00, alpha); // arar..
        //temp1 = _mm_unpacklo_epi8 (b00, g00);  // gbgb..
        //_mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (temp1, temp)); // lo argbargb lo
        //_mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (temp1, temp)); // lo argbargb hi

        //temp = _mm_unpackhi_epi8 (r00, alpha);
        //temp1 = _mm_unpackhi_epi8 (b00, g00);
        //_mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (temp1, temp)); // hi argbargb lo
        //_mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (temp1, temp)); // hi argbargb hi
        //}}}
        //}
      //}

    //mState = eLoaded;
    //}
  //}}}
#else
  //{{{
  void cVideoDecode::cFrame::setYuv420Bgra (int width, int height, uint8_t* buffer, int stride) {
    cLog::log (LOGERROR, "setNv12mfxBgra not implemented for linux");
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420Rgba (int width, int height, uint8_t* buffer, int stride) {
    cLog::log (LOGERROR, "setNv12mfxRgba not implemented  for linux");
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420PlanarRgba (int width, int height, uint8_t** data, int* linesize) {

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
#endif

// cVideoDecode
//{{{
cVideoDecode::~cVideoDecode() {

  for (auto frame : mFramePool)
    delete frame;

  mFramePool.clear();
  }
//}}}
//{{{
void cVideoDecode::clear (uint64_t pts) {

  for (auto frame : mFramePool)
    if ((frame->getState() != cFrame::eLoaded) || (frame->getPts() < pts))
      frame->clear();
  }
//}}}
//{{{
cVideoDecode::cFrame* cVideoDecode::findPlayFrame() {
// returns nearest frame within a 25fps frame of mPlayPts, nullptr if none

  uint64_t nearDist = 90000 / 25;

  cVideoDecode::cFrame* nearFrame = nullptr;
  for (auto frame : mFramePool) {
    if (frame->getState() == cVideoDecode::cFrame::eLoaded) {
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
cVideoDecode::cFrame* cVideoDecode::getFreeFrame (uint64_t pts) {
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

#ifdef _WIN32
// cMfxVideoDecode
//{{{
cMfxVideoDecode::cMfxVideoDecode (bool bgra) : cVideoDecode (bgra) {

  mfxVersion kMfxVersion = { 0,1 };
  mSession.Init (MFX_IMPL_AUTO, &kMfxVersion);
  }
//}}}
//{{{
cMfxVideoDecode::~cMfxVideoDecode() {

  MFXVideoDECODE_Close (mSession);

  for (auto surface : mSurfacePool)
    delete surface;
  }
//}}}
//{{{
void cMfxVideoDecode::decodeFrame (uint8_t* pes, unsigned int pesSize, int pesNumInChunk, uint64_t pts) {

  mBitstream.Data = pes;
  mBitstream.DataOffset = 0;
  mBitstream.DataLength = pesSize;
  mBitstream.MaxLength = pesSize;
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

  //cLog::log (LOGINFO, "cMfxVideoDecode PES " + dec(pesNumInChunk) +
  //                     " pts:" + getPtsString (pts) + " size:" + dec(pesSize));

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
        if (mBgra)
          frame->setYuv420Bgra (surface->Info.Width, surface->Info.Height, surface->Data.Y, surface->Data.Pitch);
        else
          frame->setYuv420Rgba (surface->Info.Width, surface->Info.Height, surface->Data.Y, surface->Data.Pitch);
        }
      }
    }
  }
//}}}
//{{{
mfxFrameSurface1* cMfxVideoDecode::getFreeSurface() {
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
#endif

// cFFmpegVideoDecode
//{{{
cFFmpegVideoDecode::cFFmpegVideoDecode (bool bgra) : cVideoDecode (bgra) {

  mAvParser = av_parser_init (AV_CODEC_ID_H264);
  mAvCodec = avcodec_find_decoder (AV_CODEC_ID_H264);
  mAvContext = avcodec_alloc_context3 (mAvCodec);
  avcodec_open2 (mAvContext, mAvCodec, NULL);
  }
//}}}
//{{{
cFFmpegVideoDecode::~cFFmpegVideoDecode() {

  if (mAvContext)
    avcodec_close (mAvContext);
  if (mAvParser)
    av_parser_close (mAvParser);
  }
//}}}
//{{{
void cFFmpegVideoDecode::decodeFrame (uint8_t* pes, unsigned int pesSize, int pesNumInChunk, uint64_t pts) {

  // ffmpeg doesn't maintain correct avFrame.pts, but does decode frames in presentation order
  if (pesNumInChunk == 0)
    mDecodePts = pts;

  //cLog::log (LOGINFO, "cFFmpegVideoDecode PES " + dec(pesNumInChunk) +
  //                     " pts:" + getPtsString (pts) + " decodePts:" + getPtsString (mDecodePts) + " size:" + dec(pesSize));

  AVPacket avPacket;
  av_init_packet (&avPacket);

  auto avFrame = av_frame_alloc();

  auto pesPtr = pes;
  auto pesLeft = pesSize;
  while (pesLeft) {
    auto bytesUsed = av_parser_parse2 (mAvParser, mAvContext,
                                       &avPacket.data, &avPacket.size,
                                       pesPtr, (int)pesLeft, pts, AV_NOPTS_VALUE, 0);
    avPacket.pts = pts;

    pesPtr += bytesUsed;
    pesLeft -= bytesUsed;
    if (avPacket.size) {
      //cLog::log (LOGINFO, "decode  start");
      auto ret = avcodec_send_packet (mAvContext, &avPacket);
      while (ret >= 0) {
        ret = avcodec_receive_frame (mAvContext, avFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
          break;

        mWidth = avFrame->width;
        mHeight = avFrame->height;
        cVideoDecode::cFrame* frame = getFreeFrame (mDecodePts);
        //cLog::log (LOGINFO, "decode done");
        //frame->setNv12ffmpegSws (mWidth, mHeight, avFrame->data, avFrame->linesize);
        frame->setYuv420PlanarRgba (mWidth, mHeight, avFrame->data, avFrame->linesize);
        //cLog::log (LOGINFO, "yuv2rgb done");

        // fake pts from avContext framerate
        mDecodePts += (90000 * mAvContext->framerate.den) / mAvContext->framerate.num;
        }
      }
    }

  av_frame_free (&avFrame);
  }
//}}}
