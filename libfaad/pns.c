/*{{{  includes*/
#include "common.h"
#include "structs.h"

#include "pns.h"
/*}}}*/

/* The function gen_rand_vector(addr, size) generates a vector of length
   <size> with signed random values of average energy MEAN_NRG per random
   value. A suitable random number generator can be realized using one
   multiplication/accumulation per random value.
*/
/*{{{*/
static INLINE void gen_rand_vector(real_t *spec, int16_t scale_factor, uint16_t size,
                                   uint8_t sub,
                                   /* RNG states */ uint32_t *__r1, uint32_t *__r2)
{
    uint16_t i;
    real_t energy = 0.0;

    real_t scale = (real_t)1.0/(real_t)size;

    for (i = 0; i < size; i++)
    {
        real_t tmp = scale*(real_t)(int32_t)ne_rng(__r1, __r2);
        spec[i] = tmp;
        energy += tmp*tmp;
    }

    scale = (real_t)1.0/(real_t)sqrt(energy);
    scale *= (real_t)pow(2.0, 0.25 * scale_factor);
    for (i = 0; i < size; i++)
    {
        spec[i] *= scale;
    }
}
/*}}}*/
/*{{{*/
void pns_decode(ic_stream *ics_left, ic_stream *ics_right,
                real_t *spec_left, real_t *spec_right, uint16_t frame_len,
                uint8_t channel_pair, uint8_t object_type,
                /* RNG states */ uint32_t *__r1, uint32_t *__r2)
{
    uint8_t g, sfb, b;
    uint16_t size, offs;

    uint8_t group = 0;
    uint16_t nshort = frame_len >> 3;

    uint8_t sub = 0;

    for (g = 0; g < ics_left->num_window_groups; g++)
    {
        /* Do perceptual noise substitution decoding */
        for (b = 0; b < ics_left->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < ics_left->max_sfb; sfb++)
            {
                if (is_noise(ics_left, g, sfb))
                {
#ifdef LTP_DEC
                    /* Simultaneous use of LTP and PNS is not prevented in the
                       syntax. If both LTP, and PNS are enabled on the same
                       scalefactor band, PNS takes precedence, and no prediction
                       is applied to this band.
                    */
                    ics_left->ltp.long_used[sfb] = 0;
                    ics_left->ltp2.long_used[sfb] = 0;
#endif

                    /* For scalefactor bands coded using PNS the corresponding
                       predictors are switched to "off".
                    */
                    ics_left->pred.prediction_used[sfb] = 0;
                    offs = ics_left->swb_offset[sfb];
                    size = min(ics_left->swb_offset[sfb+1], ics_left->swb_offset_max) - offs;

                    /* Generate random vector */
                    gen_rand_vector(&spec_left[(group*nshort)+offs],
                        ics_left->scale_factors[g][sfb], size, sub, __r1, __r2);
                }

/* From the spec:
   If the same scalefactor band and group is coded by perceptual noise
   substitution in both channels of a channel pair, the correlation of
   the noise signal can be controlled by means of the ms_used field: While
   the default noise generation process works independently for each channel
   (separate generation of random vectors), the same random vector is used
   for both channels if ms_used[] is set for a particular scalefactor band
   and group. In this case, no M/S stereo coding is carried out (because M/S
   stereo coding and noise substitution coding are mutually exclusive).
   If the same scalefactor band and group is coded by perceptual noise
   substitution in only one channel of a channel pair the setting of ms_used[]
   is not evaluated.
*/
                if (channel_pair)
                {
                    if (is_noise(ics_right, g, sfb))
                    {
                        if (((ics_left->ms_mask_present == 1) &&
                            (ics_left->ms_used[g][sfb])) ||
                            (ics_left->ms_mask_present == 2))
                        {
                            uint16_t c;

                            offs = ics_right->swb_offset[sfb];
                            size = min(ics_right->swb_offset[sfb+1], ics_right->swb_offset_max) - offs;

                            for (c = 0; c < size; c++)
                            {
                                spec_right[(group*nshort) + offs + c] =
                                    spec_left[(group*nshort) + offs + c];
                            }
                        } else /*if (ics_left->ms_mask_present == 0)*/ {
#ifdef LTP_DEC
                            ics_right->ltp.long_used[sfb] = 0;
                            ics_right->ltp2.long_used[sfb] = 0;
#endif
                            ics_right->pred.prediction_used[sfb] = 0;
                            offs = ics_right->swb_offset[sfb];
                            size = min(ics_right->swb_offset[sfb+1], ics_right->swb_offset_max) - offs;

                            /* Generate random vector */
                            gen_rand_vector(&spec_right[(group*nshort)+offs],
                                ics_right->scale_factors[g][sfb], size, sub, __r1, __r2);
                        }
                    }
                }
            } /* sfb */
            group++;
        } /* b */
    } /* g */
}
/*}}}*/
