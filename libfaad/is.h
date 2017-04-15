#pragma once

#include "syntax.h"

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec,
               uint16_t frame_len);

static INLINE int8_t is_intensity(ic_stream *ics, uint8_t group, uint8_t sfb)
{
    switch (ics->sfb_cb[group][sfb])
    {
    case INTENSITY_HCB:
        return 1;
    case INTENSITY_HCB2:
        return -1;
    default:
        return 0;
    }
}

static INLINE int8_t invert_intensity(ic_stream *ics, uint8_t group, uint8_t sfb)
{
    if (ics->ms_mask_present == 1)
        return (1-2*ics->ms_used[group][sfb]);
    return 1;
}
