#pragma once

#include "filtbank.h"

uint8_t is_ltp_ot (uint8_t object_type);

void lt_prediction (ic_stream *ics, ltp_info *ltp, real_t *spec,
                    int16_t *lt_pred_stat, fb_info *fb, uint8_t win_shape,
                    uint8_t win_shape_prev, uint8_t sr_index, uint8_t object_type, uint16_t frame_len);

void lt_update_state (int16_t *lt_pred_stat, real_t *time, real_t *overlap, uint16_t frame_len, uint8_t object_type);
