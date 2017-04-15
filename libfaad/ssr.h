#pragma once

#define SSR_BANDS 4
#define PQFTAPS 96

void ssr_decode(ssr_info *ssr, fb_info *fb, uint8_t window_sequence,
                uint8_t window_shape, uint8_t window_shape_prev,
                real_t *freq_in, real_t *time_out, real_t *overlap,
                real_t ipqf_buffer[SSR_BANDS][96/4],
                real_t *prev_fmd, uint16_t frame_len);


static void ssr_gain_control(ssr_info *ssr, real_t *data, real_t *output,
                             real_t *overlap, real_t *prev_fmd, uint8_t band,
                             uint8_t window_sequence, uint16_t frame_len);
static void ssr_gc_function(ssr_info *ssr, real_t *prev_fmd,
                            real_t *gc_function, uint8_t window_sequence,
                            uint16_t frame_len);


void ssr_ipqf(ssr_info *ssr, real_t *in_data, real_t *out_data,
              real_t buffer[SSR_BANDS][96/4],
              uint16_t frame_len, uint8_t bands);
fb_info *ssr_filter_bank_init(uint16_t frame_len);
void ssr_filter_bank_end(fb_info *fb);

/*non overlapping inverse filterbank */
void ssr_ifilter_bank(fb_info *fb,
                      uint8_t window_sequence,
                      uint8_t window_shape,
                      uint8_t window_shape_prev,
                      real_t *freq_in,
                      real_t *time_out,
                      uint16_t frame_len);
