#pragma once
/*
 * Normative references:
 *  - ISO/IEC 14496-3 Subpart 4:1998(E)
 *    (Advanced Audio Coding, Time/Frequency Coding)
 *  - ISO/IEC 13818-1:2013(E) (MPEG-2 Systems)
 */
#include <stdint.h>   /* uint8_t, uint16_t, etc... */
#include <stdbool.h>  /* bool */

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

/*****************************************************************************
 * ADTS header
 *****************************************************************************/
#define ADTS_HEADER_SIZE        7
#define ADTS_CRC_SIZE           2
#define ADTS_SAMPLES_PER_BLOCK  1024
#define ADTS_PROFILE_MAIN       0
#define ADTS_PROFILE_LC         1
#define ADTS_PROFILE_SSR        2

/* fixed header */
static inline void adts_set_sync(uint8_t *p_adts)
{
    p_adts[0] = 0xff;
    p_adts[1] = 0xf9; /* protection absent */
    p_adts[2] = 0x0;
    p_adts[3] = 0x0;
    p_adts[4] = 0x0;
    p_adts[5] = 0x0;
    p_adts[6] = 0x0;
}

static inline bool adts_get_protection_absent(const uint8_t *p_adts)
{
    return !!(p_adts[1] & 0x01);
}

static inline void adts_clear_protection_absent(uint8_t *p_adts)
{
    p_adts[1] &= ~0x01;
}

static inline uint8_t adts_get_profile(const uint8_t *p_adts)
{
    return p_adts[2] >> 6;
}

static inline void adts_set_profile(uint8_t *p_adts, uint8_t i_profile)
{
    p_adts[2] &= ~0xc0;
    p_adts[2] |= i_profile << 6;
}

static inline uint8_t adts_get_sampling_freq(const uint8_t *p_adts)
{
    return (p_adts[2] & 0x3c) >> 2;
}

static inline void adts_set_sampling_freq(uint8_t *p_adts, uint8_t i_freq)
{
    p_adts[2] &= ~0x3c;
    p_adts[2] |= (i_freq & 0xf) << 2;
}

static inline uint8_t adts_get_channels(const uint8_t *p_adts)
{
    return ((p_adts[2] & 0x01) << 2) | (p_adts[3] >> 6);
}

static inline void adts_set_channels(uint8_t *p_adts, uint8_t i_channels)
{
    p_adts[2] &= ~0x03; /* also clear out the private bit */
    p_adts[2] |= (i_channels & 0x7) >> 2;
    p_adts[3] &= ~0xc0;
    p_adts[3] |= (i_channels & 0x7) << 6;
}

static inline bool adts_get_copy(const uint8_t *p_adts)
{
    return !!(p_adts[3] & 0x20);
}

static inline void adts_set_copy(uint8_t *p_adts)
{
    p_adts[3] |= 0x20;
}

static inline bool adts_get_home(const uint8_t *p_adts)
{
    return !!(p_adts[3] & 0x10);
}

static inline void adts_set_home(uint8_t *p_adts)
{
    p_adts[3] |= 0x10;
}

/* variable header */
static inline void adts_get_cp_id(const uint8_t *p_adts, bool *pb_bit, bool *pb_start)
{
    *pb_bit = !!(p_adts[3] & 0x08);
    *pb_start = !!(p_adts[3] & 0x04);
}

static inline void adts_set_cp_id(uint8_t *p_adts, bool b_bit, bool b_start)
{
    p_adts[3] &= ~0x0c;
    if (b_bit)
        p_adts[3] |= 0x08;
    if (b_start)
        p_adts[3] |= 0x04;
}

static inline uint16_t adts_get_length(const uint8_t *p_adts)
{
    return ((p_adts[3] & 0x03) << 11) | (p_adts[4] << 3) | (p_adts[5] >> 5);
}

static inline void adts_set_length(uint8_t *p_adts, uint16_t i_length)
{
    p_adts[3] &= ~0x03;
    p_adts[3] |= (i_length >> 11) & 0x03;
    p_adts[4] = i_length >> 3;
    p_adts[5] &= ~0xe0;
    p_adts[5] |= (i_length << 5) & 0xe0;
}

static inline uint16_t adts_get_fullness(const uint8_t *p_adts)
{
    return ((p_adts[5] & 0x1f) << 6) | (p_adts[6] >> 2);
}

static inline void adts_set_fullness(uint8_t *p_adts, uint16_t i_fullness)
{
    p_adts[5] &= ~0x1f;
    p_adts[5] |= (i_fullness >> 6) & 0x1f;
    p_adts[6] &= ~0xfc;
    p_adts[6] |= (i_fullness << 2) & 0xfc;
}

/* blocks == number of blocks - 1 */
static inline uint8_t adts_get_num_blocks(const uint8_t *p_adts)
{
    return (p_adts[6] & 0x03);
}

static inline void adts_set_num_blocks(uint8_t *p_adts, uint8_t i_blocks_min1)
{
    p_adts[6] &= ~0x03;
    p_adts[6] |= i_blocks_min1 & 0x03;
}

static inline bool adts_sync_compare(const uint8_t *p_adts1, const uint8_t *p_adts2)
{
    return p_adts1[0] == p_adts2[0] &&
           p_adts1[1] == p_adts2[1] &&
           p_adts1[2] == p_adts2[2] &&
           (p_adts1[3] & 0xfc) == (p_adts2[3] & 0xfc);
}

/* same but only takes into account meaningful fields */
static inline bool adts_sync_compare_formats(const uint8_t *p_adts1, const uint8_t *p_adts2)
{
    return p_adts1[0] == p_adts2[0] &&
           (p_adts1[1] & 0xfe) == (p_adts2[1] & 0xfe) &&
           p_adts1[2] == p_adts2[2] &&
           (p_adts1[3] & 0xc0) == (p_adts2[3] & 0xc0);
}

/*****************************************************************************
 * LOAS header
 *****************************************************************************/
#define LOAS_HEADER_SIZE        3

static inline void loas_set_sync(uint8_t *p)
{
    p[0] = 0x56;
    p[1] = 0xe0;
    p[2] = 0x0;
}

static inline uint16_t loas_get_length(const uint8_t *p)
{
    return ((p[1] & 0x01f) << 8) | p[2];
}

static inline void loas_set_length(uint8_t *p, uint16_t val)
{
    p[1] = 0xe0 | (val >> 8);
    p[2] = val & 0xff;
}

/*****************************************************************************
 * AudioSpecificConfig
 *****************************************************************************/
#define ASC_TYPE_MAIN           1
#define ASC_TYPE_LC             2
#define ASC_TYPE_SSR            3
#define ASC_TYPE_LTP            4
#define ASC_TYPE_SBR            5
#define ASC_TYPE_SCALABLE       6
#define ASC_TYPE_TWINVQ         7
#define ASC_TYPE_CELP           8
#define ASC_TYPE_HVXC           9
#define ASC_TYPE_ER_LC          17
#define ASC_TYPE_ER_LTP         19
#define ASC_TYPE_ER_SCALABLE    20
#define ASC_TYPE_ER_TWINVQ      21
#define ASC_TYPE_ER_BSAC        22
#define ASC_TYPE_ER_LD          23
#define ASC_TYPE_PS             29

#define ASC_FLT_VARIABLE        0
#define ASC_FLT_FIXED           1
#define ASC_FLT_CELP_2          3
#define ASC_FLT_CELP_FIXED      4
#define ASC_FLT_CELP_4          5
#define ASC_FLT_HVXC_FIXED      6
#define ASC_FLT_HVXC_4          7

/* Returns the profile_and_level structure for MPEG-4 audio descriptor */
static inline uint8_t asc_get_profilelevel(uint8_t asc_aot, uint8_t channels, uint32_t rate)
{
    uint8_t profilelevel;
    switch (asc_aot) {
        case ASC_TYPE_LC:
            profilelevel = 0x50;
            break;

        case ASC_TYPE_SBR:
            profilelevel = 0x58;
            break;

        case ASC_TYPE_ER_LD:
            return 0x38;

        case ASC_TYPE_PS:
            profilelevel = 0x60;
            break;

        default:
            return 0xf;
    }

    if (rate > 48000)
        return profilelevel + 3;
    if (channels > 2)
        return profilelevel + 2;
    if (rate > 24000)
        return profilelevel + 1;
    return profilelevel;
}

/*****************************************************************************
 * AAC syntactic elements
 *****************************************************************************/
#define AAC_SYN_SCE             0
#define AAC_SYN_CPE             1
#define AAC_SYN_CCE             2
#define AAC_SYN_LFE             3
#define AAC_SYN_DSE             4
#define AAC_SYN_PCE             5
#define AAC_SYN_FIL             6
#define AAC_SYN_END             7

//{{{
#ifdef __cplusplus
}
#endif
//}}}
