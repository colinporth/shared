#pragma once

#define BYTE_NUMBIT_LD  3
#define bit2byte(a) ((a+7)>>BYTE_NUMBIT_LD)

//{{{
typedef struct _bitfile
{
    /* bit input */
    uint32_t bufa;
    uint32_t bufb;
    uint32_t bits_left;
    uint32_t buffer_size; /* size of the buffer in bytes */
    uint32_t bytes_left;
    uint8_t error;
    uint32_t *tail;
    uint32_t *start;
    const void *buffer;
} bitfile;
//}}}

void faad_initbits (bitfile* ld, const void* buffer, const uint32_t buffer_size);
void faad_initbits_rev (bitfile* ld, void* buffer, uint32_t bits_in_buffer);
void faad_endbits (bitfile* ld);

uint32_t faad_get_processed_bits (bitfile* ld);
uint8_t faad_byte_align (bitfile* ld);
void faad_flushbits_ex (bitfile* ld, uint32_t bits);

void faad_rewindbits (bitfile* ld);
void faad_resetbits (bitfile* ld, int bits);

uint8_t* faad_getbitbuffer (bitfile* ld, uint32_t bits);

//{{{
/* circumvent memory alignment errors on ARM */
static INLINE uint32_t getdword (void* mem) {

  uint32_t tmp;
#ifndef ARCH_IS_BIG_ENDIAN
  ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[3];
  ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[2];
  ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[1];
  ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[0];
#else
  ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[0];
  ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[1];
  ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[2];
  ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[3];
#endif

  return tmp;
  }
//}}}
//{{{
/* reads only n bytes from the stream instead of the standard 4 */
static INLINE uint32_t getdword_n (void* mem, int n) {

  uint32_t tmp = 0;
#ifndef ARCH_IS_BIG_ENDIAN
  switch (n) {
    case 3:
      ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[2];
    case 2:
      ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[1];
    case 1:
      ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[0];
    default:
      break;
    }
#else
  switch (n) {
    case 3:
      ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[2];
    case 2:
      ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[1];
    case 1:
       ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[0];
    default:
      break;
    }
#endif

  return tmp;
  }
//}}}
//{{{
static INLINE uint32_t faad_showbits (bitfile* ld, uint32_t bits) {

  if (bits <= ld->bits_left)
    //return (ld->bufa >> (ld->bits_left - bits)) & bitmask[bits];
    return (ld->bufa << (32 - ld->bits_left)) >> (32 - bits);

  bits -= ld->bits_left;
  //return ((ld->bufa & bitmask[ld->bits_left]) << bits) | (ld->bufb >> (32 - bits));
  return ((ld->bufa & ((1<<ld->bits_left)-1)) << bits) | (ld->bufb >> (32 - bits));
  }
//}}}
//{{{
static INLINE void faad_flushbits (bitfile* ld, uint32_t bits) {

  /* do nothing if error */
  if (ld->error != 0)
    return;

  if (bits < ld->bits_left)
    ld->bits_left -= bits;
  else
    faad_flushbits_ex(ld, bits);
  }
//}}}
//{{{
/* return next n bits (right adjusted) */
static INLINE uint32_t faad_getbits (bitfile* ld, uint32_t n) {

  if (n == 0)
    return 0;

  uint32_t ret = faad_showbits (ld, n);
  faad_flushbits (ld, n);
  return ret;
  }
//}}}
//{{{
static INLINE uint8_t faad_get1bit (bitfile* ld) {

  uint8_t r;
  if (ld->bits_left > 0) {
    ld->bits_left--;
    r = (uint8_t)((ld->bufa >> ld->bits_left) & 1);
    return r;
    }

  /* bits_left == 0 */
#if 0
  r = (uint8_t)(ld->bufb >> 31);
  faad_flushbits_ex (ld, 1);
#else
  r = (uint8_t)faad_getbits (ld, 1);
#endif
  return r;
  }
//}}}
//{{{
/* reversed bitreading routines */
static INLINE uint32_t faad_showbits_rev (bitfile* ld, uint32_t bits) {

  uint8_t i;
  uint32_t B = 0;

  if (bits <= ld->bits_left) {
    for (i = 0; i < bits; i++) {
      if (ld->bufa & (1 << (i + (32 - ld->bits_left))))
        B |= (1 << (bits - i - 1));
      }
    }

  else {
    for (i = 0; i < ld->bits_left; i++) {
      if (ld->bufa & (1 << (i + (32 - ld->bits_left))))
        B |= (1 << (bits - i - 1));
      }
    for (i = 0; i < bits - ld->bits_left; i++) {
       if (ld->bufb & (1 << (i + (32-ld->bits_left))))
          B |= (1 << (bits - ld->bits_left - i - 1));
      }
    }

  return B;
  }
//}}}
//{{{
static INLINE void faad_flushbits_rev (bitfile* ld, uint32_t bits) {

  /* do nothing if error */
  if (ld->error != 0)
    return;

  if (bits < ld->bits_left)
    ld->bits_left -= bits;
  else {
    uint32_t tmp;

    ld->bufa = ld->bufb;
    tmp = getdword(ld->start);
    ld->bufb = tmp;
    ld->start--;
    ld->bits_left += (32 - bits);

    if (ld->bytes_left < 4) {
      ld->error = 1;
      ld->bytes_left = 0;
      }
    else
      ld->bytes_left -= 4;
    // if (ld->bytes_left == 0)
    //   ld->no_more_reading = 1;
    }
  }
//}}}
//{{{
static INLINE uint32_t faad_getbits_rev (bitfile* ld, uint32_t n) {

  if (n == 0)
   return 0;

  uint32_t ret = faad_showbits_rev(ld, n);
  faad_flushbits_rev(ld, n);
  return ret;
  }
//}}}

//{{{  struct bits_t
typedef struct
{
    /* bit input */
    uint32_t bufa;
    uint32_t bufb;
    int8_t len;
} bits_t;
//}}}
//{{{
static INLINE uint32_t showbits_hcr (bits_t* ld, uint8_t bits) {

  if (bits == 0)
    return 0;

  if (ld->len <= 32) {
    /* huffman_spectral_data_2 needs to read more than may be available, bits maybe > ld->len, deliver 0 than */
    if (ld->len >= bits)
      return ((ld->bufa >> (ld->len - bits)) & (0xFFFFFFFF >> (32 - bits)));
    else
      return ((ld->bufa << (bits - ld->len)) & (0xFFFFFFFF >> (32 - bits)));
    }
  else {
    if ((ld->len - bits) < 32)
      return ( (ld->bufb & (0xFFFFFFFF >> (64 - ld->len))) << (bits - ld->len + 32)) | (ld->bufa >> (ld->len - bits));
    else
      return ((ld->bufb >> (ld->len - bits - 32)) & (0xFFFFFFFF >> (32 - bits)));
    }
  }
//}}}
//{{{
/* return 1 if position is outside of buffer, 0 otherwise */
static INLINE int8_t flushbits_hcr ( bits_t* ld, uint8_t bits) {

  ld->len -= bits;
  if (ld->len < 0) {
    ld->len = 0;
    return 1;
    }
  else
    return 0;
  }
//}}}
//{{{
static INLINE int8_t getbits_hcr (bits_t* ld, uint8_t n, uint32_t *result) {

  *result = showbits_hcr (ld, n);
  return flushbits_hcr (ld, n);
  }
//}}}
//{{{
static INLINE int8_t get1bit_hcr (bits_t* ld, uint8_t *result) {

  uint32_t res;
  int8_t ret = getbits_hcr(ld, 1, &res);
  *result = (int8_t)(res & 1);
  return ret;
  }
//}}}
