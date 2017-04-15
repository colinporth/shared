// cMpeg2decoder.h
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include "../video/cYuvFrame.h"
//}}}
//{{{  const
#define SLICE_START_CODE_MIN     0x101
#define SLICE_START_CODE_MAX     0x1AF
#define USER_DATA_START_CODE     0x1B2
#define SEQUENCE_HEADER_CODE     0x1B3
#define EXTENSION_START_CODE     0x1B5
#define SEQUENCE_END_CODE        0x1B7
#define GROUP_START_CODE         0x1B8

#define SEQUENCE_EXTENSION_ID        1
#define PICTURE_CODING_EXTENSION_ID  8

// picture coding type
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3

// macroblock type
#define MACROBLOCK_INTRA                        1
#define MACROBLOCK_PATTERN                      2
#define MACROBLOCK_MOTION_BACKWARD              4
#define MACROBLOCK_MOTION_FORWARD               8
#define MACROBLOCK_QUANT                        16
#define SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG       32
#define PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS 64

// motion_type
#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8  2

// mv_format
#define MV_FIELD 0
#define MV_FRAME 1
//}}}
//{{{  tables
//{{{
static const uint8_t scan[2][64] = {
  { /* Zig-Zag scan pattern  */
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
    12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
  },
  { /* Alternate scan pattern */
    0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
    41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
    51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
    53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
  }} ;
//}}}
//{{{
/* default intra quantization matrix */
static const uint8_t default_intra_quantizer_matrix[64] = {
  8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83 } ;
//}}}
//{{{
/* non-linear quantization coefficient table */
static const uint8_t Non_Linear_quantizer_scale[32] = {
   0, 1, 2, 3, 4, 5, 6, 7,
   8,10,12,14,16,18,20,22,
  24,28,32,36,40,44,48,52,
  56,64,72,80,88,96,104,112 } ;
//}}}

//{{{  struct VLCtab
typedef struct {
  char val, len;
  } VLCtab;
//}}}
//{{{
/* Table B-3, macroblock_type in P-pictures, codes 001..1xx */
static const VLCtab PMBtab0[8] = {
  {-1,0},
  {MACROBLOCK_MOTION_FORWARD,3},
  {MACROBLOCK_PATTERN,2}, {MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,1}
};
//}}}
//{{{
/* Table B-3, macroblock_type in P-pictures, codes 000001..00011x */
static const VLCtab PMBtab1[8] = {
  {-1,0},
  {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
  {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5}, {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_INTRA,5}, {MACROBLOCK_INTRA,5}
};
//}}}
//{{{
/* Table B-4, macroblock_type in B-pictures, codes 0010..11xx */
static const VLCtab BMBtab0[16] = {
  {-1,0},
  {-1,0},
  {MACROBLOCK_MOTION_FORWARD,4},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,4},
  {MACROBLOCK_MOTION_BACKWARD,3},
  {MACROBLOCK_MOTION_BACKWARD,3},
  {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
  {MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,3},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2},
  {MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,2}
};
//}}}
//{{{
/* Table B-4, macroblock_type in B-pictures, codes 000001..00011x */
static const VLCtab BMBtab1[8] = {
  {-1,0},
  {MACROBLOCK_QUANT|MACROBLOCK_INTRA,6},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,6},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_PATTERN,6},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_QUANT|MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD|MACROBLOCK_PATTERN,5},
  {MACROBLOCK_INTRA,5},
  {MACROBLOCK_INTRA,5}
};
//}}}

//{{{
/* Table B-10, motion_code, codes 0001 ... 01xx */
static const VLCtab MVtab0[8] =
{ {-1,0}, {3,3}, {2,2}, {2,2}, {1,1}, {1,1}, {1,1}, {1,1}
};
//}}}
//{{{
/* Table B-10, motion_code, codes 0000011 ... 000011x */
static const VLCtab MVtab1[8] =
{ {-1,0}, {-1,0}, {-1,0}, {7,6}, {6,6}, {5,6}, {4,5}, {4,5}
};
//}}}
//{{{
/* Table B-10, motion_code, codes 0000001100 ... 000001011x */
static const VLCtab MVtab2[12] =
{ {16,9}, {15,9}, {14,9}, {13,9},
  {12,9}, {11,9}, {10,8}, {10,8},
  {9,8},  {9,8},  {8,8},  {8,8}
};
//}}}

//{{{
/* Table B-9, coded_block_pattern, codes 01000 ... 111xx */
static const VLCtab CBPtab0[32] =
{ {-1,0}, {-1,0}, {-1,0}, {-1,0},
  {-1,0}, {-1,0}, {-1,0}, {-1,0},
  {62,5}, {2,5},  {61,5}, {1,5},  {56,5}, {52,5}, {44,5}, {28,5},
  {40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
  {8,4},  {8,4},  {4,4},  {4,4},  {60,3}, {60,3}, {60,3}, {60,3}
};
//}}}
//{{{
/* Table B-9, coded_block_pattern, codes 00000100 ... 001111xx */
static const VLCtab CBPtab1[64] =
{ {-1,0}, {-1,0}, {-1,0}, {-1,0},
  {58,8}, {54,8}, {46,8}, {30,8},
  {57,8}, {53,8}, {45,8}, {29,8}, {38,8}, {26,8}, {37,8}, {25,8},
  {43,8}, {23,8}, {51,8}, {15,8}, {42,8}, {22,8}, {50,8}, {14,8},
  {41,8}, {21,8}, {49,8}, {13,8}, {35,8}, {19,8}, {11,8}, {7,8},
  {34,7}, {34,7}, {18,7}, {18,7}, {10,7}, {10,7}, {6,7},  {6,7},
  {33,7}, {33,7}, {17,7}, {17,7}, {9,7},  {9,7},  {5,7},  {5,7},
  {63,6}, {63,6}, {63,6}, {63,6}, {3,6},  {3,6},  {3,6},  {3,6},
  {36,6}, {36,6}, {36,6}, {36,6}, {24,6}, {24,6}, {24,6}, {24,6}
};
//}}}
//{{{
/* Table B-9, coded_block_pattern, codes 000000001 ... 000000111 */
static const VLCtab CBPtab2[8] =
{ {-1,0}, {0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9}
};
//}}}

//{{{
/* Table B-1, macroblock_address_increment, codes 00010 ... 011xx */
static const VLCtab MBAtab1[16] =
{ {-1,0}, {-1,0}, {7,5}, {6,5}, {5,4}, {5,4}, {4,4}, {4,4},
  {3,3}, {3,3}, {3,3}, {3,3}, {2,3}, {2,3}, {2,3}, {2,3}
};
//}}}
//{{{
/* Table B-1, macroblock_address_increment, codes 00000011000 ... 0000111xxxx */
static const VLCtab MBAtab2[104] =
{
  {33,11}, {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11},
  {25,11}, {24,11}, {23,11}, {22,11}, {21,10}, {21,10}, {20,10}, {20,10},
  {19,10}, {19,10}, {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {16,10},
  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},
  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},
  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},
  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},
  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},
  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},
  {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
  {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
  {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},
  {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7}
};
//}}}

//{{{
/* Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
static const VLCtab DClumtab0[32] =
{ {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
  {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
  {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
  {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {-1, 0}
};
//}}}
//{{{
/* Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
static const VLCtab DClumtab1[16] =
{ {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
  {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10,9}, {11,9}
};
//}}}
//{{{
/* Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
static const VLCtab DCchromtab0[32] =
{ {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
  {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
  {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
  {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {-1, 0}
};
//}}}
//{{{
/* Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
static const VLCtab DCchromtab1[32] =
{ {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
  {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
  {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7},
  {8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10,10}, {11,10}
};
//}}}

//{{{  struct DCTtab
typedef struct {
  char run, level, len;
  } DCTtab;
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for first (DC) coefficient)
 */
static const DCTtab DCTtabfirst[12] =
{
  {0,2,4}, {2,1,4}, {1,1,3}, {1,1,3},
  {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1},
  {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 0100 ... 1xxx (used for all other coefficients)
 */
static const DCTtab DCTtabnext[12] =
{
  {0,2,4},  {2,1,4},  {1,1,3},  {1,1,3},
  {64,0,2}, {64,0,2}, {64,0,2}, {64,0,2}, /* EOB */
  {0,1,2},  {0,1,2},  {0,1,2},  {0,1,2}
};
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 000001xx ... 00111xxx
 */
static const DCTtab DCTtab0[60] =
{
  {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
  {2,2,7}, {2,2,7}, {9,1,7}, {9,1,7},
  {0,4,7}, {0,4,7}, {8,1,7}, {8,1,7},
  {7,1,6}, {7,1,6}, {7,1,6}, {7,1,6},
  {6,1,6}, {6,1,6}, {6,1,6}, {6,1,6},
  {1,2,6}, {1,2,6}, {1,2,6}, {1,2,6},
  {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
  {13,1,8}, {0,6,8}, {12,1,8}, {11,1,8},
  {3,2,8}, {1,3,8}, {0,5,8}, {10,1,8},
  {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
  {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
  {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
  {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5}
};
//}}}
//{{{
/* Table B-15, DCT coefficients table one,
 * codes 000001xx ... 11111111
*/
static const DCTtab DCTtab0a[252] =
{
  {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
  {7,1,7}, {7,1,7}, {8,1,7}, {8,1,7},
  {6,1,7}, {6,1,7}, {2,2,7}, {2,2,7},
  {0,7,6}, {0,7,6}, {0,7,6}, {0,7,6},
  {0,6,6}, {0,6,6}, {0,6,6}, {0,6,6},
  {4,1,6}, {4,1,6}, {4,1,6}, {4,1,6},
  {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
  {1,5,8}, {11,1,8}, {0,11,8}, {0,10,8},
  {13,1,8}, {12,1,8}, {3,2,8}, {1,4,8},
  {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
  {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
  {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
  {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4}, /* EOB */
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
  {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
  {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
  {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
  {9,1,7}, {9,1,7}, {1,3,7}, {1,3,7},
  {10,1,7}, {10,1,7}, {0,8,7}, {0,8,7},
  {0,9,7}, {0,9,7}, {0,12,8}, {0,13,8},
  {2,3,8}, {4,2,8}, {0,14,8}, {0,15,8}
};
//}}}
//{{{
/* Table B-14, DCT coefficients table zero,
 * codes 0000001000 ... 0000001111
 */
static const DCTtab DCTtab1[8] =
{
  {16,1,10}, {5,2,10}, {0,7,10}, {2,3,10},
  {1,4,10}, {15,1,10}, {14,1,10}, {4,2,10}
};
//}}}
//{{{
/* Table B-15, DCT coefficients table one,
 * codes 000000100x ... 000000111x
 */
static const DCTtab DCTtab1a[8] =
{
  {5,2,9}, {5,2,9}, {14,1,9}, {14,1,9},
  {2,4,10}, {16,1,10}, {15,1,9}, {15,1,9}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000010000 ... 000000011111
 */
static const DCTtab DCTtab2[16] =
{
  {0,11,12}, {8,2,12}, {4,3,12}, {0,10,12},
  {2,4,12}, {7,2,12}, {21,1,12}, {20,1,12},
  {0,9,12}, {19,1,12}, {18,1,12}, {1,5,12},
  {3,3,12}, {0,8,12}, {6,2,12}, {17,1,12}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000010000 ... 0000000011111
 */
static const DCTtab DCTtab3[16] =
{
  {10,2,13}, {9,2,13}, {5,3,13}, {3,4,13},
  {2,5,13}, {1,7,13}, {1,6,13}, {0,15,13},
  {0,14,13}, {0,13,13}, {0,12,13}, {26,1,13},
  {25,1,13}, {24,1,13}, {23,1,13}, {22,1,13}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 00000000010000 ... 00000000011111
 */
static const DCTtab DCTtab4[16] =
{
  {0,31,14}, {0,30,14}, {0,29,14}, {0,28,14},
  {0,27,14}, {0,26,14}, {0,25,14}, {0,24,14},
  {0,23,14}, {0,22,14}, {0,21,14}, {0,20,14},
  {0,19,14}, {0,18,14}, {0,17,14}, {0,16,14}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 000000000010000 ... 000000000011111
 */
static const DCTtab DCTtab5[16] =
{
  {0,40,15}, {0,39,15}, {0,38,15}, {0,37,15},
  {0,36,15}, {0,35,15}, {0,34,15}, {0,33,15},
  {0,32,15}, {1,14,15}, {1,13,15}, {1,12,15},
  {1,11,15}, {1,10,15}, {1,9,15}, {1,8,15}
};
//}}}
//{{{
/* Table B-14/15, DCT coefficients table zero / one,
 * codes 0000000000010000 ... 0000000000011111
 */
static const DCTtab DCTtab6[16] =
{
  {1,18,16}, {1,17,16}, {1,16,16}, {1,15,16},
  {6,3,16}, {16,2,16}, {15,2,16}, {14,2,16},
  {13,2,16}, {12,2,16}, {11,2,16}, {31,1,16},
  {30,1,16}, {29,1,16}, {28,1,16}, {27,1,16}
};
//}}}

//{{{
static __declspec(align(64)) const int16_t sse2_tab_i_04[] = {
  16384, 21407, 16384,  8867, 16384, -8867, 16384,-21407,  // w05 w04 w01 w00 w13 w12 w09 w08
  16384,  8867,-16384,-21407,-16384, 21407, 16384, -8867,  // w07 w06 w03 w02 w15 w14 w11 w10
  22725, 19266, 19266, -4520, 12873,-22725,  4520,-12873,
  12873,  4520,-22725,-12873,  4520, 19266, 19266,-22725 };
//}}}
//{{{
static __declspec(align(64)) const int16_t sse2_tab_i_17[] = {
  22725, 29692, 22725, 12299, 22725,-12299, 22725,-29692,
  22725, 12299,-22725,-29692,-22725, 29692, 22725,-12299,
  31521, 26722, 26722, -6270, 17855,-31521,  6270,-17855,
  17855,  6270,-31521,-17855,  6270, 26722, 26722,-31521 };
//}}}
//{{{
static __declspec(align(64)) const int16_t sse2_tab_i_26[] = {
  21407, 27969, 21407, 11585, 21407,-11585, 21407,-27969,
  21407, 11585,-21407,-27969,-21407, 27969, 21407,-11585,
  29692, 25172, 25172, -5906, 16819,-29692,  5906,-16819,
  16819,  5906,-29692,-16819,  5906, 25172, 25172,-29692 };
//}}}
//{{{
static __declspec(align(64)) const int16_t sse2_tab_i_35[] = {
  19266, 25172, 19266, 10426, 19266,-10426, 19266,-25172,
  19266, 10426,-19266,-25172,-19266, 25172, 19266,-10426,
  26722, 22654, 22654, -5315, 15137,-26722,  5315,-15137,
  15137,  5315,-26722,-15137,  5315, 22654, 22654,-26722 };
//}}}
//}}}
static uint8_t* Clip;
#define maxVidFrames 40

class cMpeg2decoder {
public:
  //{{{
  cMpeg2decoder() {

    if (!Clip) {
      Clip = (uint8_t*)malloc(1024);
      Clip += 384;
      for (int i = -384; i < 640; i++)
        Clip[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
      }

    mBlock[0] = (int16_t*)_aligned_malloc (128 * sizeof(int16_t) * 6, 128);
    for (int i = 1; i < 6; i++)
      mBlock[i] = mBlock[i-1] + 128;

    for (int i = 0; i < 3; i++) {
      mAuxFrame[i] = nullptr;
      mCurrentFrame[i] = nullptr;
      mForwardRefFrame[i] = nullptr;
      mBackwardRefFrame[i] = nullptr;
      }
    }
  //}}}
  //{{{
  ~cMpeg2decoder() {

    for (int i = 0; i < 3; i++) {
      _aligned_free (mBackwardRefFrame[i]);
      _aligned_free (mForwardRefFrame[i]);
      _aligned_free (mAuxFrame[i]);
      }

    _aligned_free (mBlock[0]);
    }
  //}}}
  //{{{
  cYuvFrame* getYuvFrame (int i) {
    return &mYuvFrames[i];
    }
  //}}}
  //{{{
  cYuvFrame* getNearestVidFrame (uint64_t pts) {
  // find nearestVidFrame to pts
  // - can return nullPtr if no frame loaded yet

    cYuvFrame* yuvFrame = nullptr;

    int64_t nearest = 0;
    for (int i = 0; i < maxVidFrames; i++) {
      if (mYuvFrames[i].mPts) {
        int64_t absDiff = mYuvFrames[i].mPts < pts ? pts - mYuvFrames[i].mPts : mYuvFrames[i].mPts - pts;
        if (!yuvFrame || (absDiff < nearest)) {
          yuvFrame = &mYuvFrames[i];
          nearest = absDiff;
          }
        }
      }
    return yuvFrame;
    }
  //}}}
  //{{{
  void invalidateFrames() {

    for (auto i= 0; i < maxVidFrames; i++)
      mYuvFrames[i].invalidate();

    mLoadVidFrame = 0;
    }
  //}}}
  //{{{
  bool decodePes (uint8_t* pesBuffer, uint8_t* pesBufferEnd, uint64_t vidPts, uint8_t*& pesPtr) {
  // decode a frame of video, usually a pes packet

    mBufferPtr = pesBuffer;
    mBufferEnd = pesBufferEnd;
    pesPtr = pesBuffer;

    m32bits = 0;
    mBitCount = 0;
    consumeBits (0);

    if (!mGotSequenceHeader)
      while (mBufferPtr < mBufferEnd)
        if (getHeader (false) == 0x1B3) { // sequenceHeaderCode
          //{{{  allocate buffers from sequenceHeader width, height
          for (int i = 0; i < 3; i++) {
            int size = (i == 0) ? mWidth * mHeight : mChromaWidth * mChromaHeight;
            mAuxFrame[i] = (uint8_t*)_aligned_malloc (size, 128);
            mForwardRefFrame[i] = (uint8_t*)_aligned_malloc (size, 128);
            mBackwardRefFrame[i] = (uint8_t*)_aligned_malloc (size, 128);
            }
          mGotSequenceHeader = true;
          //}}}
          break;
          }

    while (mBufferPtr < mBufferEnd)
      if (getHeader (true) == 0x100) // pictureStartCode
        break;

    if (mBufferPtr < mBufferEnd) {
      //{{{  updatePictureBuffers description
      // B pics do not need to be save for future reference
      // the previously decoded reference frame is stored coincident with the location where the backward
      // reference frame is stored (backwards prediction is not needed in P pictures)
      // update pointer for potential future B pictures
      // can erase over old backward reference frame since it is not used in a P picture
      // - since any subsequent B pictures will use the previously decoded I or P frame as the backward_reference_frame
      //}}}
      for (int i = 0; i < 3; i++)
        if (mPictureCodingType == B_TYPE)
          mCurrentFrame[i] = mAuxFrame[i];
        else {
          uint8_t* tmp = mForwardRefFrame[i];
          mForwardRefFrame[i] = mBackwardRefFrame[i];
          mBackwardRefFrame[i] = tmp;
          mCurrentFrame[i] = mBackwardRefFrame[i];
          }

      decodeSlices();

      // reorderFrames write or display current or previously decoded reference frame
      int32_t linesize[2] = { mWidth,  mChromaWidth};
      mYuvFrames[mLoadVidFrame % maxVidFrames].set (vidPts,
        (mPictureCodingType == B_TYPE) ? mAuxFrame : mForwardRefFrame, linesize,
        mWidth, mHeight, (int)(pesBufferEnd - pesBuffer), mPictureCodingType);
      mLoadVidFrame++;
      pesPtr = mBufferPtr - 4;
      return true;
      }
    else
      return false;
    }
  //}}}

private:
  //{{{
  void consumeBits (int8_t numBits) {

    m32bits = (numBits == 32) ? 0 : (m32bits << numBits);

    mBitCount -= numBits;
    while (mBitCount <= 24) {
      m32bits |= (*mBufferPtr++) << (24 - mBitCount);
      mBitCount += 8;
      }
    }
  //}}}
  //{{{
  uint32_t peekBits (int8_t numBits) {
    return m32bits >> (32 - numBits);
    }
  //}}}
  //{{{
  uint32_t getBits (int8_t numBits) {

    uint32_t val = m32bits >> (32 - numBits);
    consumeBits (numBits);
    return val;
    }
  //}}}
  //{{{
  uint32_t getStartCode() {

    consumeBits (mBitCount & 7);
    while ((mBufferPtr < mBufferEnd) && (m32bits >> 8) != 0x001)
      consumeBits (8);

    return m32bits;
    }
  //}}}

  //{{{
  void picture_coding_extension() {

    mFcode[0][0] = getBits(4);
    mFcode[0][1] = getBits(4);
    mFcode[1][0] = getBits(4);
    mFcode[1][1] = getBits(4);

    mIntraDcPrecision      = getBits(2);
    auto picture_structure = getBits(2);
    auto top_field_first   = getBits(1);
    mFramePredFrameDct     = getBits(1);
    mConcealmentMotionVecs = getBits(1);
    qScaleType             = getBits(1);
    mIntraVlcFormat        = getBits(1);
    mAlternateScan         = getBits(1);
    }
  //}}}
  //{{{
  void extension_and_user_data() {
  // decode extension and user data

    auto code = getStartCode();
    while (code == EXTENSION_START_CODE || code == USER_DATA_START_CODE) {
      consumeBits (32);
      if (code == EXTENSION_START_CODE) {
        int ext_ID = getBits (4);
        switch (ext_ID) {
          case PICTURE_CODING_EXTENSION_ID:
            picture_coding_extension();
            break;
          default:
            //printf ("reserved extension start code ID %d\n", ext_ID);
            break;
          }
        }
      // else // userData skip ahead to the next start code

      code = getStartCode();
      }
    }
  //}}}
  //{{{
  void sequenceHeader() {

    auto horizontal_size = getBits (12);
    auto vertical_size = getBits (12);

    mMbWidth = (horizontal_size + 15) / 16;
    mMbHeight = 2 * ((vertical_size + 31) / 32);
    mWidth = 16 * mMbWidth;
    mHeight = 16 * mMbHeight;
    mChromaWidth = mWidth >> 1;
    mChromaHeight = mHeight >> 1;

    extension_and_user_data();
    }
  //}}}
  //{{{
  void pictureHeader() {

    mTemporalReference = getBits (10);
    mPictureCodingType = getBits (3);
    auto vbv_delay = getBits (16);

    if (mPictureCodingType == P_TYPE || mPictureCodingType == B_TYPE) {
      auto fullPelForwardVector = getBits(1);
      auto forwardFcode = getBits(3);
      }

    if (mPictureCodingType == B_TYPE) {
      auto fullPelBackwardVector = getBits(1);
      auto backwardFcode = getBits(3);
      }

    // consume extraBytes
    auto Extra_Information_Byte_Count = 0;
    while (getBits (1)) {
      consumeBits (8);
      Extra_Information_Byte_Count++;
      }
    extension_and_user_data();
    }
  //}}}
  //{{{
  void sliceHeader() {

    auto quantizerScaleCode = getBits (5);
    mQuantizerScale = qScaleType ? Non_Linear_quantizer_scale[quantizerScaleCode] : quantizerScaleCode << 1;

    auto slice_picture_id_enable = 0;
    auto slice_picture_id = 0;
    auto extra_information_slice = 0;

    if (getBits(1)) {
      auto mIntraSlice = getBits (1);
      slice_picture_id_enable = getBits (1);
      slice_picture_id = getBits (6);
      extra_information_slice = 0;
      while (getBits (1)) {
        consumeBits (8);
        extra_information_slice++;
        }
      }
    }
  //}}}
  //{{{
  uint32_t getHeader (bool reportUnexpected) {

    getStartCode();
    auto code = getBits (32);
    switch (code) {
      case 0x100 : // PICTURE_START_CODE:
        pictureHeader();
        break;

      case 0x1B3 : // SEQUENCE_HEADER_CODE:
        sequenceHeader();
        break;

      case 0x1B7: // SEQUENCE_END_CODE:
        printf ("getHeader %08x SEQUENCE_END_CODE\n", code);
        break;

      case 0x1B8 : // GROUP_START_CODE:
        break;

      default:
        if (reportUnexpected)
          printf ("getHeader unexpected %08x\n", code);
        break;
      }

    return code;
    }
  //}}}

  //{{{
  int getMacroBlockType() {

    switch (mPictureCodingType) {
      case I_TYPE:
        if (getBits (1))
          return 1;
        if (!getBits (1))
          printf ("getMacroblockType - invalid mBtype code\n");
        return 17;

      case P_TYPE: {
        int code = peekBits (6);
        if (code >= 8) {
          code >>= 3;
          consumeBits (PMBtab0[code].len);
          return PMBtab0[code].val;
          }
        if (code == 0) {
          printf ("getMacroblockType - invalid mBtype code\n");
          return 0;
          }

        consumeBits (PMBtab1[code].len);
        return PMBtab1[code].val;
        }

      case B_TYPE: {
        int code = peekBits(6);
        if (code >= 8) {
          code >>= 2;
          consumeBits (BMBtab0[code].len);
          return BMBtab0[code].val;
          }
        if (code == 0) {
          printf ("getMacroblockType - invalid mBtype code\n");
          return 0;
          }
        consumeBits(BMBtab1[code].len);
        return BMBtab1[code].val;
        }

      default:
        printf ("getMacroblockType - unrecognized picture coding type\n");
        return 0;
      }
    }
  //}}}
  //{{{
  int getCodedBlockPattern() {

    auto code = peekBits(9);
    if (code >= 128) {
      code >>= 4;
      consumeBits (CBPtab0[code].len);
      return CBPtab0[code].val;
      }
    if (code >= 8) {
      code >>= 1;
      consumeBits (CBPtab1[code].len);
      return CBPtab1[code].val;
      }
    if (code < 1) {
      printf ("getCodedBlockPattern - invalid coded_block_pattern code\n");
      return 0;
      }

    consumeBits (CBPtab2[code].len);
    return CBPtab2[code].val;
    }
  //}}}
  //{{{
  int getMacroBlockAddressInc() {

    int val = 0;

    int code;
    while ((code = peekBits (11)) < 24) {
      if (code != 15) { // if not mBstuffing
        if (code == 8)  // if mBescape
          val += 33;
        else {
          printf ("getMacroBlockAddressInc - invalid mBaddressInc code\n");
          return 1;
          }
        }
      consumeBits (11);
      }

    // mBaddress_inc == 1 ('1' is in the MSB position of the lookahead)
    if (code >= 1024) {
      consumeBits (1);
      return val + 1;
      }

    // codes 00010 ... 011xx
    if (code >= 128) {
      // remove leading zeros
      code >>= 6;
      consumeBits (MBAtab1[code].len);
      return val + MBAtab1[code].val;
      }

    // codes 00000011000 ... 0000111xxxx
    code -= 24; // remove common base
    consumeBits (MBAtab2[code].len);
    return val + MBAtab2[code].val;
    }
  //}}}
  //{{{
  void macroBlockModes (int& mBtype, int& motionType, int& mvCount, int& mvFormat, int& mvScale, int& dctType) {

    mBtype = getMacroBlockType();

    // get frame/field motion type
    motionType = 0;
    if (mBtype & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD))
      motionType = mFramePredFrameDct ? MC_FRAME : getBits(2);
    else if ((mBtype & MACROBLOCK_INTRA) && mConcealmentMotionVecs)
      motionType = MC_FRAME;

    // derive motionVecCount, mv_format and dmv
    mvCount = (motionType == MC_FIELD) ? 2 : 1;
    mvFormat = (motionType == MC_FRAME) ? MV_FRAME : MV_FIELD;

    // field mv predictions in frame pictures have to be scaled
    mvScale = mvFormat == MV_FIELD;

    // get dctType (frame DCT / field DCT)
    dctType = (!mFramePredFrameDct) && (mBtype & (MACROBLOCK_PATTERN | MACROBLOCK_INTRA)) ? getBits(1) : 0;
    }
  //}}}

  //{{{
  int getLumaDCdctDiff() {
  // parse VLC and perform dct_diff arithmetic.

    int size;
    auto code = peekBits (5);
    if (code < 31) {
      size = DClumtab0[code].val;
      consumeBits (DClumtab0[code].len);
      }
    else {
      code = peekBits (9) - 0x1f0;
      size = DClumtab1[code].val;
      consumeBits (DClumtab1[code].len);
      }

    int dct_diff = 0;
    if (size) {
      dct_diff = getBits (size);
      if ((dct_diff & (1 << (size - 1))) == 0)
        dct_diff -= (1 << size) - 1;
      }

    return dct_diff;
    }
  //}}}
  //{{{
  int getChromaDCdctDiff() {

    int size;
    auto code = peekBits (5);
    if (code < 31) {
      size = DCchromtab0[code].val;
      consumeBits (DCchromtab0[code].len);
      }
    else {
      code = peekBits (10) - 0x3e0;
      size = DCchromtab1[code].val;
      consumeBits (DCchromtab1[code].len);
      }

    int dct_diff = 0;
    if (size) {
      dct_diff = getBits (size);
      if ((dct_diff & (1 << (size - 1))) == 0)
        dct_diff -= (1 << size) - 1;
      }

    return dct_diff;
    }
  //}}}
  //{{{
  void decodeIntraBlock (int comp, int dcDctPred[]) {

    // decode DC coefficients
    int val;
    auto cc = (comp < 4) ? 0 : (comp & 1) + 1;
    if (cc == 0)
      val = (dcDctPred[0] += getLumaDCdctDiff());
    else if (cc == 1)
      val = (dcDctPred[1] += getChromaDCdctDiff());
    else
      val = (dcDctPred[2] += getChromaDCdctDiff());

    // decode AC coefficients
    mBlock[comp][0] = val << (3 - mIntraDcPrecision);
    for (auto i = 1; ; i++) {
      const DCTtab* tab;
      uint32_t code = peekBits (16);
      if (code >= 16384 && !mIntraVlcFormat)
        tab = &DCTtabnext[(code >> 12) - 4];
      else if (code >= 1024) {
        if (mIntraVlcFormat)
          tab = &DCTtab0a[(code >> 8) - 4];
        else
          tab = &DCTtab0[(code >> 8) - 4];
        }
      else if (code >= 512) {
        if (mIntraVlcFormat)
          tab = &DCTtab1a[(code >> 6) - 8];
        else
          tab = &DCTtab1[(code >> 6) - 8];
        }
      else if (code >= 256)
        tab = &DCTtab2[(code >> 4) - 16];
      else if (code >= 128)
        tab = &DCTtab3[(code >> 3) - 16];
      else if (code >= 64)
        tab = &DCTtab4[(code >> 2) - 16];
      else if (code >= 32)
        tab = &DCTtab5[(code >> 1) - 16];
      else if (code >= 16)
        tab = &DCTtab6[code - 16];
      else {
        //{{{  flaw
        printf ("invalid Huffman code in Decode_MPEG2_Intra_Block() code:%d %d \n", code, mBitCount);
        return;
        }
        //}}}
      consumeBits (tab->len);

      if (tab->run == 64) // end_of_block
        return;

      int sign, run;
      if (tab->run == 65) {
        //{{{  escape
        i += run = getBits (6);
        val = getBits (12);
        if ((val & 2047) == 0) {
          printf ("invalid escape in Decode_MPEG2_Intra_Block()\n");
          return;
          }

        if ((sign = (val >= 2048)))
          val = 4096 - val;
        }
        //}}}
      else {
        i += run = tab->run;
        val = tab->level;
        sign = getBits (1);
      }

      if (i >= 64) {
        //{{{  flaw
        printf ("DCT coeff index (i) out of bounds (intra2)\n");
        return;
        }
        //}}}

      auto j = scan [mAlternateScan][i];
      val = (val * mQuantizerScale * default_intra_quantizer_matrix[j]) >> 4;
      mBlock[comp][j] = sign ? -val : val;
      }
    }
  //}}}
  //{{{
  void decodeNonIntraBlock (int comp) {

    for (auto i = 0; ; i++) {
      const DCTtab* tab;
      auto code = peekBits (16);
      if (code >= 16384) {
        if (i == 0)
          tab = &DCTtabfirst[(code>>12)-4];
        else
          tab = &DCTtabnext[(code>>12)-4];
        }
      else if (code >= 1024)
        tab = &DCTtab0[(code>>8)-4];
      else if (code >= 512)
        tab = &DCTtab1[(code>>6)-8];
      else if (code >= 256)
        tab = &DCTtab2[(code>>4)-16];
      else if (code >= 128)
        tab = &DCTtab3[(code>>3)-16];
      else if (code >= 64)
        tab = &DCTtab4[(code>>2)-16];
      else if (code >= 32)
        tab = &DCTtab5[(code>>1)-16];
      else if (code >= 16)
        tab = &DCTtab6[code-16];
      else {
        //{{{  flaw
        printf ("invalid Huffman code in Decode_MPEG2_Non_Intra_Block()\n");
        return;
        }
        //}}}
      consumeBits (tab->len);

      if (tab->run == 64) // end_of_block
        return;

      int val, sign, run;
      if (tab->run == 65) {
        //{{{  escape
        i += run = getBits (6);
        val = getBits (12);
        if ((val & 2047) == 0) {
          printf ("invalid escape in Decode_MPEG2_Intra_Block()\n");
          return;
          }

        if ((sign = (val >= 2048)))
          val = 4096 - val;
        }
        //}}}
      else {
        i += run = tab->run;
        val = tab->level;
        sign = getBits (1);
        }

      if (i >= 64) {
        //{{{  flaw
        printf ("DCT coeff index (i) out of bounds (inter2)\n");
        return;
        }
        //}}}

      auto j = scan [mAlternateScan][i];
      val = (((val << 1) + 1) * mQuantizerScale * 16) >> 5;
      mBlock[comp][j] = sign ? -val : val;
      }
    }
  //}}}

  //{{{
  int getMotionCode() {

    if (getBits (1))
      return 0;

    int code = peekBits (9);
    if (code >= 64) {
      code >>= 6;
      consumeBits (MVtab0 [code].len);
      return getBits (1) ? -MVtab0 [code].val : MVtab0 [code].val;
      }

    if (code >= 24) {
      code >>= 3;
      consumeBits (MVtab1 [code].len);
      return getBits (1) ? -MVtab1 [code].val : MVtab1 [code].val;
      }

    if ((code -= 12) < 0) {
      return 0;
      }

    consumeBits (MVtab2 [code].len);
    return getBits (1) ? -MVtab2 [code].val : MVtab2 [code].val;
    }
  //}}}
  //{{{
  void decodeVector (int& pred, int rSize, int motion_code, int motion_residual, int fullPelVector) {
  // calculate motion vector component

    int lim = 16 << rSize;
    int vec = fullPelVector ? (pred >> 1) : pred;

    if (motion_code > 0) {
      vec += ((motion_code-1) << rSize) + motion_residual + 1;
      if (vec >= lim)
        vec -= lim + lim;
      }
    else if (motion_code < 0) {
      vec -= ((-motion_code - 1) << rSize) + motion_residual + 1;
      if (vec < -lim)
        vec += lim + lim;
      }

    pred = fullPelVector ? (vec << 1) : vec;
    }
  //}}}
  //{{{
  void motionVector (int* PMV, int hrSize, int vrSize, int mvScale, int fullPelVector) {
  // get and decode motion vector and differential motion vector for one prediction

    // horizontal component
    int motionCode = getMotionCode();
    int motionResidual = (hrSize && motionCode) ? getBits (hrSize) : 0;
    decodeVector (PMV[0], hrSize, motionCode, motionResidual, fullPelVector);

    // vertical component
    if (mvScale)
      PMV[1] >>= 1;
    motionCode = getMotionCode();
    motionResidual = (vrSize && motionCode) ? getBits (vrSize) : 0;
    decodeVector (PMV[1], vrSize, motionCode, motionResidual, fullPelVector);
    if (mvScale)
      PMV[1] <<= 1;
    }
  //}}}
  //{{{
  void motionVectors (int PMV[2][2][2], int motion_vertical_field_select[2][2], int s, int motionVecCount, int mv_format,
                      int h_r_size, int v_r_size, int mvscale) {

    if (motionVecCount == 1) {
      if (mv_format == MV_FIELD) {
        int bits = getBits(1);
        motion_vertical_field_select[1][s] = bits;
        motion_vertical_field_select[0][s] = bits;
        }
      motionVector (PMV[0][s], h_r_size, v_r_size, mvscale, 0);

      // update other motion vector predictors
      PMV[1][s][0] = PMV[0][s][0];
      PMV[1][s][1] = PMV[0][s][1];
      }

    else {
      motion_vertical_field_select[0][s] = getBits (1);
      motionVector (PMV[0][s], h_r_size, v_r_size, mvscale, 0);

      motion_vertical_field_select [1][s] = getBits (1);
      motionVector (PMV[1][s], h_r_size, v_r_size, mvscale, 0);
      }
    }
  //}}}

  //{{{
  void formPrediction (uint8_t* src[], int sfield, int dfield, int lx, int lx2, int h, int x, int y, int dx, int dy, bool average) {
  //{{{  description
  //*   1. the vectors (dx, dy) are based on cartesian frame
  //*      coordiantes along a half-pel grid (always positive numbers)
  //*      In contrast, vector[r][s][t] are differential (with positive and
  //*      negative values). As a result, deriving the integer vectors
  //*      (int_vec[t]) from dx, dy is accomplished by a simple right shift.
  //*
  //*   2. Half pel flags (xh, yh) are equivalent to the LSB (Least
  //*      Significant Bit) of the half-pel coordinates (dx,dy).
  //
  //*  the work of combining predictions (ISO/IEC 13818-2 section 7.6.7)
  //*  is distributed among several other stages.  This is accomplished by
  //*  folding line offsets into the source and destination (src,dst)
  //*  addresses (note the call arguments to formPrediction() in Predict()),
  //*  line stride variables lx and lx2, the block dimension variables (w,h),
  //*  average_flag, and by the very order in which Predict() is called.
  //*  This implementation design (implicitly different than the spec) was chosen for its elegance. */
  //}}}

    int srcOffset = (sfield ? lx2 >> 1 : 0) + lx * (y + (dy >> 1)) + x + (dx >> 1);
    uint8_t* sY = src[0] + srcOffset;
    uint8_t* dY = mCurrentFrame[0] + (dfield ? lx2 >> 1 : 0) + lx * y + x;
    switch ((average << 2) + ((dx & 1) << 1) + (dy & 1)) {
      //{{{
      case 0: { // d[i] = s[i];
        for (int loop = 0; loop < h; loop++) {
          *(__m128i*)dY = _mm_loadu_si128 ((__m128i*)sY);
          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 1: { // d[i] = (s[i]+s[i+lx]+1)>>1;
        for (int loop = 0; loop < h; loop++) {
          *(__m128i*)dY = _mm_avg_epu8 (_mm_loadu_si128((__m128i*)sY), _mm_loadu_si128 ((__m128i*)(sY+lx)));
          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 2: { // d[i] = (s[i]+s[i+1]+1)>>1;
        for (int loop = 0; loop < h; loop++) {
          *(__m128i*)dY = _mm_avg_epu8 (_mm_loadu_si128 ((__m128i*)sY), _mm_loadu_si128 ((__m128i*)(sY+1)));
          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 3: { // d[i] = (s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2;
        // (a+b+c+d+2)>>2 = avg(avg(a,b)+avg(c,d)) - (a^b)|(c^d) & avg(a,b)^avg(c,d) & 0x01
        __m128i shade = _mm_set1_epi8(1);
        for (int loop = 0; loop < h; loop++) {
          __m128i pixel1 = _mm_loadu_si128 ((__m128i*)sY);
          __m128i pixel2 = _mm_loadu_si128 ((__m128i*)(sY + 1));
          __m128i pixel3 = _mm_loadu_si128 ((__m128i*)(sY + lx));
          __m128i pixel4 = _mm_loadu_si128 ((__m128i*)(sY + lx + 1));

          __m128i avg12 = _mm_avg_epu8 (pixel1, pixel2);
          __m128i avg34 = _mm_avg_epu8 (pixel3, pixel4);
          __m128i avg = _mm_avg_epu8 (avg12, avg34);

          __m128i xor12 = _mm_xor_si128 (pixel1, pixel2);
          __m128i xor34 = _mm_xor_si128 (pixel3, pixel4);
          __m128i or1234 = _mm_or_si128 (xor12, xor34);
          __m128i xoravg = _mm_xor_si128 (avg12, avg34);
          __m128i offset = _mm_and_si128 (_mm_and_si128(or1234, xoravg), shade);

          *(__m128i*)dY = _mm_sub_epi8 (avg, offset);

          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 4: { // d[i] = (s[i]+d[i]+1)>>1;
        for (int loop = 0; loop < h; loop++) {
          *(__m128i*)dY = _mm_avg_epu8 (_mm_loadu_si128 ((__m128i*)sY), *(__m128i*)dY);
          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 5: { // d[i] = ((s[i]+s[i+lx]+1)>>1) + d[i] + 1)>>1;
        for (int loop = 0; loop < h; loop++) {
          __m128i avg = _mm_avg_epu8 (_mm_loadu_si128 ((__m128i*)sY), _mm_loadu_si128 ((__m128i*)(sY+lx)));
          *(__m128i*)dY = _mm_avg_epu8 (avg , *(__m128i*)dY);
          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 6: { // d[i] = (((s[i]+s[i+1]+1)>>1) + d[1] + 1)>>1;
        for (int loop = 0; loop < h; loop++) {
          __m128i avg = _mm_avg_epu8 (_mm_loadu_si128 ((__m128i*)sY), _mm_loadu_si128 ((__m128i*)(sY+1)));
          *(__m128i*)dY = _mm_avg_epu8 (avg, *(__m128i*)dY);
          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 7: { // d[i] = (((s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2) + d[i] + 1)>>1;
        __m128i shade = _mm_set1_epi8 (1);
        for (int loop = 0; loop < h; loop++) {
          __m128i pixel1 = _mm_loadu_si128 ((__m128i*)sY);
          __m128i pixel2 = _mm_loadu_si128 ((__m128i*)(sY + 1));
          __m128i pixel3 = _mm_loadu_si128 ((__m128i*)(sY + lx));
          __m128i pixel4 = _mm_loadu_si128 ((__m128i*)(sY + lx + 1));

          __m128i avg12 = _mm_avg_epu8 (pixel1, pixel2);
          __m128i avg34 = _mm_avg_epu8 (pixel3, pixel4);
          __m128i avg = _mm_avg_epu8 (avg12, avg34);

          __m128i xor12 = _mm_xor_si128 (pixel1, pixel2);
          __m128i xor34 = _mm_xor_si128 (pixel3, pixel4);
          __m128i or1234 = _mm_or_si128 (xor12, xor34);
          __m128i xoravg = _mm_xor_si128 (avg12, avg34);
          __m128i offset = _mm_and_si128 (_mm_and_si128(or1234, xoravg), shade);

          *(__m128i*)dY = _mm_avg_epu8 (_mm_sub_epi8(avg, offset), *(__m128i*)dY);

          sY += lx2;
          dY += lx2;
          }
        }
        break;
      //}}}
      }

    lx >>= 1;
    lx2 >>= 1;
    x >>= 1;
    dx /= 2;
    h >>= 1;
    y >>= 1;
    dy /= 2;
    int sOffset = (sfield ? lx2 >> 1: 0) + lx * (y + (dy >> 1)) + x + (dx >> 1);
    int dOffset = (dfield ? lx2 >> 1: 0) + lx * y + x;
    uint8_t* sCr = src[1] + sOffset;
    uint8_t* dCr = mCurrentFrame[1] + dOffset;
    uint8_t* sCb = src[2] + sOffset;
    uint8_t* dCb = mCurrentFrame[2] + dOffset;

    switch ((average << 2) + ((dx & 1) << 1) + (dy & 1)) {
  #ifdef _M_IX86
      //{{{
      case 0: {
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCb = *(__m64*)sCb;
          sCb += lx2;
          dCb += lx2;

          *(__m64*)dCr = *(__m64*)sCr;
          sCr += lx2;
          dCr += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 1:
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCr = _m_pavgb(*(__m64*)sCr, *(__m64*)(sCr+lx));
          sCr += lx2;
          dCr += lx2;

          *(__m64*)dCb = _m_pavgb(*(__m64*)sCb, *(__m64*)(sCb+lx));
          sCb += lx2;
          dCb += lx2;
          }
        break;
      //}}}
      //{{{
      case 2:
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCr = _m_pavgb(*(__m64*)sCr, *(__m64*)(sCr+1));
          sCr += lx2;
          dCr += lx2;

          *(__m64*)dCb = _m_pavgb(*(__m64*)sCb, *(__m64*)(sCb+1));
          sCb += lx2;
          dCb += lx2;
          }
        break;
      //}}}
      //{{{
      case 3: {
        __m64 shade = _mm_set1_pi8(1);
        for (int loop = 0; loop < h; loop++) {
          __m64 pixel1 = *(__m64*)sCr;
          __m64 pixel2 = *(__m64*)(sCr+1);
          __m64 pixel3 = *(__m64*)(sCr+lx);
          __m64 pixel4 = *(__m64*)(sCr+lx+1);

          __m64 avg12 = _m_pavgb(pixel1, pixel2);
          __m64 avg34 = _m_pavgb(pixel3, pixel4);
          __m64 avg =   _m_pavgb(avg12, avg34);

          __m64 xor12 = _m_pxor(pixel1, pixel2);
          __m64 xor34 = _m_pxor(pixel3, pixel4);
          __m64 or1234 = _m_por(xor12, xor34);
          __m64 xoravg = _m_pxor(avg12, avg34);

          __m64 offset = _m_pand(_m_pand(or1234, xoravg), shade);
          *(__m64*)dCr = _m_psubb(avg, offset);
          sCr += lx2;
          dCr += lx2;

          pixel1 = *(__m64*)sCb;
          pixel2 = *(__m64*)(sCb+1);
          pixel3 = *(__m64*)(sCb+lx);
          pixel4 = *(__m64*)(sCb+lx+1);

          avg12 = _m_pavgb(pixel1, pixel2);
          avg34 = _m_pavgb(pixel3, pixel4);
          avg =   _m_pavgb(avg12, avg34);
          xor12 = _m_pxor(pixel1, pixel2);

          xor34 = _m_pxor(pixel3, pixel4);
          or1234 = _m_por(xor12, xor34);
          xoravg = _m_pxor(avg12, avg34);

          offset = _m_pand(_m_pand(or1234, xoravg), shade);
          *(__m64*)dCb = _m_psubb(avg, offset);
          sCb += lx2;
          dCb += lx2;
          }
        }
        break;
      //}}}
      //{{{
      case 4:
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCr = _m_pavgb(*(__m64*)sCr, *(__m64*)dCr);
          sCr += lx2;
          dCr += lx2;

          *(__m64*)dCb = _m_pavgb(*(__m64*)sCb, *(__m64*)dCb);
          sCb += lx2;
          dCb += lx2;
          }
        break;
      //}}}
      //{{{
      case 5:
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCr = _m_pavgb (_m_pavgb(*(__m64*)sCr, *(__m64*)(sCr+lx)), *(__m64*)dCr);
          sCr += lx2;
          dCr += lx2;

          *(__m64*)dCb = _m_pavgb (_m_pavgb(*(__m64*)sCb, *(__m64*)(sCb+lx)), *(__m64*)dCb);
          sCb += lx2;
          dCb += lx2;
          }
        break;
      //}}}
      //{{{
      case 6:
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCr = _m_pavgb (_m_pavgb (*(__m64*)sCr, *(__m64*)(sCr+1)), *(__m64*)dCr);
          sCr += lx2;
          dCr += lx2;

          *(__m64*)dCb = _m_pavgb (_m_pavgb (*(__m64*)sCb, *(__m64*)(sCb+1)), *(__m64*)dCb);
          sCb += lx2;
          dCb += lx2;
          }
        break;
      //}}}
      //{{{
      case 7: {
        __m64 shade = _mm_set1_pi8(1);

        for (int loop = 0; loop < h; loop++) {
          __m64 pixel1 = *(__m64*)sCr;
          __m64 pixel2 = *(__m64*)(sCr+1);
          __m64 pixel3 = *(__m64*)(sCr+lx);
          __m64 pixel4 = *(__m64*)(sCr+lx+1);

          __m64 avg12 = _m_pavgb (pixel1, pixel2);
          __m64 avg34 = _m_pavgb (pixel3, pixel4);
          __m64 avg =   _m_pavgb (avg12, avg34);
          __m64 xor12 = _m_pxor (pixel1, pixel2);
          __m64 xor34 = _m_pxor (pixel3, pixel4);

          __m64 or1234 = _m_por (xor12, xor34);
          __m64 xoravg = _m_pxor (avg12, avg34);
          __m64 offset = _m_pand (_m_pand (or1234, xoravg), shade);

          *(__m64*)dCr = _m_pavgb (_m_psubb (avg, offset), *(__m64*)dCr);
          sCr += lx2;
          dCr += lx2;

          pixel1 = *(__m64*)sCb;
          pixel2 = *(__m64*)(sCb+1);
          pixel3 = *(__m64*)(sCb+lx);
          pixel4 = *(__m64*)(sCb+lx+1);

          avg12 = _m_pavgb (pixel1, pixel2);
          avg34 = _m_pavgb (pixel3, pixel4);
          avg =   _m_pavgb (avg12, avg34);
          xor12 = _m_pxor (pixel1, pixel2);
          xor34 = _m_pxor (pixel3, pixel4);

          or1234 = _m_por (xor12, xor34);
          xoravg = _m_pxor (avg12, avg34);
          offset = _m_pand (_m_pand (or1234, xoravg), shade);

          *(__m64*)dCb = _m_pavgb (_m_psubb (avg, offset), *(__m64*)dCb);
          sCb += lx2;
          dCb += lx2;
          }
        }
        break;
      //}}}
      }
  #else
      //{{{
      case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        for (int loop = 0; loop < h; loop++) {
          *(__m64*)dCb = *(__m64*)sCb;
          sCb += lx2;
          dCb += lx2;
          *(__m64*)dCr = *(__m64*)sCr;
          sCr += lx2;
          dCr += lx2;
          }
        break;

      //if (!xh && !yh) {
        //{{{  no horizontal nor vertical half-pel
        //if (average_flag) {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++) {
              //int v = d[i] + s[i];
              //d[i] = (v + (v >= 0 ? 1 : 0)) >> 1;
              //}
            //s += lx2;
            //d += lx2;
            //}
          //}

        //else {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++)
              //d[i] = s[i];
            //s += lx2;
            //d += lx2;
            //}
          //}
        //}
        //}}}
      //else if (!xh && yh) {
        //{{{  no horizontal but vertical half-pel
        //if (average_flag) {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++) {
              //int v = d[i] + ((uint32_t)(s[i] + s[i+lx] +1) >>1);
              //d[i]= (v + (v >= 0 ? 1 : 0)) >> 1;
              //}
            //s += lx2;
            //d += lx2;
            //}
          //}
        //else {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++)
              //d[i] = (uint32_t)(s[i] + s[i+lx] + 1) >> 1;
            //s += lx2;
            //d += lx2;
            //}
          //}
        //}
        //}}}
      //else if (xh && !yh) {
        //{{{  horizontal but no vertical half-pel
        //if (average_flag) {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++) {
              //int v = d[i] + ((uint32_t)(s[i] + s[i+1] + 1) >> 1);
              //d[i] = (v + (v >= 0 ? 1 : 0)) >> 1;
              //}
            //s += lx2;
            //d += lx2;
            //}
          //}
        //else {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++)
              //d[i] = (uint32_t)(s[i] + s[i+1] + 1) >> 1;
            //s += lx2;
            //d += lx2;
            //}
          //}
        //}
        //}}}
      //else {
        //{{{  horizontal and vertical half-pel
        //if (average_flag) {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++) {
              //int v = d[i] + ((uint32_t)(s[i] + s[i+1] + s[i+lx] + s[i+lx+1] + 2) >> 2);
              //d[i] = (v + (v >= 0 ? 1 : 0)) >> 1;
              //}
            //s += lx2;
            //d += lx2;
            //}
          //}
        //else {
          //for (int j = 0; j < h; j++) {
            //for (int i = 0; i < w; i++)
              //d[i] = (uint32_t)(s[i] + s[i+1] + s[i+lx] + s[i+lx+1] + 2) >> 2;
            //s += lx2;
            //d += lx2;
            //}
          //}
        //}
        //}}}
      //}}}
      }
  #endif
    }
  //}}}
  //{{{
  void formPredictions (int bx, int by, int mBtype, int motionType, int PMV[2][2][2], int motionVertField[2][2]) {

    bool average = false;
    if ((mBtype & MACROBLOCK_MOTION_FORWARD) || mPictureCodingType == P_TYPE) {
      if (motionType == MC_FRAME || !(mBtype & MACROBLOCK_MOTION_FORWARD)) {
        // frame-based prediction, broken into top and bottom halves for spatial scalability prediction purposes
        formPrediction (mForwardRefFrame, 0, 0, mWidth, mWidth << 1, 8, bx, by, PMV[0][0][0], PMV[0][0][1], 0);
        formPrediction (mForwardRefFrame, 1, 1, mWidth, mWidth << 1, 8, bx, by, PMV[0][0][0], PMV[0][0][1], 0);
        }
      else {
        // top field prediction
        formPrediction (mForwardRefFrame, motionVertField[0][0], 0, mWidth << 1, mWidth << 1, 8,
                        bx, by >> 1, PMV[0][0][0], PMV[0][0][1]>>1, 0);
        // bottom field prediction
        formPrediction (mForwardRefFrame, motionVertField[1][0], 1, mWidth << 1, mWidth << 1, 8,
                        bx, by >> 1, PMV[1][0][0], PMV[1][0][1]>>1, 0);
        }
      average = true;
      }

    if (mBtype & MACROBLOCK_MOTION_BACKWARD) {
      if (motionType == MC_FRAME) {
        // frame-based prediction
        formPrediction (mBackwardRefFrame, 0, 0, mWidth, mWidth << 1, 8, bx, by, PMV[0][1][0], PMV[0][1][1], average);
        formPrediction (mBackwardRefFrame, 1, 1, mWidth, mWidth << 1, 8, bx, by, PMV[0][1][0], PMV[0][1][1], average);
        }
      else {
        // top field prediction
        formPrediction (mBackwardRefFrame, motionVertField[0][1], 0, mWidth << 1, mWidth << 1, 8,
                        bx, by >> 1, PMV[0][1][0], PMV[0][1][1]>>1, average);
        // bottom field prediction
        formPrediction (mBackwardRefFrame, motionVertField[1][1], 1, mWidth << 1, mWidth << 1, 8,
                        bx, by >> 1, PMV[1][1][0], PMV[1][1][1]>>1, average);
        }
      }
    }
  //}}}
  //{{{
  void idctSSE2 (int16_t* block) {

    //{{{  DCT_8_INV_ROWX2 macro
    #define DCT_8_INV_ROWX2(tab1, tab2) {  \
      r1 = _mm_shufflelo_epi16 (r1, _MM_SHUFFLE(3, 1, 2, 0));  \
      r1 = _mm_shufflehi_epi16 (r1, _MM_SHUFFLE(3, 1, 2, 0));  \
      a0 = _mm_madd_epi16 (_mm_shuffle_epi32 (r1, _MM_SHUFFLE(0, 0, 0, 0)), *(__m128i*)(tab1+8*0));  \
      a1 = _mm_madd_epi16 (_mm_shuffle_epi32 (r1, _MM_SHUFFLE(1, 1, 1, 1)), *(__m128i*)(tab1+8*2));  \
      a2 = _mm_madd_epi16 (_mm_shuffle_epi32 (r1, _MM_SHUFFLE(2, 2, 2, 2)), *(__m128i*)(tab1+8*1));  \
      a3 = _mm_madd_epi16 (_mm_shuffle_epi32 (r1, _MM_SHUFFLE(3, 3, 3, 3)), *(__m128i*)(tab1+8*3));  \
      s0 = _mm_add_epi32 (_mm_add_epi32 (a0, round_row), a2);  \
      s1 = _mm_add_epi32 (a1, a3);  \
      p0 = _mm_srai_epi32 (_mm_add_epi32 (s0, s1), 11);  \
      p1 = _mm_shuffle_epi32 (_mm_srai_epi32 (_mm_sub_epi32(s0, s1), 11), _MM_SHUFFLE(0, 1, 2, 3));  \
      r2 = _mm_shufflelo_epi16 (r2, _MM_SHUFFLE(3, 1, 2, 0));  \
      r2 = _mm_shufflehi_epi16 (r2, _MM_SHUFFLE(3, 1, 2, 0));  \
      b0 = _mm_madd_epi16 (_mm_shuffle_epi32 (r2, _MM_SHUFFLE(0, 0, 0, 0)), *(__m128i*)(tab2+8*0));  \
      b1 = _mm_madd_epi16 (_mm_shuffle_epi32 (r2, _MM_SHUFFLE(1, 1, 1, 1)), *(__m128i*)(tab2+8*2));  \
      b2 = _mm_madd_epi16 (_mm_shuffle_epi32 (r2, _MM_SHUFFLE(2, 2, 2, 2)), *(__m128i*)(tab2+8*1));  \
      b3 = _mm_madd_epi16 (_mm_shuffle_epi32 (r2, _MM_SHUFFLE(3, 3, 3, 3)), *(__m128i*)(tab2+8*3));  \
      s2 = _mm_add_epi32 (_mm_add_epi32 (b0, round_row), b2);  \
      s3 = _mm_add_epi32 (b3, b1);  \
      p2 = _mm_srai_epi32 (_mm_add_epi32 (s2, s3), 11);  \
      p3 = _mm_shuffle_epi32 (_mm_srai_epi32 (_mm_sub_epi32(s2, s3), 11), _MM_SHUFFLE(0, 1, 2, 3));  \
      r1 = _mm_packs_epi32 (p0, p1);  \
      r2 = _mm_packs_epi32 (p2, p3);  \
    }
    //}}}
    __m128i r1, r2, a0, a1, a2, a3, b0, b1, b2, b3, s0, s1, s2, s3, p0, p1, p2, p3;
    __m128i round_row = _mm_set_epi16 (0, 1024, 0, 1024, 0, 1024, 0, 1024);

    r1 = *(__m128i*)(block+8*0);
    r2 = *(__m128i*)(block+8*1);
    DCT_8_INV_ROWX2(sse2_tab_i_04, sse2_tab_i_17);
    *(__m128i*)(block+8*0) = r1;
    *(__m128i*)(block+8*1) = r2;

    r1 = *(__m128i*)(block+8*4);
    r2 = *(__m128i*)(block+8*7);
    DCT_8_INV_ROWX2(sse2_tab_i_04, sse2_tab_i_17);
    *(__m128i*)(block+8*4) = r1;
    *(__m128i*)(block+8*7) = r2;

    r1 = *(__m128i*)(block+8*2);
    r2 = *(__m128i*)(block+8*3);
    DCT_8_INV_ROWX2(sse2_tab_i_26, sse2_tab_i_35);
    *(__m128i*)(block+8*2) = r1;
    *(__m128i*)(block+8*3) = r2;

    r1 = *(__m128i*)(block+8*6);
    r2 = *(__m128i*)(block+8*5);
    DCT_8_INV_ROWX2(sse2_tab_i_26, sse2_tab_i_35);
    *(__m128i*)(block+8*6) = r1;
    *(__m128i*)(block+8*5) = r2;

    __m128i x0 = *(__m128i*)(block+8*0);
    __m128i x1 = *(__m128i*)(block+8*1);
    __m128i x2 = *(__m128i*)(block+8*2);
    __m128i x3 = *(__m128i*)(block+8*3);
    __m128i x4 = *(__m128i*)(block+8*4);
    __m128i x5 = *(__m128i*)(block+8*5);
    __m128i x6 = *(__m128i*)(block+8*6);
    __m128i x7 = *(__m128i*)(block+8*7);

    __m128i tan1 = _mm_set1_epi16 (13036);
    __m128i tan2 = _mm_set1_epi16 (27146);
    __m128i tan3 = _mm_set1_epi16 (-21746);
    __m128i cos4 = _mm_set1_epi16 (-19195);
    __m128i round_err = _mm_set1_epi16 (1);
    __m128i round_col = _mm_set1_epi16 (32);
    __m128i round_corr = _mm_set1_epi16 (31);

    __m128i tp765 = _mm_adds_epi16 (_mm_mulhi_epi16 (x7, tan1), x1);
    __m128i tp465 = _mm_subs_epi16 (_mm_mulhi_epi16 (x1, tan1), x7);
    __m128i tm765 = _mm_adds_epi16 (_mm_mulhi_epi16 (x5, tan3), _mm_adds_epi16 (x5, x3));
    __m128i tm465 = _mm_subs_epi16 (x5, _mm_adds_epi16 (_mm_mulhi_epi16 (x3, tan3), x3));

    __m128i t7 = _mm_adds_epi16 (_mm_adds_epi16(tp765, tm765), round_err);
    __m128i tp65 = _mm_subs_epi16 (tp765, tm765);
    __m128i t4 = _mm_adds_epi16 (tp465, tm465);
    __m128i tm65 = _mm_adds_epi16 (_mm_subs_epi16(tp465, tm465), round_err);

    __m128i tmp1 = _mm_adds_epi16 (tp65, tm65);
    __m128i t6 = _mm_or_si128 (_mm_adds_epi16(_mm_mulhi_epi16 (tmp1, cos4), tmp1), round_err);
    __m128i tmp2 = _mm_subs_epi16 (tp65, tm65);
    __m128i t5 = _mm_or_si128 (_mm_adds_epi16(_mm_mulhi_epi16 (tmp2, cos4), tmp2), round_err);

    __m128i tp03 = _mm_adds_epi16 (x0, x4);
    __m128i tp12 = _mm_subs_epi16 (x0, x4);
    __m128i tm03 = _mm_adds_epi16 (_mm_mulhi_epi16( x6, tan2), x2);
    __m128i tm12 = _mm_subs_epi16 (_mm_mulhi_epi16 (x2, tan2), x6);

    __m128i t0 = _mm_adds_epi16 (_mm_adds_epi16 (tp03, tm03), round_col);
    __m128i t3 = _mm_adds_epi16 (_mm_subs_epi16 (tp03, tm03), round_corr);
    __m128i t1 = _mm_adds_epi16 (_mm_adds_epi16 (tp12, tm12), round_col);
    __m128i t2 = _mm_adds_epi16 (_mm_subs_epi16 (tp12, tm12), round_corr);

    *(__m128i*)(block+8*0) = _mm_srai_epi16 (_mm_adds_epi16(t0, t7), 6);
    *(__m128i*)(block+8*7) = _mm_srai_epi16 (_mm_subs_epi16(t0, t7), 6);
    *(__m128i*)(block+8*1) = _mm_srai_epi16 (_mm_adds_epi16(t1, t6), 6);
    *(__m128i*)(block+8*6) = _mm_srai_epi16 (_mm_subs_epi16(t1, t6), 6);
    *(__m128i*)(block+8*2) = _mm_srai_epi16 (_mm_adds_epi16(t2, t5), 6);
    *(__m128i*)(block+8*5) = _mm_srai_epi16 (_mm_subs_epi16(t2, t5), 6);
    *(__m128i*)(block+8*3) = _mm_srai_epi16 (_mm_adds_epi16(t3, t4), 6);
    *(__m128i*)(block+8*4) = _mm_srai_epi16 (_mm_subs_epi16(t3, t4), 6);
    }
  //}}}
  //{{{
  void addBlock (int16_t* block, bool luma, int comp, int bx, int by, int dctType, int intra) {
  // move/add 8x8-Block from block[comp] to backward_reference_frame
  // copy reconstructed 8x8 block from block[comp] to current_frame[]

    int lineInc;
    uint8_t* refFramePtr;
    if (luma) {
      if (dctType) {
        // luma field DCT coding
        lineInc = (mWidth << 1);
        refFramePtr = mCurrentFrame[0] + mWidth * (by + ((comp & 2) >> 1)) + bx + ((comp & 1) << 3);
        }
      else {
        // luma frame DCT coding
        lineInc = mWidth;
        refFramePtr = mCurrentFrame[0] + mWidth * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
        }
      }
    else {
      // chroma scale coordinates, frame DCT coding
      lineInc = mChromaWidth;
      refFramePtr = mCurrentFrame[(comp & 1) + 1] + mChromaWidth * ((by >> 1) + ((comp & 2) << 2)) + (bx >> 1) + (comp & 8);
      }

  //#ifdef _M_X64
  #ifdef _M_IX86
    //{{{  x86 intrinsics
    __m64 *src = (__m64*)block;
    if (intra) {
      __m64 offset = _mm_set1_pi16 (128);
      for (int loop = 0; loop < 8; loop++) {
        __m64 sum1 = _m_paddw (*src++, offset);
        __m64 sum2 = _m_paddw (*src++, offset);
        *(__m64*)refFramePtr = _m_packuswb (sum1, sum2);
        refFramePtr += lineInc;
        }
      }
    else {
       __m64 zero = _mm_setzero_si64();
      for (int loop = 0; loop < 8; loop++) {
        __m64 sum1 = _m_paddw (*src++, _m_punpcklbw (*(__m64*)refFramePtr, zero));
        __m64 sum2 = _m_paddw (*src++, _m_punpckhbw (*(__m64*)refFramePtr, zero));
        *(__m64*)refFramePtr = _m_packuswb (sum1, sum2);
        refFramePtr += lineInc;
        }
      }
    //}}}
  #else
    //{{{
    //lineInc -= 8;
    //if (intra)
      //for (int j = 0; j < 8; j++) {
        //for (int i = 0; i < 8; i++)
          //*refFramePtr++ = *block++ + 128;
        //refFramePtr += lineInc;
        //}
    //else
      //for (int j = 0; j < 8; j++) {
        //for (int i = 0; i < 8; i++)
          //*refFramePtr++ += *block++;
        //refFramePtr += lineInc;
        //}
    //}}}
    //{{{
    lineInc -= 8;

    if (intra) {
      for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++)
          *refFramePtr++ = Clip [*block++ + 128];
        refFramePtr += lineInc;
        }
      }
    else {
      for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++)
          *refFramePtr++ = Clip [*block++ + *refFramePtr];
        refFramePtr += lineInc;
        }
      }
    //}}}
  #endif
    }
  //}}}
  //{{{
  void motionCompensation (int mbAddress, int mbType, int motionType, int PMV[2][2][2], int motionVertField[2][2], int dctType) {

    // derive current macroblock position within picture
    int bx = 16 * (mbAddress % mMbWidth);
    int by = 16 * (mbAddress / mMbWidth);

    // motion compensation
    if (!(mbType & MACROBLOCK_INTRA))
      formPredictions (bx, by, mbType, motionType, PMV, motionVertField);

    for (int i = 0; i < 6; i++) {
      idctSSE2 (mBlock[i]);
      addBlock (mBlock[i], i < 4, i, bx, by, dctType, mbType & MACROBLOCK_INTRA);
      }
    }
  //}}}
  //{{{
  void decodeMacroblock (int& mbType, int& motionType, int& dctType, int PMV[2][2][2], int dcDctPred[3], int motionVertField[2][2]) {

    int mvCount, mvFormat, mvScale;
    macroBlockModes (mbType, motionType, mvCount, mvFormat, mvScale, dctType);

    int quantizerScaleCode;
    if (mbType & MACROBLOCK_QUANT) {
      quantizerScaleCode = getBits (5);
      quantizerScaleCode = qScaleType ? Non_Linear_quantizer_scale[quantizerScaleCode] : (quantizerScaleCode << 1);
      }

    // decode forward motion vectors
    if ((mbType & MACROBLOCK_MOTION_FORWARD) || ((mbType & MACROBLOCK_INTRA) && mConcealmentMotionVecs))
      motionVectors (PMV, motionVertField, 0, mvCount, mvFormat, mFcode[0][0]-1, mFcode[0][1]-1, mvScale);

    // decode backward motion vectors
    if (mbType & MACROBLOCK_MOTION_BACKWARD)
      motionVectors (PMV, motionVertField, 1, mvCount, mvFormat, mFcode[1][0]-1, mFcode[1][1]-1, mvScale);

    if ((mbType & MACROBLOCK_INTRA) && mConcealmentMotionVecs)
      consumeBits (1); // remove marker_bit

    int coded_block_pattern;
    if (mbType & MACROBLOCK_PATTERN)
      coded_block_pattern = getCodedBlockPattern();
    else
      coded_block_pattern = (mbType & MACROBLOCK_INTRA) ? (1 << 6) - 1 : 0;

    // decode blocks
    memset (mBlock[0], 0, 128 * sizeof(int16_t)*6);
    for (auto i = 0; i < 6; i++)
      if (coded_block_pattern & (1 << (6 - 1 - i)))
        (mbType & MACROBLOCK_INTRA) ? decodeIntraBlock (i, dcDctPred) : decodeNonIntraBlock (i);

    // reset intra_dc predictors
    if (!(mbType & MACROBLOCK_INTRA))
      dcDctPred[0] = dcDctPred[1] = dcDctPred[2] = 0;

    // reset motion vector predictors
    if ((mbType & MACROBLOCK_INTRA) && !mConcealmentMotionVecs) {
      // intra mb without concealment motion vectors, reset motion vector predictors
      PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
      PMV[0][1][0] = PMV[0][1][1] = PMV[1][1][0] = PMV[1][1][1] = 0;
      }

    // special "No_MC" mBtype case prediction in P
    if ((mPictureCodingType == P_TYPE) && !(mbType & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_INTRA))) {
      // non-intra mb without forward mv in P
      PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
      motionType = MC_FRAME;
      }
    }
  //}}}
  //{{{
  void decodeSlices() {

    auto code = getStartCode();
    while ((code >= SLICE_START_CODE_MIN) && (code <= SLICE_START_CODE_MAX)) {
      consumeBits (32);
      sliceHeader();

      int dcDctPred[3] = {0,0,0};
      int PMV[2][2][2] = {0,0,0,0,0,0,0,0};

      int mbAddressInc = getMacroBlockAddressInc();
      for (int mbAddress = ((code & 255) - 1) * mMbWidth; mbAddress < (mMbWidth * mMbHeight); mbAddress++) {
        if (mbAddressInc == 0) {
          if (peekBits(23) == 0)
            goto getNextStartCode;
          else
            mbAddressInc = getMacroBlockAddressInc();
          }

        int mBtype, motionType, dctType, motionVertField[2][2];
        if (mbAddressInc == 1)
          decodeMacroblock (mBtype, motionType, dctType, PMV, dcDctPred, motionVertField);
        else { // skip macroBlock
          dcDctPred[0] = dcDctPred[1] = dcDctPred[2] = 0;
          memset (mBlock[0], 0, 128 * sizeof(int16_t) * 6);
          if (mPictureCodingType == P_TYPE)
            PMV[0][0][0] = PMV[0][0][1] = PMV[1][0][0] = PMV[1][0][1] = 0;
          motionType = MC_FRAME;
          mBtype &= ~MACROBLOCK_INTRA;
          }

        motionCompensation (mbAddress, mBtype, motionType, PMV, motionVertField, dctType);
        mbAddressInc--;
        }
      return;

   getNextStartCode:
      code = getStartCode();
      }
    }
  //}}}

  //{{{  vars
  int16_t* mBlock[6];

  uint8_t* mAuxFrame[3];
  uint8_t* mCurrentFrame[3];
  uint8_t* mForwardRefFrame[3];
  uint8_t* mBackwardRefFrame[3];

  int mLoadVidFrame = 0;
  int mMaxVidFrame = maxVidFrames;
  cYuvFrame mYuvFrames[maxVidFrames];

  // bitStream
  int8_t mBitCount = 0;
  uint32_t m32bits = 0;
  uint8_t* mBufferPtr = NULL;
  uint8_t* mBufferEnd = NULL;
  bool mGotSequenceHeader = false;

  // pic size
  int mWidth = 0;
  int mHeight = 0;
  int mChromaWidth = 0;
  int mChromaHeight = 0;
  int mMbWidth = 0;
  int mMbHeight = 0;

  // header values
  int mTemporalReference = 0;
  int mPictureCodingType = 0;

  int qScaleType = 0;
  int mAlternateScan = 0;
  int mQuantizerScale = 0;
  int mFcode[2][2];
  int mIntraDcPrecision = 0;
  int mFramePredFrameDct = 0;
  int mConcealmentMotionVecs = 0;
  int mIntraVlcFormat = 0;
  //}}}
  };
