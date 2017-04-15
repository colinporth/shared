#pragma once

#include "neaacdec.h"
#include "bits.h"

int8_t AudioSpecificConfig2 (uint8_t *pBuffer, uint32_t buffer_size,
                             mp4AudioSpecificConfig* mp4ASC, program_config* pce, uint8_t short_form);

int8_t AudioSpecificConfigFromBitfile (bitfile* ld, mp4AudioSpecificConfig* mp4ASC,
                                       program_config* pce, uint32_t bsize, uint8_t short_form);
