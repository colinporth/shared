#pragma once
// ETSI EN 300 468 V1.11.1 (2010-04) (SI in DVB systems)
//{{{  includes
#include "../../common.h"
#include "../../mpeg/psi/psi.h"
#include "../../mpeg/psi/descriptors.h"
#include "../../dvb/si/tdt.h"
//}}}
//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

// Time Offset Table
#define TOT_PID                 0x14
#define TOT_TABLE_ID            0x73
#define TOT_HEADER_SIZE         (PSI_HEADER_SIZE + 7)

#define tot_set_utc tdt_set_utc
#define tot_get_utc tdt_get_utc

//{{{
static inline void tot_init(uint8_t *p_tot)
{
    psi_init(p_tot, false);
    psi_set_tableid(p_tot, TOT_TABLE_ID);
    p_tot[8] = 0xf0;
}
//}}}
//{{{
static inline void tot_set_length(uint8_t *p_tot, uint16_t i_tot_length)
{
    psi_set_length(p_tot, TOT_HEADER_SIZE + PSI_CRC_SIZE - PSI_HEADER_SIZE
                    + i_tot_length);
}
//}}}

//{{{
static inline void tot_set_desclength(uint8_t *p_tot, uint16_t i_length)
{
    p_tot[8] &= ~0xf;
    p_tot[8] |= i_length >> 8;
    p_tot[9] = i_length & 0xff;
}
//}}}
//{{{
static inline uint16_t tot_get_desclength(const uint8_t *p_tot)
{
    return ((p_tot[8] & 0xf) << 8) | p_tot[9];
}
//}}}
//{{{
static inline uint8_t *tot_get_descs(uint8_t *p_tot)
{
    return &p_tot[8];
}
//}}}

//{{{
static inline bool tot_validate(const uint8_t *p_tot)
{
    uint16_t i_section_size = psi_get_length(p_tot) + PSI_HEADER_SIZE;
    uint8_t i_tid = psi_get_tableid(p_tot);

    if (psi_get_syntax(p_tot) || i_tid != TOT_TABLE_ID
         || i_section_size < TOT_HEADER_SIZE + PSI_CRC_SIZE)
        return false;

    /* TOT is a syntax0 table with CRC */
    if (!psi_check_crc(p_tot))
        return false;

    if (i_section_size < TOT_HEADER_SIZE
         || i_section_size < TOT_HEADER_SIZE + tot_get_desclength(p_tot))
        return false;

    if (!descs_validate(p_tot + 8))
        return false;

    return true;
}
//}}}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
