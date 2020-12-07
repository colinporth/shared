#pragma once
//Normative references:
//  - ETSI EN 300 468 V1.11.1 (2010-04) (SI in DVB systems)
//{{{  includes
#include "../../common.h"
#include "../../mpeg/psi/psi.h"
#include "../../mpeg/psi/descriptors.h"
#include "../../dvb/si/nit.h"
//}}}
//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

#define BAT_PID                 0x11
#define BAT_TABLE_ID            0x4a
#define BAT_HEADER_SIZE         (PSI_HEADER_SIZE_SYNTAX1 + 2)
#define BAT_HEADER2_SIZE        2
#define BAT_TS_SIZE             6

#define bat_set_bouquet_id psi_set_tableidext
#define bat_get_bouquet_id psi_get_tableidext

//{{{
static inline void bat_init(uint8_t *p_bat)
{
    psi_init(p_bat, true);
    psi_set_tableid(p_bat, BAT_TABLE_ID);
    p_bat[8] = 0xf0;
}
//}}}

#define bat_set_length      nit_set_length
#define bat_set_desclength  nit_set_desclength
#define bat_get_desclength  nit_get_desclength
#define bat_get_descs       nit_get_descs
#define bath_init           nith_init
#define bath_set_tslength   nith_set_tslength
#define bath_get_tslength   nith_get_tslength
#define bat_get_header2     nit_get_header2
#define batn_init           nitn_init
#define batn_set_tsid       nitn_set_tsid
#define batn_get_tsid       nitn_get_tsid
#define batn_set_onid       nitn_set_onid
#define batn_get_onid       nitn_get_onid
#define batn_set_desclength nitn_set_desclength
#define batn_get_desclength nitn_get_desclength
#define batn_get_descs      nitn_get_descs
#define bat_get_ts          nit_get_ts
#define bat_validate_ts     nit_validate_ts
#define bat_table_find_ts   nit_table_find_ts
#define bat_table_validate  nit_table_validate

static inline bool bat_validate(const uint8_t *p_bat)
{
    uint16_t i_section_size = psi_get_length(p_bat) + PSI_HEADER_SIZE
                               - PSI_CRC_SIZE;
    const uint8_t *p_bat_n;

    if (!psi_get_syntax(p_bat) || psi_get_tableid(p_bat) != BAT_TABLE_ID)
        return false;

    if (i_section_size < BAT_HEADER_SIZE
         || i_section_size < BAT_HEADER_SIZE + bat_get_desclength(p_bat))
        return false;

    if (!descs_validate(p_bat + 8))
        return false;

    p_bat_n = p_bat + BAT_HEADER_SIZE + bat_get_desclength(p_bat);

    if (bath_get_tslength(p_bat_n) != p_bat + i_section_size - p_bat_n
         - BAT_HEADER2_SIZE)
        return false;
    p_bat_n += BAT_HEADER2_SIZE;

    while (p_bat_n + BAT_TS_SIZE - p_bat <= i_section_size
            && p_bat_n + BAT_TS_SIZE + batn_get_desclength(p_bat_n) - p_bat
                <= i_section_size) {
        if (!descs_validate(p_bat_n + 4))
            return false;

        p_bat_n += BAT_TS_SIZE + batn_get_desclength(p_bat_n);
    }

    return (p_bat_n - p_bat == i_section_size);
}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
