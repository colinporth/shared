#include "aaccommon.h"
//{{{  const tables
//{{{
/* sample rates (table 4.5.1) */
const int sampRateTab[NUM_SAMPLE_RATES]  = {
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025,  8000
  };
//}}}
//{{{
/* max scalefactor band for prediction (main profile only) */
const int predSFBMax[NUM_SAMPLE_RATES]  = {
  33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
  };
//}}}

//{{{
/* channel mapping (table 1.6.3.4) (-1 = unknown, so need to determine mapping based on rules in 8.5.1) */
const int channelMapTab[NUM_DEF_CHAN_MAPS]  = {
  -1, 1, 2, 3, 4, 5, 6, 8
  };
//}}}
//{{{
/* number of channels in each element (SCE, CPE, etc.)
 * see AACElementID in aaccommon.h
 */
const int elementNumChans[NUM_ELEMENTS]  = {
  1, 2, 0, 1, 0, 0, 0, 0
  };
//}}}

/* scale factor band tables */
//{{{
/* total number of scale factor bands in one window */
const unsigned int /*char*/ sfBandTotalShort[NUM_SAMPLE_RATES]  = {
  12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
  };
//}}}
//{{{
const unsigned int /*char*/ sfBandTotalLong[NUM_SAMPLE_RATES]  = {
  41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
  };
//}}}
//{{{
const int sfBandTabShortOffset[NUM_SAMPLE_RATES]  = {
  0, 0, 0, 13, 13, 13, 28, 28, 44, 44, 44, 60
  };
//}}}
//{{{
const /*short*/ int sfBandTabShort[76]  = {
  /* short block 64, 88, 96 kHz [13] (tables 4.5.24, 4.5.26) */
  0,   4,   8,  12,  16,  20,  24,  32,  40,  48,  64,  92, 128,

  /* short block 32, 44, 48 kHz [15] (table 4.5.15) */
  0,   4,   8,  12,  16,  20,  28,  36,  44,  56,  68,  80,  96, 112, 128,

  /* short block 22, 24 kHz [16] (table 4.5.22) */
  0,   4,   8,  12,  16,  20,  24,  28,  36,  44,  52,  64,  76,  92, 108, 128,

  /* short block 11, 12, 16 kHz [16] (table 4.5.20) */
  0,   4,   8,  12,  16,  20,  24,  28,  32,  40,  48,  60,  72,  88, 108, 128,

  /* short block 8 kHz [16] (table 4.5.18) */
  0,   4,   8,  12,  16,  20,  24,  28,  36,  44,  52,  60,  72,  88, 108, 128
  };
//}}}
//{{{
const int sfBandTabLongOffset[NUM_SAMPLE_RATES]  = {
  0, 0, 42, 90, 90, 140, 192, 192, 240, 240, 240, 284
  };
//}}}
//{{{
const /*short*/ int sfBandTabLong[325]  = {
  /* long block 88, 96 kHz [42] (table 4.5.25) */
    0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,   52,
   56,  64,  72,  80,  88,  96, 108, 120, 132, 144, 156, 172, 188,  212,
  240, 276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024,

  /* long block 64 kHz [48] (table 4.5.13) */
    0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,   64,
   72,  80,  88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268, 304, 344,  384,
  424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824, 864, 904, 944, 984, 1024,

  /* long block 44, 48 kHz [50] (table 4.5.14) */
    0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  48,  56,  64,  72,   80,  88,
   96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384,  416, 448,
  480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 1024,

  /* long block 32 kHz [52] (table 4.5.16) */
    0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  48,  56,  64,  72,   80,  88,  96,
  108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416,  448, 480, 512,
  544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 992, 1024,

  /* long block 22, 24 kHz [48] (table 4.5.21) */
    0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  52,  60,  68,   76,
   84,  92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220, 240, 260,  284,
  308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704, 768, 832, 896, 960, 1024,

  /* long block 11, 12, 16 kHz [44] (table 4.5.19) */
    0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88, 100,  112, 124,
  136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320,  344, 368,
  396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024,

  /* long block 8 kHz [41] (table 4.5.17) */
    0,  12,  24,  36,  48,  60,  72,  84,  96, 108, 120, 132,  144, 156,
  172, 188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372,  396, 420,
  448, 476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
  };
//}}}

//{{{
/* pow(2, i/4.0) for i = [0,1,2,3], format = Q30 */
static const int pow14[4] = {
  0x40000000, 0x4c1bf829, 0x5a82799a, 0x6ba27e65
  };
//}}}
//{{{
/* pow(2, i/4.0) * pow(j, 4.0/3.0) for i = [0,1,2,3],  j = [0,1,2,...,15]
 * format = Q28 for j = [0-3], Q25 for j = [4-15]
 */
static const int pow43_14[4][16] = {
  {
  0x00000000, 0x10000000, 0x285145f3, 0x453a5cdb, /* Q28 */
  0x0cb2ff53, 0x111989d6, 0x15ce31c8, 0x1ac7f203, /* Q25 */
  0x20000000, 0x257106b9, 0x2b16b4a3, 0x30ed74b4, /* Q25 */
  0x36f23fa5, 0x3d227bd3, 0x437be656, 0x49fc823c, /* Q25 */
  },
  {
  0x00000000, 0x1306fe0a, 0x2ff221af, 0x52538f52,
  0x0f1a1bf4, 0x1455ccc2, 0x19ee62a8, 0x1fd92396,
  0x260dfc14, 0x2c8694d8, 0x333dcb29, 0x3a2f5c7a,
  0x4157aed5, 0x48b3aaa3, 0x50409f76, 0x57fc3010,
  },
  {
  0x00000000, 0x16a09e66, 0x39047c0f, 0x61e734aa,
  0x11f59ac4, 0x182ec633, 0x1ed66a45, 0x25dfc55a,
  0x2d413ccd, 0x34f3462d, 0x3cefc603, 0x4531ab69,
  0x4db4adf8, 0x56752054, 0x5f6fcfcd, 0x68a1eca1,
  },
  {
  0x00000000, 0x1ae89f99, 0x43ce3e4b, 0x746d57b2,
  0x155b8109, 0x1cc21cdc, 0x24ac1839, 0x2d0a479e,
  0x35d13f33, 0x3ef80748, 0x48775c93, 0x524938cd,
  0x5c68841d, 0x66d0df0a, 0x717e7bfe, 0x7c6e0305,
  },
  };
//}}}
//{{{
/* pow(j, 4.0 / 3.0) for j = [16,17,18,...,63], format = Q23 */
static const int pow43[48] = {
  0x1428a2fa, 0x15db1bd6, 0x1796302c, 0x19598d85,
  0x1b24e8bb, 0x1cf7fcfa, 0x1ed28af2, 0x20b4582a,
  0x229d2e6e, 0x248cdb55, 0x26832fda, 0x28800000,
  0x2a832287, 0x2c8c70a8, 0x2e9bc5d8, 0x30b0ff99,
  0x32cbfd4a, 0x34eca001, 0x3712ca62, 0x393e6088,
  0x3b6f47e0, 0x3da56717, 0x3fe0a5fc, 0x4220ed72,
  0x44662758, 0x46b03e7c, 0x48ff1e87, 0x4b52b3f3,
  0x4daaebfd, 0x5007b497, 0x5268fc62, 0x54ceb29c,
  0x5738c721, 0x59a72a59, 0x5c19cd35, 0x5e90a129,
  0x610b9821, 0x638aa47f, 0x660db90f, 0x6894c90b,
  0x6b1fc80c, 0x6daeaa0d, 0x70416360, 0x72d7e8b0,
  0x75722ef9, 0x78102b85, 0x7ab1d3ec, 0x7d571e09,
  };
//}}}
//}}}

//{{{  bitstream
//{{{
/**************************************************************************************
 * Function:    RefillBitstreamCache
 *
 * Description: read new data from bitstream buffer into 32-bit cache
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *
 * Outputs:     updated bitstream info struct
 *
 * Return:      none
 *
 * Notes:       only call when iCache is completely drained (resets bitOffset to 0)
 *              always loads 4 new bytes except when bsi->nBytes < 4 (end of buffer)
 *              stores data as big-endian in cache, regardless of machine endian-ness
 **************************************************************************************/
//Optimized for REV16, REV32 (FB)
inline static void RefillBitstreamCache (BitStreamInfo *bsi)
{
  int nBytes = bsi->nBytes;
  if (nBytes >= 4) {
    /* optimize for common case, independent of machine endian-ness */
    bsi->iCache  = (*bsi->bytePtr++) << 24;
    bsi->iCache |= (*bsi->bytePtr++) << 16;
    bsi->iCache |= (*bsi->bytePtr++) <<  8;
    bsi->iCache |= (*bsi->bytePtr++);

    bsi->cachedBits = 32;
    bsi->nBytes -= 4;
  } else {
    bsi->iCache = 0;
    while (nBytes--) {
      bsi->iCache |= (*bsi->bytePtr++);
      bsi->iCache <<= 8;
    }
    bsi->iCache <<= ((3 - bsi->nBytes)*8);
    bsi->cachedBits = 8*bsi->nBytes;
    bsi->nBytes = 0;
  }
}
//}}}

//{{{
/**************************************************************************************
 * Function:    SetBitstreamPointer
 *
 * Description: initialize bitstream reader
 *
 * Inputs:      pointer to BitStreamInfo struct
 *              number of bytes in bitstream
 *              pointer to byte-aligned buffer of data to read from
 *
 * Outputs:     initialized bitstream info struct
 *
 * Return:      none
 **************************************************************************************/
void SetBitstreamPointer (BitStreamInfo *bsi, int nBytes, unsigned char *buf)
{
  /* init bitstream */
  bsi->bytePtr = buf;
  bsi->iCache = 0;    /* 4-byte unsigned int */
  bsi->cachedBits = 0;  /* i.e. zero bits in cache */
  bsi->nBytes = nBytes;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    GetBits
 *
 * Description: get bits from bitstream, advance bitstream pointer
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              number of bits to get from bitstream
 *
 * Outputs:     updated bitstream info struct
 *
 * Return:      the next nBits bits of data from bitstream buffer
 *
 * Notes:       nBits must be in range [0, 31], nBits outside this range masked by 0x1f
 *              for speed, does not indicate error if you overrun bit buffer
 *              if nBits == 0, returns 0
 **************************************************************************************/
unsigned int GetBits (BitStreamInfo *bsi, int nBits)
{
  unsigned int data, lowBits;

  nBits &= 0x1f;              /* nBits mod 32 to avoid unpredictable results like >> by negative amount */
  data = bsi->iCache >> (31 - nBits);   /* unsigned >> so zero-extend */
  data >>= 1;               /* do as >> 31, >> 1 so that nBits = 0 works okay (returns 0) */
  bsi->iCache <<= nBits;          /* left-justify cache */
  bsi->cachedBits -= nBits;       /* how many bits have we drawn from the cache so far */

  /* if we cross an int boundary, refill the cache */
  if (bsi->cachedBits < 0) {
    lowBits = -bsi->cachedBits;
    RefillBitstreamCache(bsi);
    data |= bsi->iCache >> (32 - lowBits);    /* get the low-order bits */

    bsi->cachedBits -= lowBits;     /* how many bits have we drawn from the cache so far */
    bsi->iCache <<= lowBits;      /* left-justify cache */
  }

  return data;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    GetBitsNoAdvance
 *
 * Description: get bits from bitstream, do not advance bitstream pointer
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              number of bits to get from bitstream
 *
 * Outputs:     none (state of BitStreamInfo struct left unchanged)
 *
 * Return:      the next nBits bits of data from bitstream buffer
 *
 * Notes:       nBits must be in range [0, 31], nBits outside this range masked by 0x1f
 *              for speed, does not indicate error if you overrun bit buffer
 *              if nBits == 0, returns 0
 **************************************************************************************/
unsigned int GetBitsNoAdvance (BitStreamInfo *bsi, int nBits)
{
  unsigned char *buf;
  unsigned int data, iCache;
  signed int lowBits;

  nBits &= 0x1f;              /* nBits mod 32 to avoid unpredictable results like >> by negative amount */
  data = bsi->iCache >> (31 - nBits);   /* unsigned >> so zero-extend */
  data >>= 1;               /* do as >> 31, >> 1 so that nBits = 0 works okay (returns 0) */
  lowBits = nBits - bsi->cachedBits;    /* how many bits do we have left to read */

  /* if we cross an int boundary, read next bytes in buffer */
  if (lowBits > 0) {
    iCache = 0;
    buf = bsi->bytePtr;
    while (lowBits > 0) {
      iCache <<= 8;
      if (buf < bsi->bytePtr + bsi->nBytes)
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
/**************************************************************************************
 * Function:    AdvanceBitstream
 *
 * Description: move bitstream pointer ahead
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              number of bits to advance bitstream
 *
 * Outputs:     updated bitstream info struct
 *
 * Return:      none
 *
 * Notes:       generally used following GetBitsNoAdvance(bsi, maxBits)
 **************************************************************************************/
void AdvanceBitstream (BitStreamInfo *bsi, int nBits)
{
  nBits &= 0x1f;
  if (nBits > bsi->cachedBits) {
    nBits -= bsi->cachedBits;
    RefillBitstreamCache(bsi);
  }
  bsi->iCache <<= nBits;
  bsi->cachedBits -= nBits;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    CalcBitsUsed
 *
 * Description: calculate how many bits have been read from bitstream
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *              pointer to start of bitstream buffer
 *              bit offset into first byte of startBuf (0-7)
 *
 * Outputs:     none
 *
 * Return:      number of bits read from bitstream, as offset from startBuf:startOffset
 **************************************************************************************/
int CalcBitsUsed (BitStreamInfo *bsi, unsigned char *startBuf, int startOffset)
{
  int bitsUsed;

  bitsUsed  = (int)(bsi->bytePtr - startBuf) * 8;
  bitsUsed -= bsi->cachedBits;
  bitsUsed -= startOffset;

  return bitsUsed;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    ByteAlignBitstream
 *
 * Description: bump bitstream pointer to start of next byte
 *
 * Inputs:      pointer to initialized BitStreamInfo struct
 *
 * Outputs:     byte-aligned bitstream BitStreamInfo struct
 *
 * Return:      none
 *
 * Notes:       if bitstream is already byte-aligned, do nothing
 **************************************************************************************/
void ByteAlignBitstream (BitStreamInfo *bsi)
{
  int offset;

  offset = bsi->cachedBits & 0x07;
  AdvanceBitstream(bsi, offset);
}
//}}}
//}}}
//{{{  decode, huffman
#define APPLY_SIGN(v, s)    {(v) ^= ((signed int)(s) >> 31); (v) -= ((signed int)(s) >> 31);}

#define GET_QUAD_SIGNBITS(v)  (((unsigned int)(v) << 17) >> 29) /* bits 14-12, unsigned */
#define GET_QUAD_W(v)     (((signed int)(v) << 20) >>   29) /* bits 11-9, sign-extend */
#define GET_QUAD_X(v)     (((signed int)(v) << 23) >>   29) /* bits  8-6, sign-extend */
#define GET_QUAD_Y(v)     (((signed int)(v) << 26) >>   29) /* bits  5-3, sign-extend */
#define GET_QUAD_Z(v)     (((signed int)(v) << 29) >>   29) /* bits  2-0, sign-extend */

#define GET_PAIR_SIGNBITS(v)  (((unsigned int)(v) << 20) >> 30) /* bits 11-10, unsigned */
#define GET_PAIR_Y(v)     (((signed int)(v) << 22) >>   27) /* bits  9-5, sign-extend */
#define GET_PAIR_Z(v)     (((signed int)(v) << 27) >>   27) /* bits  4-0, sign-extend */

#define GET_ESC_SIGNBITS(v)   (((unsigned int)(v) << 18) >> 30) /* bits 13-12, unsigned */
#define GET_ESC_Y(v)      (((signed int)(v) << 20) >>   26) /* bits 11-6, sign-extend */
#define GET_ESC_Z(v)      (((signed int)(v) << 26) >>   26) /* bits  5-0, sign-extend */

//{{{
const HuffInfo huffTabSpecInfo[11]  = {
  /* table 0 not used */
  {11, {  1,  0,  0,  0,  8,  0, 24,  0, 24,  8, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0},   0},
  { 9, {  0,  0,  1,  1,  7, 24, 15, 19, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},  81},
  {16, {  1,  0,  0,  4,  2,  6,  3,  5, 15, 15,  8,  9,  3,  3,  5,  2,  0,  0,  0,  0}, 162},
  {12, {  0,  0,  0, 10,  6,  0,  9, 21,  8, 14, 11,  2,  0,  0,  0,  0,  0,  0,  0,  0}, 243},
  {13, {  1,  0,  0,  4,  4,  0,  4, 12, 12, 12, 18, 10,  4,  0,  0,  0,  0,  0,  0,  0}, 324},
  {11, {  0,  0,  0,  9,  0, 16, 13,  8, 23,  8,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0}, 405},
  {12, {  1,  0,  2,  1,  0,  4,  5, 10, 14, 15,  8,  4,  0,  0,  0,  0,  0,  0,  0,  0}, 486},
  {10, {  0,  0,  1,  5,  7, 10, 14, 15,  8,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, 550},
  {15, {  1,  0,  2,  1,  0,  4,  3,  8, 11, 20, 31, 38, 32, 14,  4,  0,  0,  0,  0,  0}, 614},
  {12, {  0,  0,  0,  3,  8, 14, 17, 25, 31, 41, 22,  8,  0,  0,  0,  0,  0,  0,  0,  0}, 783},
  {12, {  0,  0,  0,  2,  6,  7, 16, 59, 55, 95, 43,  6,  0,  0,  0,  0,  0,  0,  0,  0}, 952},
};
//}}}
//{{{
const signed short huffTabSpec[1241]  = {
  /* spectrum table 1 [81] (signed) */
  0x0000, 0x0200, 0x0e00, 0x0007, 0x0040, 0x0001, 0x0038, 0x0008, 0x01c0, 0x03c0, 0x0e40, 0x0039, 0x0078, 0x01c8, 0x000f, 0x0240,
  0x003f, 0x0fc0, 0x01f8, 0x0238, 0x0047, 0x0e08, 0x0009, 0x0208, 0x01c1, 0x0048, 0x0041, 0x0e38, 0x0201, 0x0e07, 0x0207, 0x0e01,
  0x01c7, 0x0278, 0x0e78, 0x03c8, 0x004f, 0x0079, 0x01c9, 0x01cf, 0x03f8, 0x0239, 0x007f, 0x0e48, 0x0e0f, 0x0fc8, 0x01f9, 0x03c1,
  0x03c7, 0x0e47, 0x0ff8, 0x01ff, 0x0049, 0x020f, 0x0241, 0x0e41, 0x0248, 0x0fc1, 0x0e3f, 0x0247, 0x023f, 0x0e39, 0x0fc7, 0x0e09,
  0x0209, 0x03cf, 0x0e79, 0x0e4f, 0x03f9, 0x0249, 0x0fc9, 0x027f, 0x0fcf, 0x0fff, 0x0279, 0x03c9, 0x0e49, 0x0e7f, 0x0ff9, 0x03ff,
  0x024f,
  /* spectrum table 2 [81] (signed) */
  0x0000, 0x0200, 0x0e00, 0x0001, 0x0038, 0x0007, 0x01c0, 0x0008, 0x0040, 0x01c8, 0x0e40, 0x0078, 0x000f, 0x0047, 0x0039, 0x0e07,
  0x03c0, 0x0238, 0x0fc0, 0x003f, 0x0208, 0x0201, 0x01c1, 0x0e08, 0x0041, 0x01f8, 0x0e01, 0x01c7, 0x0e38, 0x0240, 0x0048, 0x0009,
  0x0207, 0x0079, 0x0239, 0x0e78, 0x01cf, 0x03c8, 0x0247, 0x0209, 0x0e48, 0x01f9, 0x0248, 0x0e0f, 0x0ff8, 0x0e39, 0x03f8, 0x0278,
  0x03c1, 0x0e47, 0x0fc8, 0x0e09, 0x0fc1, 0x0fc7, 0x01ff, 0x020f, 0x023f, 0x007f, 0x0049, 0x0e41, 0x0e3f, 0x004f, 0x03c7, 0x01c9,
  0x0241, 0x03cf, 0x0e79, 0x03f9, 0x0fff, 0x0e4f, 0x0e49, 0x0249, 0x0fcf, 0x03c9, 0x0e7f, 0x0fc9, 0x027f, 0x03ff, 0x0ff9, 0x0279,
  0x024f,
  /* spectrum table 3 [81] (unsigned) */
  0x0000, 0x1200, 0x1001, 0x1040, 0x1008, 0x2240, 0x2009, 0x2048, 0x2041, 0x2208, 0x3049, 0x2201, 0x3248, 0x4249, 0x3209, 0x3241,
  0x1400, 0x1002, 0x200a, 0x2440, 0x3288, 0x2011, 0x3051, 0x2280, 0x304a, 0x3448, 0x1010, 0x2088, 0x2050, 0x1080, 0x2042, 0x2408,
  0x4289, 0x3089, 0x3250, 0x4251, 0x3281, 0x2210, 0x3211, 0x2081, 0x4449, 0x424a, 0x3441, 0x320a, 0x2012, 0x3052, 0x3488, 0x3290,
  0x2202, 0x2401, 0x3091, 0x2480, 0x4291, 0x3242, 0x3409, 0x4252, 0x4489, 0x2090, 0x308a, 0x3212, 0x3481, 0x3450, 0x3490, 0x3092,
  0x4491, 0x4451, 0x428a, 0x4292, 0x2082, 0x2410, 0x3282, 0x3411, 0x444a, 0x3442, 0x4492, 0x448a, 0x4452, 0x340a, 0x2402, 0x3482,
  0x3412,
  /* spectrum table 4 [81] (unsigned) */
  0x4249, 0x3049, 0x3241, 0x3248, 0x3209, 0x1200, 0x2240, 0x0000, 0x2009, 0x2208, 0x2201, 0x2048, 0x1001, 0x2041, 0x1008, 0x1040,
  0x4449, 0x4251, 0x4289, 0x424a, 0x3448, 0x3441, 0x3288, 0x3409, 0x3051, 0x304a, 0x3250, 0x3089, 0x320a, 0x3281, 0x3242, 0x3211,
  0x2440, 0x2408, 0x2280, 0x2401, 0x2042, 0x2088, 0x200a, 0x2050, 0x2081, 0x2202, 0x2011, 0x2210, 0x1400, 0x1002, 0x1080, 0x1010,
  0x4291, 0x4489, 0x4451, 0x4252, 0x428a, 0x444a, 0x3290, 0x3488, 0x3450, 0x3091, 0x3052, 0x3481, 0x308a, 0x3411, 0x3212, 0x4491,
  0x3282, 0x340a, 0x3442, 0x4292, 0x4452, 0x448a, 0x2090, 0x2480, 0x2012, 0x2410, 0x2082, 0x2402, 0x4492, 0x3092, 0x3490, 0x3482,
  0x3412,
  /* spectrum table 5 [81] (signed) */
  0x0000, 0x03e0, 0x0020, 0x0001, 0x001f, 0x003f, 0x03e1, 0x03ff, 0x0021, 0x03c0, 0x0002, 0x0040, 0x001e, 0x03df, 0x0041, 0x03fe,
  0x0022, 0x03c1, 0x005f, 0x03e2, 0x003e, 0x03a0, 0x0060, 0x001d, 0x0003, 0x03bf, 0x0023, 0x0061, 0x03fd, 0x03a1, 0x007f, 0x003d,
  0x03e3, 0x03c2, 0x0042, 0x03de, 0x005e, 0x03be, 0x007e, 0x03c3, 0x005d, 0x0062, 0x0043, 0x03a2, 0x03dd, 0x001c, 0x0380, 0x0081,
  0x0080, 0x039f, 0x0004, 0x009f, 0x03fc, 0x0024, 0x03e4, 0x0381, 0x003c, 0x007d, 0x03bd, 0x03a3, 0x03c4, 0x039e, 0x0082, 0x005c,
  0x0044, 0x0063, 0x0382, 0x03dc, 0x009e, 0x007c, 0x039d, 0x0383, 0x0064, 0x03a4, 0x0083, 0x009d, 0x03bc, 0x009c, 0x0384, 0x0084,
  0x039c,
  /* spectrum table 6 [81] (signed) */
  0x0000, 0x0020, 0x001f, 0x0001, 0x03e0, 0x0021, 0x03e1, 0x003f, 0x03ff, 0x005f, 0x0041, 0x03c1, 0x03df, 0x03c0, 0x03e2, 0x0040,
  0x003e, 0x0022, 0x001e, 0x03fe, 0x0002, 0x005e, 0x03c2, 0x03de, 0x0042, 0x03a1, 0x0061, 0x007f, 0x03e3, 0x03bf, 0x0023, 0x003d,
  0x03fd, 0x0060, 0x03a0, 0x001d, 0x0003, 0x0062, 0x03be, 0x03c3, 0x0043, 0x007e, 0x005d, 0x03dd, 0x03a2, 0x0063, 0x007d, 0x03bd,
  0x03a3, 0x003c, 0x03fc, 0x0081, 0x0381, 0x039f, 0x0024, 0x009f, 0x03e4, 0x001c, 0x0382, 0x039e, 0x0044, 0x03dc, 0x0380, 0x0082,
  0x009e, 0x03c4, 0x0080, 0x005c, 0x0004, 0x03bc, 0x03a4, 0x007c, 0x009d, 0x0064, 0x0083, 0x0383, 0x039d, 0x0084, 0x0384, 0x039c,
  0x009c,
  /* spectrum table 7 [64] (unsigned) */
  0x0000, 0x0420, 0x0401, 0x0821, 0x0841, 0x0822, 0x0440, 0x0402, 0x0861, 0x0823, 0x0842, 0x0460, 0x0403, 0x0843, 0x0862, 0x0824,
  0x0881, 0x0825, 0x08a1, 0x0863, 0x0844, 0x0404, 0x0480, 0x0882, 0x0845, 0x08a2, 0x0405, 0x08c1, 0x04a0, 0x0826, 0x0883, 0x0865,
  0x0864, 0x08a3, 0x0846, 0x08c2, 0x0827, 0x0866, 0x0406, 0x04c0, 0x0884, 0x08e1, 0x0885, 0x08e2, 0x08a4, 0x08c3, 0x0847, 0x08e3,
  0x08c4, 0x08a5, 0x0886, 0x0867, 0x04e0, 0x0407, 0x08c5, 0x08a6, 0x08e4, 0x0887, 0x08a7, 0x08e5, 0x08e6, 0x08c6, 0x08c7, 0x08e7,
  /* spectrum table 8 [64] (unsigned) */
  0x0821, 0x0841, 0x0420, 0x0822, 0x0401, 0x0842, 0x0000, 0x0440, 0x0402, 0x0861, 0x0823, 0x0862, 0x0843, 0x0863, 0x0881, 0x0824,
  0x0882, 0x0844, 0x0460, 0x0403, 0x0883, 0x0864, 0x08a2, 0x08a1, 0x0845, 0x0825, 0x08a3, 0x0865, 0x0884, 0x08a4, 0x0404, 0x0885,
  0x0480, 0x0846, 0x08c2, 0x08c1, 0x0826, 0x0866, 0x08c3, 0x08a5, 0x04a0, 0x08c4, 0x0405, 0x0886, 0x08e1, 0x08e2, 0x0847, 0x08c5,
  0x08e3, 0x0827, 0x08a6, 0x0867, 0x08c6, 0x08e4, 0x04c0, 0x0887, 0x0406, 0x08e5, 0x08e6, 0x08c7, 0x08a7, 0x04e0, 0x0407, 0x08e7,
  /* spectrum table 9 [169] (unsigned) */
  0x0000, 0x0420, 0x0401, 0x0821, 0x0841, 0x0822, 0x0440, 0x0402, 0x0861, 0x0842, 0x0823, 0x0460, 0x0403, 0x0843, 0x0862, 0x0824,
  0x0881, 0x0844, 0x0825, 0x0882, 0x0863, 0x0404, 0x0480, 0x08a1, 0x0845, 0x0826, 0x0864, 0x08a2, 0x08c1, 0x0883, 0x0405, 0x0846,
  0x04a0, 0x0827, 0x0865, 0x0828, 0x0901, 0x0884, 0x08a3, 0x08c2, 0x08e1, 0x0406, 0x0902, 0x0848, 0x0866, 0x0847, 0x0885, 0x0921,
  0x0829, 0x08e2, 0x04c0, 0x08a4, 0x08c3, 0x0903, 0x0407, 0x0922, 0x0868, 0x0886, 0x0867, 0x0408, 0x0941, 0x08c4, 0x0849, 0x08a5,
  0x0500, 0x04e0, 0x08e3, 0x0942, 0x0923, 0x0904, 0x082a, 0x08e4, 0x08c5, 0x08a6, 0x0888, 0x0887, 0x0869, 0x0961, 0x08a8, 0x0520,
  0x0905, 0x0943, 0x084a, 0x0409, 0x0962, 0x0924, 0x08c6, 0x0981, 0x0889, 0x0906, 0x082b, 0x0925, 0x0944, 0x08a7, 0x08e5, 0x084b,
  0x082c, 0x0982, 0x0963, 0x086a, 0x08a9, 0x08c7, 0x0907, 0x0964, 0x040a, 0x08e6, 0x0983, 0x0540, 0x0945, 0x088a, 0x08c8, 0x084c,
  0x0926, 0x0927, 0x088b, 0x0560, 0x08c9, 0x086b, 0x08aa, 0x0908, 0x08e8, 0x0985, 0x086c, 0x0965, 0x08e7, 0x0984, 0x0966, 0x0946,
  0x088c, 0x08e9, 0x08ab, 0x040b, 0x0986, 0x08ca, 0x0580, 0x0947, 0x08ac, 0x08ea, 0x0928, 0x040c, 0x0967, 0x0909, 0x0929, 0x0948,
  0x08eb, 0x0987, 0x08cb, 0x090b, 0x0968, 0x08ec, 0x08cc, 0x090a, 0x0949, 0x090c, 0x092a, 0x092b, 0x092c, 0x094b, 0x0989, 0x094a,
  0x0969, 0x0988, 0x096a, 0x098a, 0x098b, 0x094c, 0x096b, 0x096c, 0x098c,
  /* spectrum table 10 [169] (unsigned) */
  0x0821, 0x0822, 0x0841, 0x0842, 0x0420, 0x0401, 0x0823, 0x0862, 0x0861, 0x0843, 0x0863, 0x0440, 0x0402, 0x0844, 0x0882, 0x0824,
  0x0881, 0x0000, 0x0883, 0x0864, 0x0460, 0x0403, 0x0884, 0x0845, 0x08a2, 0x0825, 0x08a1, 0x08a3, 0x0865, 0x08a4, 0x0885, 0x08c2,
  0x0846, 0x08c3, 0x0480, 0x08c1, 0x0404, 0x0826, 0x0866, 0x08a5, 0x08c4, 0x0886, 0x08c5, 0x08e2, 0x0867, 0x0847, 0x08a6, 0x0902,
  0x08e3, 0x04a0, 0x08e1, 0x0405, 0x0901, 0x0827, 0x0903, 0x08e4, 0x0887, 0x0848, 0x08c6, 0x08e5, 0x0828, 0x0868, 0x0904, 0x0888,
  0x08a7, 0x0905, 0x08a8, 0x08e6, 0x08c7, 0x0922, 0x04c0, 0x08c8, 0x0923, 0x0869, 0x0921, 0x0849, 0x0406, 0x0906, 0x0924, 0x0889,
  0x0942, 0x0829, 0x08e7, 0x0907, 0x0925, 0x08e8, 0x0943, 0x08a9, 0x0944, 0x084a, 0x0941, 0x086a, 0x0926, 0x08c9, 0x0500, 0x088a,
  0x04e0, 0x0962, 0x08e9, 0x0963, 0x0946, 0x082a, 0x0961, 0x0927, 0x0407, 0x0908, 0x0945, 0x086b, 0x08aa, 0x0909, 0x0965, 0x0408,
  0x0964, 0x084b, 0x08ea, 0x08ca, 0x0947, 0x088b, 0x082b, 0x0982, 0x0928, 0x0983, 0x0966, 0x08ab, 0x0984, 0x0967, 0x0985, 0x086c,
  0x08cb, 0x0520, 0x0948, 0x0540, 0x0981, 0x0409, 0x088c, 0x0929, 0x0986, 0x084c, 0x090a, 0x092a, 0x082c, 0x0968, 0x0987, 0x08eb,
  0x08ac, 0x08cc, 0x0949, 0x090b, 0x0988, 0x040a, 0x08ec, 0x0560, 0x094a, 0x0969, 0x096a, 0x040b, 0x096b, 0x092b, 0x094b, 0x0580,
  0x090c, 0x0989, 0x094c, 0x092c, 0x096c, 0x098b, 0x040c, 0x098a, 0x098c,
  /* spectrum table 11 [289] (unsigned) */
  0x0000, 0x2041, 0x2410, 0x1040, 0x1001, 0x2081, 0x2042, 0x2082, 0x2043, 0x20c1, 0x20c2, 0x1080, 0x2083, 0x1002, 0x20c3, 0x2101,
  0x2044, 0x2102, 0x2084, 0x2103, 0x20c4, 0x10c0, 0x1003, 0x2141, 0x2142, 0x2085, 0x2104, 0x2045, 0x2143, 0x20c5, 0x2144, 0x2105,
  0x2182, 0x2086, 0x2181, 0x2183, 0x20c6, 0x2046, 0x2110, 0x20d0, 0x2405, 0x2403, 0x2404, 0x2184, 0x2406, 0x1100, 0x2106, 0x1004,
  0x2090, 0x2145, 0x2150, 0x2407, 0x2402, 0x2408, 0x2087, 0x21c2, 0x20c7, 0x2185, 0x2146, 0x2190, 0x240a, 0x21c3, 0x21c1, 0x2409,
  0x21d0, 0x2050, 0x2047, 0x2107, 0x240b, 0x21c4, 0x240c, 0x2210, 0x2401, 0x2186, 0x2250, 0x2088, 0x2147, 0x2290, 0x240d, 0x2203,
  0x2202, 0x20c8, 0x1140, 0x240e, 0x22d0, 0x21c5, 0x2108, 0x2187, 0x21c6, 0x1005, 0x2204, 0x240f, 0x2310, 0x2048, 0x2201, 0x2390,
  0x2148, 0x2350, 0x20c9, 0x2205, 0x21c7, 0x2089, 0x2206, 0x2242, 0x2243, 0x23d0, 0x2109, 0x2188, 0x1180, 0x2244, 0x2149, 0x2207,
  0x21c8, 0x2049, 0x2283, 0x1006, 0x2282, 0x2241, 0x2245, 0x210a, 0x208a, 0x2246, 0x20ca, 0x2189, 0x2284, 0x2208, 0x2285, 0x2247,
  0x22c3, 0x204a, 0x11c0, 0x2286, 0x21c9, 0x20cb, 0x214a, 0x2281, 0x210b, 0x22c2, 0x2342, 0x218a, 0x2343, 0x208b, 0x1400, 0x214b,
  0x22c5, 0x22c4, 0x2248, 0x21ca, 0x2209, 0x1010, 0x210d, 0x1007, 0x20cd, 0x22c6, 0x2341, 0x2344, 0x2303, 0x208d, 0x2345, 0x220a,
  0x218b, 0x2288, 0x2287, 0x2382, 0x2304, 0x204b, 0x210c, 0x22c1, 0x20cc, 0x204d, 0x2302, 0x21cb, 0x20ce, 0x214c, 0x214d, 0x2384,
  0x210e, 0x22c7, 0x2383, 0x2305, 0x2346, 0x2306, 0x1200, 0x22c8, 0x208c, 0x2249, 0x2385, 0x218d, 0x228a, 0x23c2, 0x220b, 0x224a,
  0x2386, 0x2289, 0x214e, 0x22c9, 0x2381, 0x208e, 0x218c, 0x204c, 0x2348, 0x1008, 0x2347, 0x21cc, 0x2307, 0x21cd, 0x23c3, 0x2301,
  0x218e, 0x208f, 0x23c5, 0x23c4, 0x204e, 0x224b, 0x210f, 0x2387, 0x220d, 0x2349, 0x220c, 0x214f, 0x20cf, 0x228b, 0x22ca, 0x2308,
  0x23c6, 0x23c7, 0x220e, 0x23c1, 0x21ce, 0x1240, 0x1009, 0x224d, 0x224c, 0x2309, 0x2388, 0x228d, 0x2389, 0x230a, 0x218f, 0x21cf,
  0x224e, 0x23c8, 0x22cb, 0x22ce, 0x204f, 0x228c, 0x228e, 0x234b, 0x234a, 0x22cd, 0x22cc, 0x220f, 0x238b, 0x234c, 0x230d, 0x23c9,
  0x238a, 0x1280, 0x230b, 0x224f, 0x100a, 0x230c, 0x12c0, 0x230e, 0x228f, 0x234d, 0x100d, 0x238c, 0x23ca, 0x23cb, 0x22cf, 0x238d,
  0x1340, 0x100b, 0x234e, 0x23cc, 0x23cd, 0x230f, 0x1380, 0x238e, 0x234f, 0x1300, 0x238f, 0x100e, 0x100c, 0x23ce, 0x13c0, 0x100f,
  0x23cf,
};
//}}}
//{{{
const HuffInfo huffTabScaleFactInfo  =
  {19, { 1,  0,  1,  3,  2,  4,  3,  5,  4,  6,  6,  6,  5,  8,  4,  7,  3,  7, 46,  0},   0};

/* note - includes offset of -60 (4.6.2.3 in spec) */
const signed short huffTabScaleFact[121]  = {
    /* scale factor table [121] */
     0,   -1,    1,   -2,    2,   -3,    3,   -4,    4,   -5,    5,    6,   -6,    7,   -7,    8,
    -8,    9,   -9,   10,  -10,  -11,   11,   12,  -12,   13,  -13,   14,  -14,   16,   15,   17,
    18,  -15,  -17,  -16,   19,  -18,  -19,   20,  -20,   21,  -21,   22,  -22,   23,  -23,  -25,
    25,  -27,  -24,  -26,   24,  -28,   27,   29,  -30,  -29,   26,  -31,  -34,  -33,  -32,  -36,
    28,  -35,  -38,  -37,   30,  -39,  -41,  -57,  -59,  -58,  -60,   38,   39,   40,   41,   42,
    57,   37,   31,   32,   33,   34,   35,   36,   44,   51,   52,   53,   54,   55,   56,   50,
    45,   46,   47,   48,   49,   58,  -54,  -52,  -51,  -50,  -55,   43,   60,   59,  -56,  -53,
   -45,  -44,  -42,  -40,  -43,  -49,  -48,  -46,  -47,
};
//}}}

//{{{
/**************************************************************************************
 * Function:    DecodeHuffmanScalar
 *
 * Description: decode one Huffman symbol from bitstream
 *
 * Inputs:      pointers to Huffman table and info struct
 *              left-aligned bit buffer with >= huffTabInfo->maxBits bits
 *
 * Outputs:     decoded symbol in *val
 *
 * Return:      number of bits in symbol
 *
 * Notes:       assumes canonical Huffman codes:
 *                first CW always 0, we have "count" CW's of length "nBits" bits
 *                starting CW for codes of length nBits+1 =
 *                  (startCW[nBits] + count[nBits]) << 1
 *                if there are no codes at nBits, then we just keep << 1 each time
 *                  (since count[nBits] = 0)
 **************************************************************************************/
int DecodeHuffmanScalar (const signed short*huffTab, const HuffInfo* huffTabInfo, unsigned int bitBuf, signed int* val)
{
    unsigned int count, start, shift, t;
  const unsigned /*char*/ int *countPtr;
  const signed short *map;

  map = huffTab + huffTabInfo->offset;
  countPtr = huffTabInfo->count;

  start = 0;
  count = 0;
  shift = 32;
  do {
    start += count;
    start <<= 1;
    map += count;
    count = *countPtr++;
    shift--;
    t = (bitBuf >> shift) - start;
  } while (t >= count);
  *val = (signed int) (*(const uint16_t*)(&map[t]));
  return (int)(countPtr - huffTabInfo->count);
}
//}}}

//{{{
/**************************************************************************************
 * Function:    UnpackZeros
 *
 * Description: fill a section of coefficients with zeros
 *
 * Inputs:      number of coefficients
 *
 * Outputs:     nVals zeros, starting at coef
 *
 * Return:      none
 *
 * Notes:       assumes nVals is always a multiple of 4 because all scalefactor bands
 *                are a multiple of 4 coefficients long
 **************************************************************************************/
static void UnpackZeros (int nVals, int *coef)
{
  while (nVals > 0) {
    *coef++ = 0;
    *coef++ = 0;
    *coef++ = 0;
    *coef++ = 0;
    nVals -= 4;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    UnpackQuads
 *
 * Description: decode a section of 4-way vector Huffman coded coefficients
 *
 * Inputs       BitStreamInfo struct pointing to start of codewords for this section
 *              index of Huffman codebook
 *              number of coefficients
 *
 * Outputs:     nVals coefficients, starting at coef
 *
 * Return:      none
 *
 * Notes:       assumes nVals is always a multiple of 4 because all scalefactor bands
 *                are a multiple of 4 coefficients long
 **************************************************************************************/
static void UnpackQuads (BitStreamInfo* bsi, int cb, int nVals, int* coef)
{
  int w, x, y, z, maxBits, nCodeBits, nSignBits, val;
  unsigned int bitBuf;

  maxBits = huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET].maxBits + 4;
  while (nVals > 0) {
    /* decode quad */
    bitBuf = GetBitsNoAdvance(bsi, maxBits) << (32 - maxBits);
    nCodeBits = DecodeHuffmanScalar(huffTabSpec, &huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET], bitBuf, &val);

    w = GET_QUAD_W(val);
    x = GET_QUAD_X(val);
    y = GET_QUAD_Y(val);
    z = GET_QUAD_Z(val);

    bitBuf <<= nCodeBits;
    nSignBits = (int)GET_QUAD_SIGNBITS(val);
    AdvanceBitstream(bsi, nCodeBits + nSignBits);
    if (nSignBits) {
      if (w)  {APPLY_SIGN(w, bitBuf); bitBuf <<= 1;}
      if (x)  {APPLY_SIGN(x, bitBuf); bitBuf <<= 1;}
      if (y)  {APPLY_SIGN(y, bitBuf); bitBuf <<= 1;}
      if (z)  {APPLY_SIGN(z, bitBuf); bitBuf <<= 1;}
    }
    *coef++ = w; *coef++ = x; *coef++ = y; *coef++ = z;
    nVals -= 4;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    UnpackPairsNoEsc
 *
 * Description: decode a section of 2-way vector Huffman coded coefficients,
 *                using non-esc tables (5 through 10)
 *
 * Inputs       BitStreamInfo struct pointing to start of codewords for this section
 *              index of Huffman codebook (must not be the escape codebook)
 *              number of coefficients
 *
 * Outputs:     nVals coefficients, starting at coef
 *
 * Return:      none
 *
 * Notes:       assumes nVals is always a multiple of 2 because all scalefactor bands
 *                are a multiple of 4 coefficients long
 **************************************************************************************/
static void UnpackPairsNoEsc (BitStreamInfo* bsi, int cb, int nVals, int* coef)
{
  int y, z, maxBits, nCodeBits, nSignBits, val;
  unsigned int bitBuf;

  maxBits = huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET].maxBits + 2;
  while (nVals > 0) {
    /* decode pair */
    bitBuf = GetBitsNoAdvance(bsi, maxBits) << (32 - maxBits);
    nCodeBits = DecodeHuffmanScalar(huffTabSpec, &huffTabSpecInfo[cb-HUFFTAB_SPEC_OFFSET], bitBuf, &val);

    y = GET_PAIR_Y(val);
    z = GET_PAIR_Z(val);

    bitBuf <<= nCodeBits;
    nSignBits = GET_PAIR_SIGNBITS(val);
    AdvanceBitstream(bsi, nCodeBits + nSignBits);
    if (nSignBits) {
      if (y)  {APPLY_SIGN(y, bitBuf); bitBuf <<= 1;}
      if (z)  {APPLY_SIGN(z, bitBuf); bitBuf <<= 1;}
    }
    *coef++ = y; *coef++ = z;
    nVals -= 2;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    UnpackPairsEsc
 *
 * Description: decode a section of 2-way vector Huffman coded coefficients,
 *                using esc table (11)
 *
 * Inputs       BitStreamInfo struct pointing to start of codewords for this section
 *              index of Huffman codebook (must be the escape codebook)
 *              number of coefficients
 *
 * Outputs:     nVals coefficients, starting at coef
 *
 * Return:      none
 *
 * Notes:       assumes nVals is always a multiple of 2 because all scalefactor bands
 *                are a multiple of 4 coefficients long
 **************************************************************************************/
static void UnpackPairsEsc (BitStreamInfo* bsi, int cb, int nVals, int* coef)
{
  int y, z, maxBits, nCodeBits, nSignBits, n, val;
  unsigned int bitBuf;

  maxBits = huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET].maxBits + 2;
  while (nVals > 0) {
    /* decode pair with escape value */
    bitBuf = GetBitsNoAdvance(bsi, maxBits) << (32 - maxBits);
    nCodeBits = DecodeHuffmanScalar(huffTabSpec, &huffTabSpecInfo[cb-HUFFTAB_SPEC_OFFSET], bitBuf, &val);

    y = GET_ESC_Y(val);
    z = GET_ESC_Z(val);

    bitBuf <<= nCodeBits;
    nSignBits = GET_ESC_SIGNBITS(val);
    AdvanceBitstream(bsi, nCodeBits + nSignBits);

    if (y == 16) {
      n = 4;
      while (GetBits(bsi, 1) == 1)
        n++;
      y = (1 << n) + GetBits(bsi, n);
    }
    if (z == 16) {
      n = 4;
      while (GetBits(bsi, 1) == 1)
        n++;
      z = (1 << n) + GetBits(bsi, n);
    }

    if (nSignBits) {
      if (y)  {APPLY_SIGN(y, bitBuf); bitBuf <<= 1;}
      if (z)  {APPLY_SIGN(z, bitBuf); bitBuf <<= 1;}
    }

    *coef++ = y; *coef++ = z;
    nVals -= 2;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeSpectrumLong
 *
 * Description: decode transform coefficients for frame with one long block
 *
 * Inputs:      platform specific info struct
 *              BitStreamInfo struct pointing to start of spectral data
 *                (14496-3, table 4.4.29)
 *              index of current channel
 *
 * Outputs:     decoded, quantized coefficients for this channel
 *
 * Return:      none
 *
 * Notes:       adds in pulse data if present
 *              fills coefficient buffer with zeros in any region not coded with
 *                codebook in range [1, 11] (including sfb's above sfbMax)
 **************************************************************************************/
void DecodeSpectrumLong (PSInfoBase* psi, BitStreamInfo* bsi, int ch)
{
  int i, sfb, cb, nVals, offset;
  const /*short*/ int *sfbTab;
  unsigned char *sfbCodeBook;
  int *coef;
  ICSInfo *icsInfo;
  PulseInfo *pi;

  coef = psi->coef[ch];
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  /* decode long block */
  sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
  sfbCodeBook = psi->sfbCodeBook[ch];
  for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
    cb = *sfbCodeBook++;
    nVals = sfbTab[sfb+1] - sfbTab[sfb];

    if (cb == 0)
      UnpackZeros(nVals, coef);
    else if (cb <= 4)
      UnpackQuads(bsi, cb, nVals, coef);
    else if (cb <= 10)
      UnpackPairsNoEsc(bsi, cb, nVals, coef);
    else if (cb == 11)
      UnpackPairsEsc(bsi, cb, nVals, coef);
    else
      UnpackZeros(nVals, coef);

    coef += nVals;
  }

  /* fill with zeros above maxSFB */
  nVals = NSAMPS_LONG - sfbTab[sfb];
  UnpackZeros(nVals, coef);

  /* add pulse data, if present */
  pi = &psi->pulseInfo[ch];
  if (pi->pulseDataPresent) {
    coef = psi->coef[ch];
    offset = sfbTab[pi->startSFB];
    for (i = 0; i < pi->numPulse; i++) {
      offset += pi->offset[i];
      if (coef[offset] > 0)
        coef[offset] += pi->amp[i];
      else
        coef[offset] -= pi->amp[i];
    }
    ASSERT(offset < NSAMPS_LONG);
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeSpectrumShort
 *
 * Description: decode transform coefficients for frame with eight short blocks
 *
 * Inputs:      platform specific info struct
 *              BitStreamInfo struct pointing to start of spectral data
 *                (14496-3, table 4.4.29)
 *              index of current channel
 *
 * Outputs:     decoded, quantized coefficients for this channel
 *
 * Return:      none
 *
 * Notes:       fills coefficient buffer with zeros in any region not coded with
 *                codebook in range [1, 11] (including sfb's above sfbMax)
 *              deinterleaves window groups into 8 windows
 **************************************************************************************/
void DecodeSpectrumShort (PSInfoBase* psi, BitStreamInfo* bsi, int ch)
{
  int gp, cb, nVals=0, win, offset, sfb;
  const /*short*/ int *sfbTab;
  unsigned char *sfbCodeBook;
  int *coef;
  ICSInfo *icsInfo;

  coef = psi->coef[ch];
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  /* decode short blocks, deinterleaving in-place */
  sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
  sfbCodeBook = psi->sfbCodeBook[ch];
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
      nVals = sfbTab[sfb+1] - sfbTab[sfb];
      cb = *sfbCodeBook++;

      for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
        offset = win*NSAMPS_SHORT;
        if (cb == 0)
          UnpackZeros(nVals, coef + offset);
        else if (cb <= 4)
          UnpackQuads(bsi, cb, nVals, coef + offset);
        else if (cb <= 10)
          UnpackPairsNoEsc(bsi, cb, nVals, coef + offset);
        else if (cb == 11)
          UnpackPairsEsc(bsi, cb, nVals, coef + offset);
        else
          UnpackZeros(nVals, coef + offset);
      }
      coef += nVals;
    }

    /* fill with zeros above maxSFB */
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      offset = win*NSAMPS_SHORT;
      nVals = NSAMPS_SHORT - sfbTab[sfb];
      UnpackZeros(nVals, coef + offset);
    }
    coef += nVals;
    coef += (icsInfo->winGroupLen[gp] - 1)*NSAMPS_SHORT;
  }

  ASSERT(coef == psi->coef[ch] + NSAMPS_LONG);
}
//}}}

//{{{
/**************************************************************************************
 * Function:    DecodeSectionData
 *
 * Description: decode section data (scale factor band groupings and
 *                associated Huffman codebooks)
 *
 * Inputs:      BitStreamInfo struct pointing to start of ICS info
 *                (14496-3, table 4.4.25)
 *              window sequence (short or long blocks)
 *              number of window groups (1 for long blocks, 1-8 for short blocks)
 *              max coded scalefactor band
 *
 * Outputs:     index of Huffman codebook for each scalefactor band in each section
 *
 * Return:      none
 *
 * Notes:       sectCB, sectEnd, sfbCodeBook, ordered by window groups for short blocks
 **************************************************************************************/
static void DecodeSectionData (BitStreamInfo *bsi, int winSequence, int numWinGrp, int maxSFB, unsigned char *sfbCodeBook)
{
  int g, cb, sfb;
  int sectLen, sectLenBits, sectLenIncr, sectEscapeVal;

  sectLenBits = (winSequence == 2 ? 3 : 5);
  sectEscapeVal = (1 << sectLenBits) - 1;

  for (g = 0; g < numWinGrp; g++) {
    sfb = 0;
    while (sfb < maxSFB) {
      cb = GetBits(bsi, 4); /* next section codebook */
      sectLen = 0;
      do {
        sectLenIncr = GetBits(bsi, sectLenBits);
        sectLen += sectLenIncr;
      } while (sectLenIncr == sectEscapeVal);

      sfb += sectLen;
      while (sectLen--)
        *sfbCodeBook++ = (unsigned char)cb;
    }
    ASSERT(sfb == maxSFB);
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeOneScaleFactor
 *
 * Description: decode one scalefactor using scalefactor Huffman codebook
 *
 * Inputs:      BitStreamInfo struct pointing to start of next coded scalefactor
 *
 * Outputs:     updated BitstreamInfo struct
 *
 * Return:      one decoded scalefactor, including index_offset of -60
 **************************************************************************************/
static int DecodeOneScaleFactor (BitStreamInfo *bsi)
{
  int nBits, val;
  unsigned int bitBuf;

  /* decode next scalefactor from bitstream */
  bitBuf = GetBitsNoAdvance(bsi, huffTabScaleFactInfo.maxBits) << (32 - huffTabScaleFactInfo.maxBits);
  nBits = DecodeHuffmanScalar(huffTabScaleFact, &huffTabScaleFactInfo, bitBuf, &val);
  AdvanceBitstream(bsi, nBits);
  return val;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeScaleFactors
 *
 * Description: decode scalefactors, PNS energy, and intensity stereo weights
 *
 * Inputs:      BitStreamInfo struct pointing to start of ICS info
 *                (14496-3, table 4.4.26)
 *              number of window groups (1 for long blocks, 1-8 for short blocks)
 *              max coded scalefactor band
 *              global gain (starting value for differential scalefactor coding)
 *              index of Huffman codebook for each scalefactor band in each section
 *
 * Outputs:     decoded scalefactor for each section
 *
 * Return:      none
 *
 * Notes:       sfbCodeBook, scaleFactors ordered by window groups for short blocks
 *              for section with codebook 13, scaleFactors buffer has decoded PNS
 *                energy instead of regular scalefactor
 *              for section with codebook 14 or 15, scaleFactors buffer has intensity
 *                stereo weight instead of regular scalefactor
 **************************************************************************************/
static void DecodeScaleFactors (BitStreamInfo *bsi, int numWinGrp, int maxSFB, int globalGain,
                  unsigned char *sfbCodeBook, short *scaleFactors)
{
  int g, sfbCB, nrg, npf, val, sf, is;

  /* starting values for differential coding */
  sf = globalGain;
  is = 0;
  nrg = globalGain - 90 - 256;
  npf = 1;

  for (g = 0; g < numWinGrp * maxSFB; g++) {
    sfbCB = *sfbCodeBook++;

    if (sfbCB  == 14 || sfbCB == 15) {
      /* intensity stereo - differential coding */
      val = DecodeOneScaleFactor(bsi);
      is += val;
      *scaleFactors++ = (short)is;
    } else if (sfbCB == 13) {
      /* PNS - first energy is directly coded, rest are Huffman coded (npf = noise_pcm_flag) */
      if (npf) {
        val = GetBits(bsi, 9);
        npf = 0;
      } else {
        val = DecodeOneScaleFactor(bsi);
      }
      nrg += val;
      *scaleFactors++ = (short)nrg;
    } else if (sfbCB >= 1 && sfbCB <= 11) {
      /* regular (non-zero) region - differential coding */
      val = DecodeOneScaleFactor(bsi);
      sf += val;
      *scaleFactors++ = (short)sf;
    } else {
      /* inactive scalefactor band if codebook 0 */
      *scaleFactors++ = 0;
    }
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodePulseInfo
 *
 * Description: decode pulse information
 *
 * Inputs:      BitStreamInfo struct pointing to start of pulse info
 *                (14496-3, table 4.4.7)
 *
 * Outputs:     updated PulseInfo struct
 *
 * Return:      none
 **************************************************************************************/
static void DecodePulseInfo (BitStreamInfo *bsi, PulseInfo *pi)
{
  int i;

  pi->numPulse = GetBits(bsi, 2) + 1;   /* add 1 here */
  pi->startSFB = GetBits(bsi, 6);
  for (i = 0; i < pi->numPulse; i++) {
    pi->offset[i] = GetBits(bsi, 5);
    pi->amp[i] = GetBits(bsi, 4);
  }
}
//}}}

//{{{
/**************************************************************************************
 * Function:    DecodeICSInfo
 *
 * Description: decode individual channel stream info
 *
 * Inputs:      BitStreamInfo struct pointing to start of ICS info
 *                (14496-3, table 4.4.6)
 *              sample rate index
 *
 * Outputs:     updated icsInfo struct
 *
 * Return:      none
 **************************************************************************************/
void DecodeICSInfo (BitStreamInfo *bsi, ICSInfo *icsInfo, int sampRateIdx)
{
  int sfb, g, mask;

  icsInfo->icsResBit =      GetBits(bsi, 1);
  icsInfo->winSequence =    GetBits(bsi, 2);
  icsInfo->winShape =       GetBits(bsi, 1);
  if (icsInfo->winSequence == 2) {
    /* short block */
    icsInfo->maxSFB =     GetBits(bsi, 4);
    icsInfo->sfGroup =    GetBits(bsi, 7);
    icsInfo->numWinGroup =    1;
    icsInfo->winGroupLen[0] = 1;
    mask = 0x40;  /* start with bit 6 */
    for (g = 0; g < 7; g++) {
      if (icsInfo->sfGroup & mask)  {
        icsInfo->winGroupLen[icsInfo->numWinGroup - 1]++;
      } else {
        icsInfo->numWinGroup++;
        icsInfo->winGroupLen[icsInfo->numWinGroup - 1] = 1;
      }
      mask >>= 1;
    }
  } else {
    /* long block */
    icsInfo->maxSFB =               GetBits(bsi, 6);
    icsInfo->predictorDataPresent = GetBits(bsi, 1);
    if (icsInfo->predictorDataPresent) {
      icsInfo->predictorReset =   GetBits(bsi, 1);
      if (icsInfo->predictorReset)
        icsInfo->predictorResetGroupNum = GetBits(bsi, 5);
      for (sfb = 0; sfb < MIN(icsInfo->maxSFB, predSFBMax[sampRateIdx]); sfb++)
        icsInfo->predictionUsed[sfb] = GetBits(bsi, 1);
    }
    icsInfo->numWinGroup = 1;
    icsInfo->winGroupLen[0] = 1;
  }
}
//}}}
//}}}
//{{{  fft
#define NUM_FFT_SIZES 2
static const int nfftTab[NUM_FFT_SIZES] = {64, 512};
static const uint8_t nfftlog2Tab[NUM_FFT_SIZES] = {6, 9};

#define SQRT1_2 0x5a82799a  /* sqrt(1/2) in Q31 */
#define swapcplx(p0,p1) t = p0; t1 = *(&(p0)+1); p0 = p1; *(&(p0)+1) = *(&(p1)+1); p1 = t; *(&(p1)+1) = t1

//{{{
/* Twiddle tables for FFT
 * format = Q30
 *
 * for (k = 4; k <= N/4; k <<= 1) {
 *   for (j = 0; j < k; j++) {
 *     double wr1, wi1, wr2, wi2, wr3, wi3;
 *
 *     wr1 = cos(1.0 * M_PI * j / (2*k));
 *     wi1 = sin(1.0 * M_PI * j / (2*k));
 *     wr1 = (wr1 + wi1);
 *     wi1 = -wi1;
 *
 *     wr2 = cos(2.0 * M_PI * j / (2*k));
 *     wi2 = sin(2.0 * M_PI * j / (2*k));
 *     wr2 = (wr2 + wi2);
 *     wi2 = -wi2;
 *
 *     wr3 = cos(3.0 * M_PI * j / (2*k));
 *     wi3 = sin(3.0 * M_PI * j / (2*k));
 *     wr3 = (wr3 + wi3);
 *     wi3 = -wi3;
 *
 *     if (k & 0xaaaaaaaa) {
 *       w_odd[iodd++] = (float)wr2;
 *       w_odd[iodd++] = (float)wi2;
 *       w_odd[iodd++] = (float)wr1;
 *       w_odd[iodd++] = (float)wi1;
 *       w_odd[iodd++] = (float)wr3;
 *       w_odd[iodd++] = (float)wi3;
 *     } else {
 *       w_even[ieven++] = (float)wr2;
 *       w_even[ieven++] = (float)wi2;
 *       w_even[ieven++] = (float)wr1;
 *       w_even[ieven++] = (float)wi1;
 *       w_even[ieven++] = (float)wr3;
 *       w_even[ieven++] = (float)wi3;
 *     }
 *   }
 * }
 */
//}}}
//{{{
const int twidTabOdd[8*6 + 32*6 + 128*6]  = {
  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x539eba45, 0xe7821d59,
  0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59,
  0x539eba45, 0xc4df2862, 0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x22a2f4f8, 0xc4df2862,
  0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862,
  0xac6145bb, 0x187de2a7, 0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x45f704f7, 0xf9ba1651,
  0x43103085, 0xfcdc1342, 0x48b2b335, 0xf69bf7c9, 0x4b418bbe, 0xf383a3e2, 0x45f704f7, 0xf9ba1651,
  0x4fd288dc, 0xed6bf9d1, 0x4fd288dc, 0xed6bf9d1, 0x48b2b335, 0xf69bf7c9, 0x553805f2, 0xe4a2eff6,
  0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x569cc31b, 0xe1d4a2c8,
  0x4da1fab5, 0xf0730342, 0x5a6690ae, 0xd5052d97, 0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1,
  0x5a12e720, 0xce86ff2a, 0x5a12e720, 0xd76619b6, 0x51d1dc80, 0xea70658a, 0x57cc15bc, 0xc91af976,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x5a12e720, 0xce86ff2a,
  0x553805f2, 0xe4a2eff6, 0x4da1fab5, 0xc1eb0209, 0x58c542c5, 0xcac933ae, 0x569cc31b, 0xe1d4a2c8,
  0x45f704f7, 0xc04ee4b8, 0x569cc31b, 0xc78e9a1d, 0x57cc15bc, 0xdf18f0ce, 0x3cc85709, 0xc013bc39,
  0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060, 0x4fd288dc, 0xc2c17d52,
  0x5987b08a, 0xd9e01006, 0x26b2a794, 0xc3bdbdf6, 0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6,
  0x1a4608ab, 0xc78e9a1d, 0x45f704f7, 0xc04ee4b8, 0x5a6690ae, 0xd5052d97, 0x0d47d096, 0xcc983f70,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x396b3199, 0xc04ee4b8,
  0x5a6690ae, 0xd09441bb, 0xf2b82f6a, 0xd9e01006, 0x3248d382, 0xc13ad060, 0x5a12e720, 0xce86ff2a,
  0xe5b9f755, 0xe1d4a2c8, 0x2aaa7c7f, 0xc2c17d52, 0x5987b08a, 0xcc983f70, 0xd94d586c, 0xea70658a,
  0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x1a4608ab, 0xc78e9a1d,
  0x57cc15bc, 0xc91af976, 0xc337a8f7, 0xfcdc1342, 0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d,
  0xba08fb09, 0x0645e9af, 0x08df1a8c, 0xce86ff2a, 0x553805f2, 0xc6250a18, 0xb25e054b, 0x0f8cfcbe,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xf720e574, 0xd76619b6,
  0x51d1dc80, 0xc3bdbdf6, 0xa833ea44, 0x20e70f32, 0xee57aa21, 0xdc71898d, 0x4fd288dc, 0xc2c17d52,
  0xa5ed18e0, 0x2899e64a, 0xe5b9f755, 0xe1d4a2c8, 0x4da1fab5, 0xc1eb0209, 0xa5996f52, 0x2f6bbe45,
  0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52, 0xd5558381, 0xed6bf9d1,
  0x48b2b335, 0xc0b15502, 0xaac7fa0e, 0x39daf5e8, 0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8,
  0xb02d7724, 0x3d3e82ae, 0xc694ce67, 0xf9ba1651, 0x43103085, 0xc013bc39, 0xb74d4ccb, 0x3f4eaafe,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x418d2621, 0xfe6deaa1,
  0x40c7d2bd, 0xff36f170, 0x424ff28f, 0xfda4f351, 0x43103085, 0xfcdc1342, 0x418d2621, 0xfe6deaa1,
  0x4488e37f, 0xfb4ab7db, 0x4488e37f, 0xfb4ab7db, 0x424ff28f, 0xfda4f351, 0x46aa0d6d, 0xf8f21e8e,
  0x45f704f7, 0xf9ba1651, 0x43103085, 0xfcdc1342, 0x48b2b335, 0xf69bf7c9, 0x475a5c77, 0xf82a6c6a,
  0x43cdd89a, 0xfc135231, 0x4aa22036, 0xf4491311, 0x48b2b335, 0xf69bf7c9, 0x4488e37f, 0xfb4ab7db,
  0x4c77a88e, 0xf1fa3ecb, 0x49ffd417, 0xf50ef5de, 0x454149fc, 0xfa824bfd, 0x4e32a956, 0xefb047f2,
  0x4b418bbe, 0xf383a3e2, 0x45f704f7, 0xf9ba1651, 0x4fd288dc, 0xed6bf9d1, 0x4c77a88e, 0xf1fa3ecb,
  0x46aa0d6d, 0xf8f21e8e, 0x5156b6d9, 0xeb2e1dbe, 0x4da1fab5, 0xf0730342, 0x475a5c77, 0xf82a6c6a,
  0x52beac9f, 0xe8f77acf, 0x4ec05432, 0xeeee2d9d, 0x4807eb4b, 0xf7630799, 0x5409ed4b, 0xe6c8d59c,
  0x4fd288dc, 0xed6bf9d1, 0x48b2b335, 0xf69bf7c9, 0x553805f2, 0xe4a2eff6, 0x50d86e6d, 0xebeca36c,
  0x495aada2, 0xf5d544a7, 0x56488dc5, 0xe28688a4, 0x51d1dc80, 0xea70658a, 0x49ffd417, 0xf50ef5de,
  0x573b2635, 0xe0745b24, 0x52beac9f, 0xe8f77acf, 0x4aa22036, 0xf4491311, 0x580f7b19, 0xde6d1f65,
  0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x5471e2e6, 0xe61086bc,
  0x4bde1089, 0xf2beafed, 0x595c3e2a, 0xda8249b4, 0x553805f2, 0xe4a2eff6, 0x4c77a88e, 0xf1fa3ecb,
  0x59d438e5, 0xd8a00bae, 0x55f104dc, 0xe3399167, 0x4d0e4de2, 0xf136580d, 0x5a2d0957, 0xd6cb76c9,
  0x569cc31b, 0xe1d4a2c8, 0x4da1fab5, 0xf0730342, 0x5a6690ae, 0xd5052d97, 0x573b2635, 0xe0745b24,
  0x4e32a956, 0xefb047f2, 0x5a80baf6, 0xd34dcdb4, 0x57cc15bc, 0xdf18f0ce, 0x4ec05432, 0xeeee2d9d,
  0x5a7b7f1a, 0xd1a5ef90, 0x584f7b58, 0xddc29958, 0x4f4af5d1, 0xee2cbbc1, 0x5a56deec, 0xd00e2639,
  0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1, 0x5a12e720, 0xce86ff2a, 0x592d59da, 0xdb25f566,
  0x50570819, 0xecabef3d, 0x59afaf4c, 0xcd110216, 0x5987b08a, 0xd9e01006, 0x50d86e6d, 0xebeca36c,
  0x592d59da, 0xcbacb0bf, 0x59d438e5, 0xd8a00bae, 0x5156b6d9, 0xeb2e1dbe, 0x588c1404, 0xca5a86c4,
  0x5a12e720, 0xd76619b6, 0x51d1dc80, 0xea70658a, 0x57cc15bc, 0xc91af976, 0x5a43b190, 0xd6326a88,
  0x5249daa2, 0xe9b38223, 0x56eda1a0, 0xc7ee77b3, 0x5a6690ae, 0xd5052d97, 0x52beac9f, 0xe8f77acf,
  0x55f104dc, 0xc6d569be, 0x5a7b7f1a, 0xd3de9156, 0x53304df6, 0xe83c56cf, 0x54d69714, 0xc5d03118,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x5a7b7f1a, 0xd1a5ef90,
  0x5409ed4b, 0xe6c8d59c, 0x5249daa2, 0xc402a33c, 0x5a6690ae, 0xd09441bb, 0x5471e2e6, 0xe61086bc,
  0x50d86e6d, 0xc33aee27, 0x5a43b190, 0xcf89e3e8, 0x54d69714, 0xe55937d5, 0x4f4af5d1, 0xc2884e6e,
  0x5a12e720, 0xce86ff2a, 0x553805f2, 0xe4a2eff6, 0x4da1fab5, 0xc1eb0209, 0x59d438e5, 0xcd8bbb6d,
  0x55962bc0, 0xe3edb628, 0x4bde1089, 0xc1633f8a, 0x5987b08a, 0xcc983f70, 0x55f104dc, 0xe3399167,
  0x49ffd417, 0xc0f1360b, 0x592d59da, 0xcbacb0bf, 0x56488dc5, 0xe28688a4, 0x4807eb4b, 0xc0950d1d,
  0x58c542c5, 0xcac933ae, 0x569cc31b, 0xe1d4a2c8, 0x45f704f7, 0xc04ee4b8, 0x584f7b58, 0xc9edeb50,
  0x56eda1a0, 0xe123e6ad, 0x43cdd89a, 0xc01ed535, 0x57cc15bc, 0xc91af976, 0x573b2635, 0xe0745b24,
  0x418d2621, 0xc004ef3f, 0x573b2635, 0xc8507ea7, 0x57854ddd, 0xdfc606f1, 0x3f35b59d, 0xc0013bd3,
  0x569cc31b, 0xc78e9a1d, 0x57cc15bc, 0xdf18f0ce, 0x3cc85709, 0xc013bc39, 0x55f104dc, 0xc6d569be,
  0x580f7b19, 0xde6d1f65, 0x3a45e1f7, 0xc03c6a07, 0x553805f2, 0xc6250a18, 0x584f7b58, 0xddc29958,
  0x37af354c, 0xc07b371e, 0x5471e2e6, 0xc57d965d, 0x588c1404, 0xdd196538, 0x350536f1, 0xc0d00db6,
  0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060, 0x52beac9f, 0xc449d892,
  0x58fb0568, 0xdbcb0cce, 0x2f7afdfc, 0xc1bb5a11, 0x51d1dc80, 0xc3bdbdf6, 0x592d59da, 0xdb25f566,
  0x2c9caf6c, 0xc2517e31, 0x50d86e6d, 0xc33aee27, 0x595c3e2a, 0xda8249b4, 0x29aee694, 0xc2fd08a9,
  0x4fd288dc, 0xc2c17d52, 0x5987b08a, 0xd9e01006, 0x26b2a794, 0xc3bdbdf6, 0x4ec05432, 0xc2517e31,
  0x59afaf4c, 0xd93f4e9e, 0x23a8fb93, 0xc4935b3c, 0x4da1fab5, 0xc1eb0209, 0x59d438e5, 0xd8a00bae,
  0x2092f05f, 0xc57d965d, 0x4c77a88e, 0xc18e18a7, 0x59f54bee, 0xd8024d59, 0x1d719810, 0xc67c1e18,
  0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6, 0x1a4608ab, 0xc78e9a1d, 0x49ffd417, 0xc0f1360b,
  0x5a2d0957, 0xd6cb76c9, 0x17115bc0, 0xc8b4ab32, 0x48b2b335, 0xc0b15502, 0x5a43b190, 0xd6326a88,
  0x13d4ae08, 0xc9edeb50, 0x475a5c77, 0xc07b371e, 0x5a56deec, 0xd59afadb, 0x10911f04, 0xcb39edca,
  0x45f704f7, 0xc04ee4b8, 0x5a6690ae, 0xd5052d97, 0x0d47d096, 0xcc983f70, 0x4488e37f, 0xc02c64a6,
  0x5a72c63b, 0xd4710883, 0x09f9e6a1, 0xce0866b8, 0x43103085, 0xc013bc39, 0x5a7b7f1a, 0xd3de9156,
  0x06a886a0, 0xcf89e3e8, 0x418d2621, 0xc004ef3f, 0x5a80baf6, 0xd34dcdb4, 0x0354d741, 0xd11c3142,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x3e68fb62, 0xc004ef3f,
  0x5a80baf6, 0xd2317756, 0xfcab28bf, 0xd4710883, 0x3cc85709, 0xc013bc39, 0x5a7b7f1a, 0xd1a5ef90,
  0xf9577960, 0xd6326a88, 0x3b1e5335, 0xc02c64a6, 0x5a72c63b, 0xd11c3142, 0xf606195f, 0xd8024d59,
  0x396b3199, 0xc04ee4b8, 0x5a6690ae, 0xd09441bb, 0xf2b82f6a, 0xd9e01006, 0x37af354c, 0xc07b371e,
  0x5a56deec, 0xd00e2639, 0xef6ee0fc, 0xdbcb0cce, 0x35eaa2c7, 0xc0b15502, 0x5a43b190, 0xcf89e3e8,
  0xec2b51f8, 0xddc29958, 0x341dbfd3, 0xc0f1360b, 0x5a2d0957, 0xcf077fe1, 0xe8eea440, 0xdfc606f1,
  0x3248d382, 0xc13ad060, 0x5a12e720, 0xce86ff2a, 0xe5b9f755, 0xe1d4a2c8, 0x306c2624, 0xc18e18a7,
  0x59f54bee, 0xce0866b8, 0xe28e67f0, 0xe3edb628, 0x2e88013a, 0xc1eb0209, 0x59d438e5, 0xcd8bbb6d,
  0xdf6d0fa1, 0xe61086bc, 0x2c9caf6c, 0xc2517e31, 0x59afaf4c, 0xcd110216, 0xdc57046d, 0xe83c56cf,
  0x2aaa7c7f, 0xc2c17d52, 0x5987b08a, 0xcc983f70, 0xd94d586c, 0xea70658a, 0x28b1b544, 0xc33aee27,
  0x595c3e2a, 0xcc217822, 0xd651196c, 0xecabef3d, 0x26b2a794, 0xc3bdbdf6, 0x592d59da, 0xcbacb0bf,
  0xd3635094, 0xeeee2d9d, 0x24ada23d, 0xc449d892, 0x58fb0568, 0xcb39edca, 0xd0850204, 0xf136580d,
  0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x2092f05f, 0xc57d965d,
  0x588c1404, 0xca5a86c4, 0xcafac90f, 0xf5d544a7, 0x1e7de5df, 0xc6250a18, 0x584f7b58, 0xc9edeb50,
  0xc850cab4, 0xf82a6c6a, 0x1c6427a9, 0xc6d569be, 0x580f7b19, 0xc9836582, 0xc5ba1e09, 0xfa824bfd,
  0x1a4608ab, 0xc78e9a1d, 0x57cc15bc, 0xc91af976, 0xc337a8f7, 0xfcdc1342, 0x1823dc7d, 0xc8507ea7,
  0x57854ddd, 0xc8b4ab32, 0xc0ca4a63, 0xff36f170, 0x15fdf758, 0xc91af976, 0x573b2635, 0xc8507ea7,
  0xbe72d9df, 0x0192155f, 0x13d4ae08, 0xc9edeb50, 0x56eda1a0, 0xc7ee77b3, 0xbc322766, 0x03ecadcf,
  0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d, 0xba08fb09, 0x0645e9af, 0x0f7944a7, 0xcbacb0bf,
  0x56488dc5, 0xc730e997, 0xb7f814b5, 0x089cf867, 0x0d47d096, 0xcc983f70, 0x55f104dc, 0xc6d569be,
  0xb6002be9, 0x0af10a22, 0x0b145041, 0xcd8bbb6d, 0x55962bc0, 0xc67c1e18, 0xb421ef77, 0x0d415013,
  0x08df1a8c, 0xce86ff2a, 0x553805f2, 0xc6250a18, 0xb25e054b, 0x0f8cfcbe, 0x06a886a0, 0xcf89e3e8,
  0x54d69714, 0xc5d03118, 0xb0b50a2f, 0x11d3443f, 0x0470ebdc, 0xd09441bb, 0x5471e2e6, 0xc57d965d,
  0xaf279193, 0x14135c94, 0x0238a1c6, 0xd1a5ef90, 0x5409ed4b, 0xc52d3d18, 0xadb6255e, 0x164c7ddd,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xfdc75e3a, 0xd3de9156,
  0x53304df6, 0xc4935b3c, 0xab2968ec, 0x1aa6c82b, 0xfb8f1424, 0xd5052d97, 0x52beac9f, 0xc449d892,
  0xaa0efb24, 0x1cc66e99, 0xf9577960, 0xd6326a88, 0x5249daa2, 0xc402a33c, 0xa9125e60, 0x1edc1953,
  0xf720e574, 0xd76619b6, 0x51d1dc80, 0xc3bdbdf6, 0xa833ea44, 0x20e70f32, 0xf4ebafbf, 0xd8a00bae,
  0x5156b6d9, 0xc37b2b6a, 0xa773ebfc, 0x22e69ac8, 0xf2b82f6a, 0xd9e01006, 0x50d86e6d, 0xc33aee27,
  0xa6d2a626, 0x24da0a9a, 0xf086bb59, 0xdb25f566, 0x50570819, 0xc2fd08a9, 0xa65050b4, 0x26c0b162,
  0xee57aa21, 0xdc71898d, 0x4fd288dc, 0xc2c17d52, 0xa5ed18e0, 0x2899e64a, 0xec2b51f8, 0xddc29958,
  0x4f4af5d1, 0xc2884e6e, 0xa5a92114, 0x2a650525, 0xea0208a8, 0xdf18f0ce, 0x4ec05432, 0xc2517e31,
  0xa58480e6, 0x2c216eaa, 0xe7dc2383, 0xe0745b24, 0x4e32a956, 0xc21d0eb8, 0xa57f450a, 0x2dce88aa,
  0xe5b9f755, 0xe1d4a2c8, 0x4da1fab5, 0xc1eb0209, 0xa5996f52, 0x2f6bbe45, 0xe39bd857, 0xe3399167,
  0x4d0e4de2, 0xc1bb5a11, 0xa5d2f6a9, 0x30f8801f, 0xe1821a21, 0xe4a2eff6, 0x4c77a88e, 0xc18e18a7,
  0xa62bc71b, 0x32744493, 0xdf6d0fa1, 0xe61086bc, 0x4bde1089, 0xc1633f8a, 0xa6a3c1d6, 0x33de87de,
  0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52, 0xdb525dc3, 0xe8f77acf,
  0x4aa22036, 0xc114ccb9, 0xa7f084e7, 0x367c9a7e, 0xd94d586c, 0xea70658a, 0x49ffd417, 0xc0f1360b,
  0xa8c4d9cb, 0x37af8159, 0xd74e4abc, 0xebeca36c, 0x495aada2, 0xc0d00db6, 0xa9b7723b, 0x38cf1669,
  0xd5558381, 0xed6bf9d1, 0x48b2b335, 0xc0b15502, 0xaac7fa0e, 0x39daf5e8, 0xd3635094, 0xeeee2d9d,
  0x4807eb4b, 0xc0950d1d, 0xabf612b5, 0x3ad2c2e8, 0xd177fec6, 0xf0730342, 0x475a5c77, 0xc07b371e,
  0xad415361, 0x3bb6276e, 0xcf93d9dc, 0xf1fa3ecb, 0x46aa0d6d, 0xc063d405, 0xaea94927, 0x3c84d496,
  0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8, 0xb02d7724, 0x3d3e82ae, 0xcbe2402d, 0xf50ef5de,
  0x454149fc, 0xc03c6a07, 0xb1cd56aa, 0x3de2f148, 0xca155d39, 0xf69bf7c9, 0x4488e37f, 0xc02c64a6,
  0xb3885772, 0x3e71e759, 0xc850cab4, 0xf82a6c6a, 0x43cdd89a, 0xc01ed535, 0xb55ddfca, 0x3eeb3347,
  0xc694ce67, 0xf9ba1651, 0x43103085, 0xc013bc39, 0xb74d4ccb, 0x3f4eaafe, 0xc4e1accb, 0xfb4ab7db,
  0x424ff28f, 0xc00b1a20, 0xb955f293, 0x3f9c2bfb, 0xc337a8f7, 0xfcdc1342, 0x418d2621, 0xc004ef3f,
  0xbb771c81, 0x3fd39b5a, 0xc197049e, 0xfe6deaa1, 0x40c7d2bd, 0xc0013bd3, 0xbdb00d71, 0x3ff4e5e0,
};
//}}}
//{{{
const int twidTabEven[4*6 + 16*6 + 64*6]  = {
  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x5a82799a, 0xd2bec333,
  0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333,
  0x00000000, 0xd2bec333, 0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x4b418bbe, 0xf383a3e2,
  0x45f704f7, 0xf9ba1651, 0x4fd288dc, 0xed6bf9d1, 0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2,
  0x58c542c5, 0xdc71898d, 0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1, 0x5a12e720, 0xce86ff2a,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x58c542c5, 0xcac933ae,
  0x569cc31b, 0xe1d4a2c8, 0x45f704f7, 0xc04ee4b8, 0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d,
  0x3248d382, 0xc13ad060, 0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6, 0x1a4608ab, 0xc78e9a1d,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x3248d382, 0xc13ad060,
  0x5a12e720, 0xce86ff2a, 0xe5b9f755, 0xe1d4a2c8, 0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae,
  0xcdb72c7e, 0xf383a3e2, 0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d, 0xba08fb09, 0x0645e9af,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xee57aa21, 0xdc71898d,
  0x4fd288dc, 0xc2c17d52, 0xa5ed18e0, 0x2899e64a, 0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060,
  0xa73abd3b, 0x3536cc52, 0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8, 0xb02d7724, 0x3d3e82ae,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x43103085, 0xfcdc1342,
  0x418d2621, 0xfe6deaa1, 0x4488e37f, 0xfb4ab7db, 0x45f704f7, 0xf9ba1651, 0x43103085, 0xfcdc1342,
  0x48b2b335, 0xf69bf7c9, 0x48b2b335, 0xf69bf7c9, 0x4488e37f, 0xfb4ab7db, 0x4c77a88e, 0xf1fa3ecb,
  0x4b418bbe, 0xf383a3e2, 0x45f704f7, 0xf9ba1651, 0x4fd288dc, 0xed6bf9d1, 0x4da1fab5, 0xf0730342,
  0x475a5c77, 0xf82a6c6a, 0x52beac9f, 0xe8f77acf, 0x4fd288dc, 0xed6bf9d1, 0x48b2b335, 0xf69bf7c9,
  0x553805f2, 0xe4a2eff6, 0x51d1dc80, 0xea70658a, 0x49ffd417, 0xf50ef5de, 0x573b2635, 0xe0745b24,
  0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x553805f2, 0xe4a2eff6,
  0x4c77a88e, 0xf1fa3ecb, 0x59d438e5, 0xd8a00bae, 0x569cc31b, 0xe1d4a2c8, 0x4da1fab5, 0xf0730342,
  0x5a6690ae, 0xd5052d97, 0x57cc15bc, 0xdf18f0ce, 0x4ec05432, 0xeeee2d9d, 0x5a7b7f1a, 0xd1a5ef90,
  0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1, 0x5a12e720, 0xce86ff2a, 0x5987b08a, 0xd9e01006,
  0x50d86e6d, 0xebeca36c, 0x592d59da, 0xcbacb0bf, 0x5a12e720, 0xd76619b6, 0x51d1dc80, 0xea70658a,
  0x57cc15bc, 0xc91af976, 0x5a6690ae, 0xd5052d97, 0x52beac9f, 0xe8f77acf, 0x55f104dc, 0xc6d569be,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x5a6690ae, 0xd09441bb,
  0x5471e2e6, 0xe61086bc, 0x50d86e6d, 0xc33aee27, 0x5a12e720, 0xce86ff2a, 0x553805f2, 0xe4a2eff6,
  0x4da1fab5, 0xc1eb0209, 0x5987b08a, 0xcc983f70, 0x55f104dc, 0xe3399167, 0x49ffd417, 0xc0f1360b,
  0x58c542c5, 0xcac933ae, 0x569cc31b, 0xe1d4a2c8, 0x45f704f7, 0xc04ee4b8, 0x57cc15bc, 0xc91af976,
  0x573b2635, 0xe0745b24, 0x418d2621, 0xc004ef3f, 0x569cc31b, 0xc78e9a1d, 0x57cc15bc, 0xdf18f0ce,
  0x3cc85709, 0xc013bc39, 0x553805f2, 0xc6250a18, 0x584f7b58, 0xddc29958, 0x37af354c, 0xc07b371e,
  0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060, 0x51d1dc80, 0xc3bdbdf6,
  0x592d59da, 0xdb25f566, 0x2c9caf6c, 0xc2517e31, 0x4fd288dc, 0xc2c17d52, 0x5987b08a, 0xd9e01006,
  0x26b2a794, 0xc3bdbdf6, 0x4da1fab5, 0xc1eb0209, 0x59d438e5, 0xd8a00bae, 0x2092f05f, 0xc57d965d,
  0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6, 0x1a4608ab, 0xc78e9a1d, 0x48b2b335, 0xc0b15502,
  0x5a43b190, 0xd6326a88, 0x13d4ae08, 0xc9edeb50, 0x45f704f7, 0xc04ee4b8, 0x5a6690ae, 0xd5052d97,
  0x0d47d096, 0xcc983f70, 0x43103085, 0xc013bc39, 0x5a7b7f1a, 0xd3de9156, 0x06a886a0, 0xcf89e3e8,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x3cc85709, 0xc013bc39,
  0x5a7b7f1a, 0xd1a5ef90, 0xf9577960, 0xd6326a88, 0x396b3199, 0xc04ee4b8, 0x5a6690ae, 0xd09441bb,
  0xf2b82f6a, 0xd9e01006, 0x35eaa2c7, 0xc0b15502, 0x5a43b190, 0xcf89e3e8, 0xec2b51f8, 0xddc29958,
  0x3248d382, 0xc13ad060, 0x5a12e720, 0xce86ff2a, 0xe5b9f755, 0xe1d4a2c8, 0x2e88013a, 0xc1eb0209,
  0x59d438e5, 0xcd8bbb6d, 0xdf6d0fa1, 0xe61086bc, 0x2aaa7c7f, 0xc2c17d52, 0x5987b08a, 0xcc983f70,
  0xd94d586c, 0xea70658a, 0x26b2a794, 0xc3bdbdf6, 0x592d59da, 0xcbacb0bf, 0xd3635094, 0xeeee2d9d,
  0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x1e7de5df, 0xc6250a18,
  0x584f7b58, 0xc9edeb50, 0xc850cab4, 0xf82a6c6a, 0x1a4608ab, 0xc78e9a1d, 0x57cc15bc, 0xc91af976,
  0xc337a8f7, 0xfcdc1342, 0x15fdf758, 0xc91af976, 0x573b2635, 0xc8507ea7, 0xbe72d9df, 0x0192155f,
  0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d, 0xba08fb09, 0x0645e9af, 0x0d47d096, 0xcc983f70,
  0x55f104dc, 0xc6d569be, 0xb6002be9, 0x0af10a22, 0x08df1a8c, 0xce86ff2a, 0x553805f2, 0xc6250a18,
  0xb25e054b, 0x0f8cfcbe, 0x0470ebdc, 0xd09441bb, 0x5471e2e6, 0xc57d965d, 0xaf279193, 0x14135c94,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xfb8f1424, 0xd5052d97,
  0x52beac9f, 0xc449d892, 0xaa0efb24, 0x1cc66e99, 0xf720e574, 0xd76619b6, 0x51d1dc80, 0xc3bdbdf6,
  0xa833ea44, 0x20e70f32, 0xf2b82f6a, 0xd9e01006, 0x50d86e6d, 0xc33aee27, 0xa6d2a626, 0x24da0a9a,
  0xee57aa21, 0xdc71898d, 0x4fd288dc, 0xc2c17d52, 0xa5ed18e0, 0x2899e64a, 0xea0208a8, 0xdf18f0ce,
  0x4ec05432, 0xc2517e31, 0xa58480e6, 0x2c216eaa, 0xe5b9f755, 0xe1d4a2c8, 0x4da1fab5, 0xc1eb0209,
  0xa5996f52, 0x2f6bbe45, 0xe1821a21, 0xe4a2eff6, 0x4c77a88e, 0xc18e18a7, 0xa62bc71b, 0x32744493,
  0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52, 0xd94d586c, 0xea70658a,
  0x49ffd417, 0xc0f1360b, 0xa8c4d9cb, 0x37af8159, 0xd5558381, 0xed6bf9d1, 0x48b2b335, 0xc0b15502,
  0xaac7fa0e, 0x39daf5e8, 0xd177fec6, 0xf0730342, 0x475a5c77, 0xc07b371e, 0xad415361, 0x3bb6276e,
  0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8, 0xb02d7724, 0x3d3e82ae, 0xca155d39, 0xf69bf7c9,
  0x4488e37f, 0xc02c64a6, 0xb3885772, 0x3e71e759, 0xc694ce67, 0xf9ba1651, 0x43103085, 0xc013bc39,
  0xb74d4ccb, 0x3f4eaafe, 0xc337a8f7, 0xfcdc1342, 0x418d2621, 0xc004ef3f, 0xbb771c81, 0x3fd39b5a,
};
//}}}

/* bit reverse tables for FFT */
const int bitrevtabOffset[NUM_IMDCT_SIZES] = {0, 17};
//{{{
const unsigned char bitrevtab[17 + 129]  = {
/* nfft = 64 */
0x01, 0x08, 0x02, 0x04, 0x03, 0x0c, 0x05, 0x0a, 0x07, 0x0e, 0x0b, 0x0d, 0x00, 0x06, 0x09, 0x0f,
0x00,

/* nfft = 512 */
0x01, 0x40, 0x02, 0x20, 0x03, 0x60, 0x04, 0x10, 0x05, 0x50, 0x06, 0x30, 0x07, 0x70, 0x09, 0x48,
0x0a, 0x28, 0x0b, 0x68, 0x0c, 0x18, 0x0d, 0x58, 0x0e, 0x38, 0x0f, 0x78, 0x11, 0x44, 0x12, 0x24,
0x13, 0x64, 0x15, 0x54, 0x16, 0x34, 0x17, 0x74, 0x19, 0x4c, 0x1a, 0x2c, 0x1b, 0x6c, 0x1d, 0x5c,
0x1e, 0x3c, 0x1f, 0x7c, 0x21, 0x42, 0x23, 0x62, 0x25, 0x52, 0x26, 0x32, 0x27, 0x72, 0x29, 0x4a,
0x2b, 0x6a, 0x2d, 0x5a, 0x2e, 0x3a, 0x2f, 0x7a, 0x31, 0x46, 0x33, 0x66, 0x35, 0x56, 0x37, 0x76,
0x39, 0x4e, 0x3b, 0x6e, 0x3d, 0x5e, 0x3f, 0x7e, 0x43, 0x61, 0x45, 0x51, 0x47, 0x71, 0x4b, 0x69,
0x4d, 0x59, 0x4f, 0x79, 0x53, 0x65, 0x57, 0x75, 0x5b, 0x6d, 0x5f, 0x7d, 0x67, 0x73, 0x6f, 0x7b,
0x00, 0x08, 0x14, 0x1c, 0x22, 0x2a, 0x36, 0x3e, 0x41, 0x49, 0x55, 0x5d, 0x63, 0x6b, 0x77, 0x7f,
0x00,

};
//}}}
const unsigned char uniqueIDTab[8] = {0x5f, 0x4b, 0x43, 0x5f, 0x5f, 0x4a, 0x52, 0x5f};

//{{{
/**************************************************************************************
 * Function:    BitReverse
 *
 * Description: Ken's fast in-place bit reverse, using super-small table
 *
 * Inputs:      buffer of samples
 *              table index (for transform size)
 *
 * Outputs:     bit-reversed samples in same buffer
 *
 * Return:      none
 **************************************************************************************/
 static void BitReverse (int* inout, int tabidx)
{
  int *part0, *part1;
  int a,b, t,t1;
  const unsigned char* tab = bitrevtab + bitrevtabOffset[tabidx];
  uint8_t nbits = nfftlog2Tab[tabidx];

//static const int nfftTab[NUM_FFT_SIZES] = {64, 512};
//static const uint8_t nfftlog2Tab[NUM_FFT_SIZES] = {6, 9};

  part0 = inout;
  part1 = inout + nfftTab[tabidx];

  while ((a = (*(const uint8_t*)(tab++))) != 0) {
    b = (*(const uint8_t*)(tab++));
    swapcplx(part0[4*a+0], part0[4*b+0]); /* 0xxx0 <-> 0yyy0 */
    swapcplx(part0[4*a+2], part1[4*b+0]); /* 0xxx1 <-> 1yyy0 */
    swapcplx(part1[4*a+0], part0[4*b+2]); /* 1xxx0 <-> 0yyy1 */
    swapcplx(part1[4*a+2], part1[4*b+2]); /* 1xxx1 <-> 1yyy1 */
    }

  do {
    swapcplx(part0[4*a+2], part1[4*a+0]); /* 0xxx1 <-> 1xxx0 */
    } while ((a =(*(const uint8_t*)(tab++))) != 0);
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R4FirstPass
 *
 * Description: radix-4 trivial pass for decimation-in-time FFT
 *
 * Inputs:      buffer of (bit-reversed) samples
 *              number of R4 butterflies per group (i.e. nfft / 4)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 2 guard bits, gains no integer bits,
 *                guard bits out = guard bits in - 2
 **************************************************************************************/
 static void R4FirstPass (int* x, int bg)
{
    int ar, ai, br, bi, cr, ci, dr, di;

  for (; bg != 0; bg--) {

    ar = x[0] + x[2];
    br = x[0] - x[2];
    ai = x[1] + x[3];
    bi = x[1] - x[3];
    cr = x[4] + x[6];
    dr = x[4] - x[6];
    ci = x[5] + x[7];
    di = x[5] - x[7];

    /* max per-sample gain = 4.0 (adding 4 inputs together) */
    x[0] = ar + cr;
    x[4] = ar - cr;
    x[1] = ai + ci;
    x[5] = ai - ci;
    x[2] = br + di;
    x[6] = br - di;
    x[3] = bi - dr;
    x[7] = bi + dr;

    x += 8;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R8FirstPass
 *
 * Description: radix-8 trivial pass for decimation-in-time FFT
 *
 * Inputs:      buffer of (bit-reversed) samples
 *              number of R8 butterflies per group (i.e. nfft / 8)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 3 guard bits, gains 1 integer bit
 *              guard bits out = guard bits in - 3 (if inputs are full scale)
 *                or guard bits in - 2 (if inputs bounded to +/- sqrt(2)/2)
 *              see scaling comments in code
 **************************************************************************************/
 static void R8FirstPass (int* x, int bg)
{
    int ar, ai, br, bi, cr, ci, dr, di;
  int sr, si, tr, ti, ur, ui, vr, vi;
  int wr, wi, xr, xi, yr, yi, zr, zi;

  for (; bg != 0; bg--) {

    ar = x[0] + x[2];
    br = x[0] - x[2];
    ai = x[1] + x[3];
    bi = x[1] - x[3];
    cr = x[4] + x[6];
    dr = x[4] - x[6];
    ci = x[5] + x[7];
    di = x[5] - x[7];

    sr = ar + cr;
    ur = ar - cr;
    si = ai + ci;
    ui = ai - ci;
    tr = br - di;
    vr = br + di;
    ti = bi + dr;
    vi = bi - dr;

    ar = x[ 8] + x[10];
    br = x[ 8] - x[10];
    ai = x[ 9] + x[11];
    bi = x[ 9] - x[11];
    cr = x[12] + x[14];
    dr = x[12] - x[14];
    ci = x[13] + x[15];
    di = x[13] - x[15];

    /* max gain of wr/wi/yr/yi vs input = 2
     *  (sum of 4 samples >> 1)
     */
    wr = (ar + cr) >> 1;
    yr = (ar - cr) >> 1;
    wi = (ai + ci) >> 1;
    yi = (ai - ci) >> 1;

    /* max gain of output vs input = 4
     *  (sum of 4 samples >> 1 + sum of 4 samples >> 1)
     */
    x[ 0] = (sr >> 1) + wr;
    x[ 8] = (sr >> 1) - wr;
    x[ 1] = (si >> 1) + wi;
    x[ 9] = (si >> 1) - wi;
    x[ 4] = (ur >> 1) + yi;
    x[12] = (ur >> 1) - yi;
    x[ 5] = (ui >> 1) - yr;
    x[13] = (ui >> 1) + yr;

    ar = br - di;
    cr = br + di;
    ai = bi + dr;
    ci = bi - dr;

    /* max gain of xr/xi/zr/zi vs input = 4*sqrt(2)/2 = 2*sqrt(2)
     *  (sum of 8 samples, multiply by sqrt(2)/2, implicit >> 1 from Q31)
     */
    xr = MULSHIFT32(SQRT1_2, ar - ai);
    xi = MULSHIFT32(SQRT1_2, ar + ai);
    zr = MULSHIFT32(SQRT1_2, cr - ci);
    zi = MULSHIFT32(SQRT1_2, cr + ci);

    /* max gain of output vs input = (2 + 2*sqrt(2) ~= 4.83)
     *  (sum of 4 samples >> 1, plus xr/xi/zr/zi with gain of 2*sqrt(2))
     * in absolute terms, we have max gain of appx 9.656 (4 + 0.707*8)
     *  but we also gain 1 int bit (from MULSHIFT32 or from explicit >> 1)
     */
    x[ 6] = (tr >> 1) - xr;
    x[14] = (tr >> 1) + xr;
    x[ 7] = (ti >> 1) - xi;
    x[15] = (ti >> 1) + xi;
    x[ 2] = (vr >> 1) + zi;
    x[10] = (vr >> 1) - zi;
    x[ 3] = (vi >> 1) - zr;
    x[11] = (vi >> 1) + zr;

    x += 16;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R4Core
 *
 * Description: radix-4 pass for decimation-in-time FFT
 *
 * Inputs:      buffer of samples
 *              number of R4 butterflies per group
 *              number of R4 groups per pass
 *              pointer to twiddle factors tables
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       gain 2 integer bits per pass (see scaling comments in code)
 *              min 1 GB in
 *              gbOut = gbIn - 1 (short block) or gbIn - 2 (long block)
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
 static void R4Core (int* x, int bg, int gp, int* wtab)
{
  int ar, ai, br, bi, cr, ci, dr, di, tr, ti;
  int wd, ws, wi;
  int i, j, step;
  int *xptr, *wptr;

  for (; bg != 0; gp <<= 2, bg >>= 2) {

    step = 2*gp;
    xptr = x;

    /* max per-sample gain, per group < 1 + 3*sqrt(2) ~= 5.25 if inputs x are full-scale
     * do 3 groups for long block, 2 groups for short block (gain 2 int bits per group)
     *
     * very conservative scaling:
     *   group 1: max gain = 5.25,           int bits gained = 2, gb used = 1 (2^3 = 8)
     *   group 2: max gain = 5.25^2 = 27.6,  int bits gained = 4, gb used = 1 (2^5 = 32)
     *   group 3: max gain = 5.25^3 = 144.7, int bits gained = 6, gb used = 2 (2^8 = 256)
     */
    for (i = bg; i != 0; i--) {

      wptr = wtab;

      for (j = gp; j != 0; j--) {

        ar = xptr[0];
        ai = xptr[1];
        xptr += step;

        /* gain 2 int bits for br/bi, cr/ci, dr/di (MULSHIFT32 by Q30)
         * gain 1 net GB
         */
        ws = wptr[0];
        wi = wptr[1];
        br = xptr[0];
        bi = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, br + bi);
        br = MULSHIFT32(wd, br) - tr; /* cos*br + sin*bi */
        bi = MULSHIFT32(ws, bi) + tr; /* cos*bi - sin*br */
        xptr += step;

        ws = wptr[2];
        wi = wptr[3];
        cr = xptr[0];
        ci = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, cr + ci);
        cr = MULSHIFT32(wd, cr) - tr;
        ci = MULSHIFT32(ws, ci) + tr;
        xptr += step;

        ws = wptr[4];
        wi = wptr[5];
        dr = xptr[0];
        di = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, dr + di);
        dr = MULSHIFT32(wd, dr) - tr;
        di = MULSHIFT32(ws, di) + tr;
        wptr += 6;

        tr = ar;
        ti = ai;
        ar = (tr >> 2) - br;
        ai = (ti >> 2) - bi;
        br = (tr >> 2) + br;
        bi = (ti >> 2) + bi;

        tr = cr;
        ti = ci;
        cr = tr + dr;
        ci = di - ti;
        dr = tr - dr;
        di = di + ti;

        xptr[0] = ar + ci;
        xptr[1] = ai + dr;
        xptr -= step;
        xptr[0] = br - cr;
        xptr[1] = bi - di;
        xptr -= step;
        xptr[0] = ar - ci;
        xptr[1] = ai - dr;
        xptr -= step;
        xptr[0] = br + cr;
        xptr[1] = bi + di;
        xptr += 2;
      }
      xptr += 3*step;
    }
    wtab += 3*step;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R4FFT
 *
 * Description: Ken's very fast in-place radix-4 decimation-in-time FFT
 *
 * Inputs:      table index (for transform size)
 *              buffer of samples (non bit-reversed)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 5 guard bits in for nfft <= 512
 *              gbOut = gbIn - 4 (assuming input is from PreMultiply)
 *              gains log2(nfft) - 2 int bits total
 *                so gain 7 int bits (LONG), 4 int bits (SHORT)
 **************************************************************************************/
void R4FFT (int tabidx, int* x)
{
  int order = nfftlog2Tab[tabidx];
  int nfft = nfftTab[tabidx];

  /* decimation in time */
  BitReverse(x, tabidx);

  if (order & 0x1) {
    /* long block: order = 9, nfft = 512 */
    R8FirstPass(x, nfft >> 3);            /* gain 1 int bit,  lose 2 GB */
    R4Core(x, nfft >> 5, 8, (int *)twidTabOdd);   /* gain 6 int bits, lose 2 GB */
  } else {
    /* short block: order = 6, nfft = 64 */
    R4FirstPass(x, nfft >> 2);            /* gain 0 int bits, lose 2 GB */
    R4Core(x, nfft >> 4, 4, (int *)twidTabEven);  /* gain 4 int bits, lose 1 GB */
  }
}
//}}}
//}}}
//{{{  dct4
static const int nmdctTab[NUM_IMDCT_SIZES] = {128, 1024};
static const int postSkip[NUM_IMDCT_SIZES] = {15, 1};

//{{{
/**************************************************************************************
 * Function:    PreMultiply
 *
 * Description: pre-twiddle stage of DCT4
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       minimum 1 GB in, 2 GB out, gains 5 (short) or 8 (long) frac bits
 *              i.e. gains 2-7= -5 int bits (short) or 2-10 = -8 int bits (long)
 *              normalization by -1/N is rolled into tables here (see trigtabs.c)
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
static void PreMultiply (int tabidx, int* zbuf1)
{
  int i, nmdct, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  zbuf2 = zbuf1 + nmdct - 1;
  csptr = cos4sin4tab + cos4sin4tabOffset[tabidx];

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = nmdct >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0);
    ai2 = *(zbuf1 + 1);
    ai1 = *(zbuf2 + 0);
    ar2 = *(zbuf2 - 1);

    /* gain 2 ints bit from MULSHIFT32 by Q30, but drop 7 or 10 int bits from table scaling of 1/M
     * max per-sample gain (ignoring implicit scaling) = MAX(sin(angle)+cos(angle)) = 1.414
     * i.e. gain 1 GB since worst case is sin(angle) = cos(angle) = 0.707 (Q30), gain 2 from
     *   extra sign bits, and eat one in adding
     */
    t  = MULSHIFT32(sin2a, ar1 + ai1);
    z2 = MULSHIFT32(cps2a, ai1) - t;
    cms2 = cps2a - 2*sin2a;
    z1 = MULSHIFT32(cms2, ar1) + t;
    *zbuf1++ = z1;  /* cos*ar1 + sin*ai1 */
    *zbuf1++ = z2;  /* cos*ai1 - sin*ar1 */

    t  = MULSHIFT32(sin2b, ar2 + ai2);
    z2 = MULSHIFT32(cps2b, ai2) - t;
    cms2 = cps2b - 2*sin2b;
    z1 = MULSHIFT32(cms2, ar2) + t;
    *zbuf2-- = z2;  /* cos*ai2 - sin*ar2 */
    *zbuf2-- = z1;  /* cos*ar2 + sin*ai2 */
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    PostMultiply
 *
 * Description: post-twiddle stage of DCT4
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       minimum 1 GB in, 2 GB out - gains 2 int bits
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
static void PostMultiply (int tabidx, int* fft1)
{
  int i, nmdct, ar1, ai1, ar2, ai2, skipFactor;
  int t, cms2, cps2, sin2;
  int *fft2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  csptr = cos1sin1tab;
  skipFactor = postSkip[tabidx];
  fft2 = fft1 + nmdct - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin)
   */
  cps2 = *csptr++;
  sin2 = *csptr;
  csptr += skipFactor;
  cms2 = cps2 - 2*sin2;

  for (i = nmdct >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ar2 = *(fft2 - 1);
    ai2 = *(fft2 + 0);

    /* gain 2 ints bit from MULSHIFT32 by Q30
     * max per-sample gain = MAX(sin(angle)+cos(angle)) = 1.414
     * i.e. gain 1 GB since worst case is sin(angle) = cos(angle) = 0.707 (Q30), gain 2 from
     *   extra sign bits, and eat one in adding
     */
    t = MULSHIFT32(sin2, ar1 + ai1);
    *fft2-- = t - MULSHIFT32(cps2, ai1);  /* sin*ar1 - cos*ai1 */
    *fft1++ = t + MULSHIFT32(cms2, ar1);  /* cos*ar1 + sin*ai1 */
    cps2 = *csptr++;
    sin2 = *csptr;
    csptr += skipFactor;

    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    *fft2-- = t - MULSHIFT32(cps2, ai2);  /* sin*ar1 - cos*ai1 */
    cms2 = cps2 - 2*sin2;
    *fft1++ = t + MULSHIFT32(cms2, ar2);  /* cos*ar1 + sin*ai1 */
  }
}
//}}}

//{{{
/**************************************************************************************
 * Function:    PreMultiplyRescale
 *
 * Description: pre-twiddle stage of DCT4, with rescaling for extra guard bits
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *              number of guard bits to add to input before processing
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       see notes on PreMultiply(), above
 **************************************************************************************/
 static void PreMultiplyRescale (int tabidx, int* zbuf1, int es)
{
  int i, nmdct, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  zbuf2 = zbuf1 + nmdct - 1;
  csptr = cos4sin4tab + cos4sin4tabOffset[tabidx];

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = nmdct >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0) >> es;
    ai1 = *(zbuf2 + 0) >> es;
    ai2 = *(zbuf1 + 1) >> es;

    t  = MULSHIFT32(sin2a, ar1 + ai1);
    z2 = MULSHIFT32(cps2a, ai1) - t;
    cms2 = cps2a - 2*sin2a;
    z1 = MULSHIFT32(cms2, ar1) + t;
    *zbuf1++ = z1;
    *zbuf1++ = z2;

    ar2 = *(zbuf2 - 1) >> es; /* do here to free up register used for es */

    t  = MULSHIFT32(sin2b, ar2 + ai2);
    z2 = MULSHIFT32(cps2b, ai2) - t;
    cms2 = cps2b - 2*sin2b;
    z1 = MULSHIFT32(cms2, ar2) + t;
    *zbuf2-- = z2;
    *zbuf2-- = z1;

  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    PostMultiplyRescale
 *
 * Description: post-twiddle stage of DCT4, with rescaling for extra guard bits
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *              number of guard bits to remove from output
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       clips output to [-2^30, 2^30 - 1], guaranteeing at least 1 guard bit
 *              see notes on PostMultiply(), above
 **************************************************************************************/
 static void PostMultiplyRescale (int tabidx, int* fft1, int es)
{
  int i, nmdct, ar1, ai1, ar2, ai2, skipFactor, z;
  int t, cs2, sin2;
  int *fft2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  csptr = cos1sin1tab;
  skipFactor = postSkip[tabidx];
  fft2 = fft1 + nmdct - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin)
   */
  cs2 = *csptr++;
  sin2 = *csptr;
  csptr += skipFactor;

  for (i = nmdct >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ai2 = *(fft2 + 0);

    t = MULSHIFT32(sin2, ar1 + ai1);
    z = t - MULSHIFT32(cs2, ai1);
    CLIP_2N_SHIFT(z, es);
    *fft2-- = z;
    cs2 -= 2*sin2;
    z = t + MULSHIFT32(cs2, ar1);
    CLIP_2N_SHIFT(z, es);
    *fft1++ = z;

    cs2 = *csptr++;
    sin2 = *csptr;
    csptr += skipFactor;

    ar2 = *fft2;
    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    z = t - MULSHIFT32(cs2, ai2);
    CLIP_2N_SHIFT(z, es);
    *fft2-- = z;
    cs2 -= 2*sin2;
    z = t + MULSHIFT32(cs2, ar2);
    CLIP_2N_SHIFT(z, es);
    *fft1++ = z;
    cs2 += 2*sin2;
  }
}
//}}}

//{{{
/**************************************************************************************
 * Function:    DCT4
 *
 * Description: type-IV DCT
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *              number of guard bits in the input buffer
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       operates in-place
 *              if number of guard bits in input is < GBITS_IN_DCT4, the input is
 *                scaled (>>) before the DCT4 and rescaled (<<, with clipping) after
 *                the DCT4 (rare)
 *              the output has FBITS_LOST_DCT4 fewer fraction bits than the input
 *              the output will always have at least 1 guard bit (GBITS_IN_DCT4 >= 4)
 *              int bits gained per stage (PreMul + FFT + PostMul)
 *                 short blocks = (-5 + 4 + 2) = 1 total
 *                 long blocks =  (-8 + 7 + 2) = 1 total
 **************************************************************************************/
void DCT4 (int tabidx, int *coef, int gb)
{
  int es;

  /* fast in-place DCT-IV - adds guard bits if necessary */
  if (gb < GBITS_IN_DCT4) {
    es = GBITS_IN_DCT4 - gb;
    PreMultiplyRescale(tabidx, coef, es);
    R4FFT(tabidx, coef);
    PostMultiplyRescale(tabidx, coef, es);
  } else {
    PreMultiply(tabidx, coef);
    R4FFT(tabidx, coef);
    PostMultiply(tabidx, coef);
  }
}
//}}}
//}}}
//{{{  imdct
#define RND_VAL   (1 << (FBITS_OUT_IMDCT-1))

//{{{
/**************************************************************************************
 * Function:    IMDCT
 *
 * Description: inverse transform and convert to 16-bit PCM
 *
 * Inputs:      valid AACDecInfo struct
 *              index of current channel (0 for SCE/LFE, 0 or 1 for CPE)
 *              output channel (range = [0, nChans-1])
 *
 * Outputs:     complete frame of decoded PCM, after inverse transform
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       If AAC_ENABLE_SBR is defined at compile time then window + overlap
 *                does NOT clip to 16-bit PCM and does NOT interleave channels
 *              If AAC_ENABLE_SBR is NOT defined at compile time, then window + overlap
 *                does clip to 16-bit PCM and interleaves channels
 *              If SBR is enabled at compile time, but we don't know whether it is
 *                actually used for this frame (e.g. the first frame of a stream),
 *                we need to produce both clipped 16-bit PCM in outbuf AND
 *                unclipped 32-bit PCM in the SBR input buffer. In this case we make
 *                a separate pass over the 32-bit PCM to produce 16-bit PCM output.
 *                This inflicts a slight performance hit when decoding non-SBR files.
 **************************************************************************************/
int IMDCT (AACDecInfo *aacDecInfo, int ch, int chOut, short *outbuf)
{
  int i;
  PSInfoBase *psi;
  ICSInfo *icsInfo;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);
  outbuf += chOut;

  /* optimized type-IV DCT (operates inplace) */
  if (icsInfo->winSequence == 2) {
    /* 8 short blocks */
    for (i = 0; i < 8; i++)
      DCT4(0, psi->coef[ch] + i*128, psi->gbCurrent[ch]);
  } else {
    /* 1 long block */
    DCT4(1, psi->coef[ch], psi->gbCurrent[ch]);
  }

  /* window, overlap-add, don't clip to short (send to SBR decoder)
   * store the decoded 32-bit samples in top half (second AAC_MAX_NSAMPS samples) of coef buffer
   */
  if (icsInfo->winSequence == 0)
    DecWindowOverlapNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 1)
    DecWindowOverlapLongStartNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 2)
    DecWindowOverlapShortNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 3)
    DecWindowOverlapLongStopNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);

  if (!aacDecInfo->sbrEnabled) {
    for (i = 0; i < AAC_MAX_NSAMPS; i++) {
      *outbuf = CLIPTOSHORT((psi->sbrWorkBuf[ch][i] + RND_VAL) >> FBITS_OUT_IMDCT);
      outbuf += aacDecInfo->nChans;
    }
  }

  aacDecInfo->rawSampleBuf[ch] = psi->sbrWorkBuf[ch];
  aacDecInfo->rawSampleBytes = sizeof(int);
  aacDecInfo->rawSampleFBits = FBITS_OUT_IMDCT;

  psi->prevWinShape[chOut] = icsInfo->winShape;

  return 0;
}
//}}}
//}}}
//{{{  dequant
#define SF_OFFSET  100

/* sqrt(0.5), format = Q31 */
#define SQRTHALF 0x5a82799a

// Minimax polynomial approximation to pow(x, 4/3), over the range
//  poly43lo: x = [0.5, 0.7071]
//  poly43hi: x = [0.7071, 1.0]
// Relative error < 1E-7
// Coefs are scaled by 4, 2, 1, 0.5, 0.25
static const int poly43lo[5] = { 0x29a0bda9, 0xb02e4828, 0x5957aa1b, 0x236c498d, 0xff581859 };
static const int poly43hi[5] = { 0x10852163, 0xd333f6a4, 0x46e9408b, 0x27c2cef0, 0xfef577b4 };

// pow2exp[i] = pow(2, i*4/3) exponent
static const int pow2exp[8] = { 14, 13, 11, 10, 9, 7, 6, 5 };
//{{{
/* pow2exp[i] = pow(2, i*4/3) fraction */
static const int pow2frac[8] = {
  0x6597fa94, 0x50a28be6, 0x7fffffff, 0x6597fa94,
  0x50a28be6, 0x7fffffff, 0x6597fa94, 0x50a28be6
  };
//}}}
//{{{
/**************************************************************************************
 * Function:    DequantBlock
 *
 * Description: dequantize one block of transform coefficients (in-place)
 *
 * Inputs:      quantized transform coefficients, range = [0, 8191]
 *              number of samples to dequantize
 *              scalefactor for this block of data, range = [0, 256]
 *
 * Outputs:     dequantized transform coefficients in Q(FBITS_OUT_DQ_OFF)
 *
 * Return:      guard bit mask (OR of abs value of all dequantized coefs)
 *
 * Notes:       applies dequant formula y = pow(x, 4.0/3.0) * pow(2, (scale - 100)/4.0)
 *                * pow(2, FBITS_OUT_DQ_OFF)
 *              clips outputs to Q(FBITS_OUT_DQ_OFF)
 *              output has no minimum number of guard bits
 **************************************************************************************/
static int DequantBlock (int* inbuf, int nSamps, int scale)
{
  int iSamp, scalef, scalei, x, y, gbMask, shift, tab4[4];
  const int *tab16, *coef;

  if (nSamps <= 0)
    return 0;

  scale -= SF_OFFSET; /* new range = [-100, 156] */

  /* with two's complement numbers, scalei/scalef factorization works for pos and neg values of scale:
   *  [+4...+7] >> 2 = +1, [ 0...+3] >> 2 = 0, [-4...-1] >> 2 = -1, [-8...-5] >> 2 = -2 ...
   *  (-1 & 0x3) = 3, (-2 & 0x3) = 2, (-3 & 0x3) = 1, (0 & 0x3) = 0
   *
   * Example: 2^(-5/4) = 2^(-1) * 2^(-1/4) = 2^-2 * 2^(3/4)
   */
  tab16 = pow43_14[scale & 0x3];
  scalef = pow14[scale & 0x3];
  scalei = (scale >> 2) + FBITS_OUT_DQ_OFF;

  /* cache first 4 values:
   * tab16[j] = Q28 for j = [0,3]
   * tab4[x] = x^(4.0/3.0) * 2^(0.25*scale), Q(FBITS_OUT_DQ_OFF)
   */
  shift = 28 - scalei;
  if (shift > 31) {
    tab4[0] = tab4[1] = tab4[2] = tab4[3] = 0;
  } else if (shift <= 0) {
    shift = -shift;
    if (shift > 31)
      shift = 31;
    for (x = 0; x < 4; x++) {
      y = tab16[x];
      if (y > (0x7fffffff >> shift))
        y = 0x7fffffff;   /* clip (rare) */
      else
        y <<= shift;
      tab4[x] = y;
    }
  } else {
    tab4[0] = 0;
    tab4[1] = tab16[1] >> shift;
    tab4[2] = tab16[2] >> shift;
    tab4[3] = tab16[3] >> shift;
  }

  gbMask = 0;
  do {
    iSamp = *inbuf;
    x = FASTABS(iSamp);

    if (x < 4) {
      y = tab4[x];
    } else  {

      if (x < 16) {
        /* result: y = Q25 (tab16 = Q25) */
        y = tab16[x];
        shift = 25 - scalei;
      } else if (x < 64) {
        /* result: y = Q21 (pow43tab[j] = Q23, scalef = Q30) */
        y = pow43[x-16];
        shift = 21 - scalei;
        y = MULSHIFT32(y, scalef);
      } else {
        /* normalize to [0x40000000, 0x7fffffff]
         * input x = [64, 8191] = [64, 2^13-1]
         * ranges:
         *  shift = 7:   64 -  127
         *  shift = 6:  128 -  255
         *  shift = 5:  256 -  511
         *  shift = 4:  512 - 1023
         *  shift = 3: 1024 - 2047
         *  shift = 2: 2048 - 4095
         *  shift = 1: 4096 - 8191
         */
        x <<= 17;
        shift = 0;
        if (x < 0x08000000)
          x <<= 4, shift += 4;
        if (x < 0x20000000)
          x <<= 2, shift += 2;
        if (x < 0x40000000)
          x <<= 1, shift += 1;

        coef = (x < SQRTHALF) ? poly43lo : poly43hi;

        /* polynomial */
        y = coef[0];
        y = MULSHIFT32(y, x) + coef[1];
        y = MULSHIFT32(y, x) + coef[2];
        y = MULSHIFT32(y, x) + coef[3];
        y = MULSHIFT32(y, x) + coef[4];
        y = MULSHIFT32(y, pow2frac[shift]) << 3;

        /* fractional scale
         * result: y = Q21 (pow43tab[j] = Q23, scalef = Q30)
         */
        y = MULSHIFT32(y, scalef);  /* now y is Q24 */
        shift = 24 - scalei - pow2exp[shift];
      }

      /* integer scale */
      if (shift <= 0) {
        shift = -shift;
        if (shift > 31)
          shift = 31;

        if (y > (0x7fffffff >> shift))
          y = 0x7fffffff;   /* clip (rare) */
        else
          y <<= shift;
      } else {
        if (shift > 31)
          shift = 31;
        y >>= shift;
      }
    }

    /* sign and store (gbMask used to count GB's) */
    gbMask |= y;

    /* apply sign */
    iSamp >>= 31;
    y ^= iSamp;
    y -= iSamp;

    *inbuf++ = y;
  } while (--nSamps);

  return gbMask;
}
//}}}

//{{{
/**************************************************************************************
 * Function:    Dequantize
 *
 * Description: dequantize all transform coefficients for one channel
 *
 * Inputs:      valid AACDecInfo struct (including unpacked, quantized coefficients)
 *              index of current channel
 *
 * Outputs:     dequantized coefficients, including short-block deinterleaving
 *              flags indicating if intensity and/or PNS is active
 *              minimum guard bit count for dequantized coefficients
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int Dequantize (AACDecInfo* aacDecInfo, int ch)
{
  int gp, cb, sfb, win, width, nSamps, gbMask;
  int *coef;
  const int /*short*/ *sfbTab;
  unsigned char *sfbCodeBook;
  short *scaleFactors;
  PSInfoBase *psi;
  ICSInfo *icsInfo;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  if (icsInfo->winSequence == 2) {
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
    nSamps = NSAMPS_SHORT;
  } else {
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
    nSamps = NSAMPS_LONG;
  }
  coef = psi->coef[ch];
  sfbCodeBook = psi->sfbCodeBook[ch];
  scaleFactors = psi->scaleFactors[ch];

  psi->intensityUsed[ch] = 0;
  psi->pnsUsed[ch] = 0;
  gbMask = 0;
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
        /* dequantize one scalefactor band (not necessary if codebook is intensity or PNS)
         * for zero codebook, still run dequantizer in case non-zero pulse data was added
         */
        cb = (int)(sfbCodeBook[sfb]);
        width = sfbTab[sfb+1] - sfbTab[sfb];
        if (cb >= 0 && cb <= 11)
          gbMask |= DequantBlock(coef, width, scaleFactors[sfb]);
        else if (cb == 13)
          psi->pnsUsed[ch] = 1;
        else if (cb == 14 || cb == 15)
          psi->intensityUsed[ch] = 1; /* should only happen if ch == 1 */
        coef += width;
      }
      coef += (nSamps - sfbTab[icsInfo->maxSFB]);
    }
    sfbCodeBook += icsInfo->maxSFB;
    scaleFactors += icsInfo->maxSFB;
  }
  aacDecInfo->pnsUsed |= psi->pnsUsed[ch];  /* set flag if PNS used for any channel */

  /* calculate number of guard bits in dequantized data */
  psi->gbCurrent[ch] = CLZ(gbMask) - 1;

  return ERR_AAC_NONE;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DeinterleaveShortBlocks
 *
 * Description: deinterleave transform coefficients in short blocks for one channel
 *
 * Inputs:      valid AACDecInfo struct (including unpacked, quantized coefficients)
 *              index of current channel
 *
 * Outputs:     deinterleaved coefficients (window groups into 8 separate windows)
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       only necessary if deinterleaving not part of Huffman decoding
 **************************************************************************************/
int DeinterleaveShortBlocks (AACDecInfo* aacDecInfo, int ch)
{
  (void)aacDecInfo;
  (void)ch;
  /* not used for this implementation - short block deinterleaving performed during Huffman decoding */
  return ERR_AAC_NONE;
}
//}}}
//}}}
//{{{  concealment
//{{{
/**************************************************************************************
 * Function:    DecodeSingleChannelElement
 *
 * Description: decode one SCE
 *
 * Inputs:      BitStreamInfo struct pointing to start of SCE (14496-3, table 4.4.4)
 *
 * Outputs:     updated element instance tag
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       doesn't decode individual channel stream (part of DecodeNoiselessData)
 **************************************************************************************/
static int DecodeSingleChannelElement (AACDecInfo *aacDecInfo, BitStreamInfo *bsi)
{
  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;

  /* read instance tag */
  aacDecInfo->currInstTag = GetBits(bsi, NUM_INST_TAG_BITS);

  return 0;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeChannelPairElement
 *
 * Description: decode one CPE
 *
 * Inputs:      BitStreamInfo struct pointing to start of CPE (14496-3, table 4.4.5)
 *
 * Outputs:     updated element instance tag
 *              updated commonWin
 *              updated ICS info, if commonWin == 1
 *              updated mid-side stereo info, if commonWin == 1
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       doesn't decode individual channel stream (part of DecodeNoiselessData)
 **************************************************************************************/
static int DecodeChannelPairElement (AACDecInfo *aacDecInfo, BitStreamInfo *bsi)
{
  int sfb, gp, maskOffset;
  unsigned char currBit, *maskPtr;
  PSInfoBase *psi;
  ICSInfo *icsInfo;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = psi->icsInfo;

  /* read instance tag */
  aacDecInfo->currInstTag = GetBits(bsi, NUM_INST_TAG_BITS);

  /* read common window flag and mid-side info (if present)
   * store msMask bits in psi->msMaskBits[] as follows:
   *  long blocks -  pack bits for each SFB in range [0, maxSFB) starting with lsb of msMaskBits[0]
   *  short blocks - pack bits for each SFB in range [0, maxSFB), for each group [0, 7]
   * msMaskPresent = 0 means no M/S coding
   *               = 1 means psi->msMaskBits contains 1 bit per SFB to toggle M/S coding
   *               = 2 means all SFB's are M/S coded (so psi->msMaskBits is not needed)
   */
  psi->commonWin = GetBits(bsi, 1);
  if (psi->commonWin) {
    DecodeICSInfo(bsi, icsInfo, psi->sampRateIdx);
    psi->msMaskPresent = GetBits(bsi, 2);
    if (psi->msMaskPresent == 1) {
      maskPtr = psi->msMaskBits;
      *maskPtr = 0;
      maskOffset = 0;
      for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
        for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
          currBit = (unsigned char)GetBits(bsi, 1);
          *maskPtr |= currBit << maskOffset;
          if (++maskOffset == 8) {
            maskPtr++;
            *maskPtr = 0;
            maskOffset = 0;
          }
        }
      }
    }
  }

  return 0;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeLFEChannelElement
 *
 * Description: decode one LFE
 *
 * Inputs:      BitStreamInfo struct pointing to start of LFE (14496-3, table 4.4.9)
 *
 * Outputs:     updated element instance tag
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       doesn't decode individual channel stream (part of DecodeNoiselessData)
 **************************************************************************************/
static int DecodeLFEChannelElement (AACDecInfo *aacDecInfo, BitStreamInfo *bsi)
{
  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;

  /* read instance tag */
  aacDecInfo->currInstTag = GetBits(bsi, NUM_INST_TAG_BITS);

  return 0;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeDataStreamElement
 *
 * Description: decode one DSE
 *
 * Inputs:      BitStreamInfo struct pointing to start of DSE (14496-3, table 4.4.10)
 *
 * Outputs:     updated element instance tag
 *              filled in data stream buffer
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
static int DecodeDataStreamElement (AACDecInfo *aacDecInfo, BitStreamInfo *bsi)
{
  unsigned int byteAlign, dataCount;
  unsigned char *dataBuf;
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  aacDecInfo->currInstTag = GetBits(bsi, NUM_INST_TAG_BITS);
  byteAlign = GetBits(bsi, 1);
  dataCount = GetBits(bsi, 8);
  if (dataCount == 255)
    dataCount += GetBits(bsi, 8);

  if (byteAlign)
    ByteAlignBitstream(bsi);

  psi->dataCount = dataCount;
  dataBuf = psi->dataBuf;
  while (dataCount--)
    *dataBuf++ = GetBits(bsi, 8);

  return 0;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeFillElement
 *
 * Description: decode one fill element
 *
 * Inputs:      BitStreamInfo struct pointing to start of fill element
 *                (14496-3, table 4.4.11)
 *
 * Outputs:     updated element instance tag
 *              unpacked extension payload
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
static int DecodeFillElement (AACDecInfo *aacDecInfo, BitStreamInfo *bsi)
{
  unsigned int fillCount;
  unsigned char *fillBuf;
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  fillCount = GetBits(bsi, 4);
  if (fillCount == 15)
    fillCount += (GetBits(bsi, 8) - 1);

  psi->fillCount = fillCount;
  fillBuf = psi->fillBuf;
  while (fillCount--)
    *fillBuf++ = GetBits(bsi, 8);

  aacDecInfo->currInstTag = -1; /* fill elements don't have instance tag */
  aacDecInfo->fillExtType = 0;

  /* check for SBR
   * aacDecInfo->sbrEnabled is sticky (reset each raw_data_block), so for multichannel
   *    need to verify that all SCE/CPE/ICCE have valid SBR fill element following, and
   *    must upsample by 2 for LFE
   */
  if (psi->fillCount > 0) {
    aacDecInfo->fillExtType = (int)((psi->fillBuf[0] >> 4) & 0x0f);
    if (aacDecInfo->fillExtType == EXT_SBR_DATA || aacDecInfo->fillExtType == EXT_SBR_DATA_CRC)
      aacDecInfo->sbrEnabled = 1;
  }

  aacDecInfo->fillBuf = psi->fillBuf;
  aacDecInfo->fillCount = psi->fillCount;

  return 0;
}
//}}}

//{{{
/**************************************************************************************
 * Function:    DecodeProgramConfigElement
 *
 * Description: decode one PCE
 *
 * Inputs:      BitStreamInfo struct pointing to start of PCE (14496-3, table 4.4.2)
 *
 * Outputs:     filled-in ProgConfigElement struct
 *              updated BitStreamInfo struct
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       #define KEEP_PCE_COMMENTS to save the comment field of the PCE
 *                (otherwise we just skip it in the bitstream, to save memory)
 **************************************************************************************/
int DecodeProgramConfigElement (ProgConfigElement *pce, BitStreamInfo *bsi)
{
  int i;

  pce->elemInstTag =   GetBits(bsi, 4);
  pce->profile =       GetBits(bsi, 2);
  pce->sampRateIdx =   GetBits(bsi, 4);
  pce->numFCE =        GetBits(bsi, 4);
  pce->numSCE =        GetBits(bsi, 4);
  pce->numBCE =        GetBits(bsi, 4);
  pce->numLCE =        GetBits(bsi, 2);
  pce->numADE =        GetBits(bsi, 3);
  pce->numCCE =        GetBits(bsi, 4);

  pce->monoMixdown = GetBits(bsi, 1) << 4;  /* present flag */
  if (pce->monoMixdown)
    pce->monoMixdown |= GetBits(bsi, 4);  /* element number */

  pce->stereoMixdown = GetBits(bsi, 1) << 4;  /* present flag */
  if (pce->stereoMixdown)
    pce->stereoMixdown  |= GetBits(bsi, 4); /* element number */

  pce->matrixMixdown = GetBits(bsi, 1) << 4;  /* present flag */
  if (pce->matrixMixdown) {
    pce->matrixMixdown  |= GetBits(bsi, 2) << 1;  /* index */
    pce->matrixMixdown  |= GetBits(bsi, 1);     /* pseudo-surround enable */
  }

  for (i = 0; i < pce->numFCE; i++) {
    pce->fce[i]  = GetBits(bsi, 1) << 4;  /* is_cpe flag */
    pce->fce[i] |= GetBits(bsi, 4);     /* tag select */
  }

  for (i = 0; i < pce->numSCE; i++) {
    pce->sce[i]  = GetBits(bsi, 1) << 4;  /* is_cpe flag */
    pce->sce[i] |= GetBits(bsi, 4);     /* tag select */
  }

  for (i = 0; i < pce->numBCE; i++) {
    pce->bce[i]  = GetBits(bsi, 1) << 4;  /* is_cpe flag */
    pce->bce[i] |= GetBits(bsi, 4);     /* tag select */
  }

  for (i = 0; i < pce->numLCE; i++)
    pce->lce[i] = GetBits(bsi, 4);      /* tag select */

  for (i = 0; i < pce->numADE; i++)
    pce->ade[i] = GetBits(bsi, 4);      /* tag select */

  for (i = 0; i < pce->numCCE; i++) {
    pce->cce[i]  = GetBits(bsi, 1) << 4;  /* independent/dependent flag */
    pce->cce[i] |= GetBits(bsi, 4);     /* tag select */
  }


  ByteAlignBitstream(bsi);

#ifdef KEEP_PCE_COMMENTS
  pce->commentBytes = GetBits(bsi, 8);
  for (i = 0; i < pce->commentBytes; i++)
    pce->commentField[i] = GetBits(bsi, 8);
#else
  /* eat comment bytes and throw away */
  i = GetBits(bsi, 8);
  while (i--)
    GetBits(bsi, 8);
#endif

  return 0;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeNextElement
 *
 * Description: decode next syntactic element in AAC frame
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer containing next element
 *              pointer to bit offset
 *              pointer to number of valid bits remaining in buf
 *
 * Outputs:     type of element decoded (aacDecInfo->currBlockID)
 *              type of element decoded last time (aacDecInfo->prevBlockID)
 *              updated aacDecInfo state, depending on which element was decoded
 *              updated buffer pointer
 *              updated bit offset
 *              updated number of available bits
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int DecodeNextElement (AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail)
{
  int err, bitsUsed;
  PSInfoBase *psi;
  BitStreamInfo bsi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  /* init bitstream reader */
  SetBitstreamPointer(&bsi, (*bitsAvail + 7) >> 3, *buf);
  GetBits(&bsi, *bitOffset);

  /* read element ID (save last ID for SBR purposes) */
  aacDecInfo->prevBlockID = aacDecInfo->currBlockID;
  aacDecInfo->currBlockID = GetBits(&bsi, NUM_SYN_ID_BITS);

  /* set defaults (could be overwritten by DecodeXXXElement(), depending on currBlockID) */
  psi->commonWin = 0;

  err = 0;
  switch (aacDecInfo->currBlockID) {
  case AAC_ID_SCE:
    err = DecodeSingleChannelElement(aacDecInfo, &bsi);
    break;
  case AAC_ID_CPE:
    err = DecodeChannelPairElement(aacDecInfo, &bsi);
    break;
  case AAC_ID_CCE:
    /* TODO - implement CCE decoding */
    break;
  case AAC_ID_LFE:
    err = DecodeLFEChannelElement(aacDecInfo, &bsi);
    break;
  case AAC_ID_DSE:
    err = DecodeDataStreamElement(aacDecInfo, &bsi);
    break;
  case AAC_ID_PCE:
    err = DecodeProgramConfigElement(psi->pce + 0, &bsi);
    break;
  case AAC_ID_FIL:
    err = DecodeFillElement(aacDecInfo, &bsi);
    break;
  case AAC_ID_END:
    break;
  }
  if (err)
    return ERR_AAC_SYNTAX_ELEMENT;

  /* update bitstream reader */
  bitsUsed = CalcBitsUsed(&bsi, *buf, *bitOffset);
  *buf += (bitsUsed + *bitOffset) >> 3;
  *bitOffset = (bitsUsed + *bitOffset) & 0x07;
  *bitsAvail -= bitsUsed;

  if (*bitsAvail < 0)
    return ERR_AAC_INDATA_UNDERFLOW;

  return ERR_AAC_NONE;
}
//}}}
//}}}
//{{{  pns
#define NUM_ITER_INVSQRT  4
#define X0_COEF_2 0xc0000000  /* Q29: -2.0 */
#define X0_OFF_2  0x60000000  /* Q29:  3.0 */
#define Q26_3   0x0c000000  /* Q26:  3.0 */

//{{{
/**************************************************************************************
 * Function:    InvRootR
 *
 * Description: use Newton's method to solve for x = 1/sqrt(r)
 *
 * Inputs:      r in Q30 format, range = [0.25, 1] (normalize inputs to this range)
 *
 * Outputs:     none
 *
 * Return:      x = Q29, range = (1, 2)
 *
 * Notes:       guaranteed to converge and not overflow for any r in this range
 *
 *              xn+1  = xn - f(xn)/f'(xn)
 *              f(x)  = 1/sqrt(r) - x = 0 (find root)
 *                    = 1/x^2 - r
 *              f'(x) = -2/x^3
 *
 *              so xn+1 = xn/2 * (3 - r*xn^2)
 *
 *              NUM_ITER_INVSQRT = 3, maxDiff = 1.3747e-02
 *              NUM_ITER_INVSQRT = 4, maxDiff = 3.9832e-04
 **************************************************************************************/
static int InvRootR (int r)
{
  int i, xn, t;

  /* use linear equation for initial guess
   * x0 = -2*r + 3 (so x0 always >= correct answer in range [0.25, 1))
   * xn = Q29 (at every step)
   */
  xn = (MULSHIFT32(r, X0_COEF_2) << 2) + X0_OFF_2;

  for (i = 0; i < NUM_ITER_INVSQRT; i++) {
    t = MULSHIFT32(xn, xn);         /* Q26 = Q29*Q29 */
    t = Q26_3 - (MULSHIFT32(r, t) << 2);  /* Q26 = Q26 - (Q31*Q26 << 1) */
    xn = MULSHIFT32(xn, t) << (6 - 1);    /* Q29 = (Q29*Q26 << 6), and -1 for division by 2 */
  }

  /* clip to range (1.0, 2.0)
   * (because of rounding, this can converge to xn slightly > 2.0 when r is near 0.25)
   */
  if (xn >> 30)
    xn = (1 << 30) - 1;

  return xn;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    Get32BitVal
 *
 * Description: generate 32-bit unsigned random number
 *
 * Inputs:      last number calculated (seed, first time through)
 *
 * Outputs:     new number, saved in *last
 *
 * Return:      32-bit number, uniformly distributed between [0, 2^32)
 *
 * Notes:       uses simple linear congruential generator
 **************************************************************************************/
static unsigned int Get32BitVal (unsigned int *last)
{
  unsigned int r = *last;

  /* use same coefs as MPEG reference code (classic LCG)
   * use unsigned multiply to force reliable wraparound behavior in C (mod 2^32)
   */
  r = (1664525U * r) + 1013904223U;
  *last = r;

  return r;
}

//}}}
//{{{
/**************************************************************************************
 * Function:    ScaleNoiseVector
 *
 * Description: apply scaling to vector of noise coefficients for one scalefactor band
 *
 * Inputs:      unscaled coefficients
 *              number of coefficients in vector (one scalefactor band of coefs)
 *              scalefactor for this band (i.e. noise energy)
 *
 * Outputs:     nVals coefficients in Q(FBITS_OUT_DQ_OFF)
 *
 * Return:      guard bit mask (OR of abs value of all noise coefs)
 **************************************************************************************/
static int ScaleNoiseVector (int *coef, int nVals, int sf)
{

  int i, c, spec, energy, sq, scalef, scalei, invSqrtEnergy, z, gbMask;
  energy = 0;
  for (i = 0; i < nVals; i++) {
    spec = coef[i];

    /* max nVals = max SFB width = 96, so energy can gain < 2^7 bits in accumulation */
    sq = (spec * spec) >> 8;    /* spec*spec range = (-2^30, 2^30) */
    energy += sq;
  }

  /* unless nVals == 1 (or the number generator is broken...), this should not happen */
  if (energy == 0)
    return 0; /* coef[i] must = 0 for i = [0, nVals-1], so gbMask = 0 */

  /* pow(2, sf/4) * pow(2, FBITS_OUT_DQ_OFF) */
  scalef = pow14[sf & 0x3];
  scalei = (sf >> 2) + FBITS_OUT_DQ_OFF;

  /* energy has implied factor of 2^-8 since we shifted the accumulator
   * normalize energy to range [0.25, 1.0), calculate 1/sqrt(1), and denormalize
   *   i.e. divide input by 2^(30-z) and convert to Q30
   *        output of 1/sqrt(i) now has extra factor of 2^((30-z)/2)
   *        for energy > 0, z is an even number between 0 and 28
   * final scaling of invSqrtEnergy:
   *  2^(15 - z/2) to compensate for implicit 2^(30-z) factor in input
   *  +4 to compensate for implicit 2^-8 factor in input
   */
  z = CLZ(energy) - 2;          /* energy has at least 2 leading zeros (see acc loop) */
  z &= 0xfffffffe;            /* force even */
  invSqrtEnergy = InvRootR(energy << z);  /* energy << z must be in range [0x10000000, 0x40000000] */
  scalei -= (15 - z/2 + 4);       /* nInt = 1/sqrt(energy) in Q29 */

  /* normalize for final scaling */
  z = CLZ(invSqrtEnergy) - 1;
  invSqrtEnergy <<= z;
  scalei -= (z - 3 - 2);  /* -2 for scalef, z-3 for invSqrtEnergy */
  scalef = MULSHIFT32(scalef, invSqrtEnergy); /* scalef (input) = Q30, invSqrtEnergy = Q29 * 2^z */
  gbMask = 0;

  if (scalei < 0) {
    scalei = -scalei;
    if (scalei > 31)
      scalei = 31;
    for (i = 0; i < nVals; i++) {
      c = MULSHIFT32(coef[i], scalef) >> scalei;
      gbMask |= FASTABS(c);
      coef[i] = c;
    }
  } else {
    /* for scalei <= 16, no clipping possible (coef[i] is < 2^15 before scaling)
     * for scalei > 16, just saturate exponent (rare)
     *   scalef is close to full-scale (since we normalized invSqrtEnergy)
     * remember, we are just producing noise here
     */
    if (scalei > 16)
      scalei = 16;
    for (i = 0; i < nVals; i++) {
      c = MULSHIFT32(coef[i] << scalei, scalef);
      coef[i] = c;
      gbMask |= FASTABS(c);
    }
  }

  return gbMask;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    GenerateNoiseVector
 *
 * Description: create vector of noise coefficients for one scalefactor band
 *
 * Inputs:      seed for number generator
 *              number of coefficients to generate
 *
 * Outputs:     buffer of nVals coefficients, range = [-2^15, 2^15)
 *              updated seed for number generator
 *
 * Return:      none
 **************************************************************************************/
static void GenerateNoiseVector (int *coef, int *last, int nVals)
{
  int i;

  for (i = 0; i < nVals; i++)
    coef[i] = ((signed int)Get32BitVal((unsigned int *)last)) >> 16;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    CopyNoiseVector
 *
 * Description: copy vector of noise coefficients for one scalefactor band from L to R
 *
 * Inputs:      buffer of left coefficients
 *              number of coefficients to copy
 *
 * Outputs:     buffer of right coefficients
 *
 * Return:      none
 **************************************************************************************/
static void CopyNoiseVector (int *coefL, int *coefR, int nVals)
{
  int i;

  for (i = 0; i < nVals; i++)
    coefR[i] = coefL[i];
}
//}}}

//{{{
/**************************************************************************************
 * Function:    PNS
 *
 * Description: apply perceptual noise substitution, if enabled (MPEG-4 only)
 *
 * Inputs:      valid AACDecInfo struct
 *              index of current channel
 *
 * Outputs:     shaped noise in scalefactor bands where PNS is active
 *              updated minimum guard bit count for this channel
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
int PNS (AACDecInfo *aacDecInfo, int ch)
{
  int gp, sfb, win, width, nSamps, gb, gbMask;
  int *coef;
  const /*short*/ int *sfbTab;
  unsigned char *sfbCodeBook;
  short *scaleFactors;
  int msMaskOffset, checkCorr, genNew;
  unsigned char msMask;
  unsigned char *msMaskPtr;
  PSInfoBase *psi;
  ICSInfo *icsInfo;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  if (!psi->pnsUsed[ch])
    return 0;

  if (icsInfo->winSequence == 2) {
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
    nSamps = NSAMPS_SHORT;
  } else {
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
    nSamps = NSAMPS_LONG;
  }
  coef = psi->coef[ch];
  sfbCodeBook = psi->sfbCodeBook[ch];
  scaleFactors = psi->scaleFactors[ch];
  checkCorr = (aacDecInfo->currBlockID == AAC_ID_CPE && psi->commonWin == 1 ? 1 : 0);

  gbMask = 0;
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      msMaskPtr = psi->msMaskBits + ((gp*icsInfo->maxSFB) >> 3);
      msMaskOffset = ((gp*icsInfo->maxSFB) & 0x07);
      msMask = (*msMaskPtr++) >> msMaskOffset;

      for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
        width = sfbTab[sfb+1] - sfbTab[sfb];
        if (sfbCodeBook[sfb] == 13) {
          if (ch == 0) {
            /* generate new vector, copy into ch 1 if it's possible that the channels will be correlated
             * if ch 1 has PNS enabled for this SFB but it's uncorrelated (i.e. ms_used == 0),
             *    the copied values will be overwritten when we process ch 1
             */
            GenerateNoiseVector(coef, &psi->pnsLastVal, width);
            if (checkCorr && psi->sfbCodeBook[1][gp*icsInfo->maxSFB + sfb] == 13)
              CopyNoiseVector(coef, psi->coef[1] + (coef - psi->coef[0]), width);
          } else {
            /* generate new vector if no correlation between channels */
            genNew = 1;
            if (checkCorr && psi->sfbCodeBook[0][gp*icsInfo->maxSFB + sfb] == 13) {
              if ( (psi->msMaskPresent == 1 && (msMask & 0x01)) || psi->msMaskPresent == 2 )
                genNew = 0;
            }
            if (genNew)
              GenerateNoiseVector(coef, &psi->pnsLastVal, width);
          }
          gbMask |= ScaleNoiseVector(coef, width, psi->scaleFactors[ch][gp*icsInfo->maxSFB + sfb]);
        }
        coef += width;

        /* get next mask bit (should be branchless on ARM) */
        msMask >>= 1;
        if (++msMaskOffset == 8) {
          msMask = *msMaskPtr++;
          msMaskOffset = 0;
        }
      }
      coef += (nSamps - sfbTab[icsInfo->maxSFB]);
    }
    sfbCodeBook += icsInfo->maxSFB;
    scaleFactors += icsInfo->maxSFB;
  }

  /* update guard bit count if necessary */
  gb = CLZ(gbMask) - 1;
  if (psi->gbCurrent[ch] > gb)
    psi->gbCurrent[ch] = gb;

  return 0;
}
//}}}
//}}}
//{{{  tns
#define FBITS_LPC_COEFS 20

/* TNS max bands (table 4.139) and max order (table 4.138) */
const int tnsMaxBandsShortOffset[AAC_NUM_PROFILES]  = {0, 0, 12};
//{{{
const unsigned /*char*/ int tnsMaxBandsShort[2*NUM_SAMPLE_RATES]  = {
  9,  9, 10, 14, 14, 14, 14, 14, 14, 14, 14, 14,   /* short block, Main/LC */
  7,  7,  7,  6,  6,  6,  7,  7,  8,  8,  8,  7    /* short block, SSR */
  };
//}}}
const unsigned /*char*/ int tnsMaxOrderShort[AAC_NUM_PROFILES]  = {7, 7, 7};
const int tnsMaxBandsLongOffset[AAC_NUM_PROFILES]  = {0, 0, 12};
//{{{
const unsigned int /*char*/ tnsMaxBandsLong[2*NUM_SAMPLE_RATES] = {
  31, 31, 34, 40, 42, 51, 46, 46, 42, 42, 42, 39,   /* long block, Main/LC */
  28, 28, 27, 26, 26, 26, 29, 29, 23, 23, 23, 19,   /* long block, SSR */
  };
//}}}
const unsigned /*char*/ int tnsMaxOrderLong[AAC_NUM_PROFILES]  = {20, 12, 12};

//{{{
/* inverse quantization tables for TNS filter coefficients, format = Q31
 * see bottom of file for table generation
 * negative (vs. spec) since we use MADD for filter kernel
 */
//}}}
//{{{
/*  Code to generate invQuantXXX[] tables
 *  {
 *    int res, i, t;
 *    double powScale, iqfac, iqfac_m, d;
 *
 *    powScale = pow(2.0, 31) * -1.0; / ** make coefficients negative for using MADD in kernel ** /
 *    for (res = 3; res <= 4; res++) {
 *      iqfac =   ( ((1 << (res-1)) - 0.5) * (2.0 / M_PI) );
 *      iqfac_m = ( ((1 << (res-1)) + 0.5) * (2.0 / M_PI) );
 *      printf("static const int invQuant%d[16] = {\n", res);
 *      for (i = 0; i < 16; i++) {
 *        / ** extend bottom 4 bits into signed, 2's complement number ** /
 *        t = (i << 28) >> 28;
 *
 *        if (t >= 0)   d = sin(t / iqfac);
 *        else          d = sin(t / iqfac_m);
 *
 *        d *= powScale;
 *        printf("0x%08x, ", (int)(d > 0 ? d + 0.5 : d - 0.5));
 *        if ((i & 0x07) == 0x07)
 *           printf("\n");
 *      }
 *      printf("};\n\n");
 *    }
 *  }
 */
//}}}
//{{{
static const int invQuant3[16]  = {
  0x00000000, 0xc8767f65, 0x9becf22c, 0x83358feb, 0x83358feb, 0x9becf22c, 0xc8767f65, 0x00000000,
  0x2bc750e9, 0x5246dd49, 0x6ed9eba1, 0x7e0e2e32, 0x7e0e2e32, 0x6ed9eba1, 0x5246dd49, 0x2bc750e9,
  };
//}}}
//{{{
static const int invQuant4[16]  = {
  0x00000000, 0xe5632654, 0xcbf00dbe, 0xb4c373ee, 0xa0e0a15f, 0x9126145f, 0x8643c7b3, 0x80b381ac,
  0x7f7437ad, 0x7b1d1a49, 0x7294b5f2, 0x66256db2, 0x563ba8aa, 0x4362210e, 0x2e3d2abb, 0x17851aad,
  };
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeLPCCoefs
 *
 * Description: decode LPC coefficients for TNS
 *
 * Inputs:      order of TNS filter
 *              resolution of coefficients (3 or 4 bits)
 *              coefficients unpacked from bitstream
 *              scratch buffer (b) of size >= order
 *
 * Outputs:     LPC coefficients in Q(FBITS_LPC_COEFS), in 'a'
 *
 * Return:      none
 *
 * Notes:       assumes no guard bits in input transform coefficients
 *              a[i] = Q(FBITS_LPC_COEFS), don't store a0 = 1.0
 *                (so a[0] = first delay tap, etc.)
 *              max abs(a[i]) < log2(order), so for max order = 20 a[i] < 4.4
 *                (up to 3 bits of gain) so a[i] has at least 31 - FBITS_LPC_COEFS - 3
 *                guard bits
 *              to ensure no intermediate overflow in all-pole filter, set
 *                FBITS_LPC_COEFS such that number of guard bits >= log2(max order)
 **************************************************************************************/
static void DecodeLPCCoefs (int order, int res, signed char* filtCoef, int* a, int* b)
{
  int i, m, t;
  const int *invQuantTab;

  if (res == 3)     invQuantTab = invQuant3;
  else if (res == 4)    invQuantTab = invQuant4;
  else          return;

  for (m = 0; m < order; m++) {
    t = invQuantTab[filtCoef[m] & 0x0f];  /* t = Q31 */
    for (i = 0; i < m; i++)
      b[i] = a[i] - (MULSHIFT32(t, a[m-i-1]) << 1);
    for (i = 0; i < m; i++)
      a[i] = b[i];
    a[m] = t >> (31 - FBITS_LPC_COEFS);
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    FilterRegion
 *
 * Description: apply LPC filter to one region of coefficients
 *
 * Inputs:      number of transform coefficients in this region
 *              direction flag (forward = 1, backward = -1)
 *              order of filter
 *              'size' transform coefficients
 *              'order' LPC coefficients in Q(FBITS_LPC_COEFS)
 *              scratch buffer for history (must be >= order samples long)
 *
 * Outputs:     filtered transform coefficients
 *
 * Return:      guard bit mask (OR of abs value of all filtered transform coefs)
 *
 * Notes:       assumes no guard bits in input transform coefficients
 *              gains 0 int bits
 *              history buffer does not need to be preserved between regions
 **************************************************************************************/
static int FilterRegion (int size, int dir, int order, int* audioCoef, int* a, int* hist)
{
  int i, j, y, hi32, inc, gbMask;
  U64 sum64;

  /* init history to 0 every time */
  for (i = 0; i < order; i++)
    hist[i] = 0;

  sum64.w64 = 0;     /* avoid warning */
  gbMask = 0;
  inc = (dir ? -1 : 1);
  do {
    /* sum64 = a0*y[n] = 1.0*y[n] */
    y = *audioCoef;
    sum64.r.hi32 = y >> (32 - FBITS_LPC_COEFS);
    sum64.r.lo32 = y << FBITS_LPC_COEFS;

    /* sum64 += (a1*y[n-1] + a2*y[n-2] + ... + a[order-1]*y[n-(order-1)]) */
        for (j = order - 1; j > 0; j--) {
      sum64.w64 = MADD64(sum64.w64, hist[j], a[j]);
            hist[j] = hist[j-1];
    }
    sum64.w64 = MADD64(sum64.w64, hist[0], a[0]);
    y = (sum64.r.hi32 << (32 - FBITS_LPC_COEFS)) | (sum64.r.lo32 >> FBITS_LPC_COEFS);

    /* clip output (rare) */
    hi32 = sum64.r.hi32;
    if ((hi32 >> 31) != (hi32 >> (FBITS_LPC_COEFS-1)))
      y = (hi32 >> 31) ^ 0x7fffffff;

    hist[0] = y;
    *audioCoef = y;
    audioCoef += inc;
    gbMask |= FASTABS(y);
  } while (--size);

  return gbMask;
}
//}}}

//{{{
/**************************************************************************************
 * Function:    TNSFilter
 *
 * Description: apply temporal noise shaping, if enabled
 *
 * Inputs:      valid AACDecInfo struct
 *              index of current channel
 *
 * Outputs:     updated transform coefficients
 *              updated minimum guard bit count for this channel
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
int TNSFilter (AACDecInfo* aacDecInfo, int ch)
{
  int win, winLen, nWindows, nSFB, filt, bottom, top, order, maxOrder, dir;
  int start, end, size, tnsMaxBand, numFilt, gbMask;
  int *audioCoef;
  unsigned char *filtLength, *filtOrder, *filtRes, *filtDir;
  signed char *filtCoef;
  const unsigned /*char*/ int *tnsMaxBandTab;
  const /*short*/ int *sfbTab;
  ICSInfo *icsInfo;
  TNSInfo *ti;
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);
  ti = &psi->tnsInfo[ch];

  if (!ti->tnsDataPresent)
    return 0;

  if (icsInfo->winSequence == 2) {
    nWindows = NWINDOWS_SHORT;
    winLen = NSAMPS_SHORT;
    nSFB = sfBandTotalShort[psi->sampRateIdx];
    maxOrder = tnsMaxOrderShort[aacDecInfo->profile];
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
    tnsMaxBandTab = tnsMaxBandsShort + tnsMaxBandsShortOffset[aacDecInfo->profile];
    tnsMaxBand = tnsMaxBandTab[psi->sampRateIdx];
  } else {
    nWindows = NWINDOWS_LONG;
    winLen = NSAMPS_LONG;
    nSFB = sfBandTotalLong[psi->sampRateIdx];
    maxOrder = tnsMaxOrderLong[aacDecInfo->profile];
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
    tnsMaxBandTab = tnsMaxBandsLong + tnsMaxBandsLongOffset[aacDecInfo->profile];
    tnsMaxBand = tnsMaxBandTab[psi->sampRateIdx];
  }

  if (tnsMaxBand > icsInfo->maxSFB)
    tnsMaxBand = icsInfo->maxSFB;

  filtRes =    ti->coefRes;
  filtLength = ti->length;
  filtOrder =  ti->order;
  filtDir =    ti->dir;
  filtCoef =   ti->coef;

  gbMask = 0;
  audioCoef =  psi->coef[ch];
  for (win = 0; win < nWindows; win++) {
    bottom = nSFB;
    numFilt = ti->numFilt[win];
    for (filt = 0; filt < numFilt; filt++) {
      top = bottom;
      bottom = top - *filtLength++;
      bottom = MAX(bottom, 0);
      order = *filtOrder++;
      order = MIN(order, maxOrder);

      if (order) {
        start = sfbTab[MIN(bottom, tnsMaxBand)];
        end   = sfbTab[MIN(top, tnsMaxBand)];
        size = end - start;
        if (size > 0) {
          dir = *filtDir++;
          if (dir)
            start = end - 1;

          DecodeLPCCoefs(order, filtRes[win], filtCoef, psi->tnsLPCBuf, psi->tnsWorkBuf);
          gbMask |= FilterRegion(size, dir, order, audioCoef + start, psi->tnsLPCBuf, psi->tnsWorkBuf);
        }
        filtCoef += order;
      }
    }
    audioCoef += winLen;
  }

  /* update guard bit count if necessary */
  size = CLZ(gbMask) - 1;
  if (psi->gbCurrent[ch] > size)
    psi->gbCurrent[ch] = size;

  return 0;
}
//}}}
//}}}
//{{{  stereo
//{{{
/* pow142[0][i] = -pow(2, i/4.0)
 * pow142[1][i] = +pow(2, i/4.0)
 *
 * i = [0,1,2,3]
 * format = Q30
 */
static const int pow142[2][4]  = {
  { 0xc0000000, 0xb3e407d7, 0xa57d8666, 0x945d819b },
  { 0x40000000, 0x4c1bf829, 0x5a82799a, 0x6ba27e65 }
};
//}}}
//{{{
/**************************************************************************************
 * Function:    StereoProcessGroup
 *
 * Description: apply mid-side and intensity stereo to group of transform coefficients
 *
 * Inputs:      dequantized transform coefficients for both channels
 *              pointer to appropriate scalefactor band table
 *              mid-side mask enabled flag
 *              buffer with mid-side mask (one bit for each scalefactor band)
 *              bit offset into mid-side mask buffer
 *              max coded scalefactor band
 *              buffer of codebook indices for right channel
 *              buffer of scalefactors for right channel, range = [0, 256]
 *
 * Outputs:     updated transform coefficients in Q(FBITS_OUT_DQ_OFF)
 *              updated minimum guard bit count for both channels
 *
 * Return:      none
 *
 * Notes:       assume no guard bits in input
 *              gains 0 int bits
 **************************************************************************************/
static void StereoProcessGroup (int *coefL, int *coefR, const /*short*/ int *sfbTab,
                int msMaskPres, unsigned char *msMaskPtr, int msMaskOffset, int maxSFB,
                unsigned char *cbRight, short *sfRight, int *gbCurrent)
{
  int sfb, width, cbIdx, sf, cl, cr, scalef, scalei;
  int gbMaskL, gbMaskR;
  unsigned char msMask;

  msMask = (*msMaskPtr++) >> msMaskOffset;
  gbMaskL = 0;
  gbMaskR = 0;

  for (sfb = 0; sfb < maxSFB; sfb++) {
    width = sfbTab[sfb+1] - sfbTab[sfb];  /* assume >= 0 (see sfBandTabLong/sfBandTabShort) */
    cbIdx = cbRight[sfb];

    if (cbIdx == 14 || cbIdx == 15) {
      /* intensity stereo */
      if (msMaskPres == 1 && (msMask & 0x01))
        cbIdx ^= 0x01;        /* invert_intensity(): 14 becomes 15, or 15 becomes 14 */
      sf = -sfRight[sfb];       /* negative since we use identity 0.5^(x) = 2^(-x) (see spec) */
      cbIdx &= 0x01;          /* choose - or + scale factor */
      scalef = pow142[cbIdx][sf & 0x03];
      scalei = (sf >> 2) + 2;     /* +2 to compensate for scalef = Q30 */

      if (scalei > 0) {
        if (scalei > 30)
          scalei = 30;
        do {
          cr = MULSHIFT32(*coefL++, scalef);
          CLIP_2N(cr, 31-scalei);
          cr <<= scalei;
          gbMaskR |= FASTABS(cr);
          *coefR++ = cr;
        } while (--width);
      } else {
        scalei = -scalei;
        if (scalei > 31)
          scalei = 31;
        do {
          cr = MULSHIFT32(*coefL++, scalef) >> scalei;
          gbMaskR |= FASTABS(cr);
          *coefR++ = cr;
        } while (--width);
      }
    } else if ( cbIdx != 13 && ((msMaskPres == 1 && (msMask & 0x01)) || msMaskPres == 2) ) {
      /* mid-side stereo (assumes no GB in inputs) */
      do {
        cl = *coefL;
        cr = *coefR;

        if ( (FASTABS(cl) | FASTABS(cr)) >> 30 ) {
          /* avoid overflow (rare) */
          cl >>= 1;
          sf = cl + (cr >> 1);  CLIP_2N(sf, 30);  sf <<= 1;
          cl = cl - (cr >> 1);  CLIP_2N(cl, 30);  cl <<= 1;
        } else {
          /* usual case */
          sf = cl + cr;
          cl -= cr;
        }

        *coefL++ = sf;
        gbMaskL |= FASTABS(sf);
        *coefR++ = cl;
        gbMaskR |= FASTABS(cl);
      } while (--width);

    } else {
      /* nothing to do */
      coefL += width;
      coefR += width;
    }

    /* get next mask bit (should be branchless on ARM) */
    msMask >>= 1;
    if (++msMaskOffset == 8) {
      msMask = *msMaskPtr++;
      msMaskOffset = 0;
    }
  }

  cl = CLZ(gbMaskL) - 1;
  if (gbCurrent[0] > cl)
    gbCurrent[0] = cl;

  cr = CLZ(gbMaskR) - 1;
  if (gbCurrent[1] > cr)
    gbCurrent[1] = cr;

  return;
}
//}}}

//{{{
/**************************************************************************************
 * Function:    StereoProcess
 *
 * Description: apply mid-side and intensity stereo, if enabled
 *
 * Inputs:      valid AACDecInfo struct (including dequantized transform coefficients)
 *
 * Outputs:     updated transform coefficients in Q(FBITS_OUT_DQ_OFF)
 *              updated minimum guard bit count for both channels
 *
 * Return:      0 if successful, -1 if error
 **************************************************************************************/
int StereoProcess (AACDecInfo *aacDecInfo)
{
  PSInfoBase *psi;
  ICSInfo *icsInfo;
  int gp, win, nSamps, msMaskOffset;
  int *coefL, *coefR;
  unsigned char *msMaskPtr;
  const /*short*/ int *sfbTab;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  /* mid-side and intensity stereo require common_window == 1 (see MPEG4 spec, Correction 2, 2004) */
  if (psi->commonWin != 1 || aacDecInfo->currBlockID != AAC_ID_CPE)
    return 0;

  /* nothing to do */
  if (!psi->msMaskPresent && !psi->intensityUsed[1])
    return 0;

  icsInfo = &(psi->icsInfo[0]);
  if (icsInfo->winSequence == 2) {
    sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
    nSamps = NSAMPS_SHORT;
  } else {
    sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
    nSamps = NSAMPS_LONG;
  }
  coefL = psi->coef[0];
  coefR = psi->coef[1];

  /* do fused mid-side/intensity processing for each block (one long or eight short) */
  msMaskOffset = 0;
  msMaskPtr = psi->msMaskBits;
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      StereoProcessGroup(coefL, coefR, sfbTab, psi->msMaskPresent,
        msMaskPtr, msMaskOffset, icsInfo->maxSFB, psi->sfbCodeBook[1] + gp*icsInfo->maxSFB,
        psi->scaleFactors[1] + gp*icsInfo->maxSFB, psi->gbCurrent);
      coefL += nSamps;
      coefR += nSamps;
    }
    /* we use one bit per sfb, so there are maxSFB bits for each window group */
    msMaskPtr += (msMaskOffset + icsInfo->maxSFB) >> 3;
    msMaskOffset = (msMaskOffset + icsInfo->maxSFB) & 0x07;
  }

  ASSERT(coefL == psi->coef[0] + 1024);
  ASSERT(coefR == psi->coef[1] + 1024);

  return 0;
}
//}}}
//}}}
//{{{  noiseless
static const signed char sgnMask[3] = {0x02,  0x04,  0x08};
static const signed char negMask[3] = {~0x03, ~0x07, ~0x0f};
//{{{
/**************************************************************************************
 * Function:    DecodeTNSInfo
 *
 * Description: decode TNS filter information
 *
 * Inputs:      BitStreamInfo struct pointing to start of TNS info
 *                (14496-3, table 4.4.27)
 *              window sequence (short or long blocks)
 *
 * Outputs:     updated TNSInfo struct
 *              buffer of decoded (signed) TNS filter coefficients
 *
 * Return:      none
 **************************************************************************************/
static void DecodeTNSInfo (BitStreamInfo *bsi, int winSequence, TNSInfo *ti, signed char *tnsCoef)
{
  int i, w, f, coefBits, compress;
  signed char c, s, n;

  unsigned char *filtLength, *filtOrder, *filtDir;

  filtLength = ti->length;
  filtOrder =  ti->order;
  filtDir =    ti->dir;

  if (winSequence == 2) {
    /* short blocks */
    for (w = 0; w < NWINDOWS_SHORT; w++) {
      ti->numFilt[w] = GetBits(bsi, 1);
      if (ti->numFilt[w]) {
        ti->coefRes[w] = GetBits(bsi, 1) + 3;
        *filtLength =    GetBits(bsi, 4);
        *filtOrder =     GetBits(bsi, 3);
        if (*filtOrder) {
          *filtDir++ =      GetBits(bsi, 1);
          compress =        GetBits(bsi, 1);
          coefBits = (int)ti->coefRes[w] - compress;  /* 2, 3, or 4 */
          s = sgnMask[coefBits - 2];
          n = negMask[coefBits - 2];
          for (i = 0; i < *filtOrder; i++) {
            c = GetBits(bsi, coefBits);
            if (c & s)  c |= n;
            *tnsCoef++ = c;
          }
        }
        filtLength++;
        filtOrder++;
      }
    }
  } else {
    /* long blocks */
    ti->numFilt[0] = GetBits(bsi, 2);
    if (ti->numFilt[0])
      ti->coefRes[0] = GetBits(bsi, 1) + 3;
    for (f = 0; f < ti->numFilt[0]; f++) {
      *filtLength =      GetBits(bsi, 6);
      *filtOrder =       GetBits(bsi, 5);
      if (*filtOrder) {
        *filtDir++ =     GetBits(bsi, 1);
        compress =       GetBits(bsi, 1);
        coefBits = (int)ti->coefRes[0] - compress;  /* 2, 3, or 4 */
        s = sgnMask[coefBits - 2];
        n = negMask[coefBits - 2];
        for (i = 0; i < *filtOrder; i++) {
          c = GetBits(bsi, coefBits);
          if (c & s)  c |= n;
          *tnsCoef++ = c;
        }
      }
      filtLength++;
      filtOrder++;
    }
  }
}
//}}}
//{{{
/* bitstream field lengths for gain control data:
 *   gainBits[winSequence][0] = maxWindow (how many gain windows there are)
 *   gainBits[winSequence][1] = locBitsZero (bits for alocCode if window == 0)
 *   gainBits[winSequence][2] = locBits (bits for alocCode if window != 0)
 */
static const unsigned char gainBits[4][3] = {
  {1, 5, 5},  /* long */
  {2, 4, 2},  /* start */
  {8, 2, 2},  /* short */
  {2, 4, 5},  /* stop */
};
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeGainControlInfo
 *
 * Description: decode gain control information (SSR profile only)
 *
 * Inputs:      BitStreamInfo struct pointing to start of gain control info
 *                (14496-3, table 4.4.12)
 *              window sequence (short or long blocks)
 *
 * Outputs:     updated GainControlInfo struct
 *
 * Return:      none
 **************************************************************************************/
static void DecodeGainControlInfo (BitStreamInfo *bsi, int winSequence, GainControlInfo *gi)
{
  int bd, wd, ad;
  int locBits, locBitsZero, maxWin;

  gi->maxBand = GetBits(bsi, 2);
  maxWin =      (int)gainBits[winSequence][0];
  locBitsZero = (int)gainBits[winSequence][1];
  locBits =     (int)gainBits[winSequence][2];

  for (bd = 1; bd <= gi->maxBand; bd++) {
    for (wd = 0; wd < maxWin; wd++) {
      gi->adjNum[bd][wd] = GetBits(bsi, 3);
      for (ad = 0; ad < gi->adjNum[bd][wd]; ad++) {
        gi->alevCode[bd][wd][ad] = GetBits(bsi, 4);
        gi->alocCode[bd][wd][ad] = GetBits(bsi, (wd == 0 ? locBitsZero : locBits));
      }
    }
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    DecodeICS
 *
 * Description: decode individual channel stream
 *
 * Inputs:      platform specific info struct
 *              BitStreamInfo struct pointing to start of individual channel stream
 *                (14496-3, table 4.4.24)
 *              index of current channel
 *
 * Outputs:     updated section data, scale factor data, pulse data, TNS data,
 *                and gain control data
 *
 * Return:      none
 **************************************************************************************/
static void DecodeICS (PSInfoBase *psi, BitStreamInfo *bsi, int ch)
{
  int globalGain;
  ICSInfo *icsInfo;
  PulseInfo *pi;
  TNSInfo *ti;
  GainControlInfo *gi;

  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  globalGain = GetBits(bsi, 8);
  if (!psi->commonWin)
    DecodeICSInfo(bsi, icsInfo, psi->sampRateIdx);

  DecodeSectionData(bsi, icsInfo->winSequence, icsInfo->numWinGroup, icsInfo->maxSFB, psi->sfbCodeBook[ch]);

  DecodeScaleFactors(bsi, icsInfo->numWinGroup, icsInfo->maxSFB, globalGain, psi->sfbCodeBook[ch], psi->scaleFactors[ch]);

  pi = &psi->pulseInfo[ch];
  pi->pulseDataPresent = GetBits(bsi, 1);
  if (pi->pulseDataPresent)
    DecodePulseInfo(bsi, pi);

  ti = &psi->tnsInfo[ch];
  ti->tnsDataPresent = GetBits(bsi, 1);
  if (ti->tnsDataPresent)
    DecodeTNSInfo(bsi, icsInfo->winSequence, ti, ti->coef);

  gi = &psi->gainControlInfo[ch];
  gi->gainControlDataPresent = GetBits(bsi, 1);
  if (gi->gainControlDataPresent)
    DecodeGainControlInfo(bsi, icsInfo->winSequence, gi);
}
//}}}

//{{{
/**************************************************************************************
 * Function:    DecodeNoiselessData
 *
 * Description: decode noiseless data (side info and transform coefficients)
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer pointing to start of individual channel stream
 *                (14496-3, table 4.4.24)
 *              pointer to bit offset
 *              pointer to number of valid bits remaining in buf
 *              index of current channel
 *
 * Outputs:     updated global gain, section data, scale factor data, pulse data,
 *                TNS data, gain control data, and spectral data
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int DecodeNoiselessData (AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail, int ch)
{
  int bitsUsed;
  BitStreamInfo bsi;
  PSInfoBase *psi;
  ICSInfo *icsInfo;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  SetBitstreamPointer(&bsi, (*bitsAvail+7) >> 3, *buf);
  GetBits(&bsi, *bitOffset);

  DecodeICS(psi, &bsi, ch);

  if (icsInfo->winSequence == 2)
    DecodeSpectrumShort(psi, &bsi, ch);
  else
    DecodeSpectrumLong(psi, &bsi, ch);

  bitsUsed = CalcBitsUsed(&bsi, *buf, *bitOffset);
  *buf += ((bitsUsed + *bitOffset) >> 3);
  *bitOffset = ((bitsUsed + *bitOffset) & 0x07);
  *bitsAvail -= bitsUsed;

  aacDecInfo->sbDeinterleaveReqd[ch] = 0;
  aacDecInfo->tnsUsed |= psi->tnsInfo[ch].tnsDataPresent; /* set flag if TNS used for any channel */

  return ERR_AAC_NONE;
}
//}}}
//}}}

//{{{
 /**************************************************************************************
 * Function:    UnpackADTSHeader
 * Description: parse the ADTS frame header and initialize decoder state
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer with complete ADTS frame header (byte aligned)
 *                header size = 7 bytes, plus 2 if CRC
 * Outputs:     filled in ADTS struct
 *              updated buffer pointer
 *              updated bit offset
 *              updated number of available bits
 * Return:      0 if successful, error code (< 0) if error
 * TODO:        test CRC
 *              verify that fixed fields don't change between frames
 **************************************************************************************/
int UnpackADTSHeader (AACDecInfo* aacDecInfo, unsigned char** buf, int* bitOffset, int* bitsAvail)
{
  int bitsUsed;
  PSInfoBase *psi;
  BitStreamInfo bsi;
  ADTSHeader *fhADTS;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  fhADTS = &(psi->fhADTS);

  /* init bitstream reader */
  SetBitstreamPointer(&bsi, (*bitsAvail + 7) >> 3, *buf);
  GetBits(&bsi, *bitOffset);

  /* verify that first 12 bits of header are syncword */
  if (GetBits(&bsi, 12) != 0x0fff) {
    return ERR_AAC_INVALID_ADTS_HEADER;
  }

  /* fixed fields - should not change from frame to frame */
  fhADTS->id =               GetBits(&bsi, 1);
  fhADTS->layer =            GetBits(&bsi, 2);
  fhADTS->protectBit =       GetBits(&bsi, 1);
  fhADTS->profile =          GetBits(&bsi, 2);
  fhADTS->sampRateIdx =      GetBits(&bsi, 4);
  fhADTS->privateBit =       GetBits(&bsi, 1);
  fhADTS->channelConfig =    GetBits(&bsi, 3);
  fhADTS->origCopy =         GetBits(&bsi, 1);
  fhADTS->home =             GetBits(&bsi, 1);

  /* variable fields - can change from frame to frame */
  fhADTS->copyBit =          GetBits(&bsi, 1);
  fhADTS->copyStart =        GetBits(&bsi, 1);
  fhADTS->frameLength =      GetBits(&bsi, 13);
  fhADTS->bufferFull =       GetBits(&bsi, 11);
  fhADTS->numRawDataBlocks = GetBits(&bsi, 2) + 1;

  /* note - MPEG4 spec, correction 1 changes how CRC is handled when protectBit == 0 and numRawDataBlocks > 1 */
  if (fhADTS->protectBit == 0)
    fhADTS->crcCheckWord = GetBits(&bsi, 16);

  /* byte align */
  ByteAlignBitstream(&bsi); /* should always be aligned anyway */

  /* check validity of header */
  if (fhADTS->layer != 0 || fhADTS->profile != AAC_PROFILE_LC ||
    fhADTS->sampRateIdx >= NUM_SAMPLE_RATES || fhADTS->channelConfig >= NUM_DEF_CHAN_MAPS)
    return ERR_AAC_INVALID_ADTS_HEADER;

  /* update codec info */
  psi->sampRateIdx = fhADTS->sampRateIdx;
  if (!psi->useImpChanMap)
    psi->nChans = channelMapTab[fhADTS->channelConfig];

  /* syntactic element fields will be read from bitstream for each element */
  aacDecInfo->prevBlockID = AAC_ID_INVALID;
  aacDecInfo->currBlockID = AAC_ID_INVALID;
  aacDecInfo->currInstTag = -1;

  /* fill in user-accessible data (TODO - calc bitrate, handle tricky channel config cases) */
  aacDecInfo->bitRate = 0;
  aacDecInfo->nChans = psi->nChans;
  aacDecInfo->sampRate = sampRateTab[psi->sampRateIdx];
  aacDecInfo->profile = fhADTS->profile;
  aacDecInfo->sbrEnabled = 0;
  aacDecInfo->adtsBlocksLeft = fhADTS->numRawDataBlocks;

  /* update bitstream reader */
  bitsUsed = CalcBitsUsed(&bsi, *buf, *bitOffset);
  *buf += (bitsUsed + *bitOffset) >> 3;
  *bitOffset = (bitsUsed + *bitOffset) & 0x07;
  *bitsAvail -= bitsUsed ;
  if (*bitsAvail < 0)
    return ERR_AAC_INDATA_UNDERFLOW;

  return ERR_AAC_NONE;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    GetADTSChannelMapping
 * Description: determine the number of channels from implicit mapping rules
 * Inputs:      valid AACDecInfo struct
 *              pointer to start of raw_data_block
 *              bit offset
 *              bits available
 * Outputs:     updated number of channels
 * Return:      0 if successful, error code (< 0) if error
 * Notes:       calculates total number of channels using rules in 14496-3, 4.5.1.2.1
 *              does not attempt to deduce speaker geometry
 **************************************************************************************/
int GetADTSChannelMapping (AACDecInfo* aacDecInfo, unsigned char* buf, int bitOffset, int bitsAvail)
{
  int ch, nChans, elementChans, err;
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  nChans = 0;
  do {
    /* parse next syntactic element */
    err = DecodeNextElement(aacDecInfo, &buf, &bitOffset, &bitsAvail);
    if (err)
      return err;

    elementChans = elementNumChans[aacDecInfo->currBlockID];
    nChans += elementChans;

    for (ch = 0; ch < elementChans; ch++) {
      err = DecodeNoiselessData(aacDecInfo, &buf, &bitOffset, &bitsAvail, ch);
      if (err)
        return err;
    }
  } while (aacDecInfo->currBlockID != AAC_ID_END);

  if (nChans <= 0)
    return ERR_AAC_CHANNEL_MAP;

  /* update number of channels in codec state and user-accessible info structs */
  psi->nChans = nChans;
  aacDecInfo->nChans = psi->nChans;
  psi->useImpChanMap = 1;

  return ERR_AAC_NONE;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    PrepareRawBlock
 * Description: reset per-block state variables for raw blocks (no ADTS headers)
 * Inputs:      valid AACDecInfo struct
 * Outputs:     updated state variables in aacDecInfo
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int PrepareRawBlock (AACDecInfo* aacDecInfo)
{
  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;

  /* syntactic element fields will be read from bitstream for each element */
  aacDecInfo->prevBlockID = AAC_ID_INVALID;
  aacDecInfo->currBlockID = AAC_ID_INVALID;
  aacDecInfo->currInstTag = -1;

  /* fill in user-accessible data */
  aacDecInfo->bitRate = 0;
  aacDecInfo->sbrEnabled = 0;

  return ERR_AAC_NONE;
}
//}}}

// main decoder
//{{{
HAACDecoder AACInitDecoder() {

  AACDecInfo* aacDecInfo = (AACDecInfo *)malloc (sizeof(AACDecInfo));
  memset (aacDecInfo, 0, sizeof(AACDecInfo));

  aacDecInfo->psInfoBase = malloc (sizeof(PSInfoBase));
  memset (aacDecInfo->psInfoBase, 0, sizeof(PSInfoBase));

  InitSBR (aacDecInfo);
  return (HAACDecoder)aacDecInfo;
  }
//}}}

//{{{
int AACFindSyncWord (unsigned char* buf, int nBytes) {

  int i;
  for (i = 0; i < nBytes - 1; i++)
    if ( (buf[i+0] & SYNCWORDH) == SYNCWORDH && (buf[i+1] & SYNCWORDL) == SYNCWORDL )
      return i;

  return -1;
  }
//}}}
//{{{
/**************************************************************************************
 * Function:    AACFlushCodec
 * Description: flush internal codec state (after seeking, for example)
 * Inputs:      valid AAC decoder instance pointer (HAACDecoder)
 * Outputs:     updated state variables in aacDecInfo
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int AACFlushCodec (HAACDecoder hAACDecoder) {

  // reset common state variables which change per-frame
  // don't touch state variables which are (usually) constant for entire clip
  //  (nChans, sampRate, profile, format, sbrEnabled)
  AACDecInfo* aacDecInfo = (AACDecInfo *)hAACDecoder;
  aacDecInfo->prevBlockID = AAC_ID_INVALID;
  aacDecInfo->currBlockID = AAC_ID_INVALID;
  aacDecInfo->currInstTag = -1;
  for (int ch = 0; ch < MAX_NCHANS_ELEM; ch++)
    aacDecInfo->sbDeinterleaveReqd[ch] = 0;
  aacDecInfo->adtsBlocksLeft = 0;
  aacDecInfo->tnsUsed = 0;
  aacDecInfo->pnsUsed = 0;

  /* reset internal codec state (flush overlap buffers, etc.) */
  PSInfoBase* psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  memset (psi->overlap, 0, AAC_MAX_NCHANS * AAC_MAX_NSAMPS * sizeof(int));
  memset (psi->prevWinShape, 0, AAC_MAX_NCHANS * sizeof(int));

  FlushCodecSBR (aacDecInfo);

  return ERR_AAC_NONE;
  }
//}}}

//{{{
/**************************************************************************************
 * Function:    AACDecode
 *
 * Description: decode AAC frame
 *
 * Inputs:      valid AAC decoder instance pointer (HAACDecoder)
 *              double pointer to buffer of AAC data
 *              pointer to number of valid bytes remaining in inbuf
 *              pointer to outbuf, big enough to hold one frame of decoded PCM samples
 *                (outbuf must be double-sized if SBR enabled)
 *
 * Outputs:     PCM data in outbuf, interleaved LRLRLR... if stereo
 *                number of output samples = 1024 per channel (2048 if SBR enabled)
 *              updated inbuf pointer
 *              updated bytesLeft
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       inbuf pointer and bytesLeft are not updated until whole frame is
 *                successfully decoded, so if ERR_AAC_INDATA_UNDERFLOW is returned
 *                just call AACDecode again with more data in inbuf
 **************************************************************************************/
int AACDecode (HAACDecoder hAACDecoder, unsigned char **inbuf, int *bytesLeft, short *outbuf)
{
  int err, offset, bitOffset, bitsAvail;
  int ch, baseChan, elementChans;
  unsigned char *inptr;
  AACDecInfo *aacDecInfo = (AACDecInfo *)hAACDecoder;
  int baseChanSBR, elementChansSBR;

  if (!aacDecInfo)
    return ERR_AAC_NULL_POINTER;

  /* make local copies (see "Notes" above) */
  inptr = *inbuf;
  bitOffset = 0;
  bitsAvail = (*bytesLeft) << 3;

  /* first time through figure out what the file format is */
  if (aacDecInfo->format == AAC_FF_Unknown) {
    if (bitsAvail < 32)
      return ERR_AAC_INDATA_UNDERFLOW;

    /* assume ADTS by default */
    aacDecInfo->format = AAC_FF_ADTS;
  }

  /* if ADTS, search for start of next frame */
  if (aacDecInfo->format == AAC_FF_ADTS) {
    /* can have 1-4 raw data blocks per ADTS frame (header only present for first one) */
    if (aacDecInfo->adtsBlocksLeft == 0) {
      offset = AACFindSyncWord (inptr, bitsAvail >> 3);
      if (offset < 0)
        return ERR_AAC_INDATA_UNDERFLOW;
      inptr += offset;
      bitsAvail -= (offset << 3);

      err = UnpackADTSHeader(aacDecInfo, &inptr, &bitOffset, &bitsAvail);
      if (err)
        return err;

      if (aacDecInfo->nChans == -1) {
        /* figure out implicit channel mapping if necessary */
        err = GetADTSChannelMapping(aacDecInfo, inptr, bitOffset, bitsAvail);
        if (err)
          return err;
      }
    }
    aacDecInfo->adtsBlocksLeft--;
  } else if (aacDecInfo->format == AAC_FF_RAW) {
    err = PrepareRawBlock(aacDecInfo);
    if (err)
      return err;
  }



  /* check for valid number of channels */
  if (aacDecInfo->nChans > AAC_MAX_NCHANS || aacDecInfo->nChans <= 0)
    return ERR_AAC_NCHANS_TOO_HIGH;

  /* will be set later if active in this frame */
  aacDecInfo->tnsUsed = 0;
  aacDecInfo->pnsUsed = 0;

  bitOffset = 0;
  baseChan = 0;
  baseChanSBR = 0;
  do {



    /* parse next syntactic element */
    err = DecodeNextElement(aacDecInfo, &inptr, &bitOffset, &bitsAvail);
    if (err)
      return err;

    elementChans = elementNumChans[aacDecInfo->currBlockID];
    if (baseChan + elementChans > AAC_MAX_NCHANS)
      return ERR_AAC_NCHANS_TOO_HIGH;

    /* noiseless decoder and dequantizer */
    for (ch = 0; ch < elementChans; ch++) {
      err = DecodeNoiselessData(aacDecInfo, &inptr, &bitOffset, &bitsAvail, ch);

      if (err)
        return err;

      if (Dequantize(aacDecInfo, ch))
        return ERR_AAC_DEQUANT;
    }

    /* mid-side and intensity stereo */
    if (aacDecInfo->currBlockID == AAC_ID_CPE) {
      if (StereoProcess(aacDecInfo))
        return ERR_AAC_STEREO_PROCESS;
    }


    /* PNS, TNS, inverse transform */
    for (ch = 0; ch < elementChans; ch++) {
      if (PNS(aacDecInfo, ch))
        return ERR_AAC_PNS;

      if (aacDecInfo->sbDeinterleaveReqd[ch]) {
        /* deinterleave short blocks, if required */
        if (DeinterleaveShortBlocks(aacDecInfo, ch))
          return ERR_AAC_SHORT_BLOCK_DEINT;
        aacDecInfo->sbDeinterleaveReqd[ch] = 0;
      }

      if (TNSFilter(aacDecInfo, ch))
        return ERR_AAC_TNS;

      if (IMDCT(aacDecInfo, ch, baseChan + ch, outbuf))
        return ERR_AAC_IMDCT;
    }

    if (aacDecInfo->sbrEnabled && (aacDecInfo->currBlockID == AAC_ID_FIL || aacDecInfo->currBlockID == AAC_ID_LFE)) {
      if (aacDecInfo->currBlockID == AAC_ID_LFE)
        elementChansSBR = elementNumChans[AAC_ID_LFE];
      else if (aacDecInfo->currBlockID == AAC_ID_FIL && (aacDecInfo->prevBlockID == AAC_ID_SCE || aacDecInfo->prevBlockID == AAC_ID_CPE))
        elementChansSBR = elementNumChans[aacDecInfo->prevBlockID];
      else
        elementChansSBR = 0;

      if (baseChanSBR + elementChansSBR > AAC_MAX_NCHANS)
        return ERR_AAC_SBR_NCHANS_TOO_HIGH;

      /* parse SBR extension data if present (contained in a fill element) */
      if (DecodeSBRBitstream(aacDecInfo, baseChanSBR))
        return ERR_AAC_SBR_BITSTREAM;

      /* apply SBR */
      if (DecodeSBRData(aacDecInfo, baseChanSBR, outbuf))
        return ERR_AAC_SBR_DATA;

      baseChanSBR += elementChansSBR;
    }

    baseChan += elementChans;
  } while (aacDecInfo->currBlockID != AAC_ID_END);

  /* byte align after each raw_data_block */
  if (bitOffset) {
    inptr++;
    bitsAvail -= (8-bitOffset);
    bitOffset = 0;
    if (bitsAvail < 0)
      return ERR_AAC_INDATA_UNDERFLOW;
  }

  /* update pointers */
  aacDecInfo->frameCount++;
  *bytesLeft -= (int)(inptr - *inbuf);
  *inbuf = inptr;

  return ERR_AAC_NONE;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    AACGetLastFrameInfo
 *
 * Description: get info about last AAC frame decoded (number of samples decoded,
 *                sample rate, bit rate, etc.)
 *
 * Inputs:      valid AAC decoder instance pointer (HAACDecoder)
 *              pointer to AACFrameInfo struct
 *
 * Outputs:     filled-in AACFrameInfo struct
 *
 * Return:      none
 *
 * Notes:       call this right after calling AACDecode()
 **************************************************************************************/
void AACGetLastFrameInfo (HAACDecoder hAACDecoder, AACFrameInfo *aacFrameInfo) {

  AACDecInfo* aacDecInfo = (AACDecInfo*)hAACDecoder;
  aacFrameInfo->bitRate =       aacDecInfo->bitRate;
  aacFrameInfo->nChans =        aacDecInfo->nChans;
  aacFrameInfo->sampRateCore =  aacDecInfo->sampRate;
  aacFrameInfo->sampRateOut =   aacDecInfo->sampRate * (aacDecInfo->sbrEnabled ? 2 : 1);
  aacFrameInfo->bitsPerSample = 16;
  aacFrameInfo->outputSamps =   aacDecInfo->nChans * AAC_MAX_NSAMPS * (aacDecInfo->sbrEnabled ? 2 : 1);
  aacFrameInfo->profile =       aacDecInfo->profile;
  aacFrameInfo->tnsUsed =       aacDecInfo->tnsUsed;
  aacFrameInfo->pnsUsed =       aacDecInfo->pnsUsed;
  }
//}}}

//{{{
void AACFreeDecoder (HAACDecoder hAACDecoder) {

  AACDecInfo* aacDecInfo = (AACDecInfo *)hAACDecoder;
  FreeSBR (aacDecInfo);
  if (aacDecInfo)
    free (aacDecInfo->psInfoBase);
  free (aacDecInfo);
  }
//}}}
