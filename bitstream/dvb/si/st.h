#pragma once
/*
 * Normative references:
 *  - ETSI EN 300 468 V1.11.1 (2010-04) (SI in DVB systems)
 */

#include "../../common.h"
#include "../../mpeg/psi/psi.h"
#include "../../mpeg/psi/descriptors.h"

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

/*****************************************************************************
 * Stuffing Table
 *****************************************************************************/
#define ST_TABLE_ID            0x72
#define ST_HEADER_SIZE         PSI_HEADER_SIZE

static inline void st_init(uint8_t *p_st)
{
    psi_init(p_st, false);
    psi_set_tableid(p_st, ST_TABLE_ID);
    psi_set_length(p_st, 0);
}

static inline bool st_validate(const uint8_t *p_st)
{
    if (psi_get_tableid(p_st) != ST_TABLE_ID)
        return false;

    return true;
}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
