#pragma once

typedef struct
{
    int8_t index;
    uint8_t len;
    uint32_t cw;
} rvlc_huff_table;


#define ESC_VAL 7


uint8_t rvlc_scale_factor_data(ic_stream *ics, bitfile *ld);
uint8_t rvlc_decode_scale_factors(ic_stream *ics, bitfile *ld);
