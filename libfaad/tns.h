#pragma once

#define TNS_MAX_ORDER 20

void tns_decode_frame (ic_stream *ics, tns_info *tns, uint8_t sr_index,
                       uint8_t object_type, real_t *spec, uint16_t frame_len);
void tns_encode_frame (ic_stream *ics, tns_info *tns, uint8_t sr_index,
                       uint8_t object_type, real_t *spec, uint16_t frame_len);
