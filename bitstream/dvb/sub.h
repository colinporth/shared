#pragma once
/*
 * Normative references:
 *  - ETSI EN 300 743 V1.5.1 (2014-01) (Subtitling Systems)
 */

#include <stdint.h>

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

/*****************************************************************************
 * PES data field
 *****************************************************************************/
#define DVBSUB_HEADER_SIZE      2
#define DVBSUB_DATA_IDENTIFIER  0x20

/*****************************************************************************
 * Subtitling segment
 *****************************************************************************/
#define DVBSUBS_HEADER_SIZE     6
#define DVBSUBS_SYNC            0xf

#define DVBSUBS_PAGE_COMPOSITION        0x10
#define DVBSUBS_REGION_COMPOSITION      0x11
#define DVBSUBS_CLUT_DEFINITION         0x12
#define DVBSUBS_OBJECT_DATA             0x13
#define DVBSUBS_DISPLAY_DEFINITION      0x14
#define DVBSUBS_DISPARITY_SIGNALLING    0x15
#define DVBSUBS_END_OF_DISPLAY_SET      0x80

static inline uint8_t dvbsubs_get_type(const uint8_t *p_dvbsubs)
{
    return p_dvbsubs[1];
}

static inline uint16_t dvbsubs_get_page(const uint8_t *p_dvbsubs)
{
    return (p_dvbsubs[2] << 8) | p_dvbsubs[3];
}

static inline uint16_t dvbsubs_get_length(const uint8_t *p_dvbsubs)
{
    return (p_dvbsubs[4] << 8) | p_dvbsubs[5];
}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
