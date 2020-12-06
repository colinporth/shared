#pragma once
/*
 * Normative references:
 *  - ETSI EN 300 775 V1.2.1 (2003-05) (carriage of VBI in DVB)
 */

#include <stdint.h>
#include <stdbool.h>

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

/*****************************************************************************
 * PES data field
 *****************************************************************************/
#define DVBVBI_HEADER_SIZE      1
#define DVBVBI_UNIT_HEADER_SIZE 2
#define DVBVBI_DATA_IDENTIFIER  0x10

#define DVBVBI_ID_TTX_NONSUB    0x02
#define DVBVBI_ID_TTX_SUB       0x03
#define DVBVBI_ID_TTX_INVERTED  0xc0
#define DVBVBI_ID_VPS           0xc3
#define DVBVBI_ID_WSS           0xc4
#define DVBVBI_ID_CEA608        0xc5
#define DVBVBI_ID_RAWVBI        0xc6
#define DVBVBI_ID_STUFFING      0xff

#define DVBVBI_LENGTH           0x2c

/*****************************************************************************
 * Teletext data field
 *****************************************************************************/
#define DVBVBITTX_FRAMING_CODE  0xe4

static inline void dvbvbittx_set_field(uint8_t *p, bool field)
{
    p[0] |= 0xc0;
    if (field)
        p[0] |= 0x20;
    else
        p[0] &= ~0x20;
}

static inline bool dvbvbittx_get_field(const uint8_t *p)
{
    return p[0] & 0x20;
}

static inline void dvbvbittx_set_line(uint8_t *p, uint8_t line)
{
    p[0] &= ~0x1f;
    p[0] |= 0xc0 | (line & 0x1f);
}

static inline uint8_t dvbvbittx_get_line(const uint8_t *p)
{
    return p[0] & 0x1f;
}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
