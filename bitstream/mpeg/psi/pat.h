#pragma once
/*
 * Normative references:
 *  - ISO/IEC 13818-1:2007(E) (MPEG-2 Systems)
 */

#include "../../common.h"
#include "../../mpeg/psi/descriptors.h"

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

/*****************************************************************************
 * Program Association Table
 *****************************************************************************/
#define PAT_PID                 0x0
#define PAT_TABLE_ID            0x0
#define PAT_HEADER_SIZE         PSI_HEADER_SIZE_SYNTAX1
#define PAT_PROGRAM_SIZE        4

#define pat_set_tsid psi_set_tableidext
#define pat_get_tsid psi_get_tableidext

static inline void pat_init(uint8_t *p_pat)
{
    psi_init(p_pat, true);
    psi_set_tableid(p_pat, PAT_TABLE_ID);
    p_pat[1] &= ~0x40;
}

static inline void pat_set_length(uint8_t *p_pat, uint16_t i_pat_length)
{
    psi_set_length(p_pat, PAT_HEADER_SIZE + PSI_CRC_SIZE - PSI_HEADER_SIZE
                    + i_pat_length);
}

static inline void patn_init(uint8_t *p_pat_n)
{
    p_pat_n[2] = 0xe0;
}

static inline void patn_set_program(uint8_t *p_pat_n, uint16_t i_program)
{
    p_pat_n[0] = i_program >> 8;
    p_pat_n[1] = i_program & 0xff;
}

static inline uint16_t patn_get_program(const uint8_t *p_pat_n)
{
    return (p_pat_n[0] << 8) | p_pat_n[1];
}

static inline void patn_set_pid(uint8_t *p_pat_n, uint16_t i_pid)
{
    p_pat_n[2] &= ~0x1f;
    p_pat_n[2] |= i_pid >> 8;
    p_pat_n[3] = i_pid & 0xff;
}

static inline uint16_t patn_get_pid(const uint8_t *p_pat_n)
{
    return ((p_pat_n[2] & 0x1f) << 8) | p_pat_n[3];
}

static inline uint8_t *pat_get_program(uint8_t *p_pat, uint8_t n)
{
    uint8_t *p_pat_n = p_pat + PAT_HEADER_SIZE + n * PAT_PROGRAM_SIZE;
    if (p_pat_n + PAT_PROGRAM_SIZE - p_pat
         > psi_get_length(p_pat) + PSI_HEADER_SIZE - PSI_CRC_SIZE)
        return NULL;
    return p_pat_n;
}

static inline bool pat_validate(const uint8_t *p_pat)
{
    if (!psi_get_syntax(p_pat) || psi_get_tableid(p_pat) != PAT_TABLE_ID)
        return false;
    if ((psi_get_length(p_pat) - PAT_HEADER_SIZE + PSI_HEADER_SIZE
         - PSI_CRC_SIZE) % PAT_PROGRAM_SIZE)
        return false;

    return true;
}

static inline uint8_t *pat_table_find_program(uint8_t **pp_sections,
                                              uint16_t i_program)
{
    uint8_t i_last_section = psi_table_get_lastsection(pp_sections);
    uint8_t i;

    for (i = 0; i <= i_last_section; i++) {
        uint8_t *p_section = psi_table_get_section(pp_sections, i);
        uint8_t *p_program;
        int j = 0;

        while ((p_program = pat_get_program(p_section, j)) != NULL) {
            j++;
            if (patn_get_program(p_program) == i_program)
                return p_program;
        }
    }

    return NULL;
}

static inline bool pat_table_validate(uint8_t **pp_sections)
{
    uint8_t i_last_section = psi_table_get_lastsection(pp_sections);
    uint8_t i;

    for (i = 0; i <= i_last_section; i++) {
        uint8_t *p_section = psi_table_get_section(pp_sections, i);
        uint8_t *p_program;
        int j = 0;

        if (!psi_check_crc(p_section))
            return false;

        while ((p_program = pat_get_program(p_section, j)) != NULL) {
            uint8_t *p_program2 = pat_table_find_program(pp_sections,
                                      patn_get_program(p_program));
            j++;
            /* check that the program number is not already in the table
             * with another PID */
            if (patn_get_pid(p_program) != patn_get_pid(p_program2))
                return false;
        }
    }

    return true;
}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
