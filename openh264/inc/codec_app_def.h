#pragma once
#include "codec_def.h"

//{{{
#define MAX_TEMPORAL_LAYER_NUM          4
#define MAX_SPATIAL_LAYER_NUM           4
#define MAX_QUALITY_LAYER_NUM           4

#define MAX_LAYER_NUM_OF_FRAME          128
#define MAX_NAL_UNITS_IN_LAYER          128     ///< predetermined here, adjust it later if need

#define MAX_RTP_PAYLOAD_LEN             1000
#define AVERAGE_RTP_PAYLOAD_LEN         800


#define SAVED_NALUNIT_NUM_TMP           ( (MAX_SPATIAL_LAYER_NUM*MAX_QUALITY_LAYER_NUM) + 1 + MAX_SPATIAL_LAYER_NUM )  ///< SPS/PPS + SEI/SSEI + PADDING_NAL
#define MAX_SLICES_NUM_TMP              ( ( MAX_NAL_UNITS_IN_LAYER - SAVED_NALUNIT_NUM_TMP ) / 3 )


#define AUTO_REF_PIC_COUNT  -1          ///< encoder selects the number of reference frame automatically
#define UNSPECIFIED_BIT_RATE 0          ///< to do: add detail comment
//}}}

/// E.g. SDK version is 1.2.0.0, major version number is 1, minor version number is 2, and revision number is 0.
//{{{
typedef struct  _tagVersion {
  unsigned int uMajor;                  ///< The major version number
  unsigned int uMinor;                  ///< The minor version number
  unsigned int uRevision;               ///< The revision number
  unsigned int uReserved;               ///< The reserved number, it should be 0.
} OpenH264Version;
//}}}
//{{{
typedef enum {
  //Errors derived from bitstream parsing
  dsErrorFree           = 0x00,   ///< bit stream error-free
  dsFramePending        = 0x01,   ///< need more throughput to generate a frame output,
  dsRefLost             = 0x02,   ///< layer lost at reference frame with temporal id 0
  dsBitstreamError      = 0x04,   ///< error bitstreams(maybe broken internal frame) the decoder cared
  dsDepLayerLost        = 0x08,   ///< dependented layer is ever lost
  dsNoParamSets         = 0x10,   ///< no parameter set NALs involved
  dsDataErrorConcealed  = 0x20,   ///< current data error concealed specified

  //Errors derived from logic level
  dsInvalidArgument     = 0x1000, ///< invalid argument specified
  dsInitialOptExpected  = 0x2000, ///< initializing operation is expected
  dsOutOfMemory         = 0x4000, ///< out of memory due to new request
  dsDstBufNeedExpan     = 0x8000  ///< actual picture size exceeds size of dst pBuffer feed in decoder, so need expand its size
} DECODING_STATE;
//}}}
//{{{
typedef enum {
  DECODER_OPTION_END_OF_STREAM = 1,     ///< end of stream flag
  DECODER_OPTION_VCL_NAL,               ///< feedback whether or not have VCL NAL in current AU for application layer
  DECODER_OPTION_TEMPORAL_ID,           ///< feedback temporal id for application layer
  DECODER_OPTION_FRAME_NUM,             ///< feedback current decoded frame number
  DECODER_OPTION_IDR_PIC_ID,            ///< feedback current frame belong to which IDR period
  DECODER_OPTION_LTR_MARKING_FLAG,      ///< feedback wether current frame mark a LTR
  DECODER_OPTION_LTR_MARKED_FRAME_NUM,  ///< feedback frame num marked by current Frame
  DECODER_OPTION_ERROR_CON_IDC,         ///< indicate decoder error concealment method
  DECODER_OPTION_TRACE_LEVEL,
  DECODER_OPTION_TRACE_CALLBACK,        ///< a void (*)(void* context, int level, const char* message) function which receives log messages
  DECODER_OPTION_TRACE_CALLBACK_CONTEXT,///< context info of trace callbac

  DECODER_OPTION_GET_STATISTICS

} DECODER_OPTION;
//}}}
//{{{
typedef enum {
  ERROR_CON_DISABLE = 0,
  ERROR_CON_FRAME_COPY,
  ERROR_CON_SLICE_COPY,
  ERROR_CON_FRAME_COPY_CROSS_IDR,
  ERROR_CON_SLICE_COPY_CROSS_IDR,
  ERROR_CON_SLICE_COPY_CROSS_IDR_FREEZE_RES_CHANGE,
  ERROR_CON_SLICE_MV_COPY_CROSS_IDR,
  ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE
} ERROR_CON_IDC;
//}}}
//{{{
typedef enum {
  FEEDBACK_NON_VCL_NAL = 0,
  FEEDBACK_VCL_NAL,
  FEEDBACK_UNKNOWN_NAL
} FEEDBACK_VCL_NAL_IN_AU;
//}}}
//{{{
typedef enum {
  NON_VIDEO_CODING_LAYER = 0,
  VIDEO_CODING_LAYER = 1
} LAYER_TYPE;
//}}}
//{{{
typedef enum {
  SPATIAL_LAYER_0 = 0,
  SPATIAL_LAYER_1 = 1,
  SPATIAL_LAYER_2 = 2,
  SPATIAL_LAYER_3 = 3,
  SPATIAL_LAYER_ALL = 4
} LAYER_NUM;
//}}}
//{{{
typedef enum {
  VIDEO_BITSTREAM_AVC               = 0,
  VIDEO_BITSTREAM_SVC               = 1,
  VIDEO_BITSTREAM_DEFAULT           = VIDEO_BITSTREAM_SVC
} VIDEO_BITSTREAM_TYPE;
//}}}
//{{{
typedef enum {
  NO_RECOVERY_REQUSET  = 0,
  LTR_RECOVERY_REQUEST = 1,
  IDR_RECOVERY_REQUEST = 2,
  NO_LTR_MARKING_FEEDBACK = 3,
  LTR_MARKING_SUCCESS = 4,
  LTR_MARKING_FAILED = 5
} KEY_FRAME_REQUEST_TYPE;
//}}}
//{{{
typedef struct {
  unsigned int uiFeedbackType;       ///< IDR request or LTR recovery request
  unsigned int uiIDRPicId;           ///< distinguish request from different IDR
  int          iLastCorrectFrameNum;
  int          iCurrentFrameNum;     ///< specify current decoder frame_num.
} SLTRRecoverRequest;
//}}}
//{{{
typedef struct {
  unsigned int  uiFeedbackType; ///< mark failed or successful
  unsigned int  uiIDRPicId;     ///< distinguish request from different IDR
  int           iLTRFrameNum;   ///< specify current decoder frame_num
} SLTRMarkingFeedback;
//}}}
//{{{
typedef struct {
  bool   bEnableLongTermReference; ///< 1: on, 0: off
  int    iLTRRefNum;               ///< TODO: not supported to set it arbitrary yet
} SLTRConfig;
//}}}
//{{{
typedef enum {
  RC_QUALITY_MODE = 0,     ///< quality mode
  RC_BITRATE_MODE = 1,     ///< bitrate mode
  RC_BUFFERBASED_MODE = 2, ///< no bitrate control,only using buffer status,adjust the video quality
  RC_TIMESTAMP_MODE = 3, //rate control based timestamp
  RC_BITRATE_MODE_POST_SKIP = 4, ///< this is in-building RC MODE, WILL BE DELETED after algorithm tuning!
  RC_OFF_MODE = -1,         ///< rate control off mode
} RC_MODES;
//}}}
//{{{
typedef enum {
  PRO_UNKNOWN   = 0,
  PRO_BASELINE  = 66,
  PRO_MAIN      = 77,
  PRO_EXTENDED  = 88,
  PRO_HIGH      = 100,
  PRO_HIGH10    = 110,
  PRO_HIGH422   = 122,
  PRO_HIGH444   = 144,
  PRO_CAVLC444  = 244,

  PRO_SCALABLE_BASELINE = 83,
  PRO_SCALABLE_HIGH     = 86
} EProfileIdc;
//}}}
//{{{
typedef enum {
  LEVEL_UNKNOWN,
  LEVEL_1_0,
  LEVEL_1_B,
  LEVEL_1_1,
  LEVEL_1_2,
  LEVEL_1_3,
  LEVEL_2_0,
  LEVEL_2_1,
  LEVEL_2_2,
  LEVEL_3_0,
  LEVEL_3_1,
  LEVEL_3_2,
  LEVEL_4_0,
  LEVEL_4_1,
  LEVEL_4_2,
  LEVEL_5_0,
  LEVEL_5_1,
  LEVEL_5_2
} ELevelIdc;
//}}}
//{{{
enum {
  WELS_LOG_QUIET       = 0x00,          ///< quiet mode
  WELS_LOG_ERROR       = 1 << 0,        ///< error log iLevel
  WELS_LOG_WARNING     = 1 << 1,        ///< Warning log iLevel
  WELS_LOG_INFO        = 1 << 2,        ///< information log iLevel
  WELS_LOG_DEBUG       = 1 << 3,        ///< debug log, critical algo log
  WELS_LOG_DETAIL      = 1 << 4,        ///< per packet/frame log
  WELS_LOG_RESV        = 1 << 5,        ///< resversed log iLevel
  WELS_LOG_LEVEL_COUNT = 6,
  WELS_LOG_DEFAULT     = WELS_LOG_WARNING   ///< default log iLevel in Wels codec
};
//}}}
//{{{
typedef enum {
  SM_SINGLE_SLICE         = 0, ///< | SliceNum==1
  SM_FIXEDSLCNUM_SLICE    = 1, ///< | according to SliceNum        | enabled dynamic slicing for multi-thread
  SM_RASTER_SLICE         = 2, ///< | according to SlicesAssign    | need input of MB numbers each slice. In addition, if other constraint in SSliceArgument is presented, need to follow the constraints. Typically if MB num and slice size are both constrained, re-encoding may be involved.
  SM_SIZELIMITED_SLICE           = 3, ///< | according to SliceSize       | slicing according to size, the slicing will be dynamic(have no idea about slice_nums until encoding current frame)
  SM_RESERVED             = 4
} SliceModeEnum;
//}}}
//{{{
typedef struct {
  SliceModeEnum uiSliceMode;    ///< by default, uiSliceMode will be SM_SINGLE_SLICE
  unsigned int  uiSliceNum;     ///< only used when uiSliceMode=1, when uiSliceNum=0 means auto design it with cpu core number
  unsigned int  uiSliceMbNum[MAX_SLICES_NUM_TMP]; ///< only used when uiSliceMode=2; when =0 means setting one MB row a slice
  unsigned int  uiSliceSizeConstraint; ///< now only used when uiSliceMode=4
} SSliceArgument;
//}}}
//{{{
typedef struct {
  int   iVideoWidth;           ///< width of picture in luminance samples of a layer
  int   iVideoHeight;          ///< height of picture in luminance samples of a layer
  float fFrameRate;            ///< frame rate specified for a layer
  int   iSpatialBitrate;       ///< target bitrate for a spatial layer, in unit of bps
  int   iMaxSpatialBitrate;    ///< maximum  bitrate for a spatial layer, in unit of bps
  EProfileIdc  uiProfileIdc;   ///< value of profile IDC (PRO_UNKNOWN for auto-detection)
  ELevelIdc    uiLevelIdc;     ///< value of profile IDC (0 for auto-detection)
  int          iDLayerQp;      ///< value of level IDC (0 for auto-detection)

  SSliceArgument sSliceArgument;
} SSpatialLayerConfig;
//}}}
//{{{
typedef enum {
  CAMERA_VIDEO_REAL_TIME,      ///< camera video for real-time communication
  SCREEN_CONTENT_REAL_TIME,    ///< screen content signal
  CAMERA_VIDEO_NON_REAL_TIME
} EUsageType;
//}}}
//{{{
typedef enum {
  LOW_COMPLEXITY,             ///< the lowest compleixty,the fastest speed,
  MEDIUM_COMPLEXITY,          ///< medium complexity, medium speed,medium quality
  HIGH_COMPLEXITY             ///< high complexity, lowest speed, high quality
} ECOMPLEXITY_MODE;
//}}}
//{{{
typedef enum {
  CONSTANT_ID = 0,           ///< constant id in SPS/PPS
  INCREASING_ID = 0x01,      ///< SPS/PPS id increases at each IDR
  SPS_LISTING  = 0x02,       ///< using SPS in the existing list if possible
  SPS_LISTING_AND_PPS_INCREASING  = 0x03,
  SPS_PPS_LISTING  = 0x06,
} EParameterSetStrategy;
//}}}
//{{{
typedef struct {
  unsigned int          size;          ///< size of the struct
  VIDEO_BITSTREAM_TYPE  eVideoBsType;  ///< video stream type (AVC/SVC)
} SVideoProperty;
//}}}
//{{{
typedef struct TagSVCDecodingParam {
  char*     pFileNameRestructed;       ///< file name of reconstructed frame used for PSNR calculation based debug

  unsigned int  uiCpuLoad;             ///< CPU load
  unsigned char uiTargetDqLayer;       ///< setting target dq layer id

  ERROR_CON_IDC eEcActiveIdc;          ///< whether active error concealment feature in decoder
  bool bParseOnly;                     ///< decoder for parse only, no reconstruction. When it is true, SPS/PPS size should not exceed SPS_PPS_BS_SIZE (128). Otherwise, it will return error info

  SVideoProperty   sVideoProperty;    ///< video stream property
} SDecodingParam, *PDecodingParam;
//}}}
//{{{
typedef struct {
  unsigned char uiTemporalId;
  unsigned char uiSpatialId;
  unsigned char uiQualityId;
  EVideoFrameType eFrameType;
  unsigned char uiLayerType;

  int   iNalCount;              ///< count number of NAL coded already
  int*  pNalLengthInByte;       ///< length of NAL size in byte from 0 to iNalCount-1
  unsigned char*  pBsBuf;       ///< buffer of bitstream contained
} SLayerBSInfo, *PLayerBSInfo;
//}}}
//{{{
typedef struct {
  int iTemporalId;              ///< temporal ID

  /**
  * The sub sequence layers are ordered hierarchically based on their dependency on each other so that any picture in a layer shall not be
  * predicted from any picture on any higher layer.
  */
  int iSubSeqId;                ///< refer to D.2.11 Sub-sequence information SEI message semantics

  int           iLayerNum;
  SLayerBSInfo  sLayerInfo[MAX_LAYER_NUM_OF_FRAME];

  EVideoFrameType eFrameType;
  int iFrameSizeInBytes;
  long long uiTimeStamp;
} SFrameBSInfo, *PFrameBSInfo;
//}}}
//{{{
typedef struct Source_Picture_s {
  int       iColorFormat;          ///< color space type
  int       iStride[4];            ///< stride for each plane pData
  unsigned char*  pData[4];        ///< plane pData
  int       iPicWidth;             ///< luma picture width in x coordinate
  int       iPicHeight;            ///< luma picture height in y coordinate
  long long uiTimeStamp;           ///< timestamp of the source picture, unit: millisecond
} SSourcePicture;
//}}}
//{{{
typedef struct TagBitrateInfo {
  LAYER_NUM iLayer;
  int iBitrate;                    ///< the maximum bitrate
} SBitrateInfo;
//}}}
//{{{
typedef struct TagDumpLayer {
  int iLayer;
  char* pFileName;
} SDumpLayer;
//}}}
//{{{
typedef struct TagProfileInfo {
  int iLayer;
  EProfileIdc uiProfileIdc;        ///< the profile info
} SProfileInfo;
//}}}
//{{{
typedef struct TagLevelInfo {
  int iLayer;
  ELevelIdc uiLevelIdc;            ///< the level info
} SLevelInfo;
//}}}
//{{{
typedef struct TagDeliveryStatus {
  bool bDeliveryFlag;              ///< 0: the previous frame isn't delivered,1: the previous frame is delivered
  int iDropFrameType;              ///< the frame type that is dropped; reserved
  int iDropFrameSize;              ///< the frame size that is dropped; reserved
} SDeliveryStatus;
//}}}
//{{{
typedef struct TagDecoderCapability {
  int iProfileIdc;     ///< profile_idc
  int iProfileIop;     ///< profile-iop
  int iLevelIdc;       ///< level_idc
  int iMaxMbps;        ///< max-mbps
  int iMaxFs;          ///< max-fs
  int iMaxCpb;         ///< max-cpb
  int iMaxDpb;         ///< max-dpb
  int iMaxBr;          ///< max-br
  bool bRedPicCap;     ///< redundant-pic-cap
} SDecoderCapability;
//}}}
//{{{
typedef struct TagParserBsInfo {
  int iNalNum;                                 ///< total NAL number in current AU
  int iNalLenInByte [MAX_NAL_UNITS_IN_LAYER];  ///< each nal length
  unsigned char* pDstBuff;                     ///< outputted dst buffer for parsed bitstream
  int iSpsWidthInPixel;                        ///< required SPS width info
  int iSpsHeightInPixel;                       ///< required SPS height info
  unsigned long long uiInBsTimeStamp;               ///< input BS timestamp
  unsigned long long uiOutBsTimeStamp;             ///< output BS timestamp
} SParserBsInfo, *PParserBsInfo;
//}}}
//{{{
typedef struct TagVideoDecoderStatistics {
  unsigned int uiWidth;                        ///< the width of encode/decode frame
  unsigned int uiHeight;                       ///< the height of encode/decode frame

  float fAverageFrameSpeedInMs;                ///< average_Decoding_Time
  float fActualAverageFrameSpeedInMs;          ///< actual average_Decoding_Time, including freezing pictures

  unsigned int uiDecodedFrameCount;            ///< number of frames
  unsigned int uiResolutionChangeTimes;        ///< uiResolutionChangeTimes
  unsigned int uiIDRCorrectNum;                ///< number of correct IDR received

  // EC on related
  unsigned int
  uiAvgEcRatio;                                ///< when EC is on, the average ratio of total EC areas, can be an indicator of reconstruction quality
  unsigned int
  uiAvgEcPropRatio;                            ///< when EC is on, the rough average ratio of propogate EC areas, can be an indicator of reconstruction quality

  unsigned int uiEcIDRNum;                     ///< number of actual unintegrity IDR or not received but eced
  unsigned int uiEcFrameNum;                   ///<
  unsigned int uiIDRLostNum;                   ///< number of whole lost IDR
  unsigned int uiFreezingIDRNum;               ///< number of freezing IDR with error (partly received), under resolution change
  unsigned int uiFreezingNonIDRNum;            ///< number of freezing non-IDR with error

  int iAvgLumaQp;                              ///< average luma QP. default: -1, no correct frame outputted
  int iSpsReportErrorNum;                      ///< number of Sps Invalid report
  int iSubSpsReportErrorNum;                   ///< number of SubSps Invalid report
  int iPpsReportErrorNum;                      ///< number of Pps Invalid report
  int iSpsNoExistNalNum;                       ///< number of Sps NoExist Nal
  int iSubSpsNoExistNalNum;                    ///< number of SubSps NoExist Nal
  int iPpsNoExistNalNum;                       ///< number of Pps NoExist Nal

  } SDecoderStatistics; // in building, coming soon
//}}}
