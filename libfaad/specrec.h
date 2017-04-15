#pragma once

#include "syntax.h"

uint8_t window_grouping_info(NeAACDecStruct *hDecoder, ic_stream *ics);
uint8_t reconstruct_channel_pair(NeAACDecStruct *hDecoder, ic_stream *ics1, ic_stream *ics2,
                                 element *cpe, int16_t *spec_data1, int16_t *spec_data2);
uint8_t reconstruct_single_channel(NeAACDecStruct *hDecoder, ic_stream *ics, element *sce,
                                int16_t *spec_data);

