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

//{{{
static void decode_yuv (uint8_t* dst, uint8_t const* y, uint8_t const* u,uint8_t const* v, int width, int height) {

  unsigned char const* y0 = y;
  int const halfHeight = height>>1;
  int const halfWidth = width>>1;

  int Y00, Y01, Y10, Y11;
  int V, U;
  int tR, tG, tB;
  for (int h = 0; h < halfHeight; ++h) {
    unsigned char const* y1 = y0 + width;
    unsigned char* dst0 = dst;
    unsigned char* dst1 = dst0 + width * 4;
    for (int w = 0; w < halfWidth; ++w) {
      // shift
      Y00 = (*y0++) - 16;  Y01 = (*y0++) - 16;
      Y10 = (*y1++) - 16;  Y11 = (*y1++) - 16;

      V = (*u++) - 128;
      U = (*v++) - 128;

      Y00 = (Y00 > 0) ? (298*Y00) : 0;
      Y01 = (Y01 > 0) ? (298*Y01) : 0;
      Y10 = (Y10 > 0) ? (298*Y10) : 0;
      Y11 = (Y11 > 0) ? (298*Y11) : 0;

      tR = 128 + 409*V;
      tG = 128 - 100*U - 208*V;
      tB = 128 + 516*U;

      *dst0++ = (Y00 + tR > 0) ? (Y00+tR < 65535 ? (uint8_t)((Y00+tR) >> 8):0xff) : 0;
      *dst0++ = (Y00 + tG > 0) ? (Y00+tG < 65535 ? (uint8_t)((Y00+tG) >> 8):0xff) : 0;
      *dst0++ = (Y00 + tB > 0) ? (Y00+tB < 65535 ? (uint8_t)((Y00+tB) >> 8):0xff):0;
      *dst0++ = 0xFF;
      *dst0++ = (Y01 + tR > 0) ? (Y01+tR < 65535 ? (uint8_t)((Y01+tR) >> 8):0xff) : 0;
      *dst0++ = (Y01 + tG > 0) ? (Y01+tG < 65535 ? (uint8_t)((Y01+tG) >> 8):0xff) : 0;
      *dst0++ = (Y01 + tB > 0) ? (Y01+tB < 65535 ? (uint8_t)((Y01+tB) >> 8):0xff) : 0;
      *dst0++ = 0xFF;

      *dst1++ = (Y10 + tR > 0) ? (Y10+tR < 65535 ? (uint8_t)((Y10+tR) >> 8) : 0xff) : 0;
      *dst1++ = (Y10 + tG > 0) ? (Y10+tG < 65535 ? (uint8_t)((Y10+tG) >> 8) : 0xff) : 0;
      *dst1++ = (Y10 + tB > 0) ? (Y10+tB < 65535 ? (uint8_t)((Y10+tB) >> 8) : 0xff) : 0;
      *dst1++ = 0xFF;
      *dst1++ = (Y11 + tR > 0) ? (Y11+tR < 65535 ? (uint8_t)((Y11+tR) >> 8) : 0xff) : 0;
      *dst1++ = (Y11 + tG > 0) ? (Y11+tG < 65535 ? (uint8_t)((Y11+tG) >> 8) : 0xff) : 0;
      *dst1++ = (Y11 + tB > 0) ? (Y11+tB < 65535 ? (uint8_t)((Y11+tB) >> 8) : 0xff) : 0;
      *dst1++ = 0xFF;
      }

    y0 = y1;
    dst0 = dst1;
    }
  }
//}}}

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
      cLog::log (LOGINFO, "setYuv420RgbaInterleaved:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420BgraInterleaved (int width, int height, uint8_t* buffer, int stride) {

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
      cLog::log (LOGINFO, "setYuv420BgraInterleaved:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

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
      cLog::log (LOGINFO, "setYuv420setYuv420RgbaPlanarPlanarRgba:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
  //{{{
  void cVideoDecode::cFrame::setYuv420BgraPlanar (int width, int height, uint8_t** data, int* linesize) {

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
      cLog::log (LOGINFO, "setYuv420BgraPlanar:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

    mState = eLoaded;
    }
  //}}}
#else
  //{{{
  void cVideoDecode::cFrame::setYuv420RgbaPlanar (int width, int height, uint8_t** data, int* linesize) {

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

    if (true)
      decode_yuv (dst, y, u, v, width, height);
    else {
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
      }

    if (kTiming)
      cLog::log (LOGINFO, "setYuv420RgbaPlanar - NEON:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

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

  system_clock::time_point timePoint = system_clock::now();

  allocateBuffer (width, height);

  // convert yuv420Planar to rgba
  uint8_t* dstData[1] = { (uint8_t*)mBuffer };
  int dstStride[1] = { width * 4 };
  sws_scale (swsContext, data, linesize, 0, height, dstData, dstStride);

  if (kTiming)
    cLog::log (LOGINFO, "setYuv420RgbaPlanarSws:%d", duration_cast<microseconds>(system_clock::now() - timePoint).count());

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

        //if (!mSwsContext)
        //  mSwsContext = sws_getContext (avFrame->width, avFrame->height, AV_PIX_FMT_YUV420P,
        //                                avFrame->width, avFrame->height, AV_PIX_FMT_RGBA,
        //                                SWS_BILINEAR, NULL, NULL, NULL);
        mWidth = avFrame->width;
        mHeight = avFrame->height;
        cVideoDecode::cFrame* frame = getFreeFrame (mDecodePts);

        if (mBgra)
          frame->setYuv420BgraPlanar (mWidth, mHeight, avFrame->data, avFrame->linesize);
        else
          frame->setYuv420RgbaPlanar (mWidth, mHeight, avFrame->data, avFrame->linesize);
        //frame->setYuv420RgbaPlanarSws (mSwsContext, mWidth, mHeight, avFrame->data, avFrame->linesize);

        av_frame_unref (avFrame);

        // fake pts from avContext framerate
        mDecodePts += (90000 * mAvContext->framerate.den) / mAvContext->framerate.num;
        }
      }
    }

  av_frame_free (&avFrame);
  }
//}}}
