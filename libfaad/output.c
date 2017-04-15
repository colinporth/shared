/*{{{  includes*/
#include "common.h"
#include "structs.h"

#include "output.h"
/*}}}*/

#define FLOAT_SCALE (1.0f/(1<<15))
#define DM_MUL REAL_CONST(0.3203772410170407) // 1/(1+sqrt(2) + 1/sqrt(2))
#define RSQRT2 REAL_CONST(0.7071067811865475244) // 1/sqrt(2)

/*{{{*/
static INLINE real_t get_sample(real_t **input, uint8_t channel, uint16_t sample,
                                uint8_t down_matrix, uint8_t *internal_channel)
{
    if (!down_matrix)
        return input[internal_channel[channel]][sample];

    if (channel == 0)
    {
        return DM_MUL * (input[internal_channel[1]][sample] +
            input[internal_channel[0]][sample] * RSQRT2 +
            input[internal_channel[3]][sample] * RSQRT2);
    } else {
        return DM_MUL * (input[internal_channel[2]][sample] +
            input[internal_channel[0]][sample] * RSQRT2 +
            input[internal_channel[4]][sample] * RSQRT2);
    }
}
/*}}}*/

#ifndef HAS_LRINTF
  /*{{{*/
  #define CLIP(sample, max, min) \
  if (sample >= 0.0f)            \
  {                              \
      sample += 0.5f;            \
      if (sample >= max)         \
          sample = max;          \
  } else {                       \
      sample += -0.5f;           \
      if (sample <= min)         \
          sample = min;          \
  }
  /*}}}*/
#else
  /*{{{*/
  #define CLIP(sample, max, min) \
  if (sample >= 0.0f)            \
  {                              \
      if (sample >= max)         \
          sample = max;          \
  } else {                       \
      if (sample <= min)         \
          sample = min;          \
  }
  /*}}}*/
#endif

#define CONV(a,b) ((a<<1)|(b&0x1))

/*{{{*/
static void to_PCM_16bit (NeAACDecStruct* hDecoder, real_t** input,
                          uint8_t channels, uint16_t frame_len, int16_t** sample_buffer) {

  uint8_t ch, ch1;
  uint16_t i;

  switch (CONV(channels,hDecoder->downMatrix)) {
    case CONV(1,0):
    case CONV(1,1):
      for (i = 0; i < frame_len; i++) {
        real_t inp = input[hDecoder->internal_channel[0]][i];
        CLIP(inp, 32767.0f, -32768.0f);
        (*sample_buffer)[i] = (int16_t)lrintf(inp);
        }
      break;

    case CONV(2,0):
      if (hDecoder->upMatrix) {
        ch  = hDecoder->internal_channel[0];
        for (i = 0; i < frame_len; i++) {
          real_t inp0 = input[ch][i];
          CLIP(inp0, 32767.0f, -32768.0f);
          (*sample_buffer)[(i*2)+0] = (int16_t)lrintf(inp0);
          (*sample_buffer)[(i*2)+1] = (int16_t)lrintf(inp0);
          }
        } 
      else {
        ch  = hDecoder->internal_channel[0];
        ch1 = hDecoder->internal_channel[1];
        for (i = 0; i < frame_len; i++) {
          real_t inp0 = input[ch ][i];
          real_t inp1 = input[ch1][i];
          CLIP(inp0, 32767.0f, -32768.0f);
          CLIP(inp1, 32767.0f, -32768.0f);
          (*sample_buffer)[(i*2)+0] = (int16_t)lrintf(inp0);
          (*sample_buffer)[(i*2)+1] = (int16_t)lrintf(inp1);
          }
        }
      break;

    default:
      for (ch = 0; ch < channels; ch++) {
        for(i = 0; i < frame_len; i++) {
          real_t inp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);
          CLIP(inp, 32767.0f, -32768.0f);
          (*sample_buffer)[(i*channels)+ch] = (int16_t)lrintf(inp);
          }
        }
      break;
    }
  }
/*}}}*/

/*{{{*/
void *output_to_PCM (NeAACDecStruct *hDecoder,
                    real_t **input, void *sample_buffer, uint8_t channels, uint16_t frame_len, uint8_t format) {

  int16_t* short_sample_buffer = (int16_t*)sample_buffer;
  to_PCM_16bit (hDecoder, input, channels, frame_len, &short_sample_buffer);

  return sample_buffer;
  }
/*}}}*/
