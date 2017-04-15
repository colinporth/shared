#pragma once

#include "syntax.h"

#define NOISE_OFFSET 90

void pns_decode(ic_stream *ics_left, ic_stream *ics_right,
                real_t *spec_left, real_t *spec_right, uint16_t frame_len,
                uint8_t channel_pair, uint8_t object_type,
                /* RNG states */ uint32_t *__r1, uint32_t *__r2);

static INLINE uint8_t is_noise(ic_stream *ics, uint8_t group, uint8_t sfb)
{
    if (ics->sfb_cb[group][sfb] == NOISE_HCB)
        return 1;
    return 0;
}
