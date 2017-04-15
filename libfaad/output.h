#pragma once

void* output_to_PCM (NeAACDecStruct *hDecoder, real_t **input,
                    void *samplebuffer, uint8_t channels, uint16_t frame_len, uint8_t format);

