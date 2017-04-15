#pragma once

#define ALPHA      REAL_CONST(0.90625)
#define A          REAL_CONST(0.953125)

void pns_reset_pred_state (ic_stream *ics, pred_state *state);
void reset_all_predictors (pred_state *state, uint16_t frame_len);
void ic_prediction (ic_stream* ics, real_t* spec, pred_state* state, uint16_t frame_len, uint8_t sf_index);
