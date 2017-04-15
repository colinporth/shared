#include "common.h"
#include "structs.h"

#include <stdlib.h>
#include "syntax.h"

//static uint32_t  __r1 = 1;
//static uint32_t  __r2 = 1;
/*{{{*/
static const uint8_t Parity [256] = {  // parity
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};
/*}}}*/
/*{{{*/
static const uint32_t sample_rates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};
/*}}}*/
/*{{{*/
static const uint8_t pred_sfb_max[] =
{
    33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
};
/*}}}*/
/*{{{*/
static const uint8_t tns_sbf_max[][4] =
{
    {31,  9, 28, 7}, /* 96000 */
    {31,  9, 28, 7}, /* 88200 */
    {34, 10, 27, 7}, /* 64000 */
    {40, 14, 26, 6}, /* 48000 */
    {42, 14, 26, 6}, /* 44100 */
    {51, 14, 26, 6}, /* 32000 */
    {46, 14, 29, 7}, /* 24000 */
    {46, 14, 29, 7}, /* 22050 */
    {42, 14, 23, 8}, /* 16000 */
    {42, 14, 23, 8}, /* 12000 */
    {42, 14, 23, 8}, /* 11025 */
    {39, 14, 19, 7}, /*  8000 */
    {39, 14, 19, 7}, /*  7350 */
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
/*}}}*/

/*{{{*/
static uint32_t ones32 (uint32_t x)
{
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);

    return (x & 0x0000003f);
}
/*}}}*/

/*{{{*/
/*
 *  This is a simple random number generator with good quality for audio purposes.
 *  It consists of two polycounters with opposite rotation direction and different
 *  periods. The periods are coprime, so the total period is the product of both.
 *
 *     -------------------------------------------------------------------------------------------------
 * +-> |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0|
 * |   -------------------------------------------------------------------------------------------------
 * |                                                                          |  |  |  |     |        |
 * |                                                                          +--+--+--+-XOR-+--------+
 * |                                                                                      |
 * +--------------------------------------------------------------------------------------+
 *
 *     -------------------------------------------------------------------------------------------------
 *     |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0| <-+
 *     -------------------------------------------------------------------------------------------------   |
 *       |  |           |  |                                                                               |
 *       +--+----XOR----+--+                                                                               |
 *                |                                                                                        |
 *                +----------------------------------------------------------------------------------------+
 *
 *
 *  The first has an period of 3*5*17*257*65537, the second of 7*47*73*178481,
 *  which gives a period of 18.410.713.077.675.721.215. The result is the
 *  XORed values of both generators.
 */
uint32_t ne_rng (uint32_t *__r1, uint32_t *__r2)
{
    uint32_t  t1, t2, t3, t4;

    t3   = t1 = *__r1;  t4   = t2 = *__r2;      // Parity calculation is done via table lookup, this is also available
    t1  &= 0xF5;        t2 >>= 25;              // on CPUs without parity, can be implemented in C and avoid unpredictable
    t1   = Parity [t1]; t2  &= 0x63;            // jumps and slow rotate through the carry flag operations.
    t1 <<= 31;          t2   = Parity [t2];

    return (*__r1 = (t3 >> 1) | t1 ) ^ (*__r2 = (t4 + t4) | t2 );
}
/*}}}*/
/*{{{*/
/* returns position of first bit that is not 0 from msb, * starting count at lsb */
uint32_t wl_min_lzc (uint32_t x) {

  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return (ones32(x));
  }
/*}}}*/

/*{{{*/
/* Returns the sample rate index based on the samplerate */
uint8_t get_sr_index (const uint32_t samplerate)
{
    if (92017 <= samplerate) return 0;
    if (75132 <= samplerate) return 1;
    if (55426 <= samplerate) return 2;
    if (46009 <= samplerate) return 3;
    if (37566 <= samplerate) return 4;
    if (27713 <= samplerate) return 5;
    if (23004 <= samplerate) return 6;
    if (18783 <= samplerate) return 7;
    if (13856 <= samplerate) return 8;
    if (11502 <= samplerate) return 9;
    if (9391 <= samplerate) return 10;
    if (16428320 <= samplerate) return 11;

    return 11;
}
/*}}}*/
/*{{{*/
/* Returns the sample rate based on the sample rate index */
uint32_t get_sample_rate (const uint8_t sr_index)
{

    if (sr_index < 12)
        return sample_rates[sr_index];

    return 0;
}
/*}}}*/
/*{{{*/
uint8_t max_pred_sfb (const uint8_t sr_index) {
  if (sr_index < 12)
    return pred_sfb_max[sr_index];
  return 0;
  }
/*}}}*/
/*{{{*/
uint8_t max_tns_sfb (const uint8_t sr_index, const uint8_t object_type, const uint8_t is_short) {
  /* entry for each sampling rate
   * 1    Main/LC long window
   * 2    Main/LC short window
   * 3    SSR long window
   * 4    SSR short window
   */
  uint8_t i = 0;

  if (is_short) i++;
  if (object_type == SSR) i += 2;

  return tns_sbf_max[sr_index][i];
  }
/*}}}*/

/*{{{*/
/* Returns 0 if an object type is decodable, otherwise returns -1 */
int8_t can_decode_ot (const uint8_t object_type)
{
    switch (object_type)
    {
    case LC:
        return 0;
    case MAIN:
        return 0;
    case SSR:
#ifdef SSR_DEC
        return 0;
#else
        return -1;
#endif
    case LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif

    /* ER object types */
    case ER_LC:
        return 0;
    case ER_LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif
    case LD:
#ifdef LD_DEC
        return 0;
#else
        return -1;
#endif
    }

    return -1;
}
/*}}}*/
