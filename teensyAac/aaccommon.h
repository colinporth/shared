#pragma once
#include "cAacDecoder.h"

//#define AAC_ENABLE_SBR

#ifdef _WIN32
  #define bigMalloc(size,tag)   malloc (size)
  #define bigFree               free
#elif __linux__
  #define bigMalloc(size,tag)   malloc (size)
  #define bigFree               free
#else
  #include "FreeRtos.h"
  #define bigMalloc(size,tag)   pvPortMalloc(size)
  #define bigFree               vPortFree
#endif

//{{{  defines
#define MAX_NCHANS_ELEM   2
#define NUM_TERMS_RPI 5

/* 12-bit syncword */
#define SYNCWORDH     0xff
#define SYNCWORDL     0xf0

#define ADTS_HEADER_BYTES  7
#define NUM_SAMPLE_RATES  12
#define NUM_ELEMENTS       8
#define MAX_NUM_PCE_ADIF  16

#define MAX_WIN_GROUPS   8
#define MAX_SFB_SHORT   15
#define MAX_SF_BANDS    (MAX_SFB_SHORT*MAX_WIN_GROUPS)  /* worst case = 15 sfb's * 8 windows for short block */
#define MAX_MS_MASK_BYTES ((MAX_SF_BANDS + 7) >> 3)
#define MAX_PRED_SFB    41
#define MAX_TNS_FILTERS  8
#define MAX_TNS_COEFS   60
#define MAX_TNS_ORDER   20
#define MAX_PULSES       4
#define MAX_GAIN_BANDS   3
#define MAX_GAIN_WIN     8
#define MAX_GAIN_ADJUST  7

#define NUM_SYN_ID_BITS   3
#define NUM_INST_TAG_BITS 4

#define EXT_SBR_DATA     0x0d
#define EXT_SBR_DATA_CRC  0x0e

#define NSAMPS_LONG     1024
#define NSAMPS_SHORT    128

#define SF_DQ_OFFSET            15
#define FBITS_OUT_DQ            20
#define FBITS_OUT_DQ_OFF        (FBITS_OUT_DQ - SF_DQ_OFFSET)   /* number of fraction bits out of dequant, including 2^15 bias */

#define NUM_IMDCT_SIZES       2
#define NUM_SAMPLE_RATES_SBR  9 /* downsampled (single-rate) mode unsupported, so only use Fs_sbr >= 16 kHz */
#define NUM_DEF_CHAN_MAPS     8
#define MAX_HUFF_BITS        20
#define GBITS_IN_DCT4         4                       /* min guard bits in for DCT4 */

#define NUM_TIME_SLOTS      16
#define SAMPLES_PER_SLOT    2 /* RATE in spec */

#define MAX_NUM_ENV         5
#define MAX_NUM_NOISE_FLOORS    2
#define MAX_NUM_NOISE_FLOOR_BANDS 5 /* max Nq, see 4.6.18.3.6 */
#define MAX_NUM_PATCHES       5
#define MAX_NUM_SMOOTH_COEFS    5

#define HF_GEN      8
#define HF_ADJ      2

#define MAX_QMF_BANDS 48    /* max QMF subbands covered by SBR (4.6.18.3.6) */

#define FBITS_IN_QMFA 14
#define FBITS_LOST_QMFA (1 + 2 + 3 + 2 + 1) /* 1 from cTab, 2 in premul, 3 in FFT, 2 in postmul, 1 for implicit scaling by 2.0 */
#define FBITS_OUT_QMFA  (FBITS_IN_QMFA - FBITS_LOST_QMFA)
#define FBITS_IN_QMFS     FBITS_OUT_QMFA
#define FBITS_LOST_DCT4_64    (2 + 3 + 2)   /* 2 in premul, 3 in FFT, 2 in postmul */
/* lose FBITS_LOST_DCT4_64 in DCT4, gain 6 for implicit scaling by 1/64, lose 1 for cTab multiply (Q31) */
#define FBITS_OUT_QMFS   (FBITS_IN_QMFS - FBITS_LOST_DCT4_64 + 6 - 1)
#define RND_VAL    (1 << (FBITS_OUT_QMFS-1))
#define RND_VAL1   (1 << (FBITS_OUT_IMDCT-1))
#define MIN_GBITS_IN_QMFS   2
#define FBITS_OUT_DQ_ENV  29  /* dequantized env scalefactors are Q(29 - envDataDequantScale) */
#define FBITS_OUT_DQ_NOISE  24  /* range of Q_orig = [2^-24, 2^6] */
#define NOISE_FLOOR_OFFSET  6

#define NUM_QMF_DELAY_BUFS  10
#define DELAY_SAMPS_QMFA  (NUM_QMF_DELAY_BUFS * 32)
#define DELAY_SAMPS_QMFS  (NUM_QMF_DELAY_BUFS * 128)

#define NWINDOWS_LONG        1
#define NWINDOWS_SHORT       8

#define DATA_BUF_SIZE      510             /* max count = 255 + 255 */
#define FILL_BUF_SIZE      269             /* max count = 15 + 255 - 1*/
#define ADIF_COPYID_SIZE     9
#define MAX_COMMENT_BYTES  255

#define MAX_NUM_FCE         15
#define MAX_NUM_SCE         15
#define MAX_NUM_BCE         15
#define MAX_NUM_LCE          3
#define MAX_NUM_ADE          7
#define MAX_NUM_CCE          15

#define CHAN_ELEM_IS_CPE(x)     (((x) & 0x10) >> 4)  /* bit 4 = SCE/CPE flag */
#define CHAN_ELEM_GET_TAG(x)    (((x) & 0x0f) >> 0)  /* bits 3-0 = instance tag */

#define CHAN_ELEM_SET_CPE(x)    (((x) & 0x01) << 4)  /* bit 4 = SCE/CPE flag */
#define CHAN_ELEM_SET_TAG(x)    (((x) & 0x0f) << 0)  /* bits 3-0 = instance tag */

//}}}

//{{{
#ifndef ASSERT
  #if defined(_WIN32) && defined(_M_IX86) && (defined (_DEBUG) || defined (REL_ENABLE_ASSERTS))
    #define ASSERT(x) if (!(x)) __asm int 3;
  #else
    #define ASSERT(x) /* do nothing */
  #endif
#endif
//}}}
//{{{
#ifndef MAX
  #define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif
//}}}
//{{{
#ifndef MIN
  #define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif
//}}}

//{{{  enum AAC_ID
enum {
  AAC_ID_INVALID = -1,
  AAC_ID_SCE =  0,
  AAC_ID_CPE =  1,
  AAC_ID_CCE =  2,
  AAC_ID_LFE =  3,
  AAC_ID_DSE =  4,
  AAC_ID_PCE =  5,
  AAC_ID_FIL =  6,
  AAC_ID_END =  7
};
//}}}
//{{{  enum SBR_GRID
enum {
  SBR_GRID_FIXFIX = 0,
  SBR_GRID_FIXVAR = 1,
  SBR_GRID_VARFIX = 2,
  SBR_GRID_VARVAR = 3
  };
//}}}
//{{{  enum HuffTabSBR
enum {
  HuffTabSBR_tEnv15 =    0,
  HuffTabSBR_fEnv15 =    1,
  HuffTabSBR_tEnv15b =   2,
  HuffTabSBR_fEnv15b =   3,
  HuffTabSBR_tEnv30 =    4,
  HuffTabSBR_fEnv30 =    5,
  HuffTabSBR_tEnv30b =   6,
  HuffTabSBR_fEnv30b =   7,
  HuffTabSBR_tNoise30 =  8,
  HuffTabSBR_fNoise30 =  5,
  HuffTabSBR_tNoise30b = 9,
  HuffTabSBR_fNoise30b = 7
  };
//}}}
//{{{  enum AAC file format
enum {
  AAC_FF_Unknown = 0,   /* should be 0 on init */
  AAC_FF_ADTS = 1,
  AAC_FF_ADIF = 2,
  AAC_FF_RAW =  3
  };
//}}}

//{{{
typedef struct _HuffInfo {
  int maxBits;              /* number of bits in longest codeword */
  unsigned char count[MAX_HUFF_BITS];   /* count[i] = number of codes with length i+1 bits */
  int offset;               /* offset into symbol table */
  } HuffInfo;
//}}}

//{{{
typedef struct _PulseInfo {
  unsigned char pulseDataPresent;
  unsigned char numPulse;
  unsigned char startSFB;
  unsigned char offset[MAX_PULSES];
  unsigned char amp[MAX_PULSES];
  } PulseInfo;
//}}}
//{{{
typedef struct _TNSInfo {
  unsigned char tnsDataPresent;
  unsigned char numFilt[MAX_TNS_FILTERS];     /* max 1 filter each for 8 short windows, or 3 filters for 1 long window */
  unsigned char coefRes[MAX_TNS_FILTERS];
  unsigned char length[MAX_TNS_FILTERS];
  unsigned char order[MAX_TNS_FILTERS];
  unsigned char dir[MAX_TNS_FILTERS];
  signed char   coef[MAX_TNS_COEFS];              /* max 3 filters * 20 coefs for 1 long window, or 1 filter * 7 coefs for each of 8 short windows */
  } TNSInfo;
//}}}
//{{{
typedef struct _GainControlInfo {
  unsigned char gainControlDataPresent;
  unsigned char maxBand;
  unsigned char adjNum[MAX_GAIN_BANDS][MAX_GAIN_WIN];
  unsigned char alevCode[MAX_GAIN_BANDS][MAX_GAIN_WIN][MAX_GAIN_ADJUST];
  unsigned char alocCode[MAX_GAIN_BANDS][MAX_GAIN_WIN][MAX_GAIN_ADJUST];
  } GainControlInfo;
//}}}
//{{{
typedef struct _ICSInfo {
  unsigned char icsResBit;
  unsigned char winSequence;
  unsigned char winShape;
  unsigned char maxSFB;
  unsigned char sfGroup;
  unsigned char predictorDataPresent;
  unsigned char predictorReset;
  unsigned char predictorResetGroupNum;
  unsigned char predictionUsed[MAX_PRED_SFB];
  unsigned char numWinGroup;
  unsigned char winGroupLen[MAX_WIN_GROUPS];
  } ICSInfo;
//}}}

//{{{
/* need one SBRHeader per element (SCE/CPE), updated only on new header */
typedef struct _SBRHeader {
  int                   count;

  unsigned char         ampRes;
  unsigned char         startFreq;
  unsigned char         stopFreq;
  unsigned char         crossOverBand;
  unsigned char         resBitsHdr;
  unsigned char         hdrExtra1;
  unsigned char         hdrExtra2;

  unsigned char         freqScale;
  unsigned char         alterScale;
  unsigned char         noiseBands;

  unsigned char         limiterBands;
  unsigned char         limiterGains;
  unsigned char         interpFreq;
  unsigned char         smoothMode;
  } SBRHeader;
//}}}
//{{{
/* need one SBRGrid per channel, updated every frame */
typedef struct _SBRGrid {
  unsigned char         frameClass;
  unsigned char         ampResFrame;
  unsigned char         pointer;

  unsigned char         numEnv;           /* L_E */
  unsigned char         envTimeBorder[MAX_NUM_ENV+1]; /* t_E */
  unsigned char         freqRes[MAX_NUM_ENV];     /* r */

  unsigned char         numNoiseFloors;             /* L_Q */
  unsigned char         noiseTimeBorder[MAX_NUM_NOISE_FLOORS+1];  /* t_Q */

  unsigned char         numEnvPrev;
  unsigned char         numNoiseFloorsPrev;
  unsigned char         freqResPrev;
  } SBRGrid;
//}}}
//{{{
/* need one SBRFreq per element (SCE/CPE/LFE), updated only on header reset */
typedef struct _SBRFreq {
  int                   kStart;       /* k_x */
  int                   nMaster;
  int                   nHigh;
  int                   nLow;
  int                   nLimiter;       /* N_l */
  int                   numQMFBands;      /* M */
  int                   numNoiseFloorBands; /* Nq */

  int                   kStartPrev;
  int                   numQMFBandsPrev;

  unsigned char         freqMaster[MAX_QMF_BANDS + 1];  /* not necessary to save this  after derived tables are generated */
  unsigned char         freqHigh[MAX_QMF_BANDS + 1];
  unsigned char         freqLow[MAX_QMF_BANDS / 2 + 1]; /* nLow = nHigh - (nHigh >> 1) */
  unsigned char         freqNoise[MAX_NUM_NOISE_FLOOR_BANDS+1];
  unsigned char         freqLimiter[MAX_QMF_BANDS / 2 + MAX_NUM_PATCHES];   /* max (intermediate) size = nLow + numPatches - 1 */

  unsigned char         numPatches;
  unsigned char         patchNumSubbands[MAX_NUM_PATCHES + 1];
  unsigned char         patchStartSubband[MAX_NUM_PATCHES + 1];
  } SBRFreq;
//}}}
//{{{
typedef struct _SBRChan {
  int                   reset;
  unsigned char         deltaFlagEnv[MAX_NUM_ENV];
  unsigned char         deltaFlagNoise[MAX_NUM_NOISE_FLOORS];

  signed char           envDataQuant[MAX_NUM_ENV][MAX_QMF_BANDS];   /* range = [0, 127] */
  signed char           noiseDataQuant[MAX_NUM_NOISE_FLOORS][MAX_NUM_NOISE_FLOOR_BANDS];

  unsigned char         invfMode[2][MAX_NUM_NOISE_FLOOR_BANDS]; /* invfMode[0/1][band] = prev/curr */
  int                   chirpFact[MAX_NUM_NOISE_FLOOR_BANDS];   /* bwArray */
  unsigned char         addHarmonicFlag[2];           /* addHarmonicFlag[0/1] = prev/curr */
  unsigned char         addHarmonic[2][64];           /* addHarmonic[0/1][band] = prev/curr */

  int                   gbMask[2];  /* gbMask[0/1] = XBuf[0-31]/XBuf[32-39] */
  signed char           laPrev;

  int                   noiseTabIndex;
  int                   sinIndex;
  int                   gainNoiseIndex;
  int                   gTemp[MAX_NUM_SMOOTH_COEFS][MAX_QMF_BANDS];
  int                   qTemp[MAX_NUM_SMOOTH_COEFS][MAX_QMF_BANDS];
  } SBRChan;
//}}}
//{{{
typedef struct _PSInfoSBR {
  /* save for entire file */
  int                   frameCount;
  int                   sampRateIdx;

  /* state info that must be saved for each channel */
  SBRHeader             sbrHeader[AAC_MAX_NCHANS];
  SBRGrid               sbrGrid[AAC_MAX_NCHANS];
  SBRFreq               sbrFreq[AAC_MAX_NCHANS];
  SBRChan               sbrChan[AAC_MAX_NCHANS];

  /* temp variables, no need to save between blocks */
  unsigned char         dataExtra;
  unsigned char         resBitsData;
  unsigned char         extendedDataPresent;
  int                   extendedDataSize;

  signed char           envDataDequantScale[MAX_NCHANS_ELEM][MAX_NUM_ENV];
  int                   envDataDequant[MAX_NCHANS_ELEM][MAX_NUM_ENV][MAX_QMF_BANDS];
  int                   noiseDataDequant[MAX_NCHANS_ELEM][MAX_NUM_NOISE_FLOORS][MAX_NUM_NOISE_FLOOR_BANDS];

  int                   eCurr[MAX_QMF_BANDS];
  unsigned char         eCurrExp[MAX_QMF_BANDS];
  unsigned char         eCurrExpMax;
  signed char           la;

  int                   crcCheckWord;
  int                   couplingFlag;
  int                   envBand;
  int                   eOMGainMax;
  int                   gainMax;
  int                   gainMaxFBits;
  int                   noiseFloorBand;
  int                   qp1Inv;
  int                   qqp1Inv;
  int                   sMapped;
  int                   sBand;
  int                   highBand;

  int                   sumEOrigMapped;
  int                   sumECurrGLim;
  int                   sumSM;
  int                   sumQM;
  int                   gLimBoost[MAX_QMF_BANDS];
  int                   qmLimBoost[MAX_QMF_BANDS];
  int                   smBoost[MAX_QMF_BANDS];

  int                   smBuf[MAX_QMF_BANDS];
  int                   qmLimBuf[MAX_QMF_BANDS];
  int                   gLimBuf[MAX_QMF_BANDS];
  int                   gLimFbits[MAX_QMF_BANDS];

  int                   gFiltLast[MAX_QMF_BANDS];
  int                   qFiltLast[MAX_QMF_BANDS];

  /* large buffers */
  int                   delayIdxQMFA[AAC_MAX_NCHANS];
  int                   delayQMFA[AAC_MAX_NCHANS][DELAY_SAMPS_QMFA];
  int                   delayIdxQMFS[AAC_MAX_NCHANS];
  int                   delayQMFS[AAC_MAX_NCHANS][DELAY_SAMPS_QMFS];
  int                   XBufDelay[AAC_MAX_NCHANS][HF_GEN][64][2];
  int                   XBuf[32+8][64][2];
  } PSInfoSBR;
//}}}

//{{{
/* sizeof(ProgConfigElement) = 82 bytes (if KEEP_PCE_COMMENTS not defined) */
typedef struct _ProgConfigElement {
  unsigned char elemInstTag;                    /* element instance tag */
  unsigned char profile;                        /* 0 = main, 1 = LC, 2 = SSR, 3 = reserved */
  unsigned char sampRateIdx;                    /* sample rate index range = [0, 11] */
  unsigned char numFCE;                         /* number of front channel elements (max = 15) */
  unsigned char numSCE;                         /* number of side channel elements (max = 15) */
  unsigned char numBCE;                         /* number of back channel elements (max = 15) */
  unsigned char numLCE;                         /* number of LFE channel elements (max = 3) */
  unsigned char numADE;                         /* number of associated data elements (max = 7) */
  unsigned char numCCE;                         /* number of valid channel coupling elements (max = 15) */
  unsigned char monoMixdown;                    /* mono mixdown: bit 4 = present flag, bits 3-0 = element number */
  unsigned char stereoMixdown;                  /* stereo mixdown: bit 4 = present flag, bits 3-0 = element number */
  unsigned char matrixMixdown;                  /* matrix mixdown: bit 4 = present flag, bit 3 = unused,
                                                   bits 2-1 = index, bit 0 = pseudo-surround enable */
  unsigned char fce[MAX_NUM_FCE];               /* front element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
  unsigned char sce[MAX_NUM_SCE];               /* side element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
  unsigned char bce[MAX_NUM_BCE];               /* back element channel pair: bit 4 = SCE/CPE flag, bits 3-0 = inst tag */
  unsigned char lce[MAX_NUM_LCE];               /* instance tag for LFE elements */
  unsigned char ade[MAX_NUM_ADE];               /* instance tag for ADE elements */
  unsigned char cce[MAX_NUM_BCE];               /* channel coupling elements: bit 4 = switching flag, bits 3-0 = inst tag */
  } ProgConfigElement;
//}}}
//{{{
typedef struct _ADTSHeader {
  /* fixed */
  unsigned char id;                             /* MPEG bit - should be 1 */
  unsigned char layer;                          /* MPEG layer - should be 0 */
  unsigned char protectBit;                     /* 0 = CRC word follows, 1 = no CRC word */
  unsigned char profile;                        /* 0 = main, 1 = LC, 2 = SSR, 3 = reserved */
  unsigned char sampRateIdx;                    /* sample rate index range = [0, 11] */
  unsigned char privateBit;                     /* ignore */
  unsigned char channelConfig;                  /* 0 = implicit, >0 = use default table */
  unsigned char origCopy;                       /* 0 = copy, 1 = original */
  unsigned char home;                           /* ignore */

  /* variable */
  unsigned char copyBit;                        /* 1 bit of the 72-bit copyright ID (transmitted as 1 bit per frame) */
  unsigned char copyStart;                      /* 1 = this bit starts the 72-bit ID, 0 = it does not */
  int           frameLength;                    /* length of frame */
  int           bufferFull;                     /* number of 32-bit words left in enc buffer, 0x7FF = VBR */
  unsigned char numRawDataBlocks;               /* number of raw data blocks in frame */

  /* CRC */
  int           crcCheckWord;                   /* 16-bit CRC check word (present if protectBit == 0) */
  } ADTSHeader;
//}}}
//{{{
typedef struct _ADIFHeader {
  unsigned char copyBit;                        /* 0 = no copyright ID, 1 = 72-bit copyright ID follows immediately */
  unsigned char origCopy;                       /* 0 = copy, 1 = original */
  unsigned char home;                           /* ignore */
  unsigned char bsType;                         /* bitstream type: 0 = CBR, 1 = VBR */
  int           bitRate;                        /* bitRate: CBR = bits/sec, VBR = peak bits/frame, 0 = unknown */
  unsigned char numPCE;                         /* number of program config elements (max = 16) */
  int           bufferFull;                     /* bits left in bit reservoir */
  unsigned char copyID[ADIF_COPYID_SIZE];       /* optional 72-bit copyright ID */
  } ADIFHeader;
//}}}

//{{{
/* state info struct for baseline (MPEG-4 LC) decoding */
typedef struct _PSInfoBase {
  /* header information */
  ADTSHeader            fhADTS;
  ADIFHeader            fhADIF;
  ProgConfigElement     pce[MAX_NUM_PCE_ADIF];
  int                   dataCount;
  unsigned char         dataBuf[DATA_BUF_SIZE];
  int                   fillCount;
  unsigned char         fillBuf[FILL_BUF_SIZE];

  /* state information which is the same throughout whole frame */
  int                   nChans;
  int                   useImpChanMap;
  int                   sampRateIdx;

  /* state information which can be overwritten by subsequent elements within frame */
  ICSInfo               icsInfo[MAX_NCHANS_ELEM];

  int                   commonWin;
  short                 scaleFactors[MAX_NCHANS_ELEM][MAX_SF_BANDS];
  unsigned char         sfbCodeBook[MAX_NCHANS_ELEM][MAX_SF_BANDS];

  int                   msMaskPresent;
  unsigned char         msMaskBits[MAX_MS_MASK_BYTES];

  int                   pnsUsed[MAX_NCHANS_ELEM];
  int                   pnsLastVal;
  int                   intensityUsed[MAX_NCHANS_ELEM];

  PulseInfo             pulseInfo[MAX_NCHANS_ELEM];

  TNSInfo               tnsInfo[MAX_NCHANS_ELEM];
  int                   tnsLPCBuf[MAX_TNS_ORDER];
  int                   tnsWorkBuf[MAX_TNS_ORDER];

  GainControlInfo       gainControlInfo[MAX_NCHANS_ELEM];

  int                   gbCurrent[MAX_NCHANS_ELEM];
  int                   coef[MAX_NCHANS_ELEM][AAC_MAX_NSAMPS];
#ifdef AAC_ENABLE_SBR
  int                   sbrWorkBuf[MAX_NCHANS_ELEM][AAC_MAX_NSAMPS];
#endif
  /* state information which must be saved for each element and used in next frame */
  int                   overlap[AAC_MAX_NCHANS][AAC_MAX_NSAMPS];
  int                   prevWinShape[AAC_MAX_NCHANS];
  } PSInfoBase;
//}}}
