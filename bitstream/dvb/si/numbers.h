#pragma once
#include <stdint.h>   /* uint8_t, uint16_t, etc... */

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

//{{{
static inline unsigned int dvb_bcd_get(const uint8_t *p_bcd, uint8_t i_width)
{
    unsigned int i_result = 0;

    while (i_width >= 8) {
        i_result *= 10;
        i_result += (*p_bcd) >> 4;
        i_result *= 10;
        i_result += (*p_bcd) & 0xf;
        i_width -= 8;
        p_bcd++;
    }

    if (i_width == 4) {
        i_result *= 10;
        i_result += (*p_bcd) >> 4;
    }

    return i_result;
}
//}}}
//{{{
static inline void dvb_bcd_set(uint8_t *p_bcd, unsigned int i_int,
                               uint8_t i_width)
{
    /* calculate 10^(i_width/4-1) */
    unsigned int i_factor = 1, i_exp = i_width / 4 - 1, i_base = 10;
    while (i_exp)
    {
        if (i_exp & 1)
            i_factor *= i_base;
        i_exp >>= 1;
        i_base *= i_base;
    }

    while (i_factor >= 10) {
        *p_bcd = (i_int / i_factor) << 4;
        i_int %= i_factor;
        i_factor /= 10;
        *p_bcd |= (i_int / i_factor);
        i_int %= i_factor;
        i_factor /= 10;
        p_bcd++;
    }

    if (i_factor == 1) {
        *p_bcd &= 0xf;
        *p_bcd |= i_int << 4;
    }
}
//}}}
//{{{
static inline uint8_t dvb_bcd_set8(unsigned int i_int)
{
    uint8_t i_result;
    dvb_bcd_set(&i_result, i_int, 8);
    return i_result;
}
//}}}

//{{{
#ifdef __cplusplus
}
#endif
//}}}
