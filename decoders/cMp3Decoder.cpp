// cMp3Decoder.cpp - based on https://github.com/lieff/minimp3
//{{{  includes
#include <algorithm>

#include "cMp3decoder.h"

#include "../utils/cLog.h"
//}}}
//{{{  defines
#define SHORT_blockType             2
#define STOP_blockType              3

#define MODE_MONO                   3
#define MODE_JOINT_STEREO           1

#define HDR_SIZE                    4
#define HDR_IS_MONO(h)              (((h[3]) & 0xC0) == 0xC0)
#define HDR_IS_MS_STEREO(h)         (((h[3]) & 0xE0) == 0x60)
#define HDR_IS_FREE_FORMAT(h)       (((h[2]) & 0xF0) == 0)
#define HDR_IS_CRC(h)               (!((h[1]) & 1))

#define HDR_TEST_PADDING(h)         ((h[2]) & 0x2)
#define HDR_TEST_MPEG1(h)           ((h[1]) & 0x8)
#define HDR_TEST_NOT_MPEG25(h)      ((h[1]) & 0x10)
#define HDR_TEST_I_STEREO(h)        ((h[3]) & 0x10)
#define HDR_TEST_MS_STEREO(h)       ((h[3]) & 0x20)

#define HDR_GET_STEREO_MODE(h)      (((h[3]) >> 6) & 3)
#define HDR_GET_STEREO_MODE_EXT(h)  (((h[3]) >> 4) & 3)
#define HDR_GET_LAYER(h)            (((h[1]) >> 1) & 3)
#define HDR_GET_BITRATE(h)          ((h[2]) >> 4)
#define HDR_GET_SAMPLE_RATE(h)      (((h[2]) >> 2) & 3)
#define HDR_GET_MY_SAMPLE_RATE(h)   (HDR_GET_SAMPLE_RATE(h) + (((h[1] >> 3) & 1) + ((h[1] >> 4) & 1))*3)

#define HDR_IS_FRAME_576(h)         ((h[1] & 14) == 2)
#define HDR_IS_LAYER_1(h)           ((h[1] & 6) == 6)

#define BITS_DEQUANTIZER_OUT        -1
#define MAX_SCF                     (255 + BITS_DEQUANTIZER_OUT*4 - 210)
#define MAX_SCFI                    ((MAX_SCF + 3) & ~3)
//}}}

#define USE_INTRINSICS
#ifdef USE_INTRINSICS
  //{{{  intrinsics
  #include <intrin.h>
  #include <immintrin.h>

  #define VSTORE  _mm_storeu_ps
  #define VLD     _mm_loadu_ps
  #define VSET    _mm_set1_ps
  #define VADD    _mm_add_ps
  #define VSUB    _mm_sub_ps
  #define VMUL    _mm_mul_ps

  #define VMAC(a, x, y)  _mm_add_ps (a, _mm_mul_ps(x, y))
  #define VMSB(a, x, y)  _mm_sub_ps (a, _mm_mul_ps(x, y))
  #define VMUL_S(x, s)   _mm_mul_ps (x, _mm_set1_ps(s))

  #define VREV(x) _mm_shuffle_ps (x, x, _MM_SHUFFLE(0, 1, 2, 3))

  typedef __m128 f4;

  static const f4 g_max = {  32767.0f,  32767.0f,  32767.0f,  32767.0f };
  static const f4 g_min = { -32768.0f, -32768.0f, -32768.0f, -32768.0f };
  //}}}
#endif

//{{{
struct sScaleInfo {
  float   scf [3*64];
  uint8_t totalBands;
  uint8_t stereoBands;
  uint8_t bitalloc [64];
  uint8_t scfcod [64];
  };
//}}}
//{{{
struct sSubBandAlloc {
  uint8_t tabOffset;
  uint8_t codeTableWidth;
  uint8_t bandCount;
  };
//}}}
//{{{  static const tables
//{{{
static const float g_pow43 [129 + 16] = {
  0, -1, -2.519842f, -4.326749f, -6.349604f, -8.549880f, -10.902724f, -13.390518f,- 16.000000f,
  -18.720754f, -21.544347f, -24.463781f, -27.473142f, -30.567351f, -33.741992f, -36.993181f,

  0, 1, 2.519842f, 4.326749f, 6.349604f, 8.549880f, 10.902724f, 13.390518f, 16.000000f,
  18.720754f, 21.544347f, 24.463781f, 27.473142f, 30.567351f, 33.741992f, 36.993181f,
  40.317474f, 43.711787f, 47.173345f, 50.699631f, 54.288352f, 57.937408f, 61.644865f,
  65.408941f, 69.227979f, 73.100443f, 77.024898f, 81.000000f, 85.024491f,
  89.097188f,93.216975f,97.382800f,101.593667f,105.848633f,110.146801f,114.487321f,118.869381f,123.292209f,
  127.755065f,132.257246f,136.798076f,141.376907f,145.993119f,150.646117f,155.335327f,160.060199f,164.820202f,
  169.614826f,174.443577f,179.305980f,184.201575f,189.129918f,194.090580f,199.083145f,204.107210f,209.162385f,
  214.248292f,219.364564f,224.510845f,229.686789f,234.892058f,240.126328f,245.389280f,250.680604f,256.000000f,
  261.347174f,266.721841f,272.123723f,277.552547f,283.008049f,288.489971f,293.998060f,299.532071f,305.091761f,
  310.676898f,316.287249f,321.922592f,327.582707f,333.267377f,338.976394f,344.709550f,350.466646f,356.247482f,
  362.051866f,367.879608f,373.730522f,379.604427f,385.501143f,391.420496f,397.362314f,403.326427f,409.312672f,
  415.320884f,421.350905f,427.402579f,433.475750f,439.570269f,445.685987f,451.822757f,457.980436f,464.158883f,
  470.357960f,476.577530f,482.817459f,489.077615f,495.357868f,501.658090f,507.978156f,514.317941f,520.677324f,
  527.056184f,533.454404f,539.871867f,546.308458f,552.764065f,559.238575f,565.731879f,572.243870f,578.774440f,
  585.323483f,591.890898f,598.476581f,605.080431f,611.702349f,618.342238f,625.000000f,631.675540f,638.368763f,
  645.079578f
  };
//}}}

//{{{
static const uint32_t g_hz[3] = { 44100, 48000, 32000 };
//}}}
//{{{
static const uint8_t g_halfrate [2][3][15] = {
  { { 0,4,8,12,16,20,24,28,32,40,48,56,64,72,80 },
    { 0,4,8,12,16,20,24,28,32,40,48,56,64,72,80 },
    { 0,16,24,28,32,40,48,56,64,72,80,88,96,112,128 }     },
  { { 0,16,20,24,28,32,40,48,56,64,80,96,112,128,160 },
    { 0,16,24,28,32,40,48,56,64,80,96,112,128,160,192 },
    { 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224 } },
  };
//}}}

//{{{
static const struct sSubBandAlloc g_alloc_L1[] = { { 76, 4, 32 } };
//}}}
//{{{
static const struct sSubBandAlloc g_alloc_L2M2[] = { { 60, 4, 4 }, { 44, 3, 7 }, { 44, 2, 19 } };
//}}}
//{{{
static const struct sSubBandAlloc g_alloc_L2M1[] = { { 0, 4, 3 }, { 16, 4, 8 }, { 32, 3, 12 }, { 40, 2, 7 } };
//}}}
//{{{
static const struct sSubBandAlloc g_alloc_L2M1_lowrate[] = { { 44, 4, 2 }, { 44, 3, 10 } };
//}}}

//{{{
static const float g_deq_L12[18*3] = {
  #define DQ(x) 9.53674316e-07f/x, 7.56931807e-07f/x, 6.00777173e-07f/x

  DQ(3),     DQ(7),     DQ(15),    DQ(31),   DQ(63),   DQ(127),
  DQ(255),   DQ(511),   DQ(1023),  DQ(2047), DQ(4095), DQ(8191),
  DQ(16383), DQ(32767), DQ(65535), DQ(3),    DQ(5),    DQ(9)
  };
//}}}
//{{{
static const uint8_t g_bitalloc_code_tab[] = {
  0,17, 3, 4, 5,6,7, 8,9,10,11,12,13,14,15,16,
  0,17,18, 3,19,4,5, 6,7, 8, 9,10,11,12,13,16,
  0,17,18, 3,19,4,5,16,
  0,17,18,16,
  0,17,18,19, 4,5,6, 7,8, 9,10,11,12,13,14,15,
  0,17,18, 3,19,4,5, 6,7, 8, 9,10,11,12,13,14,
  0, 2, 3, 4, 5,6,7, 8,9,10,11,12,13,14,15,16
  };
//}}}
//{{{
static const uint8_t g_scf_long [8][23] = {
  { 6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54,0 },
  { 12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2,0 },
  { 6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54,0 },
  { 6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,54,62,70,76,36,0 },
  { 6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54,0 },
  { 4,4,4,4,4,4,6,6,8,8,10,12,16,20,24,28,34,42,50,54,76,158,0 },
  { 4,4,4,4,4,4,6,6,6,8,10,12,16,18,22,28,34,40,46,54,54,192,0 },
  { 4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102,26,0 }
  };
//}}}
//{{{
static const uint8_t g_scf_short [8][40] = {
  { 4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0 },
  { 8,8,8,8,8,8,8,8,8,12,12,12,16,16,16,20,20,20,24,24,24,28,28,28,36,36,36,2,2,2,2,2,2,2,2,2,26,26,26,0 },
  { 4,4,4,4,4,4,4,4,4,6,6,6,6,6,6,8,8,8,10,10,10,14,14,14,18,18,18,26,26,26,32,32,32,42,42,42,18,18,18,0 },
  { 4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,32,32,32,44,44,44,12,12,12,0 },
  { 4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0 },
  { 4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,22,22,22,30,30,30,56,56,56,0 },
  { 4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,6,6,6,10,10,10,12,12,12,14,14,14,16,16,16,20,20,20,26,26,26,66,66,66,0 },
  { 4,4,4,4,4,4,4,4,4,4,4,4,6,6,6,8,8,8,12,12,12,16,16,16,20,20,20,26,26,26,34,34,34,42,42,42,12,12,12,0 }
  };
//}}}
//{{{
static const uint8_t g_scf_mixed [8][40] = {
  { 6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0 },
  { 12,12,12,4,4,4,8,8,8,12,12,12,16,16,16,20,20,20,24,24,24,28,28,28,36,36,36,2,2,2,2,2,2,2,2,2,26,26,26,0 },
  { 6,6,6,6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,14,14,14,18,18,18,26,26,26,32,32,32,42,42,42,18,18,18,0 },
  { 6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,32,32,32,44,44,44,12,12,12,0 },
  { 6,6,6,6,6,6,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,24,24,24,30,30,30,40,40,40,18,18,18,0 },
  { 4,4,4,4,4,4,6,6,4,4,4,6,6,6,8,8,8,10,10,10,12,12,12,14,14,14,18,18,18,22,22,22,30,30,30,56,56,56,0 },
  { 4,4,4,4,4,4,6,6,4,4,4,6,6,6,6,6,6,10,10,10,12,12,12,14,14,14,16,16,16,20,20,20,26,26,26,66,66,66,0 },
  { 4,4,4,4,4,4,6,6,4,4,4,6,6,6,8,8,8,12,12,12,16,16,16,20,20,20,26,26,26,34,34,34,42,42,42,12,12,12,0 }
  };
//}}}

//{{{
static const float g_expfrac [4] = { 9.31322575e-10f,7.83145814e-10f,6.58544508e-10f,5.53767716e-10f };
//}}}
//{{{
static const uint8_t g_scf_partitions [3][28] = {
  { 6,5,5, 5,6,5,5,5,6,5, 7,3,11,10,0,0, 7, 7, 7,0, 6, 6,6,3, 8, 8,5,0 },
  { 8,9,6,12,6,9,9,9,6,9,12,6,15,18,0,0, 6,15,12,0, 6,12,9,6, 6,18,9,0 },
  { 9,9,6,12,9,9,9,9,9,9,12,6,18,18,0,0,12,12,12,0,12, 9,9,6,15,12,9,0 }
  };
//}}}
//{{{
static const uint8_t g_scfc_decode [16] = { 0,1,2,3, 12,5,6,7, 9,10,11,13, 14,15,18,19 };
//}}}

//{{{
static const uint8_t g_mod [6*4] = { 5,5,4,4,5,5,4,1,4,3,1,1,5,6,6,1,4,4,4,1,4,3,1,1 };
//}}}
//{{{
static const uint8_t g_preamp [10] = { 1,1,1,1,2,2,3,3,3,2 };
//}}}

//{{{
static const int16_t g_tabs[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    785,785,785,785,784,784,784,784,513,513,513,513,513,513,513,513,256,256,256,256,256,256,256,256,256,256,256,
    256,256,256,256,256,
    -255,1313,1298,1282,785,785,785,785,784,784,784,784,769,769,769,769,256,256,256,256,256,256,256,256,256,256,
    256,256,256,256,256,256,290,288,
    -255,1313,1298,1282,769,769,769,769,529,529,529,529,529,529,529,529,528,528,528,528,528,528,528,528,512,512,
    512,512,512,512,512,512,290,288,
    -253,-318,-351,-367,785,785,785,785,784,784,784,784,769,769,769,769,256,256,256,256,256,256,256,256,256,256,
    256,256,256,256,256,256,819,818,547,547,275,275,275,275,561,560,515,546,289,274,288,258,
    -254,-287,1329,1299,1314,1312,1057,1057,1042,1042,1026,1026,784,784,784,784,529,529,529,529,529,529,529,529,
    769,769,769,769,768,768,768,768,563,560,306,306,291,259,
    -252,-413,-477,-542,1298,-575,1041,1041,784,784,784,784,769,769,769,769,256,256,256,256,256,256,256,256,256,
    256,256,256,256,256,256,256,-383,-399,1107,1092,1106,1061,849,849,789,789,1104,1091,773,773,1076,1075,341,
    340,325,309,834,804,577,577,532,532,516,516,832,818,803,816,561,561,531,531,515,546,289,289,288,258,
    -252,-429,-493,-559,1057,1057,1042,1042,529,529,529,529,529,529,529,529,784,784,784,784,769,769,769,769,512,
    512,512,512,512,512,512,512,-382,1077,-415,1106,1061,1104,849,849,789,789,1091,1076,1029,1075,834,834,597
    ,581,340,340,339,324,804,833,532,532,832,772,818,803,817,787,816,771,290,290,290,290,288,258,
    -253,-349,-414,-447,-463,1329,1299,-479,1314,1312,1057,1057,1042,1042,1026,1026,785,785,785,785,784,784,784,
    784,769,769,769,769,768,768,768,768,-319,851,821,-335,836,850,805,849,341,340,325,336,533,533,579,579,564,564,
    773,832,578,548,563,516,321,276,306,291,304,259,
    -251,-572,-733,-830,-863,-879,1041,1041,784,784,784,784,769,769,769,769,256,256,256,256,256,256,256,256,256,
    256,256,256,256,256,256,256,-511,-527,-543,1396,1351,1381,1366,1395,1335,1380,-559,1334,1138,1138,1063,1063,
    1350,1392,1031,1031,1062,1062,1364,1363,1120,1120,1333,1348,881,881,881,881,375,374,359,373,343,358,341,325,
    791,791,1123,1122,-703,1105,1045,-719,865,865,790,790,774,774,1104,1029,338,293,323,308,-799,-815,833,788,772,
    818,803,816,322,292,307,320,561,531,515,546,289,274,288,258,
    -251,-525,-605,-685,-765,-831,-846,1298,1057,1057,1312,1282,785,785,785,785,784,784,784,784,769,769,769,769,
    512,512,512,512,512,512,512,512,1399,1398,1383,1367,1382,1396,1351,-511,1381,1366,1139,1139,1079,1079,1124,
    1124,1364,1349,1363,1333,882,882,882,882,807,807,807,807,1094,1094,1136,1136,373,341,535,535,881,775,867,822,
    774,-591,324,338,-671,849,550,550,866,864,609,609,293,336,534,534,789,835,773,-751,834,804,308,307,833,788,
    832,772,562,562,547,547,305,275,560,515,290,290,
    -252,-397,-477,-557,-622,-653,-719,-735,-750,1329,1299,1314,1057,1057,1042,1042,1312,1282,1024,1024,785,785
    ,785,785,784,784,784,784,769,769,769,769,-383,1127,1141,1111,1126,1140,1095,1110,869,869,883,883,1079,1109,
    882,882,375,374,807,868,838,881,791,-463,867,822,368,263,852,837,836,-543,610,610,550,550,352,336,534,534,865,
    774,851,821,850,805,593,533,579,564,773,832,578,578,548,548,577,577,307,276,306,291,516,560,259,259,
    -250,-2107,-2507,-2764,-2909,-2974,-3007,-3023,1041,1041,1040,1040,769,769,769,769,256,256,256,256,256,256,
    256,256,256,256,256,256,256,256,256,256,-767,-1052,-1213,-1277,-1358,-1405,-1469,-1535,-1550,-1582,-1614,
    -1647,-1662,-1694,-1726,-1759,-1774,-1807,-1822,-1854,-1886,1565,-1919,-1935,-1951,-1967,1731,1730,1580,1717,
    -1983,1729,1564,-1999,1548,-2015,-2031,1715,1595,-2047,1714,-2063,1610,-2079,1609,-2095,1323,1323,1457,1457,
    1307,1307,1712,1547,1641,1700,1699,1594,1685,1625,1442,1442,1322,1322,-780,-973,-910,1279,1278,1277,1262,1276,
    1261,1275,1215,1260,1229,-959,974,974,989,989,-943,735,478,478,495,463,506,414,-1039,1003,958,1017,927,942,987,
    957,431,476,1272,1167,1228,-1183,1256,-1199,895,895,941,941,1242,1227,1212,1135,1014,1014,490,489,503,487,910,
    1013,985,925,863,894,970,955,1012,847,-1343,831,755,755,984,909,428,366,754,559,-1391,752,486,457,924,997,698,
    698,983,893,740,740,908,877,739,739,667,667,953,938,497,287,271,271,683,606,590,712,726,574,302,302,738,736,481,
    286,526,725,605,711,636,724,696,651,589,681,666,710,364,467,573,695,466,466,301,465,379,379,709,604,665,679,316,
    316,634,633,436,436,464,269,424,394,452,332,438,363,347,408,393,448,331,422,362,407,392,421,346,406,391,376,375,
    359,1441,1306,-2367,1290,-2383,1337,-2399,-2415,1426,1321,-2431,1411,1336,-2447,-2463,-2479,1169,1169,1049,1049,
    1424,1289,1412,1352,1319,-2495,1154,1154,1064,1064,1153,1153,416,390,360,404,403,389,344,374,373,343,358,372,327,
    357,342,311,356,326,1395,1394,1137,1137,1047,1047,1365,1392,1287,1379,1334,1364,1349,1378,1318,1363,792,792,792,
    792,1152,1152,1032,1032,1121,1121,1046,1046,1120,1120,1030,1030,-2895,1106,1061,1104,849,849,789,789,1091,1076,
    1029,1090,1060,1075,833,833,309,324,532,532,832,772,818,803,561,561,531,560,515,546,289,274,288,258,
    -250,-1179,-1579,-1836,-1996,-2124,-2253,-2333,-2413,-2477,-2542,-2574,-2607,-2622,-2655,1314,1313,1298,1312,
    1282,785,785,785,785,1040,1040,1025,1025,768,768,768,768,-766,-798,-830,-862,-895,-911,-927,-943,-959,-975,-991
    ,-1007,-1023,-1039,-1055,-1070,1724,1647,-1103,-1119,1631,1767,1662,1738,1708,1723,-1135,1780,1615,1779,1599,
    1677,1646,1778,1583,-1151,1777,1567,1737,1692,1765,1722,1707,1630,1751,1661,1764,1614,1736,1676,1763,1750,1645,
    1598,1721,1691,1762,1706,1582,1761,1566,-1167,1749,1629,767,766,751,765,494,494,735,764,719,749,734,763,447,447,
    748,718,477,506,431,491,446,476,461,505,415,430,475,445,504,399,460,489,414,503,383,474,429,459,502,502,746,752,
    488,398,501,473,413,472,486,271,480,270,-1439,-1455,1357,-1471,-1487,-1503,1341,1325,-1519,1489,1463,1403,1309,
    -1535,1372,1448,1418,1476,1356,1462,1387,-1551,1475,1340,1447,1402,1386,-1567,1068,1068,1474,1461,455,380,468,
    440,395,425,410,454,364,467,466,464,453,269,409,448,268,432,1371,1473,1432,1417,1308,1460,1355,1446,1459,1431,
    1083,1083,1401,1416,1458,1445,1067,1067,1370,1457,1051,1051,1291,1430,1385,1444,1354,1415,1400,1443,1082,1082,
    1173,1113,1186,1066,1185,1050,-1967,1158,1128,1172,1097,1171,1081,-1983,1157,1112,416,266,375,400,1170,1142,
    1127,1065,793,793,1169,1033,1156,1096,1141,1111,1155,1080,1126,1140,898,898,808,808,897,897,792,792,1095,1152,
    1032,1125,1110,1139,1079,1124,882,807,838,881,853,791,-2319,867,368,263,822,852,837,866,806,865,-2399,851,352,
    262,534,534,821,836,594,594,549,549,593,593,533,533,848,773,579,579,564,578,548,563,276,276,577,576,306,291,516,560,305,305,275,259,
    -251,-892,-2058,-2620,-2828,-2957,-3023,-3039,1041,1041,1040,1040,769,769,769,769,256,256,256,256,256,256,256,
    256,256,256,256,256,256,256,256,256,-511,-527,-543,-559,1530,-575,-591,1528,1527,1407,1526,1391,1023,1023,1023,
    1023,1525,1375,1268,1268,1103,1103,1087,1087,1039,1039,1523,-604,815,815,815,815,510,495,509,479,508,463,507,
    447,431,505,415,399,-734,-782,1262,-815,1259,1244,-831,1258,1228,-847,-863,1196,-879,1253,987,987,748,-767,493,493,462,477,414,414,686,669,478,446,461,445,474,429,487,458,412,471,1266,1264,1009,1009,799,799,-1019,-1276,-1452,-1581,-1677,-1757,-1821,-1886,-1933,-1997,1257,1257,1483,1468,1512,1422,1497,1406,1467,1496,1421,1510,1134,1134,1225,1225,1466,1451,1374,1405,1252,1252,1358,1480,1164,1164,1251,1251,1238,1238,1389,1465,-1407,1054,1101,-1423,1207,-1439,830,830,1248,1038,1237,1117,1223,1148,1236,1208,411,426,395,410,379,269,1193,1222,1132,1235,1221,1116,976,976,1192,1162,1177,1220,1131,1191,963,963,-1647,961,780,-1663,558,558,994,993,437,408,393,407,829,978,813,797,947,-1743,721,721,377,392,844,950,828,890,706,706,812,859,796,960,948,843,934,874,571,571,-1919,690,555,689,421,346,539,539,944,779,918,873,932,842,903,888,570,570,931,917,674,674,-2575,1562,-2591,1609,-2607,1654,1322,1322,1441,1441,1696,1546,1683,1593,1669,1624,1426,1426,1321,1321,1639,1680,1425,1425,1305,1305,1545,1668,1608,1623,1667,1592,1638,1666,1320,1320,1652,1607,1409,1409,1304,1304,1288,1288,1664,1637,1395,1395,1335,1335,1622,1636,1394,1394,1319,1319,1606,1621,1392,1392,1137,1137,1137,1137,345,390,360,375,404,373,1047,-2751,-2767,-2783,1062,1121,1046,-2799,1077,-2815,1106,1061,789,789,1105,1104,263,355,310,340,325,354,352,262,339,324,1091,1076,1029,1090,1060,1075,833,833,788,788,1088,1028,818,818,803,803,561,561,531,531,816,771,546,546,289,274,288,258,
    -253,-317,-381,-446,-478,-509,1279,1279,-811,-1179,-1451,-1756,-1900,-2028,-2189,-2253,-2333,-2414,-2445,
    -2511,-2526,1313,1298,-2559,1041,1041,1040,1040,1025,1025,1024,1024,1022,1007,1021,991,1020,975,1019,959,
    687,687,1018,1017,671,671,655,655,1016,1015,639,639,758,758,623,623,757,607,756,591,755,575,754,559,543,543,
    1009,783,-575,-621,-685,-749,496,-590,750,749,734,748,974,989,1003,958,988,973,1002,942,987,957,972,1001,926,
    986,941,971,956,1000,910,985,925,999,894,970,-1071,-1087,-1102,1390,-1135,1436,1509,1451,1374,-1151,1405,
    1358,1480,1420,-1167,1507,1494,1389,1342,1465,1435,1450,1326,1505,1310,1493,1373,1479,1404,1492,1464,1419,
    428,443,472,397,736,526,464,464,486,457,442,471,484,482,1357,1449,1434,1478,1388,1491,1341,1490,1325,1489,
    1463,1403,1309,1477,1372,1448,1418,1433,1476,1356,1462,1387,-1439,1475,1340,1447,1402,1474,1324,1461,1371,
    1473,269,448,1432,1417,1308,1460,-1711,1459,-1727,1441,1099,1099,1446,1386,1431,1401,-1743,1289,1083,1083,
    1160,1160,1458,1445,1067,1067,1370,1457,1307,1430,1129,1129,1098,1098,268,432,267,416,266,400,-1887,1144,
    1187,1082,1173,1113,1186,1066,1050,1158,1128,1143,1172,1097,1171,1081,420,391,1157,1112,1170,1142,1127,
    1065,1169,1049,1156,1096,1141,1111,1155,1080,1126,1154,1064,1153,1140,1095,1048,-2159,1125,1110,1137,
    -2175,823,823,1139,1138,807,807,384,264,368,263,868,838,853,791,867,822,852,837,866,806,865,790,-2319,
    851,821,836,352,262,850,805,849,-2399,533,533,835,820,336,261,578,548,563,577,532,532,832,772,562,562,
    547,547,305,275,560,515,290,290,288,258 };
//}}}
//{{{
static const uint8_t g_tab32[] = {
  130,162,193,209,44,28,76,140,9,9,9,9,9,9,9,9,190,254,222,238,126, 94,157,157,109,61,173,205 };
//}}}
//{{{
static const uint8_t g_tab33[] = {
  252,236,220,204,188,172,156,140,124,108,92,76,60,44,28,12 };
//}}}
//{{{
static const int16_t g_tabindex[2*16] = {
  0, 32, 64, 98, 0, 132, 180, 218, 292, 364, 426, 538, 648, 746, 0, 1126,
  1460,1460,1460,1460,1460,1460,1460,1460, 1842,1842,1842,1842,1842,1842,1842,1842
  };
//}}}

//{{{
static const uint8_t g_linbits[] =  {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,6,8,10,13,4,5,6,7,8,9,11,13 };
//}}}
//{{{
static const float g_pan [7*2] = {
            0,           1,
  0.21132487f, 0.78867513f,
  0.36602540f, 0.63397460f,
          0.5f,       0.5f,
  0.63397460f, 0.36602540f,
  0.78867513f, 0.21132487f,
            1,           0 };
//}}}
//{{{
static const float g_aa [2][8] = {
  {0.85749293f, 0.88174200f, 0.94962865f, 0.98331459f, 0.99551782f, 0.99916056f, 0.99989920f, 0.99999316f},
  {0.51449576f, 0.47173197f, 0.31337745f, 0.18191320f, 0.09457419f, 0.04096558f, 0.01419856f, 0.00369997f}
  };
//}}}

//{{{
static const float g_twid9 [18] = {
  0.73727734f, 0.79335334f, 0.84339145f, 0.88701083f, 0.92387953f, 0.95371695f,
  0.97629601f, 0.99144486f, 0.99904822f, 0.67559021f, 0.60876143f, 0.53729961f,
  0.46174861f, 0.38268343f, 0.30070580f, 0.21643961f, 0.13052619f, 0.04361938f
  };
//}}}
//{{{
static const float g_twid3 [6] = { 0.79335334f, 0.92387953f, 0.99144486f, 0.60876143f, 0.38268343f, 0.13052619f };
//}}}

//{{{
static const float g_mdct_window [2][18] = {
  { 0.99904822f, 0.99144486f, 0.97629601f, 0.95371695f, 0.92387953f, 0.88701083f,
    0.84339145f, 0.79335334f, 0.73727734f, 0.04361938f, 0.13052619f, 0.21643961f,
    0.30070580f, 0.38268343f, 0.46174861f, 0.53729961f, 0.60876143f, 0.67559021f },

  { 1,1,1, 1,1,1,
    0.99144486f, 0.92387953f, 0.79335334f, 0,0,0,
    0,0,0, 0.13052619f, 0.38268343f, 0.60876143f }

  };
//}}}
//{{{
static const float g_sec [24] = {
  10.19000816f, 0.50060302f, 0.50241929f, 3.40760851f, 0.50547093f, 0.52249861f,
   2.05778098f, 0.51544732f, 0.56694406f, 1.48416460f, 0.53104258f, 0.64682180f,
   1.16943991f, 0.55310392f, 0.78815460f, 0.97256821f, 0.58293498f, 1.06067765f,
   0.83934963f, 0.62250412f, 1.72244716f, 0.74453628f, 0.67480832f, 5.10114861f };
//}}}
//{{{
static const float g_win[] = {
  -1,26, -31,208,218,401,-519,   2063,2000,4788, -5517,7134,  5959,35640, -39336,74992,
  -1,24, -35,202,222,347,-581,   2080,1952,4425, -5879,7640,  5288,33791, -41176,74856,
  -1,21, -38,196,225,294,-645,   2087,1893,4063, -6237,8092,  4561,31947, -43006,74630,
  -1,19, -41,190,227,244,-711,   2085,1822,3705, -6589,8492,  3776,30112, -44821,74313,
  -1,17, -45,183,228,197,-779,   2075,1739,3351, -6935,8840,  2935,28289, -46617,73908,
  -1,16, -49,176,228,153,-848,   2057,1644,3004, -7271,9139,  2037,26482, -48390,73415,
  -2,14, -53,169,227,111,-919,   2032,1535,2663, -7597,9389,  1082,24694, -50137,72835,
  -2,13, -58,161,224,72,-991,    2001,1414,2330, -7910,9592,    70,22929, -51853,72169,
  -2,11, -63,154,221,36,-1064,   1962,1280,2006, -8209,9750,  -998,21189, -53534,71420,
  -2,10, -68,147,215,2,-1137,    1919,1131,1692, -8491,9863, -2122,19478, -55178,70590,
  -3,9,  -73,139,208,-29,-1210,  1870,970,1388,  -8755,9935, -3300,17799, -56778,69679,
  -3,8,  -79,132,200,-57,-1283,  1817,794,1095,  -8998,9966, -4533,16155, -58333,68692,
  -4,7,  -85,125,189,-83,-1356,  1759,605,814,   -9219,9959, -5818,14548, -59838,67629,
  -4,7,  -91,117,177,-106,-1428, 1698,402,545,   -9416,9916, -7154,12980, -61289,66494,
  -5,6,  -97,111,163,-127,-1498, 1634,185,288,   -9585,9838, -8540,11455, -62684,65290
  };
//}}}
//}}}

// header
//{{{
uint32_t headerBitrate (const uint8_t* header) {
  return 2 * g_halfrate [!!HDR_TEST_MPEG1 (header)] [HDR_GET_LAYER (header) - 1] [HDR_GET_BITRATE (header)];
  }
//}}}
//{{{
uint32_t headerSampleRate (const uint8_t* header) {
  return g_hz [HDR_GET_SAMPLE_RATE (header)] >> (int)!HDR_TEST_MPEG1 (header) >> (int)!HDR_TEST_NOT_MPEG25 (header);
  }
//}}}
//{{{
uint32_t headerFrameSamples (const uint8_t* header) {
  return HDR_IS_LAYER_1 (header) ? 384 : (1152 >> (int)HDR_IS_FRAME_576 (header));
  }
//}}}
//{{{
int32_t headerFrameBytes (const uint8_t* header, int32_t free_format_size) {

  int32_t frame_bytes = headerFrameSamples (header) * headerBitrate (header) * 125 / headerSampleRate (header);
  if (HDR_IS_LAYER_1 (header))
    frame_bytes &= ~3; // slot align

  return frame_bytes ? frame_bytes : free_format_size;
  }
//}}}
//{{{
int32_t headerPadding (const uint8_t* header) {
  return HDR_TEST_PADDING (header) ? (HDR_IS_LAYER_1 (header) ? 4 : 1) : 0;
  }
//}}}

//{{{  layer utils
// layer 12
//{{{
const struct sSubBandAlloc* subBandAllocTable (const uint8_t* header, struct sScaleInfo* scaleInfo) {

  int32_t mode = HDR_GET_STEREO_MODE (header);
  int32_t stereoBands = (mode == MODE_MONO) ?
                           0 : (mode == MODE_JOINT_STEREO) ?
                             (HDR_GET_STEREO_MODE_EXT(header) << 2) + 4 : 32;

  int32_t numBands;
  const struct sSubBandAlloc* alloc;
  if (HDR_IS_LAYER_1 (header)) {
    numBands = 32;
    alloc = g_alloc_L1;
    }

  else if (!HDR_TEST_MPEG1 (header)) {
    numBands = 30;
    alloc = g_alloc_L2M2;
    }

  else {
    int32_t sample_rate_idx = HDR_GET_SAMPLE_RATE (header);
    uint32_t kbps = headerBitrate (header) >> (int)(mode != MODE_MONO);
    if (!kbps) /* free-format */
      kbps = 192;

    numBands = 27;
    alloc = g_alloc_L2M1;
    if (kbps < 56) {
      alloc = g_alloc_L2M1_lowrate;
      numBands = (sample_rate_idx == 2) ? 12 : 8;
      }
    else if ((kbps >= 96) && (sample_rate_idx != 1))
      numBands = 30;
    }

  scaleInfo->totalBands = (uint8_t)numBands;
  scaleInfo->stereoBands = (uint8_t)std::min (stereoBands, numBands);

  return alloc;
  }
//}}}
//{{{
void readScaleFactors (cMp3Decoder::cBitStream* bitStream, uint8_t* pba, uint8_t* scfcod, int32_t bands, float* scf) {

  for (int32_t i = 0; i < bands; i++) {
    float s = 0;
    int32_t ba = *pba++;
    int32_t mask = ba ? 4 + ((19 >> scfcod[i]) & 3) : 0;
    for (int32_t m = 4; m; m >>= 1) {
      if (mask & m) {
        int32_t b = bitStream->getBits (6);
        s = g_deq_L12[ba*3 - 6 + b % 3] * (1 << 21 >> b/3);
        }
      *scf++ = s;
      }
    }
  }
//}}}
//{{{
void readScaleInfo (const uint8_t* header, cMp3Decoder::cBitStream* bitStream, struct sScaleInfo* scaleInfo) {

  const struct sSubBandAlloc* subBandAlloc = subBandAllocTable (header, scaleInfo);

  int32_t k = 0;
  int32_t bitAllocBits = 0;
  const uint8_t* bitAllocCodeTable = g_bitalloc_code_tab;
  for (int32_t i = 0; i < scaleInfo->totalBands; i++) {
    if (i == k) {
      k += subBandAlloc->bandCount;
      bitAllocBits = subBandAlloc->codeTableWidth;
      bitAllocCodeTable = g_bitalloc_code_tab + subBandAlloc->tabOffset;
      subBandAlloc++;
      }

    uint8_t bitAlloc = bitAllocCodeTable [bitStream->getBits (bitAllocBits)];
    scaleInfo->bitalloc[2*i] = bitAlloc;
    if (i < scaleInfo->stereoBands)
      bitAlloc = bitAllocCodeTable [bitStream->getBits (bitAllocBits)];
    scaleInfo->bitalloc [2*i + 1] = scaleInfo->stereoBands ? bitAlloc : 0;
    }

  for (int32_t i = 0; i < 2 * scaleInfo->totalBands; i++)
    scaleInfo->scfcod[i] = scaleInfo->bitalloc[i] ? HDR_IS_LAYER_1 (header) ? 2 : bitStream->getBits (2) : 6;

  readScaleFactors (bitStream, scaleInfo->bitalloc, scaleInfo->scfcod, scaleInfo->totalBands*2, scaleInfo->scf);

  for (int32_t i = scaleInfo->stereoBands; i < scaleInfo->totalBands; i++)
    scaleInfo->bitalloc[2*i + 1] = 0;
  }
//}}}

//{{{
int32_t dequantizeGranule (float* granuleBuffer, cMp3Decoder::cBitStream* bitStream, struct sScaleInfo* scaleInfo, int32_t group_size) {

  int32_t choff = 576;
  for (int32_t j = 0; j < 4; j++) {
    float* dst = granuleBuffer + group_size*j;
    for (int32_t i = 0; i < 2 * scaleInfo->totalBands; i++) {
      int32_t ba = scaleInfo->bitalloc[i];
      if (ba != 0) {
        if (ba < 17) {
          int32_t half = (1 << (ba - 1)) - 1;
          for (int32_t k = 0; k < group_size; k++)
            dst[k] = (float)((int)bitStream->getBits (ba) - half);
          }
        else {
          uint32_t mod = (2 << (ba - 17)) + 1;    // 3, 5, 9
          uint32_t code = bitStream->getBits (mod + 2 - (mod >> 3));  // 5, 7, 10
          for (int32_t k = 0; k < group_size; k++, code /= mod)
            dst[k] = (float)((int)(code % mod - mod/2));
          }
        }

      dst += choff;
      choff = 18 - choff;
      }
    }

  return group_size * 4;
  }
//}}}
//{{{
void applyScf384 (sScaleInfo* scaleInfo, const float* scf, float* dst) {

  memcpy (dst + 576 + scaleInfo->stereoBands * 18, dst + scaleInfo->stereoBands * 18,
          (scaleInfo->totalBands - scaleInfo->stereoBands)*18*sizeof(float));

  for (int32_t i = 0; i < scaleInfo->totalBands; i++, dst += 18, scf += 6) {
    for (int32_t k = 0; k < 12; k++) {
      dst[k] *= scf[0];
      dst[k+576] *= scf[3];
      }
    }
  }
//}}}

// layer 3
//{{{
void dct39 (float* y) {

  float s0 = y[0];
  float s2 = y[2];
  float s4 = y[4];
  float s6 = y[6];
  float s8 = y[8];
  float t0 = s0 + s6*0.5f;
  s0 -= s6;
  float t4 = (s4 + s2) * 0.93969262f;
  float t2 = (s8 + s2) * 0.76604444f;
  s6 = (s4 - s8) * 0.17364818f;
  s4 += s8 - s2;

  s2 = s0 - s4 * 0.5f;
  y[4] = s4 + s0;
  s8 = t0 - t2 + s6;
  s0 = t0 - t4 + t2;
  s4 = t0 + t4 - s6;

  float s1 = y[1];
  float s3 = y[3];
  float s5 = y[5];
  float s7 = y[7];

  s3 *= 0.86602540f;
  t0 = (s5 + s1) * 0.98480775f;
  t4 = (s5 - s7) * 0.34202014f;
  t2 = (s1 + s7) * 0.64278761f;
  s1 = (s1 - s5 - s7) * 0.86602540f;

  s5 = t0 - s3 - t2;
  s7 = t4 - s3 - t0;
  s3 = t4 + s3 - t2;

  y[0] = s4 - s7;
  y[1] = s2 + s1;
  y[2] = s0 - s3;
  y[3] = s8 + s5;
  y[5] = s8 - s5;
  y[6] = s0 + s3;
  y[7] = s2 - s1;
  y[8] = s4 + s7;
  }
//}}}
//{{{
void reorder (float* granuleBuffer, float* scratch, const uint8_t* sfb) {

  int32_t len;
  float* src = granuleBuffer;
  float* dst = scratch;
  for (; 0 != (len = *sfb); sfb += 3, src += 2 * len) {
    for (int32_t i = 0; i < len; i++, src++) {
      *dst++ = src[0*len];
      *dst++ = src[1*len];
      *dst++ = src[2*len];
      }
    }

  memcpy (granuleBuffer, scratch, (dst - scratch) * sizeof(float));
  }
//}}}

#ifdef USE_INTRINSICS
  //{{{
  void midsideStereo (float* left, int32_t n) {

    float* right = left + 576;

    int32_t i = 0;
    for (; i < n - 3; i += 4) {
      f4 vl = VLD (left + i);
      f4 vr = VLD (right + i);
      VSTORE (left + i, VADD (vl, vr));
      VSTORE (right + i, VSUB (vl, vr));
      }

    for (; i < n; i++) {
      float a = left[i];
      float b = right[i];
      left[i] = a + b;
      right[i] = a - b;
      }
    }
  //}}}
  //{{{
  void antialias (float* granuleBuffer, int32_t numBands) {

    for (; numBands > 0; numBands--, granuleBuffer += 18) {
      for (int32_t i = 0; i < 8; i += 4) {
        f4 vu = VLD (granuleBuffer + 18 + i);
        f4 vd = VLD (granuleBuffer + 14 - i);
        f4 vc0 = VLD (g_aa[0] + i);
        f4 vc1 = VLD (g_aa[1] + i);
        vd = VREV (vd);
        VSTORE (granuleBuffer + 18 + i, VSUB (VMUL (vu, vc0), VMUL (vd, vc1)));
        vd = VADD (VMUL (vu, vc1), VMUL (vd, vc0));
        VSTORE (granuleBuffer + 14 - i, VREV (vd));
        }
      }
    }
  //}}}
  //{{{
  void imdct36 (float* granuleBuffer, float* overlap, const float* window, int32_t numBands) {

    for (int32_t j = 0; j < numBands; j++, granuleBuffer += 18, overlap += 9) {
      float co[9];
      float si[9];
      co[0] = -granuleBuffer[0];
      si[0] = granuleBuffer[17];
      for (int32_t i = 0; i < 4; i++) {
        si[8 - 2*i] =   granuleBuffer[4*i + 1] - granuleBuffer[4*i + 2];
        co[1 + 2*i] =   granuleBuffer[4*i + 1] + granuleBuffer[4*i + 2];
        si[7 - 2*i] =   granuleBuffer[4*i + 4] - granuleBuffer[4*i + 3];
        co[2 + 2*i] = -(granuleBuffer[4*i + 3] + granuleBuffer[4*i + 4]);
        }
      dct39 (co);
      dct39 (si);

      si[1] = -si[1];
      si[3] = -si[3];
      si[5] = -si[5];
      si[7] = -si[7];

      int32_t i = 0;
      for (; i < 8; i += 4) {
        f4 vovl = VLD (overlap + i);
        f4 vc = VLD (co + i);
        f4 vs = VLD (si + i);
        f4 vr0 = VLD (g_twid9 + i);
        f4 vr1 = VLD (g_twid9 + 9 + i);
        f4 vw0 = VLD (window + i);
        f4 vw1 = VLD (window + 9 + i);
        f4 vsum = VADD(VMUL (vc, vr1), VMUL(vs, vr0));
        VSTORE (overlap + i, VSUB (VMUL(vc, vr0), VMUL(vs, vr1)));
        VSTORE (granuleBuffer + i, VSUB (VMUL (vovl, vw0), VMUL(vsum, vw1)));
        vsum = VADD ( VMUL (vovl, vw1), VMUL (vsum, vw0));
        VSTORE (granuleBuffer + 14 - i, VREV (vsum));
        }

      for (; i < 9; i++) {
        float ovl = overlap[i];
        float sum = co[i] * g_twid9[9 + i] + si[i] * g_twid9[i];
        overlap[i] = co[i] * g_twid9[i] - si[i] * g_twid9[9 + i];
        granuleBuffer[i] = ovl*window[i] - sum*window[9 + i];
        granuleBuffer[17 - i] = ovl*window[9 + i] + sum*window[i];
        }
      }
    }
  //}}}
#else
  //{{{
  void midsideStereo (float* left, int32_t n) {

    float* right = left + 576;
    for (int32_t i = 0; i < n; i++) {
      float a = left[i];
      float b = right[i];
      left[i] = a + b;
      right[i] = a - b;
      }
    }
  //}}}
  //{{{
  void antialias (float* granuleBuffer, int32_t numBands) {

    for (; numBands > 0; numBands--, granuleBuffer += 18) {
      for (int32_t i = 0; i < 8; i++) {
        float u = granuleBuffer [18+i];
        float d = granuleBuffer [17-i];
        granuleBuffer [18+i] = u*g_aa [0][i] - d*g_aa [1][i];
        granuleBuffer [17-i] = u*g_aa [1][i] + d*g_aa [0][i];
        }
      }
    }
  //}}}
  //{{{
  void imdct36 (float* granuleBuffer, float* overlap, const float* window, int32_t numBands) {

    for (int32_t j = 0; j < numBands; j++, granuleBuffer += 18, overlap += 9) {
      float co[9], si[9];
      co[0] = -granuleBuffer[0];
      si[0] = granuleBuffer[17];
      for (int32_t i = 0; i < 4; i++) {
        si[8 - 2*i] = granuleBuffer[4*i + 1] - granuleBuffer[4*i + 2];
        co[1 + 2*i] = granuleBuffer[4*i + 1] + granuleBuffer[4*i + 2];
        si[7 - 2*i] = granuleBuffer[4*i + 4] - granuleBuffer[4*i + 3];
        co[2 + 2*i] = -(granuleBuffer[4*i + 3] + granuleBuffer[4*i + 4]);
        }
      dct3_9 (co);
      dct3_9 (si);

      si[1] = -si[1];
      si[3] = -si[3];
      si[5] = -si[5];
      si[7] = -si[7];

      for (int32_t i = 0; i < 9; i++) {
        float ovl = overlap[i];
        float sum = co[i] * g_twid9[9+i] + si[i] * g_twid9[i];
        overlap[i] = co[i] * g_twid9[i] - si[i] * g_twid9[9+i];
        granuleBuffer[i] = ovl * window[i] - sum * window[9+i];
        granuleBuffer[17-i] = ovl * window[9+i] + sum * window[i];
        }
      }
    }
  //}}}
#endif

//{{{
int32_t readSideInfo (cMp3Decoder::cBitStream* bitStream, struct cMp3Decoder::sGranule* granule, const uint8_t* header) {
// return offset of main data

  int32_t sr_idx = HDR_GET_MY_SAMPLE_RATE (header);
  sr_idx -= (sr_idx != 0);

  uint32_t scfsi = 0;
  int32_t granule_count = HDR_IS_MONO (header) ? 1 : 2;
  int32_t reservoirBytesNeeded;
  if (HDR_TEST_MPEG1 (header)) {
    granule_count *= 2;
    reservoirBytesNeeded = bitStream->getBits (9);
    scfsi = bitStream->getBits (7 + granule_count);
    }
  else
    reservoirBytesNeeded = bitStream->getBits (8 + granule_count) >> granule_count;

  int32_t part_23_sum = 0;
  do {
    if (HDR_IS_MONO (header))
      scfsi <<= 4;

    granule->part23length = (uint16_t)(bitStream->getBits (12));
    part_23_sum += granule->part23length;
    granule->bigValues = (uint16_t)(bitStream->getBits (9));
    if (granule->bigValues > 288)
      return -1;

    granule->globalGain = (uint8_t)(bitStream->getBits (8));
    granule->scalefac_compress = (uint16_t)(bitStream->getBits (HDR_TEST_MPEG1 (header) ? 4 : 9));
    granule->sfbtab = g_scf_long [sr_idx];
    granule->numLongSfb  = 22;
    granule->numShortSfb = 0;

    uint32_t tables;
    if (bitStream->getBits (1)) {
      granule->blockType = (uint8_t)(bitStream->getBits (2));
      if (!granule->blockType)
        return -1;
      granule->mixedBlockFlag = (uint8_t)(bitStream->getBits (1));
      granule->regionCount[0] = 7;
      granule->regionCount[1] = 255;

      if (granule->blockType == SHORT_blockType) {
        scfsi &= 0x0F0F;
        if (!granule->mixedBlockFlag) {
          granule->regionCount[0] = 8;
          granule->sfbtab = g_scf_short[sr_idx];
          granule->numLongSfb = 0;
          granule->numShortSfb = 39;
          }
        else {
          granule->sfbtab = g_scf_mixed[sr_idx];
          granule->numLongSfb = HDR_TEST_MPEG1(header) ? 8 : 6;
          granule->numShortSfb = 30;
          }
        }

      tables = bitStream->getBits (10);
      tables <<= 5;
      granule->subBlockGain[0] = (uint8_t)bitStream->getBits (3);
      granule->subBlockGain[1] = (uint8_t)bitStream->getBits (3);
      granule->subBlockGain[2] = (uint8_t)bitStream->getBits (3);
      }

    else {
      granule->blockType = 0;
      granule->mixedBlockFlag = 0;
      tables = bitStream->getBits (15);
      granule->regionCount[0] = (uint8_t)bitStream->getBits (4);
      granule->regionCount[1] = (uint8_t)bitStream->getBits (3);
      granule->regionCount[2] = 255;
      }

    granule->tableSelect[0] = (uint8_t)(tables >> 10);
    granule->tableSelect[1] = (uint8_t)((tables >> 5) & 31);
    granule->tableSelect[2] = (uint8_t)((tables) & 31);
    granule->preflag = HDR_TEST_MPEG1 (header) ? bitStream->getBits (1) : (granule->scalefac_compress >= 500);
    granule->scaleFactorScale = (uint8_t)(bitStream->getBits (1));
    granule->count1table = (uint8_t)(bitStream->getBits (1));
    granule->scfsi = (uint8_t)((scfsi >> 12) & 15);
    scfsi <<= 4;
    granule++;
    } while (--granule_count);

  if ((part_23_sum + bitStream->getBitStreamPosition()) > (bitStream->getBitStreamLimit() + reservoirBytesNeeded * 8))
    return -1;

  return reservoirBytesNeeded;
  }
//}}}

//{{{
void readScaleFactorsL3 (uint8_t* scf, uint8_t* istPos, const uint8_t* scf_size, const uint8_t* scf_count,
                         cMp3Decoder::cBitStream* bitbuf, int32_t scfsi) {

  for (int32_t i = 0; i < 4 && scf_count[i]; i++, scfsi *= 2) {
    int32_t cnt = scf_count[i];
    if (scfsi & 8)
      memcpy (scf, istPos, cnt);
    else {
      int32_t bits = scf_size[i];
      if (!bits) {
        memset (scf, 0, cnt);
        memset (istPos, 0, cnt);
        }
      else {
        int32_t max_scf = (scfsi < 0) ? (1 << bits) - 1 : -1;
        for (int32_t k = 0; k < cnt; k++) {
          int32_t s = bitbuf->getBits (bits);
          istPos[k] = (s == max_scf ? -1 : s);
          scf[k] = s;
          }
        }
      }
    istPos += cnt;
    scf += cnt;
    }

  scf[0] = scf[1] = scf[2] = 0;
  }
//}}}
//{{{
float ldExpQ2 (float y, int32_t exp_q2) {

  int32_t e;
  do {
    e = std::min (30*4, exp_q2);
    y *= g_expfrac[e & 3]*(1 << 30 >> (e >> 2));
    } while ((exp_q2 -= e) > 0);

  return y;
  }
//}}}
//{{{
void decodeScaleFactorsL3 (const uint8_t* header, uint8_t* istPos, cMp3Decoder::cBitStream* bitStream,
                           const struct cMp3Decoder::sGranule* gr, float* scf, int32_t ch) {

  const uint8_t* scf_partition = g_scf_partitions [!!gr->numShortSfb + !gr->numLongSfb];
  uint8_t scf_size[4];
  int32_t scf_shift = gr->scaleFactorScale + 1;
  int32_t scfsi = gr->scfsi;
  if (HDR_TEST_MPEG1 (header)) {
    int32_t part = g_scfc_decode[gr->scalefac_compress];
    scf_size[1] = scf_size[0] = (uint8_t)(part >> 2);
    scf_size[3] = scf_size[2] = (uint8_t)(part & 3);
    }
  else {
    int32_t k, modprod, sfc, ist = HDR_TEST_I_STEREO(header) && ch;
    sfc = gr->scalefac_compress >> ist;
    for (k = ist*3*4; sfc >= 0; sfc -= modprod, k += 4) {
      int32_t i;
      for (modprod = 1, i = 3; i >= 0; i--) {
        scf_size[i] = (uint8_t)(sfc / modprod % g_mod[k + i]);
        modprod *= g_mod[k + i];
        }
      }
    scf_partition += k;
    scfsi = -16;
    }

  uint8_t iscf[40];
  readScaleFactorsL3 (iscf, istPos, scf_size, scf_partition, bitStream, scfsi);

  if (gr->numShortSfb) {
    int32_t sh = 3 - scf_shift;
    for (int i = 0; i < gr->numShortSfb; i += 3) {
      iscf [gr->numLongSfb + i] += gr->subBlockGain[0] << sh;
      iscf [gr->numLongSfb + i + 1] += gr->subBlockGain[1] << sh;
      iscf [gr->numLongSfb + i + 2] += gr->subBlockGain[2] << sh;
      }
    }
  else if (gr->preflag) {
    for (int32_t i = 0; i < 10; i++)
      iscf[11 + i] += g_preamp[i];
    }

  int32_t gain_exp = gr->globalGain + BITS_DEQUANTIZER_OUT*4 - 210 - (HDR_IS_MS_STEREO (header) ? 2 : 0);
  float gain = ldExpQ2 (1 << (MAX_SCFI/4),  MAX_SCFI - gain_exp);
  for (int32_t i = 0; i < (int)(gr->numLongSfb + gr->numShortSfb); i++)
    scf[i] = ldExpQ2 (gain, iscf[i] << scf_shift);
  }
//}}}

//{{{
float pow43 (int32_t x) {

  if (x < 129)
    return g_pow43[16 + x];

  int32_t mult = 256;
  if (x < 1024) {
    mult = 16;
    x <<= 3;
    }

  int32_t sign = 2*x & 64;
  float frac = (float)((x & 63) - sign) / ((x & ~63) + sign);

  return g_pow43[16 + ((x + sign) >> 6)] * (1.f + frac * ((4.f/3) + frac * (2.f/9))) * mult;
  }
//}}}
//{{{
void huffman (float* dst, cMp3Decoder::cBitStream* bitStream, const struct cMp3Decoder::sGranule* granule,
              const float* scf, int32_t l3granuleLimit) {

  float one = 0.0f;
  int32_t ireg = 0;
  int32_t bigValueCount = granule->bigValues;
  const uint8_t* sfb = granule->sfbtab;

  bitStream->fillCache();

  int32_t np;
  int32_t pairsToDecode;
  while (bigValueCount > 0) {
    int32_t tableNum = granule->tableSelect[ireg];
    int32_t sfb_cnt = granule->regionCount[ireg++];
    const int16_t* codebook = g_tabs + g_tabindex[tableNum];
    int32_t linbits = g_linbits[tableNum];
    if (linbits) {
      do {
        np = *sfb++ / 2;
        pairsToDecode = std::min (bigValueCount, np);
        one = *scf++;
        do {
          int32_t w = 5;
          int32_t leaf = codebook[bitStream->peekBits (w)];
          while (leaf < 0) {
            bitStream->flushBits (w);
            w = leaf & 7;
            leaf = codebook[bitStream->peekBits (w) - (leaf >> 3)];
            }
          bitStream->flushBits (leaf >> 8);

          for (int32_t j = 0; j < 2; j++, dst++, leaf >>= 4) {
            int32_t lsb = leaf & 0x0F;
            if (lsb == 15) {
              lsb += bitStream->peekBits (linbits);
              bitStream->flushBits (linbits);
              bitStream->checkBits();
              *dst = one * pow43 (lsb) * ((int32_t)(bitStream->getBitStreamCache()) < 0 ? -1: 1);
              }
            else
              *dst = g_pow43[16 + lsb - 16*(bitStream->getBitStreamCache() >> 31)]*one;
            bitStream->flushBits (lsb ? 1 : 0);
            }
         bitStream->checkBits();
         } while (--pairsToDecode);
        } while ((bigValueCount -= np) > 0 && --sfb_cnt >= 0);
      }
    else {
      do {
        np = *sfb++ / 2;
        pairsToDecode = std::min (bigValueCount, np);
        one = *scf++;
        do {
          int32_t j, w = 5;
          int32_t leaf = codebook[bitStream->peekBits(w)];
          while (leaf < 0) {
            bitStream->flushBits (w);
            w = leaf & 7;
            leaf = codebook[bitStream->peekBits (w) - (leaf >> 3)];
            }
          bitStream->flushBits(leaf >> 8);

          for (j = 0; j < 2; j++, dst++, leaf >>= 4) {
            int32_t lsb = leaf & 0x0F;
            *dst = g_pow43[16 + lsb - 16*(bitStream->getBitStreamCache() >> 31)]*one;
            bitStream->flushBits(lsb ? 1 : 0);
            }
          bitStream->checkBits();
          } while (--pairsToDecode);
        } while ((bigValueCount -= np) > 0 && --sfb_cnt >= 0);
      }
    }

  for (np = 1 - bigValueCount;; dst += 4) {
    const uint8_t* codebook_count1 = (granule->count1table) ? g_tab33 : g_tab32;
    int32_t leaf = codebook_count1[bitStream->peekBits(4)];
    if (!(leaf & 8))
      leaf = codebook_count1[(leaf >> 3) + (bitStream->getBitStreamCache() << 4 >> (32 - (leaf & 3)))];
    bitStream->flushBits (leaf & 7);
    if (bitStream->getBitPosition() > l3granuleLimit)
      break;

    //{{{
    #define RELOAD_SCALEFACTOR  { \
      if (!--np) { \
        np = *sfb++/2; \
        if (!np) \
          break; \
        one = *scf++; \
        } \
      }
    //}}}
    //{{{
    #define DEQ_COUNT1(s) { \
      if (leaf & (128 >> s)) { \
        dst[s] = ((int32_t)(bitStream->getBitStreamCache()) < 0) ? -one : one;   \
        bitStream->flushBits (1); \
        } \
      }
    //}}}
    RELOAD_SCALEFACTOR;
    DEQ_COUNT1 (0);
    DEQ_COUNT1 (1);

    RELOAD_SCALEFACTOR;
    DEQ_COUNT1 (2);
    DEQ_COUNT1 (3);
    bitStream->checkBits();
    }

  bitStream->setBitStreamPos (l3granuleLimit);
  }
//}}}

//{{{
void intensityStereoBand (float* left, int32_t n, float kl, float kr) {

  for (int32_t i = 0; i < n; i++) {
    left[i + 576] = left[i] * kr;
    left[i] = left[i] * kl;
    }
  }
//}}}
//{{{
void stereoTopBand (const float* right, const uint8_t* sfb, int32_t numBands, int32_t max_band[3]) {

  max_band[0] = max_band[1] = max_band[2] = -1;

  for (int32_t i = 0; i < numBands; i++) {
    for (int32_t k = 0; k < sfb[i]; k += 2) {
      if (right[k] != 0 || right[k + 1] != 0) {
        max_band[i % 3] = i;
        break;
        }
      }
    right += sfb[i];
    }
  }
//}}}
//{{{
void stereoProcess (float* left, const uint8_t* istPos, const uint8_t* sfb, const uint8_t* header,
                    int32_t max_band[3], int32_t mpeg2_sh) {

  uint32_t max_pos = HDR_TEST_MPEG1 (header) ? 7 : 64;

  for (uint32_t i = 0; sfb[i]; i++) {
    uint32_t ipos = istPos[i];
    if ((int)i > max_band[i % 3] && ipos < max_pos) {
      float kl, kr, s = HDR_TEST_MS_STEREO (header) ? 1.41421356f : 1;
      if (HDR_TEST_MPEG1 (header)) {
        kl = g_pan[2*ipos];
        kr = g_pan[2*ipos + 1];
        }
      else {
        kl = 1;
        kr = ldExpQ2 (1, (ipos + 1) >> 1 << mpeg2_sh);
        if (ipos & 1) {
          kl = kr;
          kr = 1;
          }
        }
      intensityStereoBand (left, sfb[i], kl * s, kr * s);
      }
    else if (HDR_TEST_MS_STEREO (header))
      midsideStereo (left, sfb[i]);

    left += sfb[i];
    }
  }
//}}}
//{{{
void intensityStereo (float* left, uint8_t* istPos, const struct cMp3Decoder::sGranule* granule, const uint8_t* header) {

  int32_t maxBand[3];
  int32_t numSfb = granule->numLongSfb + granule->numShortSfb;
  stereoTopBand (left + 576, granule->sfbtab, numSfb, maxBand);
  if (granule->numLongSfb) {
    maxBand[0] = maxBand[1] = maxBand[2] = std::max (std::max (maxBand[0], maxBand[1]), maxBand[2]);
    }

  int32_t maxBlocks = granule->numShortSfb ? 3 : 1;
  for (int32_t i = 0; i < maxBlocks; i++) {
    int32_t defaultPos = HDR_TEST_MPEG1 (header) ? 3 : 0;
    int32_t itop = numSfb - maxBlocks + i;
    int32_t prev = itop - maxBlocks;
    istPos[itop] = maxBand[i] >= prev ? defaultPos : istPos[prev];
    }

  stereoProcess (left, istPos, granule->sfbtab, header, maxBand, granule[1].scalefac_compress & 1);
  }
//}}}

//{{{
void idct3 (float x0, float x1, float x2, float* dst) {

  float m1 = x1 * 0.86602540f;
  float a1 = x0 - x2 * 0.5f;

  dst[1] = x0 + x2;
  dst[0] = a1 + m1;
  dst[2] = a1 - m1;
  }
//}}}
//{{{
void imdct12 (float* x, float* dst, float* overlap) {

  float co [3];
  idct3 (-x[0], x[6] + x[3], x[12] + x[9], co);

  float si [3];
  idct3 (x[15], x[12] - x[9], x[6] - x[3], si);
  si[1] = -si[1];

  for (int32_t i = 0; i < 3; i++) {
    float ovl = overlap[i];
    float sum = co[i] * g_twid3[3+i] + si[i] * g_twid3[i];
    overlap[i] = co[i] * g_twid3[i] - si[i] * g_twid3[3+i];
    dst[i] = ovl * g_twid3[2-i] - sum * g_twid3[5-i];
    dst[5-i] = ovl * g_twid3[5-i] + sum * g_twid3[2-i];
    }
  }
//}}}
//{{{
void imdctShort (float* granuleBuffer, float* overlap, int numBands) {

  for (;numBands > 0; numBands--, overlap += 9, granuleBuffer += 18) {
    float tmp[18];
    memcpy (tmp, granuleBuffer, sizeof(tmp));
    memcpy (granuleBuffer, overlap, 6 * sizeof(float));

    imdct12 (tmp, granuleBuffer + 6, overlap + 6);
    imdct12 (tmp + 1, granuleBuffer + 12, overlap + 6);
    imdct12 (tmp + 2, overlap, overlap + 6);
    }
  }
//}}}
//{{{
void imdctGranule (float* granuleBuffer, float* overlap, uint32_t blockType, uint32_t numLongBands) {

  if (numLongBands) {
    imdct36 (granuleBuffer, overlap, g_mdct_window[0], numLongBands);
    granuleBuffer += 18 * numLongBands;
    overlap += 9 * numLongBands;
    }

  if (blockType == SHORT_blockType)
    imdctShort (granuleBuffer, overlap, 32 - numLongBands);
  else
    imdct36 (granuleBuffer, overlap, g_mdct_window[blockType == STOP_blockType], 32 - numLongBands);
  }
//}}}

//{{{
void changeSign (float* granuleBuffer) {

  granuleBuffer += 18;
  for (int32_t b = 0; b < 32; b += 2, granuleBuffer += 36)
    for (int32_t i = 1; i < 18; i += 2)
      granuleBuffer[i] = -granuleBuffer[i];
  }
//}}}
//}}}

//{{{
int16_t scalePcm (float sample) {

  if (sample >= 32766.5)
    return (int16_t) 32767;
  if (sample <= -32767.5)
    return (int16_t)-32768;

  // away from zero, to be compliant
  int16_t s = (int16_t)(sample + .5f);
  s -= (s < 0);

  return s;
  }
//}}}
//{{{
void synthPair (int16_t* pcm, int32_t numChannels, const float* z) {

  float a = (z[14*64] - z[0]) * 29;
  a += (z[1*64] + z[13*64]) * 213;
  a += (z[12*64] - z[2*64]) * 459;
  a += (z[3*64] + z[11*64]) * 2037;
  a += (z[10*64] - z[4*64]) * 5153;
  a += (z[5*64] + z[9*64]) * 6574;
  a += (z[8*64] - z[6*64]) * 37489;
  a +=  z[7*64] * 75038;
  pcm[0] = scalePcm (a);

  z += 2;
  a  = z[14*64] * 104;
  a += z[12*64] * 1567;
  a += z[10*64] * 9727;
  a += z[8*64] * 64019;
  a += z[6*64] * -9975;
  a += z[4*64] * -45;
  a += z[2*64] * 146;
  a += z[0*64] * -5;
  pcm[16 * numChannels] = scalePcm (a);
  }
//}}}
#ifdef USE_INTRINSICS
  //{{{
  void dctII (float* granuleBuffer, int32_t n) {

    for (int32_t k = 0; k < n; k += 4) {
      f4 t[4][8];

      f4* x = t[0];
      float* y = granuleBuffer + k;
      for (int32_t i = 0; i < 8; i++, x++) {
        f4 x0 = VLD (&y[i*18]);
        f4 x1 = VLD (&y[(15 - i)*18]);
        f4 x2 = VLD (&y[(16 + i)*18]);
        f4 x3 = VLD (&y[(31 - i)*18]);
        f4 t0 = VADD (x0, x3);
        f4 t1 = VADD (x1, x2);
        f4 t2 = VMUL_S (VSUB (x1, x2), g_sec[3*i + 0]);
        f4 t3 = VMUL_S (VSUB (x0, x3), g_sec[3*i + 1]);
        x[0] = VADD (t0, t1);
        x[8] = VMUL_S (VSUB( t0, t1), g_sec[3*i + 2]);
        x[16] = VADD (t3, t2);
        x[24] = VMUL_S (VSUB (t3, t2), g_sec[3*i + 2]);
        }

      x = t[0];
      for (int32_t i = 0; i < 4; i++, x += 8) {
        f4 x0 = x[0];
        f4 x1 = x[1];
        f4 x2 = x[2];
        f4 x3 = x[3];
        f4 x4 = x[4];
        f4 x5 = x[5];
        f4 x6 = x[6];
        f4 x7 = x[7];
        f4 xt = VSUB (x0, x7);
        x0 = VADD(x0, x7);
        x7 = VSUB (x1, x6);
        x1 = VADD (x1, x6);
        x6 = VSUB (x2, x5);
        x2 = VADD (x2, x5);
        x5 = VSUB (x3, x4);
        x3 = VADD (x3, x4);
        x4 = VSUB (x0, x3);
        x0 = VADD (x0, x3);
        x3 = VSUB (x1, x2);
        x1 = VADD (x1, x2);
        x[0] = VADD (x0, x1);
        x[4] = VMUL_S (VSUB (x0, x1), 0.70710677f);
        x5 = VADD (x5, x6);
        x6 = VMUL_S (VADD (x6, x7), 0.70710677f);
        x7 = VADD (x7, xt);
        x3 = VMUL_S (VADD (x3, x4), 0.70710677f);
        x5 = VSUB (x5, VMUL_S (x7, 0.198912367f)); /* rotate by PI/8 */
        x7 = VADD (x7, VMUL_S (x5, 0.382683432f));
        x5 = VSUB (x5, VMUL_S (x7, 0.198912367f));
        x0 = VSUB (xt, x6);
        xt = VADD (xt, x6);
        x[1] = VMUL_S (VADD(xt, x7), 0.50979561f);
        x[2] = VMUL_S (VADD(x4, x3), 0.54119611f);
        x[3] = VMUL_S (VSUB(x0, x5), 0.60134488f);
        x[5] = VMUL_S (VADD(x0, x5), 0.89997619f);
        x[6] = VMUL_S (VSUB(x4, x3), 1.30656302f);
        x[7] = VMUL_S (VSUB(xt, x7), 2.56291556f);
        }

      #define VSAVE2(i, v) _mm_storel_pi ((__m64*)(void*)&y[i*18], v)
      #define VSAVE4(i, v) VSTORE (&y[i*18], v)

      if (k > n - 3) {
        for (int32_t i = 0; i < 7; i++, y += 4*18) {
          f4 s = VADD (t[3][i], t[3][i + 1]);
          VSAVE2 (0, t[0][i]);
          VSAVE2 (1, VADD (t[2][i], s));
          VSAVE2 (2, VADD (t[1][i], t[1][i + 1]));
          VSAVE2 (3, VADD (t[2][1 + i], s));
          }
        VSAVE2 (0, t[0][7]);
        VSAVE2 (1, VADD(t[2][7], t[3][7]));
        VSAVE2 (2, t[1][7]);
        VSAVE2 (3, t[3][7]);
        }
      else {
        for (int32_t i = 0; i < 7; i++, y += 4*18) {
          f4 s = VADD (t[3][i], t[3][i + 1]);
          VSAVE4 (0, t[0][i]);
          VSAVE4 (1, VADD (t[2][i], s));
          VSAVE4 (2, VADD (t[1][i], t[1][i + 1]));
          VSAVE4 (3, VADD (t[2][1 + i], s));
          }
        VSAVE4 (0, t[0][7]);
        VSAVE4 (1, VADD (t[2][7], t[3][7]));
        VSAVE4 (2, t[1][7]);
        VSAVE4 (3, t[3][7]);
        }
      }
    }
  //}}}
  //{{{
  void synth (float* xl, int16_t* dstl, int32_t nch, float* lins) {

    float* xr = xl + 576 * (nch - 1);
    int16_t* dstr = dstl + (nch - 1);

    float* zlin = lins + 15*64;
    const float* w = g_win;

    zlin [4*15] = xl[18*16];
    zlin [4*15 + 1] = xr[18*16];
    zlin [4*15 + 2] = xl[0];
    zlin [4*15 + 3] = xr[0];

    zlin [4*31] = xl[1 + 18*16];
    zlin [4*31 + 1] = xr[1 + 18*16];
    zlin [4*31 + 2] = xl[1];
    zlin [4*31 + 3] = xr[1];

    synthPair (dstr, nch, lins + 4*15 + 1);
    synthPair (dstr + 32*nch, nch, lins + 4*15 + 64 + 1);
    synthPair (dstl, nch, lins + 4*15);
    synthPair (dstl + 32*nch, nch, lins + 4*15 + 64);

    for (int32_t i = 14; i >= 0; i--) {
      zlin [4*i] = xl [18*(31 - i)];
      zlin [4*i + 1] = xr [18*(31 - i)];
      zlin [4*i + 2] = xl [1 + 18*(31 - i)];
      zlin [4*i + 3] = xr [1 + 18*(31 - i)];
      zlin [4*i + 64] = xl [1 + 18*(1 + i)];
      zlin [4*i + 64 + 1] = xr [1 + 18*(1 + i)];
      zlin [4*i - 64 + 2] = xl [18*(1 + i)];
      zlin [4*i - 64 + 3] = xr [18*(1 + i)];

      #define VLOAD(k) f4 w0 = VSET (*w++); \
                       f4 w1 = VSET (*w++); \
                       f4 vz = VLD (&zlin[4*i - 64*k]); \
                       f4 vy = VLD (&zlin[4*i - 64*(15 - k)]);
      #define V0(k) { VLOAD (k) \
                      b = VADD (VMUL (vz, w1), VMUL (vy, w0)); \
                      a = VSUB (VMUL (vz, w0), VMUL (vy, w1)); }
      #define V1(k) { VLOAD (k) \
                      b = VADD (b, VADD (VMUL (vz, w1), VMUL (vy, w0)));  \
                      a = VADD (a, VSUB (VMUL (vz, w0), VMUL (vy, w1))); }
      #define V2(k) { VLOAD (k) \
                      b = VADD (b, VADD (VMUL (vz, w1), VMUL (vy, w0))); \
                      a = VADD (a, VSUB (VMUL (vy, w1), VMUL (vz, w0))); }
      f4 a, b;
      V0(0) V2(1) V1(2) V2(3) V1(4) V2(5) V1(6) V2(7)
        {
        __m128i pcm8 = _mm_packs_epi32 (_mm_cvtps_epi32 (_mm_max_ps (_mm_min_ps (a, g_max), g_min)),
                                        _mm_cvtps_epi32 (_mm_max_ps (_mm_min_ps (b, g_max), g_min)));
        dstr [(15 - i) * nch] = _mm_extract_epi16 (pcm8, 1);
        dstr [(17 + i) * nch] = _mm_extract_epi16 (pcm8, 5);

        dstl [(15 - i) * nch] = _mm_extract_epi16 (pcm8, 0);
        dstl [(17 + i) * nch] = _mm_extract_epi16 (pcm8, 4);

        dstr [(47 - i) * nch] = _mm_extract_epi16 (pcm8, 3);
        dstr [(49 + i) * nch] = _mm_extract_epi16 (pcm8, 7);

        dstl [(47 - i) * nch] = _mm_extract_epi16 (pcm8, 2);
        dstl [(49 + i) * nch] = _mm_extract_epi16 (pcm8, 6);
        }
      }
    }
  //}}}
#else
  //{{{
  void dctII (float* granuleBuffer, int32_t n) {

    for (int32_t k = 0; k < n; k++) {
      float t[4][8];
      float* y = granuleBuffer + k;
      float* x = t[0];
      for (int32_t i = 0; i < 8; i++, x++) {
        float x0 = y[i*18];
        float x1 = y[(15-i)*18];
        float x2 = y[(16+i)*18];
        float x3 = y[(31-i)*18];
        float t0 = x0 + x3;
        float t1 = x1 + x2;
        float t2 = (x1 - x2)*g_sec[3*i + 0];
        float t3 = (x0 - x3)*g_sec[3*i + 1];
        x[0] = t0 + t1;
        x[8] = (t0 - t1)*g_sec[3*i + 2];
        x[16] = t3 + t2;
        x[24] = (t3 - t2)*g_sec[3*i + 2];
        }

      x = t[0];
      for (int32_t i = 0; i < 4; i++, x += 8) {
        float x0 = x[0];
        float x1 = x[1];
        float x2 = x[2];
        float x3 = x[3];
        float x4 = x[4];
        float x5 = x[5];
        float x6 = x[6];
        float x7 = x[7];
        float  xt = x0 - x7;
        x0 += x7;
        x7 = x1 - x6;
        x1 += x6;
        x6 = x2 - x5;
        x2 += x5;
        x5 = x3 - x4;
        x3 += x4;
        x4 = x0 - x3;
        x0 += x3;
        x3 = x1 - x2;
        x1 += x2;
        x[0] = x0 + x1;
        x[4] = (x0 - x1) * 0.70710677f;
        x5 =  x5 + x6;
        x6 = (x6 + x7) * 0.70710677f;
        x7 =  x7 + xt;
        x3 = (x3 + x4) * 0.70710677f;
        x5 -= x7 * 0.198912367f;  /* rotate by PI/8 */
        x7 += x5 * 0.382683432f;
        x5 -= x7 * 0.198912367f;
        x0 = xt - x6; xt += x6;
        x[1] = (xt + x7) * 0.50979561f;
        x[2] = (x4 + x3) * 0.54119611f;
        x[3] = (x0 - x5) * 0.60134488f;
        x[5] = (x0 + x5) * 0.89997619f;
        x[6] = (x4 - x3) * 1.30656302f;
        x[7] = (xt - x7) * 2.56291556f;
        }

      for (int32_t i = 0; i < 7; i++, y += 4*18) {
        y[0*18] = t[0][i];
        y[1*18] = t[2][i] + t[3][i] + t[3][i + 1];
        y[2*18] = t[1][i] + t[1][i + 1];
        y[3*18] = t[2][i + 1] + t[3][i] + t[3][i + 1];
        }

      y[0*18] = t[0][7];
      y[1*18] = t[2][7] + t[3][7];
      y[2*18] = t[1][7];
      y[3*18] = t[3][7];
      }
    }
  //}}}
  //{{{
  void synth (float* xl, int16_t* dstl, int32_t numChannels, float* lins) {

    float* xr = xl + 576 * (numChannels - 1);
    int16_t* dstr = dstl + (numChannels - 1);

    float* zlin = lins + 15 * 64;
    const float* w = g_win;

    zlin [4*15] = xl[18*16];
    zlin [4*15 + 1] = xr[18 * 16];
    zlin [4*15 + 2] = xl[0];
    zlin [4*15 + 3] = xr[0];

    zlin [4*31] = xl[1 + 18 * 16];
    zlin [4*31 + 1] = xr[1 + 18 * 16];
    zlin [4*31 + 2] = xl[1];
    zlin [4*31 + 3] = xr[1];

    synth_pair (dstr, numChannels, lins + 4*15 + 1);
    synth_pair (dstr + 32*numChannels, numChannels, lins + 4*15 + 64 + 1);
    synth_pair (dstl, numChannels, lins + 4*15);
    synth_pair (dstl + 32*nch, numChannels, lins + 4*15 + 64);

    for (int32_t i = 14; i >= 0; i--) {
      zlin[4*i] = xl[18 * (31 - i)];
      zlin[4*i + 1] = xr[18 * (31 - i)];
      zlin[4*i + 2] = xl[1 + 18 * (31 - i)];
      zlin[4*i + 3] = xr[1 + 18 * (31 - i)];
      zlin[4*(i+16)]   = xl[1 + 18 * (1 + i)];
      zlin[4*(i+16) + 1] = xr[1 + 18 * (1 + i)];
      zlin[4*(i-16) + 2] = xl[18 * (1 + i)];
      zlin[4*(i-16) + 3] = xr[18 * (1 + i)];

      float a[4], b[4];
      //{{{
      #define LOAD(k) \
        float w0 = *w++; \
        float w1 = *w++; \
        float* vz = &zlin [4*i - k*64]; \
        float* vy = &zlin [4*i - (15 - k)*64];
      //}}}
      //{{{
      #define S0(k) { \
        LOAD(k); \
        for (int32_t j = 0; j < 4; j++) {       \
          b[j]  = vz[j]*w1 + vy[j]*w0;  \
          a[j]  = vz[j]*w0 - vy[j]*w1;  \
          }                             \
        }
      //}}}
      //{{{
      #define S1(k) { \
        LOAD(k); \
        for (int32_t j = 0; j < 4; j++) {       \
          b[j] += vz[j]*w1 + vy[j]*w0;  \
          a[j] += vz[j]*w0 - vy[j]*w1;  \
          }                             \
        }
      //}}}
      //{{{
      #define S2(k) { \
        LOAD(k); \
        for (int32_t j = 0; j < 4; j++) {       \
          b[j] += vz[j]*w1 + vy[j]*w0;  \
          a[j] += vy[j]*w1 - vz[j]*w0;  \
          }                             \
        }
      //}}}
      S0(0) S2(1) S1(2) S2(3) S1(4) S2(5) S1(6) S2(7)

      dstr[(15 - i) * numChannels] = scale_pcm (a[1]);
      dstr[(17 + i) * numChannels] = scale_pcm (b[1]);
      dstl[(15 - i) * numChannels] = scale_pcm (a[0]);
      dstl[(17 + i) * numChannels] = scale_pcm (b[0]);
      dstr[(47 - i) * numChannels] = scale_pcm (a[3]);
      dstr[(49 + i) * numChannels] = scale_pcm (b[3]);
      dstl[(47 - i) * numChannels] = scale_pcm (a[2]);
      dstl[(49 + i) * numChannels] = scale_pcm (b[2]);
      }
    }
  //}}}
#endif
//{{{
void synthGranule (float* qmfState, float* granuleBuffer, int32_t numBands, int32_t numChannels,
                   int16_t* pcm, float* lins) {

  for (int32_t chan = 0; chan < numChannels; chan++)
    dctII (granuleBuffer + 576 * chan, numBands);

  memcpy (lins, qmfState, sizeof(float) * 15 * 64);

  for (int32_t band = 0; band < numBands; band += 2)
    synth (granuleBuffer + band, pcm + 32 * numChannels * band, numChannels, lins + band * 64);

  memcpy (qmfState, lins + numBands * 64, sizeof(float) * 15 * 64);
  }
//}}}

// public members
//{{{
cMp3Decoder::cMp3Decoder() {
  clear();
  }
//}}}
cMp3Decoder::~cMp3Decoder() {}
//{{{
int32_t cMp3Decoder::decodeFrame (uint8_t* inBuffer, int32_t bytesLeft, float* outBuffer, bool jumped) {

  auto timePoint = std::chrono::system_clock::now();

  memcpy (mHeader, inBuffer, HDR_SIZE);
  cBitStream frameBitStream (inBuffer + HDR_SIZE, bytesLeft - HDR_SIZE);

  int32_t layer = 4 - HDR_GET_LAYER (mHeader);
  int32_t bitrate_kbps = headerBitrate (mHeader);
  int32_t numSamples = headerFrameSamples (mHeader);
  mNumChannels = HDR_IS_MONO (mHeader) ? 1 : 2;
  mSampleRate = headerSampleRate (mHeader);

  if (HDR_IS_CRC (mHeader))
    frameBitStream.getBits (16);

  // allocate 16bit sample buffer, convert to float later
  int16_t samples16 [2048*2];
  int16_t* pcm = samples16;

  if (layer == 3) {
    //{{{  layer 3
    // readSideInfo from frameBitStream
    int32_t reservoirBytesNeeded = readSideInfo (&frameBitStream, mGranules, inBuffer);
    if ((reservoirBytesNeeded < 0) || (frameBitStream.getBitStreamPosition() > frameBitStream.getBitStreamLimit())) {
      mHeader[0] = 0;
      cLog::log (LOGERROR, "mp3 decode readSideInfo problem %d %d %d",
                 reservoirBytesNeeded, frameBitStream.getBitStreamPosition(), frameBitStream.getBitStreamLimit());
      return 0;
      }

    if (restoreReservoir (&frameBitStream, reservoirBytesNeeded)) {
      // use bitStream constructed from reservoir and rest of frameBitStream
      for (int32_t granuleIndex = 0; granuleIndex < (HDR_TEST_MPEG1(mHeader) ? 2 : 1); granuleIndex++) {
        struct sGranule* granule = mGranules + (granuleIndex * mNumChannels);
        memset (mGranuleBuffer[0], 0, 576 * 2 * sizeof(float));
        for (int32_t channel = 0; channel < mNumChannels; channel++) {
          int32_t layer3gr_limit = mBitStream.getBitStreamPosition() + granule[channel].part23length;
          decodeScaleFactorsL3 (mHeader, mIstPos[channel], &mBitStream, granule + channel, mScf, channel);
          huffman (mGranuleBuffer[channel], &mBitStream, granule + channel, mScf, layer3gr_limit);
          }

        if (HDR_TEST_I_STEREO (mHeader))
          intensityStereo (mGranuleBuffer[0], mIstPos[1], granule, mHeader);
        else if (HDR_IS_MS_STEREO (mHeader))
          midsideStereo (mGranuleBuffer[0], 576);

        for (int32_t channel = 0; channel < mNumChannels; channel++, granule++) {
          int32_t aaBands = 31;
          int32_t numLongBands = (granule->mixedBlockFlag ? 2 : 0) << (int)(HDR_GET_MY_SAMPLE_RATE (mHeader) == 2);
          if (granule->numShortSfb) {
            aaBands = numLongBands - 1;
            reorder (mGranuleBuffer[channel] + numLongBands * 18, mSyn[0], granule->sfbtab + granule->numLongSfb);
            }

          antialias (mGranuleBuffer[channel], aaBands);
          imdctGranule (mGranuleBuffer[channel], mMdctOverlap[channel], granule->blockType, numLongBands);
          changeSign (mGranuleBuffer[channel]);
          }

        synthGranule (mQmfState, mGranuleBuffer[0], 18, mNumChannels, pcm, mSyn[0]);
        pcm += 576 * mNumChannels;
        }

      // save rest of bitstream to reservoir
      saveReservoir();
      }

    else {
      // unable to retore reservoir, try to get next frame right
      saveReservoir();
      cLog::log (LOGERROR, "mp3 decode restoreReservoir failed");
      return 0;
      }
    }
    //}}}
  else {
    //{{{  layer 12
    cLog::log (LOGINFO2, "mp3 layer12");

    sScaleInfo scaleInfo;
    readScaleInfo (inBuffer, &frameBitStream, &scaleInfo);

    memset (mGranuleBuffer[0], 0, 576 * 2 * sizeof(float));
    for (int32_t i = 0, granuleIndex = 0; granuleIndex < 3; granuleIndex++) {
      if (12 == (i += dequantizeGranule (mGranuleBuffer[0] + i, &frameBitStream, &scaleInfo, layer | 1))) {
        i = 0;
        applyScf384 (&scaleInfo, scaleInfo.scf + granuleIndex, mGranuleBuffer[0]);
        synthGranule (mQmfState, mGranuleBuffer[0], 12, mNumChannels, pcm, mSyn[0]);
        memset (mGranuleBuffer[0], 0, 576*2*sizeof(float));
        pcm += 384 * mNumChannels;
        }
      if (frameBitStream.getBitStreamPosition() > frameBitStream.getBitStreamLimit()) {
        mHeader[0] = 0;
        cLog::log (LOGERROR, "mp3 decode layer 12 failed");
        return 0;
        }
      }
    }
    //}}}

  // convert 16bit sample buffer to float
  int16_t* srcPtr = samples16;
  float* dstPtr = outBuffer;
  for (int32_t sample = 0; sample < numSamples * 2; sample++)
    *dstPtr++ = *srcPtr++ / (float)0x8000;

  auto took = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timePoint);
  cLog::log (LOGINFO1, "mp3:%d %4d:%3dk %dx%d@%dhz %3d %3dus %s",
             layer, bytesLeft, bitrate_kbps, numSamples, mNumChannels, mSampleRate, mReservoir, 
             took.count(), jumped ? "jump":"");

  return numSamples;
  }
//}}}

// private members
//{{{
void cMp3Decoder::clear() {

  mReservoir = 0;
  memset (mReservoirBuffer, 0, sizeof (mReservoirBuffer[511]));
  memset (mMdctOverlap, 0, sizeof (mMdctOverlap));
  memset (mQmfState, 0, sizeof (mQmfState));
  };
//}}}

//{{{
void cMp3Decoder::saveReservoir() {
// save rest of bitStream to reservoir clipped by reservoir size

  int32_t pos = (mBitStream.getBitStreamPosition() + 7) / 8u;
  int32_t remains = (mBitStream.getBitStreamLimit() / 8u) - pos;
  if (remains > MAX_BITRESERVOIR_BYTES) {
    pos += remains - MAX_BITRESERVOIR_BYTES;
    remains = MAX_BITRESERVOIR_BYTES;
    }
  if (remains > 0)
    memmove (mReservoirBuffer, mBitStreamData + pos, remains);

  mReservoir = remains;

  cLog::log (LOGINFO2, "saveReservoir Reservoir:%d pos:%d", mReservoir, pos);
  }
//}}}
//{{{
bool cMp3Decoder::restoreReservoir (cBitStream* bitStream, int32_t reservoirBytesNeeded) {

  // copy required mainDataBegin bytes of reservoir to maindata
  int32_t bytesHave = std::min (mReservoir, reservoirBytesNeeded);
  memcpy (mBitStreamData, mReservoirBuffer + std::max (0, mReservoir - reservoirBytesNeeded), bytesHave);

  // copy rest of frame bitStream to mainData
  int32_t frameBytes = (bitStream->getBitStreamLimit() - bitStream->getBitStreamPosition()) / 8;
  memcpy (mBitStreamData + bytesHave, bitStream->getBitStreamBuffer() + bitStream->getBitStreamPosition() / 8, frameBytes);

  cLog::log (LOGINFO2, "restoreReservoir %d mdb:%d fb:%d bh:%d", mReservoir, reservoirBytesNeeded, frameBytes, bytesHave);

  // use mainData as bitstream
  mBitStream.bitStreamInit (mBitStreamData, bytesHave + frameBytes);
  return mReservoir >= reservoirBytesNeeded;
  }
//}}}
