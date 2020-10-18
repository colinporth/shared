// cVideoDecode.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cVideoDecode.h"
#include <thread>

// utils
#include "../date/date.h"
#include "utils.h"
#include "cLog.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

#if defined (__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
  #define INTEL_SSE2
  #define INTEL_SSSE3 1
  #include <emmintrin.h>
  #include <tmmintrin.h>
#else
  #include <arm_neon.h>
  //{{{
  void decode_yuv_neon (uint8_t* dst, uint8_t const* y, uint8_t const* uv, int width, int height, uint8_t fill_alpha) {

    // constants
    int const stride = width * 4;
    int const itHeight = height >> 1;
    int const itWidth = width >> 3;

    uint8x8_t const Yshift = vdup_n_u8 (16);
    int16x8_t const half = vdupq_n_u16 (128);
    int32x4_t const rounding = vdupq_n_s32 (128);

    // tmp variable
    uint16x8_t t;

    // pixel block to temporary store 8 pixels
    uint8x8x4_t block;
    block.val[3] = vdup_n_u8 (fill_alpha);

    for (int j = 0; j < itHeight; ++j, y += width, dst += stride) {
      for (int i = 0; i < itWidth; ++i, y += 8, uv += 8, dst += (8 * 4)) {
         t = vmovl_u8 (vqsub_u8 (vld1_u8 (y), Yshift));
         int32x4_t const Y00 = vmulq_n_u32 (vmovl_u16(vget_low_u16 (t)), 298);
         int32x4_t const Y01 = vmulq_n_u32 (vmovl_u16(vget_high_u16 (t)), 298);

         t = vmovl_u8 (vqsub_u8 (vld1_u8(y+width), Yshift));
         int32x4_t const Y10 = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (t)), 298);
         int32x4_t const Y11 = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (t)), 298);

         // loadvu pack 4 sets of uv into a uint8x8_t, layout : { v0,u0, v1,u1, v2,u2, v3,u3 }
         t = vsubq_s16 ((int16x8_t)vmovl_u8 (vld1_u8 (uv)), half);

         // UV.val[0] : v0, v1, v2, v3, UV.val[1] : u0, u1, u2, u3
         int16x4x2_t const UV = vuzp_s16 (vget_low_s16 (t), vget_high_s16 (t));

         // tR : 128+409V, tG : 128-100U-208V, tB : 128+516U
         int32x4_t const tR = vmlal_n_s16 (rounding, UV.val[0], 409);
         int32x4_t const tG = vmlal_n_s16 (vmlal_n_s16 (rounding, UV.val[0], -208), UV.val[1], -100);
         int32x4_t const tB = vmlal_n_s16 (rounding, UV.val[1], 516);

         int32x4x2_t const R = vzipq_s32 (tR, tR); // [tR0, tR0, tR1, tR1] [ tR2, tR2, tR3, tR3]
         int32x4x2_t const G = vzipq_s32 (tG, tG); // [tG0, tG0, tG1, tG1] [ tG2, tG2, tG3, tG3]
         int32x4x2_t const B = vzipq_s32 (tB, tB); // [tB0, tB0, tB1, tB1] [ tB2, tB2, tB3, tB3]

         // upper 8 pixels
         block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (B.val[0], Y00)),
                                                   vqmovun_s32 (vaddq_s32 (B.val[1], Y01))), 8);
         block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (G.val[0], Y00)),
                                                   vqmovun_s32 (vaddq_s32 (G.val[1], Y01))), 8),
         block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (R.val[0], Y00)),
                                                   vqmovun_s32 (vaddq_s32 (R.val[1], Y01))), 8),
         vst4_u8 (dst, block);

         // lower 8 pixels
         block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (B.val[0], Y10)),
                                                   vqmovun_s32 (vaddq_s32 (B.val[1], Y11))), 8);
         block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (G.val[0], Y10)),
                                                   vqmovun_s32 (vaddq_s32 (G.val[1], Y11))), 8),
         block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (R.val[0], Y10)),
                                                   vqmovun_s32 (vaddq_s32 (R.val[1], Y11))), 8),
         vst4_u8 (dst+stride, block);
         }
       }
    }
  //}}}
#endif

#ifdef _WIN32
  #include "../../libmfx/include/mfxvideo++.h"
#endif

using namespace std;
using namespace chrono;
//}}}
constexpr bool kTiming = true;
constexpr int kMaxFramePoolSize = 16;

// cVideoDecode::cFrame
//{{{
cVideoDecode::cFrame::~cFrame() {

  #ifdef _WIN32
    _aligned_free (mBuffer);
  #else
    free (mBuffer);
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

#if defined(INTEL_SSSE3)
  // uses shuffle
  //{{{
  void cVideoDecode::cFrame::setYuv420RgbaInterleaved (int width, int height, uint8_t* buffer, int stride) {
  // intel intrinsics ssse3 convert, fast

    system_clock::time_point timePoint = system_clock::now();

    allocateBuffer (width, height);

    // const
    __m128i zero  = _mm_set1_epi32 (0x00000000);
    __m128i ysub  = _mm_set1_epi32 (0x00100010);
    __m128i facy  = _mm_set1_epi32 (0x004a004a);

    __m128i mask0 = _mm_set_epi8 (-1,6,  -1,6,  -1,4,  -1,4,  -1,2,  -1,2,  -1,0, -1,0);
    __m128i mask1 = _mm_set_epi8 (-1,14, -1,14, -1,12, -1,12, -1,10, -1,10, -1,8, -1,8);
    __m128i mask2 = _mm_set_epi8 (-1,7,  -1,7,  -1,5,  -1,5,  -1,3,  -1,3,  -1,1, -1,1);
    __m128i mask3 = _mm_set_epi8 (-1,15, -1,15, -1,13, -1,13, -1,11, -1,11, -1,9, -1,9);

    __m128i uvsub = _mm_set1_epi32 (0x00800080);
    __m128i facrv = _mm_set1_epi32 (0x00660066);
    __m128i facgu = _mm_set1_epi32 (0x00190019);
    __m128i facgv = _mm_set1_epi32 (0x00340034);
    __m128i facbu = _mm_set1_epi32 (0x00810081);

    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    // buffer pointers
    __m128i* dstrgb128r0 = (__m128i*)mBuffer;
    __m128i* dstrgb128r1 = (__m128i*)(mBuffer + width);
    __m128i* srcY128r0 = (__m128i*)buffer;
    __m128i* srcY128r1 = (__m128i*)(buffer + stride);
    __m128* srcUv128 = (__m128*)(buffer + (height*stride));

    for (int y = 0; y < height; y += 2) {
      for (int x = 0; x < width; x += 16) {
        //{{{  process row
        // row01 u,v - u0 v0 u1 v1 u2 v2 u3 v3 u4 v4 u5 v5 u6 v6 u7 v7
        __m128i uv = _mm_load_si128 ((__m128i*)srcUv128++);

        // row01 u - align with y
        // - (0.u0 0.u0 0.u1 0.u1 0.u2 0.u2 0.u3 0.u3) - uvsub
        __m128i uLo = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask0), uvsub);

        // - (0.u4 0.u4 0.u5 0.u5 0.u6 0.u6 0.u7 0.u7) - uvsub
        __m128i uHi = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask1), uvsub);
        __m128i guLo = _mm_mullo_epi16 (facgu, uLo);
        __m128i guHi = _mm_mullo_epi16 (facgu, uHi);
        __m128i buLo = _mm_mullo_epi16 (facbu, uLo);
        __m128i buHi = _mm_mullo_epi16 (facbu, uHi);

        // row01 v - align with y
        // - (0.v0 0.v0 0.v1 0.v1 0.v2 0.v2 0.v3 0.v3) - uvsub
        __m128i vLo = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask2), uvsub);
        // - (0.v4 0.v4 0.v5 0.v5 0.v6 0.v6 0.v7 0.v7) - uvsub
        __m128i vHi = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask3), uvsub);
        __m128i rvLo = _mm_mullo_epi16 (facrv, vLo);
        __m128i rvHi = _mm_mullo_epi16 (facrv, vHi);
        __m128i gvLo = _mm_mullo_epi16 (facgv, vLo);
        __m128i gvHi = _mm_mullo_epi16 (facgv, vHi);

        // y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        __m128i y = _mm_load_si128 (srcY128r0++);
        // ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        __m128i yLo = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y, zero), ysub), facy);
        // ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy
        __m128i yHi = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y, zero), ysub), facy);

        // rrrr.. saturated
        __m128i r = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, rvLo), 6),
                                      _mm_srai_epi16 (_mm_add_epi16 (yHi, rvHi), 6));
         // gggg.. saturated
        __m128i g = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yLo, guLo), gvLo), 6),
                                      _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yHi, guHi), gvHi), 6));
        // bbbb.. saturated
        __m128i b = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, buLo), 6),
                                      _mm_srai_epi16 (_mm_add_epi16 (yHi, buHi), 6));

        __m128i abab = _mm_unpacklo_epi8 (b, alpha);
        __m128i grgr = _mm_unpacklo_epi8 (r, g);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

        abab = _mm_unpackhi_epi8 (b, alpha);
        grgr = _mm_unpackhi_epi8 (r, g);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

        //row 1
        // y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        y = _mm_load_si128 (srcY128r1++);
        // ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        yLo = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y, zero), ysub), facy);
        // ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy
        yHi = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y, zero), ysub), facy);

        // rrrr.. saturated
        r = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, rvLo), 6),
                              _mm_srai_epi16 (_mm_add_epi16 (yHi, rvHi), 6));
         // gggg.. saturated
        g = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yLo, guLo), gvLo), 6),
                              _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yHi, guHi), gvHi), 6));
        // bbbb.. saturated
        b = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, buLo), 6),
                              _mm_srai_epi16 (_mm_add_epi16 (yHi, buHi), 6));

        abab = _mm_unpacklo_epi8 (b, alpha);
        grgr = _mm_unpacklo_epi8 (r, g);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));

        abab = _mm_unpackhi_epi8 (b, alpha);
        grgr = _mm_unpackhi_epi8 (r, g);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));
        }

        //}}}

      // skip a y row
      dstrgb128r0 += width / 4;
      dstrgb128r1 += width / 4;
      srcY128r0 += stride / 16;
      srcY128r1 += stride / 16;
      }

    if (kTiming)
      cLog::log (LOGINFO1, "setYuv420RgbaInterleaved:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420BgraInterleaved (int width, int height, uint8_t* buffer, int stride) {
  // intel intrinsics ssse3 convert, fast

    system_clock::time_point timePoint = system_clock::now();

    allocateBuffer (width, height);

    // const
    __m128i zero  = _mm_set1_epi32 (0x00000000);
    __m128i ysub  = _mm_set1_epi32 (0x00100010);
    __m128i facy  = _mm_set1_epi32 (0x004a004a);

    __m128i mask0 = _mm_set_epi8 (-1,6,  -1,6,  -1,4,  -1,4,  -1,2,  -1,2,  -1,0, -1,0);
    __m128i mask1 = _mm_set_epi8 (-1,14, -1,14, -1,12, -1,12, -1,10, -1,10, -1,8, -1,8);
    __m128i mask2 = _mm_set_epi8 (-1,7,  -1,7,  -1,5,  -1,5,  -1,3,  -1,3,  -1,1, -1,1);
    __m128i mask3 = _mm_set_epi8 (-1,15, -1,15, -1,13, -1,13, -1,11, -1,11, -1,9, -1,9);

    __m128i uvsub = _mm_set1_epi32 (0x00800080);
    __m128i facrv = _mm_set1_epi32 (0x00660066);
    __m128i facgu = _mm_set1_epi32 (0x00190019);
    __m128i facgv = _mm_set1_epi32 (0x00340034);
    __m128i facbu = _mm_set1_epi32 (0x00810081);

    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    // buffer pointers
    __m128i* dstrgb128r0 = (__m128i*)mBuffer;
    __m128i* dstrgb128r1 = (__m128i*)(mBuffer + width);
    __m128i* srcY128r0 = (__m128i*)buffer;
    __m128i* srcY128r1 = (__m128i*)(buffer + stride);
    __m128* srcUv128 = (__m128*)(buffer + (height*stride));

    for (int y = 0; y < height; y += 2) {
      for (int x = 0; x < width; x += 16) {
        //{{{  process row
        //row0,1 uv
        // row01 u,v - u0 v0 u1 v1 u2 v2 u3 v3 u4 v4 u5 v5 u6 v6 u7 v7
        __m128i uv = _mm_load_si128 ((__m128i*)srcUv128++);

        // row01 u - align with y
        // - (0.u0 0.u0 0.u1 0.u1 0.u2 0.u2 0.u3 0.u3) - uvsub
        __m128i uLo = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask0), uvsub);
        // - (0.u4 0.u4 0.u5 0.u5 0.u6 0.u6 0.u7 0.u7) - uvsub
        __m128i uHi = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask1), uvsub);
        __m128i guLo = _mm_mullo_epi16 (facgu, uLo);
        __m128i guHi = _mm_mullo_epi16 (facgu, uHi);
        __m128i buLo = _mm_mullo_epi16 (facbu, uLo);
        __m128i buHi = _mm_mullo_epi16 (facbu, uHi);

        // row01 v - align with y
        // - (0.v0 0.v0 0.v1 0.v1 0.v2 0.v2 0.v3 0.v3) - uvsub
        __m128i vLo = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask2), uvsub);
        // - (0.v4 0.v4 0.v5 0.v5 0.v6 0.v6 0.v7 0.v7) - uvsub
        __m128i vHi = _mm_sub_epi16 (_mm_shuffle_epi8 (uv, mask3), uvsub);
        __m128i rvLo = _mm_mullo_epi16 (facrv, vLo);
        __m128i rvHi = _mm_mullo_epi16 (facrv, vHi);
        __m128i gvLo = _mm_mullo_epi16 (facgv, vLo);
        __m128i gvHi = _mm_mullo_epi16 (facgv, vHi);

        // row 0 - y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        __m128i y = _mm_load_si128 (srcY128r0++);
        // - ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        __m128i yLo = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y, zero), ysub), facy);
        // - ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy
        __m128i yHi = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y, zero), ysub), facy);

        // rrrr.. saturated
        __m128i r = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, rvLo), 6),
                                      _mm_srai_epi16 (_mm_add_epi16 (yHi, rvHi), 6));
         // gggg.. saturated
        __m128i g = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yLo, guLo), gvLo), 6),
                                      _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yHi, guHi), gvHi), 6));
        // bbbb.. saturated
        __m128i b = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, buLo), 6),
                                      _mm_srai_epi16 (_mm_add_epi16 (yHi, buHi), 6));

        __m128i arar = _mm_unpacklo_epi8 (r, alpha);
        __m128i gbgb = _mm_unpacklo_epi8 (b, g);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (gbgb, arar));

        arar = _mm_unpackhi_epi8 (r, alpha);
        gbgb = _mm_unpackhi_epi8 (b, g);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (gbgb, arar));

        // row 1 -  y0 y1 y2 y3 y4 y5 y6 y7 y8 y9 y10 y11 y12 y13 y14 y15
        y = _mm_load_si128 (srcY128r1++);
        // - ((0.y0 0.y1 0.y2  0.y3  0.y4  0.y5  0.y6  0.y7)  - ysub) * facy
        yLo = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y, zero), ysub), facy);
        // - ((0.y8 0.y9 0.y10 0.y11 0.y12 0.y13 0.y14 0.y15) - ysub) * facy
        yHi = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y, zero), ysub), facy);

        // rrrr.. saturated
        r = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, rvLo), 6),
                              _mm_srai_epi16 (_mm_add_epi16 (yHi, rvHi), 6));
         // gggg.. saturated
        g = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yLo, guLo), gvLo), 6),
                              _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (yHi, guHi), gvHi), 6));
        // bbbb.. saturated
        b = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (yLo, buLo), 6),
                              _mm_srai_epi16 (_mm_add_epi16 (yHi, buHi), 6));

        arar = _mm_unpacklo_epi8 (r, alpha);
        gbgb = _mm_unpacklo_epi8 (b, g);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (gbgb, arar));

        arar = _mm_unpackhi_epi8 (r, alpha);
        gbgb = _mm_unpackhi_epi8 (b, g);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (gbgb, arar));
        }
        //}}}

      // skip a y row
      dstrgb128r0 += width / 4;
      dstrgb128r1 += width / 4;
      srcY128r0 += stride / 16;
      srcY128r1 += stride / 16;
      }

    if (kTiming)
      cLog::log (LOGINFO1, "setYuv420BgraInterleaved:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
#else
  //{{{
  void cVideoDecode::cFrame::setYuv420BgraInterleaved (int width, int height, uint8_t* buffer, int stride) {
    cLog::log (LOGERROR, "setYuv420BgraInterleaved not implemented for nonx86");
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420RgbaInterleaved (int width, int height, uint8_t* buffer, int stride) {
    cLog::log (LOGERROR, "setYuv420RgbaInterleaved not implemented  for nonx86");
    }
  //}}}
#endif

#if defined(INTEL_SSE2)
  //{{{
  void cVideoDecode::cFrame::setYuv420RgbaPlanar (int width, int height, uint8_t** data, int* linesize) {
  // intel intrinsics sse2 convert, fast, but sws is same sort of thing may be a bit faster in the loops

    system_clock::time_point timePoint = system_clock::now();

    allocateBuffer (width, height);

    uint8_t* yBuffer = data[0];
    uint8_t* uBuffer = data[1];
    uint8_t* vBuffer = data[2];

    int yStride = linesize[0];
    int uvStride = linesize[1];

    __m128i ysub  = _mm_set1_epi32 (0x00100010);
    __m128i uvsub = _mm_set1_epi32 (0x00800080);
    __m128i facy  = _mm_set1_epi32 (0x004a004a);
    __m128i facrv = _mm_set1_epi32 (0x00660066);
    __m128i facgu = _mm_set1_epi32 (0x00190019);
    __m128i facgv = _mm_set1_epi32 (0x00340034);
    __m128i facbu = _mm_set1_epi32 (0x00810081);
    __m128i zero  = _mm_set1_epi32 (0x00000000);
    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    // dst row pointers
    __m128i* dstrgb128r0 = (__m128i*)mBuffer;
    __m128i* dstrgb128r1 = (__m128i*)(mBuffer + width);

    for (int y = 0; y < height; y += 2) {
      // calc src row pointers
      __m128i* srcY128r0 = (__m128i*)(yBuffer + yStride*y);
      __m128i* srcY128r1 = (__m128i*)(yBuffer + yStride*y + yStride);
      __m64* srcU64 = (__m64*)(uBuffer + uvStride * (y/2));
      __m64* srcV64 = (__m64*)(vBuffer + uvStride * (y/2));

      for (int x = 0; x < width; x += 16) {
        //{{{  process row
        // row01 u
        __m128i temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcU64++), zero); // 0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
        __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
        __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

        // row01 v
        temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcV64++), zero); // 0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
        __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
        __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

        // row0
        temp = _mm_load_si128 (srcY128r0++);
        __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
        __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

        __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        __m128i r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6),
                                        _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6)); // rrrr.. saturated

        __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        __m128i g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6),
                                        _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6)); // gggg.. saturated

        __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
        __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
        __m128i b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6),
                                        _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6)); // bbbb.. saturated

        __m128i abab = _mm_unpacklo_epi8 (b00, alpha);
        __m128i grgr = _mm_unpacklo_epi8 (r00, g00);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

        abab = _mm_unpackhi_epi8 (b00, alpha);
        grgr = _mm_unpackhi_epi8 (r00, g00);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

        // row1
        temp = _mm_load_si128 (srcY128r1++);
        __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
        __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

        r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6),
                                _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6)); // rrrr.. saturated
        g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6),
                                _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6)); // gggg.. saturated
        b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6),
                                _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6)); // bbbb.. saturated

        abab = _mm_unpacklo_epi8 (b00, alpha);
        grgr = _mm_unpacklo_epi8 (r00, g00);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));

        abab = _mm_unpackhi_epi8 (b00, alpha);
        grgr = _mm_unpackhi_epi8 (r00, g00);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));
        }
        //}}}

      // skip a dst row
      dstrgb128r0 += width / 4;
      dstrgb128r1 += width / 4;
      }

    if (kTiming)
      cLog::log (LOGINFO1, "setYuv420RgbaPlanar:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420BgraPlanar (int width, int height, uint8_t** data, int* linesize) {
  // intel intrinsics sse2 convert, fast, but sws is same sort of thing may be a bit faster in the loops

    system_clock::time_point timePoint = system_clock::now();

    allocateBuffer (width, height);

    uint8_t* yBuffer = data[0];
    uint8_t* uBuffer = data[1];
    uint8_t* vBuffer = data[2];

    int yStride = linesize[0];
    int uvStride = linesize[1];

    __m128i ysub  = _mm_set1_epi32 (0x00100010);
    __m128i uvsub = _mm_set1_epi32 (0x00800080);
    __m128i facy  = _mm_set1_epi32 (0x004a004a);
    __m128i facrv = _mm_set1_epi32 (0x00660066);
    __m128i facgu = _mm_set1_epi32 (0x00190019);
    __m128i facgv = _mm_set1_epi32 (0x00340034);
    __m128i facbu = _mm_set1_epi32 (0x00810081);
    __m128i zero  = _mm_set1_epi32 (0x00000000);
    __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

    // dst row pointers
    __m128i* dstrgb128r0 = (__m128i*)mBuffer;
    __m128i* dstrgb128r1 = (__m128i*)(mBuffer + width);

    for (int y = 0; y < height; y += 2) {
      // calc src row pointers
      __m128i* srcY128r0 = (__m128i*)(yBuffer + yStride*y);
      __m128i* srcY128r1 = (__m128i*)(yBuffer + yStride*y + yStride);
      __m64* srcU64 = (__m64*)(uBuffer + uvStride * (y/2));
      __m64* srcV64 = (__m64*)(vBuffer + uvStride * (y/2));

      for (int x = 0; x < width; x += 16) {
        //{{{  process row
        // row01 u
        __m128i temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcU64++), zero); // 0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
        __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
        __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

        // row01 v
        temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcV64++), zero); // 0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
        __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
        __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

        // row0
        temp = _mm_load_si128 (srcY128r0++);
        __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
        __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

        __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        __m128i r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6),
                                        _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6)); // rrrr.. saturated

        __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        __m128i g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6),
                                        _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6)); // gggg.. saturated

        __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
        __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
        __m128i b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6),
                                        _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6)); // bbbb.. saturated

        __m128i arar = _mm_unpacklo_epi8 (r00, alpha);
        __m128i gbgb = _mm_unpacklo_epi8 (b00, g00);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (gbgb, arar));

        arar = _mm_unpackhi_epi8 (r00, alpha);
        gbgb = _mm_unpackhi_epi8 (b00, g00);
        _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (gbgb, arar));

        // row1
        temp = _mm_load_si128 (srcY128r1++);
        __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
        __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

        r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6),
                                _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6)); // rrrr.. saturated
        g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6),
                                _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6)); // gggg.. saturated
        b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6),
                                _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6)); // bbbb.. saturated

        arar = _mm_unpacklo_epi8 (r00, alpha);
        gbgb = _mm_unpacklo_epi8 (b00, g00);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (gbgb, arar));

        arar = _mm_unpackhi_epi8 (r00, alpha);
        gbgb = _mm_unpackhi_epi8 (b00, g00);
        _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (gbgb, arar));
        _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (gbgb, arar));
        }
        //}}}

      // skip a dst row
      dstrgb128r0 += width / 4;
      dstrgb128r1 += width / 4;
      }

    if (kTiming)
      cLog::log (LOGINFO1, "setYuv420BgraPlanar:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
#else
  //{{{
  void cVideoDecode::cFrame::setYuv420RgbaPlanar (int width, int height, uint8_t** data, int* linesize) {
  // ARM nenon covert, not quite working, seems slow to sws table lookup

    //{{{  convert to interleaved
    //uint8_t* uvBuffer = (uint8_t*)malloc ((width/2) * (height/2) * 2);
    //uint8_t* dst = uvBuffer;
    //uint8_t* srcu = data[1];
    //uint8_t* srcv = data[2];
    //for (int y = 0; y < height/2; y++) {
    //  for (int x = 0; x < width/2; x++) {
    //    *dst++ = *srcu++;
    //    *dst++ = *srcv++;
    //    }
    //  }
    //decode_yuv_neon ((uint8_t*)mBuffer, yBuffer, uvBuffer, width, height, 0xFF);
    //free (uvBuffer);
    //}}}
    system_clock::time_point timePoint = system_clock::now();

    allocateBuffer (width, height);

    uint8_t* y = (uint8_t*)data[0];
    uint8_t* u = (uint8_t*)data[1];
    uint8_t* v = (uint8_t*)data[2];
    uint8_t* dst = (uint8_t*)mBuffer;

    // constants
    int const stride = width * 4;
    int const itHeight = height >> 1;
    int const itWidth = width >> 3;

    uint8x8_t const Yshift = vdup_n_u8 (16);
    int16x8_t const half = vdupq_n_u16 (128);
    int32x4_t const rounding = vdupq_n_s32 (128);

    uint8x8x4_t block;
    block.val[3] = vdup_n_u8 (0xFF);

    for (int j = 0; j < itHeight; ++j, y += width, dst += stride)
      for (int i = 0; i < itWidth; ++i, y += 8, u += 4, v += 4, dst += (8 * 4)) {
        //{{{  process 8pix with ARM NEON intrinsics, 2 rows at a time
        // yrow0
        uint16x8_t t = vmovl_u8 (vqsub_u8 (vld1_u8 (y), Yshift));
        int32x4_t const Y00 = vmulq_n_u32 (vmovl_u16(vget_low_u16 (t)), 298);
        int32x4_t const Y01 = vmulq_n_u32 (vmovl_u16(vget_high_u16 (t)), 298);

        // yrow1
        t = vmovl_u8 (vqsub_u8 (vld1_u8(y+width), Yshift));
        int32x4_t const Y10 = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (t)), 298);
        int32x4_t const Y11 = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (t)), 298);

        // interleaved
        // loadvu pack 4 sets of uv into a uint8x8_t { v0,u0, v1,u1, v2,u2, v3,u3 }
        // uint16x8_t t = vsubq_s16 ((int16x8_t)vmovl_u8 (vld1_u8 (uv)), half);
        // UV.val[0] : v0, v1, v2, v3, UV.val[1] : u0, u1, u2, u3
        // int16x4x2_t const UV = vuzp_s16 (vget_low_s16 (t), vget_high_s16 (t));

        // load 4 sets of u  u0 u1 u2 u3 u4 u5 u6 u7
        uint16x8_t const u0123 = vsubq_s16 ((int16x8_t)vmovl_u8 (vld1_u8 (u)), half);
        // load 4 sets of v  v0 v1 v2 v3 v4 v5 v6 v7
        uint16x8_t const v0123 = vsubq_s16 ((int16x8_t)vmovl_u8 (vld1_u8 (v)), half);

        // UV.val[0] v0 v1 v2 v3, UV.val[1] u0 u1 u2 u3
        int16x4x2_t const UV = vuzp_s16 (vget_low_s16 (u0123), vget_low_s16 (v0123));

        // tR 128+409V, tG 128-100U-208V, tB 128+516U
        int32x4_t const tR = vmlal_n_s16 (rounding, UV.val[0], 409);
        int32x4_t const tG = vmlal_n_s16 (vmlal_n_s16 (rounding, UV.val[0], -208), UV.val[1], -100);
        int32x4_t const tB = vmlal_n_s16 (rounding, UV.val[1], 516);

        int32x4x2_t const R = vzipq_s32 (tR, tR); // [tR0, tR0, tR1, tR1] [ tR2, tR2, tR3, tR3]
        int32x4x2_t const G = vzipq_s32 (tG, tG); // [tG0, tG0, tG1, tG1] [ tG2, tG2, tG3, tG3]
        int32x4x2_t const B = vzipq_s32 (tB, tB); // [tB0, tB0, tB1, tB1] [ tB2, tB2, tB3, tB3]

        // upper 8 pixels
        block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (B.val[0], Y00)),
                                                  vqmovun_s32 (vaddq_s32 (B.val[1], Y01))), 8);
        block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (G.val[0], Y00)),
                                                  vqmovun_s32 (vaddq_s32 (G.val[1], Y01))), 8),
        block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (R.val[0], Y00)),
                                                  vqmovun_s32 (vaddq_s32 (R.val[1], Y01))), 8),
        vst4_u8 (dst, block);

        // lower 8 pixels
        block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (B.val[0], Y10)),
                                                  vqmovun_s32 (vaddq_s32 (B.val[1], Y11))), 8);
        block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (G.val[0], Y10)),
                                                  vqmovun_s32 (vaddq_s32 (G.val[1], Y11))), 8),
        block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (R.val[0], Y10)),
                                                  vqmovun_s32 (vaddq_s32 (R.val[1], Y11))), 8),
        vst4_u8 (dst+stride, block);
        }
        //}}}

    if (kTiming)
      cLog::log (LOGINFO1, "setYuv420RgbaPlanar Neon:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420BgraPlanar (int width, int height, uint8_t** data, int* linesize) {
    cLog::log (LOGERROR, "setYuv420BgraPlanar not implemented for nonx86");
    }
  //}}}
#endif
//{{{
void cVideoDecode::cFrame::setYuv420RgbaPlanarSws (SwsContext* swsContext, int width, int height, uint8_t** data, int* linesize) {
// ffmpeg convert

  system_clock::time_point timePoint = system_clock::now();

  allocateBuffer (width, height);

  // convert yuv420Planar to rgba
  uint8_t* dstData[1] = { (uint8_t*)mBuffer };
  int dstStride[1] = { width * 4 };
  sws_scale (swsContext, data, linesize, 0, height, dstData, dstStride);

  if (kTiming)
    cLog::log (LOGINFO1, "setYuv420RgbaPlanarSws:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

  mState = eLoaded;
  }
//}}}
//{{{
void cVideoDecode::cFrame::setYuv420RgbaPlanarTable (int width, int height, uint8_t** data, int* linesize) {
// table lookup convert, bug in first pix pos stride != width

  constexpr uint32_t kFix = 0x40080100;
  //{{{
  static const uint32_t kTableY[256] = {
    0x7FFFFFEDU,
    0x7FFFFFEFU,
    0x7FFFFFF0U,
    0x7FFFFFF1U,
    0x7FFFFFF2U,
    0x7FFFFFF3U,
    0x7FFFFFF4U,
    0x7FFFFFF6U,
    0x7FFFFFF7U,
    0x7FFFFFF8U,
    0x7FFFFFF9U,
    0x7FFFFFFAU,
    0x7FFFFFFBU,
    0x7FFFFFFDU,
    0x7FFFFFFEU,
    0x7FFFFFFFU,
    0x80000000U,
    0x80400801U,
    0x80A01002U,
    0x80E01803U,
    0x81202805U,
    0x81803006U,
    0x81C03807U,
    0x82004008U,
    0x82604809U,
    0x82A0500AU,
    0x82E0600CU,
    0x8340680DU,
    0x8380700EU,
    0x83C0780FU,
    0x84208010U,
    0x84608811U,
    0x84A09813U,
    0x8500A014U,
    0x8540A815U,
    0x8580B016U,
    0x85E0B817U,
    0x8620C018U,
    0x8660D01AU,
    0x86C0D81BU,
    0x8700E01CU,
    0x8740E81DU,
    0x87A0F01EU,
    0x87E0F81FU,
    0x88210821U,
    0x88811022U,
    0x88C11823U,
    0x89012024U,
    0x89412825U,
    0x89A13026U,
    0x89E14028U,
    0x8A214829U,
    0x8A81502AU,
    0x8AC1582BU,
    0x8B01602CU,
    0x8B61682DU,
    0x8BA1782FU,
    0x8BE18030U,
    0x8C418831U,
    0x8C819032U,
    0x8CC19833U,
    0x8D21A034U,
    0x8D61B036U,
    0x8DA1B837U,
    0x8E01C038U,
    0x8E41C839U,
    0x8E81D03AU,
    0x8EE1D83BU,
    0x8F21E83DU,
    0x8F61F03EU,
    0x8FC1F83FU,
    0x90020040U,
    0x90420841U,
    0x90A21042U,
    0x90E22044U,
    0x91222845U,
    0x91823046U,
    0x91C23847U,
    0x92024048U,
    0x92624849U,
    0x92A2504AU,
    0x92E2604CU,
    0x9342684DU,
    0x9382704EU,
    0x93C2784FU,
    0x94228050U,
    0x94628851U,
    0x94A29853U,
    0x9502A054U,
    0x9542A855U,
    0x9582B056U,
    0x95E2B857U,
    0x9622C058U,
    0x9662D05AU,
    0x96C2D85BU,
    0x9702E05CU,
    0x9742E85DU,
    0x97A2F05EU,
    0x97E2F85FU,
    0x98230861U,
    0x98831062U,
    0x98C31863U,
    0x99032064U,
    0x99632865U,
    0x99A33066U,
    0x99E34068U,
    0x9A434869U,
    0x9A83506AU,
    0x9AC3586BU,
    0x9B23606CU,
    0x9B63686DU,
    0x9BA3786FU,
    0x9BE38070U,
    0x9C438871U,
    0x9C839072U,
    0x9CC39873U,
    0x9D23A074U,
    0x9D63B076U,
    0x9DA3B877U,
    0x9E03C078U,
    0x9E43C879U,
    0x9E83D07AU,
    0x9EE3D87BU,
    0x9F23E87DU,
    0x9F63F07EU,
    0x9FC3F87FU,
    0xA0040080U,
    0xA0440881U,
    0xA0A41082U,
    0xA0E42084U,
    0xA1242885U,
    0xA1843086U,
    0xA1C43887U,
    0xA2044088U,
    0xA2644889U,
    0xA2A4588BU,
    0xA2E4608CU,
    0xA344688DU,
    0xA384708EU,
    0xA3C4788FU,
    0xA4248090U,
    0xA4649092U,
    0xA4A49893U,
    0xA504A094U,
    0xA544A895U,
    0xA584B096U,
    0xA5E4B897U,
    0xA624C098U,
    0xA664D09AU,
    0xA6C4D89BU,
    0xA704E09CU,
    0xA744E89DU,
    0xA7A4F09EU,
    0xA7E4F89FU,
    0xA82508A1U,
    0xA88510A2U,
    0xA8C518A3U,
    0xA90520A4U,
    0xA96528A5U,
    0xA9A530A6U,
    0xA9E540A8U,
    0xAA4548A9U,
    0xAA8550AAU,
    0xAAC558ABU,
    0xAB2560ACU,
    0xAB6568ADU,
    0xABA578AFU,
    0xAC0580B0U,
    0xAC4588B1U,
    0xAC8590B2U,
    0xACE598B3U,
    0xAD25A0B4U,
    0xAD65B0B6U,
    0xADA5B8B7U,
    0xAE05C0B8U,
    0xAE45C8B9U,
    0xAE85D0BAU,
    0xAEE5D8BBU,
    0xAF25E8BDU,
    0xAF65F0BEU,
    0xAFC5F8BFU,
    0xB00600C0U,
    0xB04608C1U,
    0xB0A610C2U,
    0xB0E620C4U,
    0xB12628C5U,
    0xB18630C6U,
    0xB1C638C7U,
    0xB20640C8U,
    0xB26648C9U,
    0xB2A658CBU,
    0xB2E660CCU,
    0xB34668CDU,
    0xB38670CEU,
    0xB3C678CFU,
    0xB42680D0U,
    0xB46690D2U,
    0xB4A698D3U,
    0xB506A0D4U,
    0xB546A8D5U,
    0xB586B0D6U,
    0xB5E6B8D7U,
    0xB626C8D9U,
    0xB666D0DAU,
    0xB6C6D8DBU,
    0xB706E0DCU,
    0xB746E8DDU,
    0xB7A6F0DEU,
    0xB7E6F8DFU,
    0xB82708E1U,
    0xB88710E2U,
    0xB8C718E3U,
    0xB90720E4U,
    0xB96728E5U,
    0xB9A730E6U,
    0xB9E740E8U,
    0xBA4748E9U,
    0xBA8750EAU,
    0xBAC758EBU,
    0xBB2760ECU,
    0xBB6768EDU,
    0xBBA778EFU,
    0xBC0780F0U,
    0xBC4788F1U,
    0xBC8790F2U,
    0xBCE798F3U,
    0xBD27A0F4U,
    0xBD67B0F6U,
    0xBDC7B8F7U,
    0xBE07C0F8U,
    0xBE47C8F9U,
    0xBEA7D0FAU,
    0xBEE7D8FBU,
    0xBF27E8FDU,
    0xBF87F0FEU,
    0xBFC7F8FFU,
    0xC0080100U,
    0xC0480901U,
    0xC0A81102U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    0xC0E82104U,
    };
  //}}}
  //{{{
  static const uint32_t kTableU[256] = {
    0x0C400103U,
    0x0C200105U,
    0x0C200107U,
    0x0C000109U,
    0x0BE0010BU,
    0x0BC0010DU,
    0x0BA0010FU,
    0x0BA00111U,
    0x0B800113U,
    0x0B600115U,
    0x0B400117U,
    0x0B400119U,
    0x0B20011BU,
    0x0B00011DU,
    0x0AE0011FU,
    0x0AE00121U,
    0x0AC00123U,
    0x0AA00125U,
    0x0A800127U,
    0x0A600129U,
    0x0A60012BU,
    0x0A40012DU,
    0x0A20012FU,
    0x0A000131U,
    0x0A000132U,
    0x09E00134U,
    0x09C00136U,
    0x09A00138U,
    0x09A0013AU,
    0x0980013CU,
    0x0960013EU,
    0x09400140U,
    0x09400142U,
    0x09200144U,
    0x09000146U,
    0x08E00148U,
    0x08C0014AU,
    0x08C0014CU,
    0x08A0014EU,
    0x08800150U,
    0x08600152U,
    0x08600154U,
    0x08400156U,
    0x08200158U,
    0x0800015AU,
    0x0800015CU,
    0x07E0015EU,
    0x07C00160U,
    0x07A00162U,
    0x07A00164U,
    0x07800166U,
    0x07600168U,
    0x0740016AU,
    0x0720016CU,
    0x0720016EU,
    0x07000170U,
    0x06E00172U,
    0x06C00174U,
    0x06C00176U,
    0x06A00178U,
    0x0680017AU,
    0x0660017CU,
    0x0660017EU,
    0x06400180U,
    0x06200182U,
    0x06000184U,
    0x05E00185U,
    0x05E00187U,
    0x05C00189U,
    0x05A0018BU,
    0x0580018DU,
    0x0580018FU,
    0x05600191U,
    0x05400193U,
    0x05200195U,
    0x05200197U,
    0x05000199U,
    0x04E0019BU,
    0x04C0019DU,
    0x04C0019FU,
    0x04A001A1U,
    0x048001A3U,
    0x046001A5U,
    0x044001A7U,
    0x044001A9U,
    0x042001ABU,
    0x040001ADU,
    0x03E001AFU,
    0x03E001B1U,
    0x03C001B3U,
    0x03A001B5U,
    0x038001B7U,
    0x038001B9U,
    0x036001BBU,
    0x034001BDU,
    0x032001BFU,
    0x032001C1U,
    0x030001C3U,
    0x02E001C5U,
    0x02C001C7U,
    0x02A001C9U,
    0x02A001CBU,
    0x028001CDU,
    0x026001CFU,
    0x024001D1U,
    0x024001D3U,
    0x022001D5U,
    0x020001D7U,
    0x01E001D8U,
    0x01E001DAU,
    0x01C001DCU,
    0x01A001DEU,
    0x018001E0U,
    0x016001E2U,
    0x016001E4U,
    0x014001E6U,
    0x012001E8U,
    0x010001EAU,
    0x010001ECU,
    0x00E001EEU,
    0x00C001F0U,
    0x00A001F2U,
    0x00A001F4U,
    0x008001F6U,
    0x006001F8U,
    0x004001FAU,
    0x004001FCU,
    0x002001FEU,
    0x00000200U,
    0xFFE00202U,
    0xFFC00204U,
    0xFFC00206U,
    0xFFA00208U,
    0xFF80020AU,
    0xFF60020CU,
    0xFF60020EU,
    0xFF400210U,
    0xFF200212U,
    0xFF000214U,
    0xFF000216U,
    0xFEE00218U,
    0xFEC0021AU,
    0xFEA0021CU,
    0xFEA0021EU,
    0xFE800220U,
    0xFE600222U,
    0xFE400224U,
    0xFE200226U,
    0xFE200228U,
    0xFE000229U,
    0xFDE0022BU,
    0xFDC0022DU,
    0xFDC0022FU,
    0xFDA00231U,
    0xFD800233U,
    0xFD600235U,
    0xFD600237U,
    0xFD400239U,
    0xFD20023BU,
    0xFD00023DU,
    0xFCE0023FU,
    0xFCE00241U,
    0xFCC00243U,
    0xFCA00245U,
    0xFC800247U,
    0xFC800249U,
    0xFC60024BU,
    0xFC40024DU,
    0xFC20024FU,
    0xFC200251U,
    0xFC000253U,
    0xFBE00255U,
    0xFBC00257U,
    0xFBC00259U,
    0xFBA0025BU,
    0xFB80025DU,
    0xFB60025FU,
    0xFB400261U,
    0xFB400263U,
    0xFB200265U,
    0xFB000267U,
    0xFAE00269U,
    0xFAE0026BU,
    0xFAC0026DU,
    0xFAA0026FU,
    0xFA800271U,
    0xFA800273U,
    0xFA600275U,
    0xFA400277U,
    0xFA200279U,
    0xFA20027BU,
    0xFA00027CU,
    0xF9E0027EU,
    0xF9C00280U,
    0xF9A00282U,
    0xF9A00284U,
    0xF9800286U,
    0xF9600288U,
    0xF940028AU,
    0xF940028CU,
    0xF920028EU,
    0xF9000290U,
    0xF8E00292U,
    0xF8E00294U,
    0xF8C00296U,
    0xF8A00298U,
    0xF880029AU,
    0xF860029CU,
    0xF860029EU,
    0xF84002A0U,
    0xF82002A2U,
    0xF80002A4U,
    0xF80002A6U,
    0xF7E002A8U,
    0xF7C002AAU,
    0xF7A002ACU,
    0xF7A002AEU,
    0xF78002B0U,
    0xF76002B2U,
    0xF74002B4U,
    0xF74002B6U,
    0xF72002B8U,
    0xF70002BAU,
    0xF6E002BCU,
    0xF6C002BEU,
    0xF6C002C0U,
    0xF6A002C2U,
    0xF68002C4U,
    0xF66002C6U,
    0xF66002C8U,
    0xF64002CAU,
    0xF62002CCU,
    0xF60002CEU,
    0xF60002CFU,
    0xF5E002D1U,
    0xF5C002D3U,
    0xF5A002D5U,
    0xF5A002D7U,
    0xF58002D9U,
    0xF56002DBU,
    0xF54002DDU,
    0xF52002DFU,
    0xF52002E1U,
    0xF50002E3U,
    0xF4E002E5U,
    0xF4C002E7U,
    0xF4C002E9U,
    0xF4A002EBU,
    0xF48002EDU,
    0xF46002EFU,
    0xF46002F1U,
    0xF44002F3U,
    0xF42002F5U,
    0xF40002F7U,
    0xF3E002F9U,
    0xF3E002FBU,
    };
  //}}}
  //{{{
  static const uint32_t kTableV[256] = {
    0x1A09A000U,
    0x19E9A800U,
    0x19A9B800U,
    0x1969C800U,
    0x1949D000U,
    0x1909E000U,
    0x18C9E800U,
    0x18A9F800U,
    0x186A0000U,
    0x182A1000U,
    0x180A2000U,
    0x17CA2800U,
    0x17AA3800U,
    0x176A4000U,
    0x172A5000U,
    0x170A6000U,
    0x16CA6800U,
    0x168A7800U,
    0x166A8000U,
    0x162A9000U,
    0x160AA000U,
    0x15CAA800U,
    0x158AB800U,
    0x156AC000U,
    0x152AD000U,
    0x14EAE000U,
    0x14CAE800U,
    0x148AF800U,
    0x146B0000U,
    0x142B1000U,
    0x13EB2000U,
    0x13CB2800U,
    0x138B3800U,
    0x134B4000U,
    0x132B5000U,
    0x12EB6000U,
    0x12CB6800U,
    0x128B7800U,
    0x124B8000U,
    0x122B9000U,
    0x11EBA000U,
    0x11ABA800U,
    0x118BB800U,
    0x114BC000U,
    0x112BD000U,
    0x10EBE000U,
    0x10ABE800U,
    0x108BF800U,
    0x104C0000U,
    0x100C1000U,
    0x0FEC2000U,
    0x0FAC2800U,
    0x0F8C3800U,
    0x0F4C4000U,
    0x0F0C5000U,
    0x0EEC5800U,
    0x0EAC6800U,
    0x0E6C7800U,
    0x0E4C8000U,
    0x0E0C9000U,
    0x0DEC9800U,
    0x0DACA800U,
    0x0D6CB800U,
    0x0D4CC000U,
    0x0D0CD000U,
    0x0CCCD800U,
    0x0CACE800U,
    0x0C6CF800U,
    0x0C4D0000U,
    0x0C0D1000U,
    0x0BCD1800U,
    0x0BAD2800U,
    0x0B6D3800U,
    0x0B2D4000U,
    0x0B0D5000U,
    0x0ACD5800U,
    0x0AAD6800U,
    0x0A6D7800U,
    0x0A2D8000U,
    0x0A0D9000U,
    0x09CD9800U,
    0x098DA800U,
    0x096DB800U,
    0x092DC000U,
    0x090DD000U,
    0x08CDD800U,
    0x088DE800U,
    0x086DF800U,
    0x082E0000U,
    0x07EE1000U,
    0x07CE1800U,
    0x078E2800U,
    0x076E3800U,
    0x072E4000U,
    0x06EE5000U,
    0x06CE5800U,
    0x068E6800U,
    0x064E7800U,
    0x062E8000U,
    0x05EE9000U,
    0x05CE9800U,
    0x058EA800U,
    0x054EB800U,
    0x052EC000U,
    0x04EED000U,
    0x04AED800U,
    0x048EE800U,
    0x044EF000U,
    0x042F0000U,
    0x03EF1000U,
    0x03AF1800U,
    0x038F2800U,
    0x034F3000U,
    0x030F4000U,
    0x02EF5000U,
    0x02AF5800U,
    0x028F6800U,
    0x024F7000U,
    0x020F8000U,
    0x01EF9000U,
    0x01AF9800U,
    0x016FA800U,
    0x014FB000U,
    0x010FC000U,
    0x00EFD000U,
    0x00AFD800U,
    0x006FE800U,
    0x004FF000U,
    0x00100000U,
    0xFFD01000U,
    0xFFB01800U,
    0xFF702800U,
    0xFF303000U,
    0xFF104000U,
    0xFED05000U,
    0xFEB05800U,
    0xFE706800U,
    0xFE307000U,
    0xFE108000U,
    0xFDD09000U,
    0xFD909800U,
    0xFD70A800U,
    0xFD30B000U,
    0xFD10C000U,
    0xFCD0D000U,
    0xFC90D800U,
    0xFC70E800U,
    0xFC30F000U,
    0xFBF10000U,
    0xFBD11000U,
    0xFB911800U,
    0xFB712800U,
    0xFB313000U,
    0xFAF14000U,
    0xFAD14800U,
    0xFA915800U,
    0xFA516800U,
    0xFA317000U,
    0xF9F18000U,
    0xF9D18800U,
    0xF9919800U,
    0xF951A800U,
    0xF931B000U,
    0xF8F1C000U,
    0xF8B1C800U,
    0xF891D800U,
    0xF851E800U,
    0xF831F000U,
    0xF7F20000U,
    0xF7B20800U,
    0xF7921800U,
    0xF7522800U,
    0xF7123000U,
    0xF6F24000U,
    0xF6B24800U,
    0xF6925800U,
    0xF6526800U,
    0xF6127000U,
    0xF5F28000U,
    0xF5B28800U,
    0xF5729800U,
    0xF552A800U,
    0xF512B000U,
    0xF4F2C000U,
    0xF4B2C800U,
    0xF472D800U,
    0xF452E800U,
    0xF412F000U,
    0xF3D30000U,
    0xF3B30800U,
    0xF3731800U,
    0xF3532800U,
    0xF3133000U,
    0xF2D34000U,
    0xF2B34800U,
    0xF2735800U,
    0xF2336800U,
    0xF2137000U,
    0xF1D38000U,
    0xF1B38800U,
    0xF1739800U,
    0xF133A800U,
    0xF113B000U,
    0xF0D3C000U,
    0xF093C800U,
    0xF073D800U,
    0xF033E000U,
    0xF013F000U,
    0xEFD40000U,
    0xEF940800U,
    0xEF741800U,
    0xEF342000U,
    0xEEF43000U,
    0xEED44000U,
    0xEE944800U,
    0xEE745800U,
    0xEE346000U,
    0xEDF47000U,
    0xEDD48000U,
    0xED948800U,
    0xED549800U,
    0xED34A000U,
    0xECF4B000U,
    0xECD4C000U,
    0xEC94C800U,
    0xEC54D800U,
    0xEC34E000U,
    0xEBF4F000U,
    0xEBB50000U,
    0xEB950800U,
    0xEB551800U,
    0xEB352000U,
    0xEAF53000U,
    0xEAB54000U,
    0xEA954800U,
    0xEA555800U,
    0xEA156000U,
    0xE9F57000U,
    0xE9B58000U,
    0xE9958800U,
    0xE9559800U,
    0xE915A000U,
    0xE8F5B000U,
    0xE8B5C000U,
    0xE875C800U,
    0xE855D800U,
    0xE815E000U,
    0xE7F5F000U,
    0xE7B60000U,
    0xE7760800U,
    0xE7561800U,
    0xE7162000U,
    0xE6D63000U,
    0xE6B64000U,
    0xE6764800U,
    0xE6365800U
    };
  //}}}

  system_clock::time_point timePoint = system_clock::now();

  allocateBuffer (width, height);

  uint8_t* yPtr = (uint8_t*)data[0];
  uint8_t* uPtr = (uint8_t*)data[1];
  uint8_t* vPtr = (uint8_t*)data[2];
  uint8_t* dstPtr = (uint8_t*)mBuffer;
  const int stride = linesize[0];

  uint8_t const* yPtr1 = yPtr + stride;
  uint8_t* dstPtr1 = dstPtr + (width * 4);
  const int dstRowInc = (width + width - stride) * 4;

  for (int y = 0; y < height; y += 2) {
    for (int x = 0; x < stride; x += 2) {
      uint32_t uv = kTableV[*uPtr++] | kTableU[*vPtr++];

      // row 0 pix 0
      uint32_t yuv = kTableY[*yPtr++] + uv;
      uint32_t fix = yuv & kFix;
      if (fix) {
        //{{{  fix
        fix -= fix >> 8;
        yuv |= fix;
        fix = kFix & ~(yuv >> 1);
        yuv += fix >> 8;
        }
        //}}}
      *dstPtr++ = yuv;
      *dstPtr++ = yuv >> 22;
      *dstPtr++ = yuv >> 11;
      *dstPtr++ = 0xFF;

      // row 0 pix 1
      yuv = kTableY[*yPtr++] + uv;
      fix = yuv & kFix;
      if (fix) {
        //{{{  fix
        fix -= fix >> 8;
        yuv |= fix;
        fix = kFix & ~(yuv >> 1);
        yuv += fix >> 8;
        }
        //}}}
      *dstPtr++ = yuv;
      *dstPtr++ = yuv >> 22;
      *dstPtr++ = yuv >> 11;
      *dstPtr++ = 0xFF;

      // row 1 pix 0
      yuv = kTableY[*yPtr1++] + uv;
      fix = yuv & kFix;
      if (fix) {
        //{{{  fix
        fix -= fix >> 8;
        yuv |= fix;
        fix = kFix & ~(yuv >> 1);
        yuv += fix >> 8;
        }
        //}}}
      *dstPtr1++ = yuv;
      *dstPtr1++ = yuv >> 22;
      *dstPtr1++ = yuv >> 11;
      *dstPtr1++ = 0xFF;

      // row 1 pix 1
      yuv = kTableY[*yPtr1++] + uv;
      fix = yuv & kFix;
      if (fix) {
        //{{{  fix
        fix -= fix >> 8;
        yuv |= fix;
        fix = kFix & ~(yuv >> 1);
        yuv += fix >> 8;
        }
        //}}}
      *dstPtr1++ = yuv;
      *dstPtr1++ = yuv >> 22;
      *dstPtr1++ = yuv >> 11;
      *dstPtr1++ = 0xFF;
      }

    dstPtr += dstRowInc;
    dstPtr1 += dstRowInc;
    yPtr += stride;
    yPtr1 += stride;
    }

  if (kTiming)
    cLog::log (LOGINFO1, "setYuv420RgbaPlanarTable :%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

  mState = eLoaded;
  }
//}}}
//{{{
void cVideoDecode::cFrame::setYuv420RgbaPlanarSimple (int width, int height, uint8_t** data, int* linesize) {
// fixed point convert

  system_clock::time_point timePoint = system_clock::now();

  allocateBuffer (width, height);

  uint8_t* y = (uint8_t*)data[0];
  uint8_t* u = (uint8_t*)data[1];
  uint8_t* v = (uint8_t*)data[2];
  uint8_t* dst = (uint8_t*)mBuffer;

  int const halfWidth = width >> 1;
  int const halfHeight = height >> 1;

  unsigned char const* y0 = y;
  uint8_t* dst0 = dst;
  for (int h = 0; h < halfHeight; ++h) {
    uint8_t const* y1 = y0 + width;
    uint8_t* dst1 = dst0 + width * 4;
    for (int w = 0; w < halfWidth; ++w) {
      // shift
      int Y00 = (*y0++) - 16;
      int Y01 = (*y0++) - 16;
      int Y10 = (*y1++) - 16;
      int Y11 = (*y1++) - 16;
      Y00 = (Y00 > 0) ? (298 * Y00) : 0;
      Y01 = (Y01 > 0) ? (298 * Y01) : 0;
      Y10 = (Y10 > 0) ? (298 * Y10) : 0;
      Y11 = (Y11 > 0) ? (298 * Y11) : 0;

      int V = (*u++) - 128;
      int U = (*v++) - 128;

      int tR = 128 + 409*V;
      int tG = 128 - 100*U - 208*V;
      int tB = 128 + 516*U;

      *dst0++ = (Y00 + tB > 0) ? (Y00+tB < 65535 ? (uint8_t)((Y00+tB) >> 8) : 0xff) : 0;
      *dst0++ = (Y00 + tG > 0) ? (Y00+tG < 65535 ? (uint8_t)((Y00+tG) >> 8) : 0xff) : 0;
      *dst0++ = (Y00 + tR > 0) ? (Y00+tR < 65535 ? (uint8_t)((Y00+tR) >> 8) : 0xff) : 0;
      *dst0++ = 0xFF;

      *dst0++ = (Y01 + tB > 0) ? (Y01+tB < 65535 ? (uint8_t)((Y01+tB) >> 8) : 0xff) : 0;
      *dst0++ = (Y01 + tG > 0) ? (Y01+tG < 65535 ? (uint8_t)((Y01+tG) >> 8) : 0xff) : 0;
      *dst0++ = (Y01 + tR > 0) ? (Y01+tR < 65535 ? (uint8_t)((Y01+tR) >> 8) : 0xff) : 0;
      *dst0++ = 0xFF;

      *dst1++ = (Y10 + tB > 0) ? (Y10+tB < 65535 ? (uint8_t)((Y10+tB) >> 8) : 0xff) : 0;
      *dst1++ = (Y10 + tG > 0) ? (Y10+tG < 65535 ? (uint8_t)((Y10+tG) >> 8) : 0xff) : 0;
      *dst1++ = (Y10 + tR > 0) ? (Y10+tR < 65535 ? (uint8_t)((Y10+tR) >> 8) : 0xff) : 0;
      *dst1++ = 0xFF;

      *dst1++ = (Y11 + tB > 0) ? (Y11+tB < 65535 ? (uint8_t)((Y11+tB) >> 8) : 0xff) : 0;
      *dst1++ = (Y11 + tG > 0) ? (Y11+tG < 65535 ? (uint8_t)((Y11+tG) >> 8) : 0xff) : 0;
      *dst1++ = (Y11 + tR > 0) ? (Y11+tR < 65535 ? (uint8_t)((Y11+tR) >> 8) : 0xff) : 0;
      *dst1++ = 0xFF;
      }

    y0 = y1;
    dst0 = dst1;
    }

  if (kTiming)
    cLog::log (LOGINFO1, "setYuv420RgbaPlanarSimple :%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

  mState = eLoaded;
  }
//}}}

//{{{
void cVideoDecode::cFrame::allocateBuffer (int width, int height) {

  if (!mBuffer) // allocate aligned buffer
    mBuffer = (uint32_t*)
    #ifdef _WIN32
      _aligned_malloc (width * height * 4, 128);
    #else
      aligned_alloc (128, width * height * 4);
    #endif
  }
//}}}

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

  while (true) {
    uint64_t playFramePts = mPlayPts - 2 * (90000 / 25);

    for (auto frame : mFramePool) {
      if ((frame->getState() == cFrame::eFree) ||
          ((frame->getState() == cFrame::eLoaded) && (frame->getPts() < playFramePts))) {
        // reuse frame before playPts minus a couple of frames
        frame->set (pts);
        return frame;
        }
      }

    if (mFramePool.size() >= kMaxFramePoolSize) {
      // free frame should come along in a frame
      std::this_thread::sleep_for (20ms);
      }
    else {
      // allocate new frame
      //cLog::log (LOGINFO, "allocate newFrame %d for %u at play:%u", mFramePool.size(), pts, mPlayPts);
      auto frame = new cFrame (pts);
      mFramePool.push_back (frame);
      return frame;
      }
    }

  // never gets here
  return 0;
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

  system_clock::time_point timePoint = system_clock::now();

  memset (&mBitstream, 0, sizeof(mfxBitstream));
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

  // decode video pes
  // - could be none or multiple frames
  // - returned by decode order, not presentation order
  //mfxStatus status = MFXVideoDECODE_Reset (mSession, &mVideoParams);
  mfxStatus status = MFX_ERR_NONE;
  while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
    mfxFrameSurface1* surface = nullptr;
    mfxSyncPoint syncDecode = nullptr;
    status = MFXVideoDECODE_DecodeFrameAsync (mSession, &mBitstream, getFreeSurface(), &surface, &syncDecode);
    if (status == MFX_ERR_NONE) {
      status = mSession.SyncOperation (syncDecode, 60000);
      if (status == MFX_ERR_NONE) {
        if (kTiming)
          cLog::log (LOGINFO1, "decodeFrame mfx:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

        auto frame = getFreeFrame (surface->Data.TimeStamp);
        if (mBgra)
          frame->setYuv420BgraInterleaved (surface->Info.Width, surface->Info.Height, surface->Data.Y, surface->Data.Pitch);
        else
          frame->setYuv420RgbaInterleaved (surface->Info.Width, surface->Info.Height, surface->Data.Y, surface->Data.Pitch);
        }
      }
    }
  }
//}}}
// private
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

  sws_freeContext (mSwsContext);
  }
//}}}
//{{{
void cFFmpegVideoDecode::decodeFrame (uint8_t* pes, unsigned int pesSize, int pesNumInChunk, uint64_t pts) {

  system_clock::time_point timePoint = system_clock::now();

  // ffmpeg doesn't maintain correct avFrame.pts, but does decode frames in presentation order
  if (pesNumInChunk == 0)
    mDecodePts = pts;

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
      auto ret = avcodec_send_packet (mAvContext, &avPacket);
      while (ret >= 0) {
        ret = avcodec_receive_frame (mAvContext, avFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
          break;
        if (kTiming)
          cLog::log (LOGINFO1, "decodeFrame FFmpeg:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

        mWidth = avFrame->width;
        mHeight = avFrame->height;
        cVideoDecode::cFrame* frame = getFreeFrame (mDecodePts);

        if (!mSwsContext)
          mSwsContext = sws_getContext (avFrame->width, avFrame->height, AV_PIX_FMT_YUV420P,
                                        avFrame->width, avFrame->height, AV_PIX_FMT_RGBA,
                                        SWS_BILINEAR, NULL, NULL, NULL);
        frame->setYuv420RgbaPlanarSws (mSwsContext, mWidth, mHeight, avFrame->data, avFrame->linesize);

        // same as sws for intel
        //if (mBgra)
        //  frame->setYuv420BgraPlanar (mWidth, mHeight, avFrame->data, avFrame->linesize);
        //else
        //  frame->setYuv420RgbaPlanar (mWidth, mHeight, avFrame->data, avFrame->linesize);
        // slower
        //frame->setYuv420RgbaPlanarSimple (mWidth, mHeight, avFrame->data, avFrame->linesize);
        //frame->setYuv420RgbaPlanarTable (mWidth, mHeight, avFrame->data, avFrame->linesize);

        av_frame_unref (avFrame);

        // fake pts from avContext framerate
        mDecodePts += (90000 * mAvContext->framerate.den) / mAvContext->framerate.num;
        }
      }
    }

  av_frame_free (&avFrame);
  }
//}}}
