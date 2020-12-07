#pragma once
// ETSI EN 300 468 V1.11.1 (2010-04) (SI in DVB systems)

//{{{  includes
#include "../../common.h"
#include "../../mpeg/psi/psi.h"
#include "../../mpeg/psi/descriptors.h"
//}}}

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

// Time and Date Table
#define TDT_PID                 0x14
#define TDT_TABLE_ID            0x70
#define TDT_HEADER_SIZE         (PSI_HEADER_SIZE + 5)

static inline void tdt_init(uint8_t *p_tdt)
{
    psi_init(p_tdt, false);
    psi_set_tableid(p_tdt, TDT_TABLE_ID);
    psi_set_length(p_tdt, TDT_HEADER_SIZE - PSI_HEADER_SIZE);
}

static inline void tdt_set_utc(uint8_t *p_tdt, uint64_t i_utc)
{
    p_tdt[3] = (i_utc >> 32) & 0xff;
    p_tdt[4] = (i_utc >> 24) & 0xff;
    p_tdt[5] = (i_utc >> 16) & 0xff;
    p_tdt[6] = (i_utc >> 8) & 0xff;
    p_tdt[7] = i_utc & 0xff;
}

static inline uint64_t tdt_get_utc(const uint8_t *p_tdt)
{
    return (uint64_t)(((uint64_t)p_tdt[3] << 32) | ((uint64_t)p_tdt[4] << 24) |
                      ((uint64_t)p_tdt[5] << 16) | ((uint64_t)p_tdt[6] << 8) | p_tdt[7]);
}

static inline bool tdt_validate(const uint8_t *p_tdt)
{
    uint16_t i_section_size = psi_get_length(p_tdt) + PSI_HEADER_SIZE;
    uint8_t i_tid = psi_get_tableid(p_tdt);

    if (psi_get_syntax(p_tdt) || i_tid != TDT_TABLE_ID
         || i_section_size < TDT_HEADER_SIZE)
        return false;

    return true;
}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
