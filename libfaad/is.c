/*{{{  includes*/
#include "common.h"
#include "structs.h"

#include "syntax.h"
#include "is.h"
/*}}}*/

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec, uint16_t frame_len) {

  uint8_t g, sfb, b;
  uint16_t i;
  real_t scale;

  uint16_t nshort = frame_len/8;
  uint8_t group = 0;

  for (g = 0; g < icsr->num_window_groups; g++) {
    /* Do intensity stereo decoding */
    for (b = 0; b < icsr->window_group_length[g]; b++) {
      for (sfb = 0; sfb < icsr->max_sfb; sfb++) {
        if (is_intensity(icsr, g, sfb)) {
          /* For scalefactor bands coded in intensity stereo the
             corresponding predictors in the right channel are switched to "off". */
          ics->pred.prediction_used[sfb] = 0;
          icsr->pred.prediction_used[sfb] = 0;
          scale = (real_t)pow(0.5, (0.25*icsr->scale_factors[g][sfb]));

          /* Scale from left to right channel,
             do not touch left channel */
          for (i = icsr->swb_offset[sfb]; i < min(icsr->swb_offset[sfb+1], ics->swb_offset_max); i++) {
            r_spec[(group*nshort)+i] = MUL_R(l_spec[(group*nshort)+i], scale);
            if (is_intensity(icsr, g, sfb) != invert_intensity(ics, g, sfb))
              r_spec[(group*nshort)+i] = -r_spec[(group*nshort)+i];
            }
          }
        }
      group++;
      }
    }
  }
