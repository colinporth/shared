#pragma once
#include "bits.h"

//{{{  defines
/* header types */
#define RAW        0
#define ADIF       1
#define ADTS       2
#define LATM       3

/* SBR signalling */
#define NO_SBR           0
#define SBR_UPSAMPLED    1
#define SBR_DOWNSAMPLED  2
#define NO_SBR_UPSAMPLED 3

/* First object type that has ER */
#define ER_OBJECT_START 17

/* Bitstream */
#define LEN_SE_ID 3
#define LEN_TAG   4
#define LEN_BYTE  8

#define EXT_FIL            0
#define EXT_FILL_DATA      1
#define EXT_DATA_ELEMENT   2
#define EXT_DYNAMIC_RANGE 11
#define ANC_DATA           0

/* Syntax elements */
#define ID_SCE 0x0
#define ID_CPE 0x1
#define ID_CCE 0x2
#define ID_LFE 0x3
#define ID_DSE 0x4
#define ID_PCE 0x5
#define ID_FIL 0x6
#define ID_END 0x7

#define ONLY_LONG_SEQUENCE   0x0
#define LONG_START_SEQUENCE  0x1
#define EIGHT_SHORT_SEQUENCE 0x2
#define LONG_STOP_SEQUENCE   0x3

#define ZERO_HCB       0
#define FIRST_PAIR_HCB 5
#define ESC_HCB        11
#define QUAD_LEN       4
#define PAIR_LEN       2
#define NOISE_HCB      13
#define INTENSITY_HCB2 14
#define INTENSITY_HCB  15

#define INVALID_SBR_ELEMENT 255
//}}}

int8_t GASpecificConfig (bitfile *ld, mp4AudioSpecificConfig *mp4ASC, program_config *pce);

uint8_t adts_frame (adts_header *adts, bitfile *ld);

void get_adif_header (adif_header *adif, bitfile *ld);
void raw_data_block (NeAACDecStruct *hDecoder, NeAACDecFrameInfo *hInfo, bitfile *ld,
                     program_config *pce, drc_info *drc);

uint8_t reordered_spectral_data (NeAACDecStruct *hDecoder, ic_stream *ics, bitfile *ld, int16_t *spectral_data);
uint32_t faad_latm_frame (latm_header *latm, bitfile *ld);
