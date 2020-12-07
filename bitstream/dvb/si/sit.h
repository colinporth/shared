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

// Selection Information Table
#define SIT_PID                 0x1f
#define SIT_TABLE_ID            0x7f
#define SIT_HEADER_SIZE         (PSI_HEADER_SIZE_SYNTAX1 + 2)
#define SIT_SERVICE_SIZE        4

//{{{
static inline void sit_init(uint8_t *p_sit)
{
    psi_init(p_sit, true);
    psi_set_tableid(p_sit, SIT_TABLE_ID);
    p_sit[1] |= 0xe0;
    psi_set_tableidext(p_sit, 0xffff);
    psi_set_section(p_sit, 0);
    psi_set_lastsection(p_sit, 0);
}
//}}}
//{{{
static inline void sit_set_length(uint8_t *p_sit, uint16_t i_sit_length)
{
    psi_set_length(p_sit, SIT_HEADER_SIZE + PSI_CRC_SIZE - PSI_HEADER_SIZE
                    + i_sit_length);
}
//}}}

//{{{
static inline uint16_t sit_get_desclength(const uint8_t *p_sit)
{
    return ((p_sit[8] & 0x0f) << 8) | p_sit[9];
}
//}}}
//{{{
static inline void sit_set_desclength(uint8_t *p_sit, uint16_t i_length)
{
    p_sit[8] = ((i_length >> 8) & 0xff) | 0xf0;
    p_sit[9] =  i_length        & 0xff;
}
//}}}

//{{{
static inline uint8_t *sit_get_descs(uint8_t *p_sit)
{
    return &p_sit[8];
}
//}}}

//{{{
static inline void sitn_init(uint8_t *p_sit_n)
{
    p_sit_n[2] = 0x80;
}
//}}}

//{{{
static inline uint16_t sitn_get_sid(const uint8_t *p_sit_n)
{
    return (p_sit_n[0] << 8) | p_sit_n[1];
}
//}}}
//{{{
static inline void sitn_set_sid(uint8_t *p_sit_n, uint16_t i_sid)
{
    p_sit_n[0] = (i_sid >> 8) & 0xff;
    p_sit_n[1] =  i_sid       & 0xff;
}
//}}}

//{{{
static inline uint8_t sitn_get_running(const uint8_t *p_sit_n)
{
    return (p_sit_n[2] & 0x70) >> 4;
}
//}}}
//{{{
static inline void sitn_set_running(uint8_t *p_sit_n, uint8_t i_running_status)
{
    p_sit_n[2] = (p_sit_n[2] &~ 0x70) | ((i_running_status & 0x07) << 4);
}
//}}}

//{{{
static inline void sitn_set_desclength(uint8_t *p_sit_n, uint16_t i_length)
{
    p_sit_n[2] = ((i_length >> 8) & 0xff) | (p_sit_n[2] & 0xf0);
    p_sit_n[3] =  i_length        & 0xff;
}
//}}}
//{{{
static inline uint16_t sitn_get_desclength(const uint8_t *p_sit_n)
{
    return ((p_sit_n[2] & 0xf) << 8) | p_sit_n[3];
}
//}}}

//{{{
static inline uint8_t *sitn_get_descs(uint8_t *p_sit_n)
{
    return &p_sit_n[2];
}
//}}}

//{{{
static inline uint8_t *sit_get_service(uint8_t *p_sit, uint8_t n)
{
    uint16_t i_section_size = psi_get_length(p_sit) + PSI_HEADER_SIZE
                               - PSI_CRC_SIZE;
    uint8_t *p_sit_n = p_sit + SIT_HEADER_SIZE + sit_get_desclength(p_sit);
    if (p_sit_n - p_sit > i_section_size) return NULL;

    while (n) {
        if (p_sit_n + SIT_SERVICE_SIZE - p_sit > i_section_size) return NULL;
        p_sit_n += SIT_SERVICE_SIZE + sitn_get_desclength(p_sit_n);
        n--;
    }
    if (p_sit_n - p_sit >= i_section_size) return NULL;
    return p_sit_n;
}
//}}}
//{{{
static inline bool sit_validate_service(const uint8_t *p_sit, const uint8_t *p_sit_n,
                                   uint16_t i_desclength)
{
    uint16_t i_section_size = psi_get_length(p_sit) + PSI_HEADER_SIZE
                               - PSI_CRC_SIZE;
    return (p_sit_n + SIT_SERVICE_SIZE + i_desclength
             <= p_sit + i_section_size);
}
//}}}

//{{{
static inline bool sit_validate(const uint8_t *p_sit)
{
    uint16_t i_section_size = psi_get_length(p_sit) + PSI_HEADER_SIZE
                               - PSI_CRC_SIZE;
    const uint8_t *p_sit_n;

    if (!psi_get_syntax(p_sit) || psi_get_section(p_sit)
         || psi_get_lastsection(p_sit)
         || psi_get_tableid(p_sit) != SIT_TABLE_ID)
        return false;

    if (!psi_check_crc(p_sit))
        return false;

    if (i_section_size < SIT_HEADER_SIZE
         || i_section_size < SIT_HEADER_SIZE + sit_get_desclength(p_sit))
        return false;

    if (!descs_validate(sit_get_descs((uint8_t *)p_sit)))
        return false;

    p_sit_n = p_sit + SIT_HEADER_SIZE + sit_get_desclength(p_sit);

    while (p_sit_n + SIT_SERVICE_SIZE - p_sit <= i_section_size
            && p_sit_n + SIT_SERVICE_SIZE + sitn_get_desclength(p_sit_n) - p_sit
                <= i_section_size) {
        if (!descs_validate(sitn_get_descs((uint8_t *)p_sit_n)))
            return false;

        p_sit_n += SIT_SERVICE_SIZE + sitn_get_desclength(p_sit_n);
    }

    return (p_sit_n - p_sit == i_section_size);
}
//}}}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
