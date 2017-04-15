// huffman.c - Huffman decoding
//{{{  includes
#include "huffman.h"
#include "aaccommon.h"
#include "cBitStream.h"
#include "tables.h"
//}}}
//{{{  defines
#define HUFFTAB_SPEC_OFFSET             1
#define APPLY_SIGN(v, s)    {(v) ^= ((signed int)(s) >> 31); (v) -= ((signed int)(s) >> 31);}

#define GET_QUAD_SIGNBITS(v)  (((unsigned int)(v) << 17) >> 29) /* bits 14-12, unsigned */
#define GET_QUAD_W(v)     (((signed int)(v) << 20) >>   29) /* bits 11-9, sign-extend */
#define GET_QUAD_X(v)     (((signed int)(v) << 23) >>   29) /* bits  8-6, sign-extend */
#define GET_QUAD_Y(v)     (((signed int)(v) << 26) >>   29) /* bits  5-3, sign-extend */
#define GET_QUAD_Z(v)     (((signed int)(v) << 29) >>   29) /* bits  2-0, sign-extend */

#define GET_PAIR_SIGNBITS(v)  (((unsigned int)(v) << 20) >> 30) /* bits 11-10, unsigned */
#define GET_PAIR_Y(v)     (((signed int)(v) << 22) >>   27) /* bits  9-5, sign-extend */
#define GET_PAIR_Z(v)     (((signed int)(v) << 27) >>   27) /* bits  4-0, sign-extend */

#define GET_ESC_SIGNBITS(v)   (((unsigned int)(v) << 18) >> 30) /* bits 13-12, unsigned */
#define GET_ESC_Y(v)      (((signed int)(v) << 20) >>   26) /* bits 11-6, sign-extend */
#define GET_ESC_Z(v)      (((signed int)(v) << 26) >>   26) /* bits  5-0, sign-extend */
//}}}

//{{{
void UnpackZeros (int nVals, int* coef) {

  while (nVals > 0) {
    *coef++ = 0;
    *coef++ = 0;
    *coef++ = 0;
    *coef++ = 0;
    nVals -= 4;
    }
  }
//}}}
//{{{
void UnpackQuads (cBitStream& bitStream, int cb, int nVals, int* coef) {


  int maxBits = huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET].maxBits + 4;
  while (nVals > 0) {
    /* decode quad */
    unsigned int bitBuf = bitStream.GetBitsNoAdvance (maxBits) << (32 - maxBits);

    int val;
    int nCodeBits = DecodeHuffmanScalar (huffTabSpec, &huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET], bitBuf, &val);
    int w = GET_QUAD_W(val);
    int x = GET_QUAD_X(val);
    int y = GET_QUAD_Y(val);
    int z = GET_QUAD_Z(val);

    bitBuf <<= nCodeBits;
    int nSignBits = (int)GET_QUAD_SIGNBITS(val);
    bitStream.AdvanceBitstream (nCodeBits + nSignBits);
    if (nSignBits) {
      if (w)  {APPLY_SIGN(w, bitBuf); bitBuf <<= 1;}
      if (x)  {APPLY_SIGN(x, bitBuf); bitBuf <<= 1;}
      if (y)  {APPLY_SIGN(y, bitBuf); bitBuf <<= 1;}
      if (z)  {APPLY_SIGN(z, bitBuf); bitBuf <<= 1;}
      }
    *coef++ = w; *coef++ = x; *coef++ = y; *coef++ = z;
    nVals -= 4;
    }
  }
//}}}
//{{{
void UnpackPairsNoEsc (cBitStream& bitStream, int cb, int nVals, int* coef) {

  int maxBits = huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET].maxBits + 2;
  while (nVals > 0) {
    /* decode pair */
    unsigned int bitBuf = bitStream.GetBitsNoAdvance(maxBits) << (32 - maxBits);

    int val;
    int nCodeBits = DecodeHuffmanScalar(huffTabSpec, &huffTabSpecInfo[cb-HUFFTAB_SPEC_OFFSET], bitBuf, &val);
    int y = GET_PAIR_Y(val);
    int z = GET_PAIR_Z(val);

    bitBuf <<= nCodeBits;
    int nSignBits = GET_PAIR_SIGNBITS(val);
    bitStream.AdvanceBitstream (nCodeBits + nSignBits);
    if (nSignBits) {
      if (y)  {
        APPLY_SIGN(y, bitBuf); 
        bitBuf <<= 1;
        }
      if (z)  {
        APPLY_SIGN(z, bitBuf); 
        bitBuf <<= 1;
        }
      }

    *coef++ = y; *coef++ = z;
    nVals -= 2;
    }
  }
//}}}
//{{{
void UnpackPairsEsc (cBitStream& bitStream, int cb, int nVals, int* coef) {

  int maxBits = huffTabSpecInfo[cb - HUFFTAB_SPEC_OFFSET].maxBits + 2;
  while (nVals > 0) {
    /* decode pair with escape value */
    unsigned int bitBuf = bitStream.GetBitsNoAdvance (maxBits) << (32 - maxBits);

    int val;
    int nCodeBits = DecodeHuffmanScalar (huffTabSpec, &huffTabSpecInfo[cb-HUFFTAB_SPEC_OFFSET], bitBuf, &val);
    int y = GET_ESC_Y(val);
    int z = GET_ESC_Z(val);

    bitBuf <<= nCodeBits;
    int nSignBits = GET_ESC_SIGNBITS(val);
    bitStream.AdvanceBitstream (nCodeBits + nSignBits);

    int n;
    if (y == 16) {
      n = 4;
      while (bitStream.GetBits(1) == 1)
        n++;
      y = (1 << n) + bitStream.GetBits(n);
      }
    if (z == 16) {
      n = 4;
      while (bitStream.GetBits(1) == 1)
        n++;
      z = (1 << n) + bitStream.GetBits(n);
      }

    if (nSignBits) {
      if (y)  {APPLY_SIGN(y, bitBuf); bitBuf <<= 1;}
      if (z)  {APPLY_SIGN(z, bitBuf); bitBuf <<= 1;}
      }

    *coef++ = y; *coef++ = z;
    nVals -= 2;
    }
  }
//}}}

//{{{
int DecodeHuffmanScalar (const signed short* huffTab, const HuffInfo* huffTabInfo, unsigned int bitBuf, signed int* val) {

  const signed short* map = huffTab + huffTabInfo->offset;
  const unsigned char* countPtr = huffTabInfo->count;

  unsigned int start = 0;
  unsigned int count = 0;
  unsigned int shift = 32;

  unsigned int t;
  do {
    start += count;
    start <<= 1;
    map += count;
    count = *countPtr++;
    shift--;
    t = (bitBuf >> shift) - start;
    } while (t >= count);

  *val = (signed int)map[t];
  return (int)(countPtr - huffTabInfo->count);
  }
//}}}
//{{{
void DecodeSectionData (cBitStream& bitStream, int winSequence, int numWinGrp, int maxSFB, unsigned char* sfbCodeBook) {
 // Description: decode section data (scale factor band groupings and associated Huffman codebooks)
 // Inputs:      bitStream struct pointing to start of ICS info (14496-3, table 4.4.25)
 //              window sequence (short or long blocks)
 //              number of window groups (1 for long blocks, 1-8 for short blocks)
 //              max coded scalefactor band
 // Outputs:     index of Huffman codebook for each scalefactor band in each section

  int sectLenBits = (winSequence == 2 ? 3 : 5);
  int sectEscapeVal = (1 << sectLenBits) - 1;

  for (int g = 0; g < numWinGrp; g++) {
    int sfb = 0;
    while (sfb < maxSFB) {
      int cb = bitStream.GetBits(4); /* next section codebook */
      int sectLen = 0;
      int sectLenIncr;
      do {
        sectLenIncr = bitStream.GetBits (sectLenBits);
        sectLen += sectLenIncr;
        } while (sectLenIncr == sectEscapeVal);

      sfb += sectLen;
      while (sectLen--)
        *sfbCodeBook++ = (unsigned char)cb;
      }
    }
  }
//}}}
//{{{
int DecodeOneSymbol (cBitStream* bitStream, int huffTabIndex) {

  const HuffInfo* hi = &(huffTabSBRInfo[huffTabIndex]);
  unsigned int bitBuf = bitStream->GetBitsNoAdvance (hi->maxBits) << (32 - hi->maxBits);

  int val;
  int nBits = DecodeHuffmanScalar (huffTabSBR, hi, bitBuf, &val);
  bitStream->AdvanceBitstream (nBits);

  return val;
  }
//}}}

//{{{
int DecodeOneScaleFactor (cBitStream& bitStream) {

  /* decode next scalefactor from bitstream */
  unsigned int bitBuf = bitStream.GetBitsNoAdvance (huffTabScaleFactInfo.maxBits) << (32 - huffTabScaleFactInfo.maxBits);

  int val;
  int nBits = DecodeHuffmanScalar (huffTabScaleFact, &huffTabScaleFactInfo, bitBuf, &val);
  bitStream.AdvanceBitstream (nBits);
  return val;
  }
//}}}
//{{{
void DecodeScaleFactors (cBitStream& bitStream, int numWinGrp, int maxSFB, int globalGain,
                         unsigned char* sfbCodeBook, short* scaleFactors) {

  int g, sfbCB, nrg, npf, val, sf, is;

  /* starting values for differential coding */
  sf = globalGain;
  is = 0;
  nrg = globalGain - 90 - 256;
  npf = 1;

  for (g = 0; g < numWinGrp * maxSFB; g++) {
    sfbCB = *sfbCodeBook++;
    if (sfbCB  == 14 || sfbCB == 15) {
      /* intensity stereo - differential coding */
      val = DecodeOneScaleFactor (bitStream);
      is += val;
      *scaleFactors++ = (short)is;
      }
    else if (sfbCB == 13) {
      /* PNS - first energy is directly coded, rest are Huffman coded (npf = noise_pcm_flag) */
      if (npf) {
        val = bitStream.GetBits(9);
        npf = 0;
        }
      else
        val = DecodeOneScaleFactor (bitStream);
      nrg += val;
      *scaleFactors++ = (short)nrg;
      }
    else if (sfbCB >= 1 && sfbCB <= 11) {
      /* regular (non-zero) region - differential coding */
      val = DecodeOneScaleFactor (bitStream);
      sf += val;
      *scaleFactors++ = (short)sf;
      }
    else /* inactive scalefactor band if codebook 0 */
      *scaleFactors++ = 0;
    }
  }
//}}}

//{{{
void DecodeSpectrumLong (cBitStream& bitStream, PSInfoBase* psi, int ch) {

  int i, sfb, cb, nVals, offset;

  int* coef = psi->coef[ch];
  ICSInfo* icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  /* decode long block */
  const short*sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
  unsigned char* sfbCodeBook = psi->sfbCodeBook[ch];
  for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
    cb = *sfbCodeBook++;
    nVals = sfbTab[sfb+1] - sfbTab[sfb];

    if (cb == 0)
      UnpackZeros (nVals, coef);
    else if (cb <= 4)
      UnpackQuads (bitStream, cb, nVals, coef);
    else if (cb <= 10)
      UnpackPairsNoEsc (bitStream, cb, nVals, coef);
    else if (cb == 11)
      UnpackPairsEsc (bitStream, cb, nVals, coef);
    else
      UnpackZeros (nVals, coef);

    coef += nVals;
    }

  /* fill with zeros above maxSFB */
  nVals = NSAMPS_LONG - sfbTab[sfb];
  UnpackZeros(nVals, coef);

  /* add pulse data, if present */
  PulseInfo* pi = &psi->pulseInfo[ch];
  if (pi->pulseDataPresent) {
    coef = psi->coef[ch];
    offset = sfbTab[pi->startSFB];
    for (i = 0; i < pi->numPulse; i++) {
      offset += pi->offset[i];
      if (coef[offset] > 0)
        coef[offset] += pi->amp[i];
      else
        coef[offset] -= pi->amp[i];
      }
    ASSERT(offset < NSAMPS_LONG);
    }
  }
//}}}
//{{{
void DecodeSpectrumShort (cBitStream& bitStream, PSInfoBase* psi, int ch) {

  int gp, cb, nVals=0, win, offset, sfb;

  int *coef = psi->coef[ch];
  ICSInfo* icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);

  /* decode short blocks, deinterleaving in-place */
  const short*sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
  unsigned char* sfbCodeBook = psi->sfbCodeBook[ch];
  for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
    for (sfb = 0; sfb < icsInfo->maxSFB; sfb++) {
      nVals = sfbTab[sfb+1] - sfbTab[sfb];
      cb = *sfbCodeBook++;

      for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
        offset = win*NSAMPS_SHORT;
        if (cb == 0)
          UnpackZeros (nVals, coef + offset);
        else if (cb <= 4)
          UnpackQuads (bitStream, cb, nVals, coef + offset);
        else if (cb <= 10)
          UnpackPairsNoEsc (bitStream, cb, nVals, coef + offset);
        else if (cb == 11)
          UnpackPairsEsc (bitStream, cb, nVals, coef + offset);
        else
          UnpackZeros (nVals, coef + offset);
        }
      coef += nVals;
      }

    /* fill with zeros above maxSFB */
    for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
      offset = win*NSAMPS_SHORT;
      nVals = NSAMPS_SHORT - sfbTab[sfb];
      UnpackZeros(nVals, coef + offset);
      }
    coef += nVals;
    coef += (icsInfo->winGroupLen[gp] - 1)*NSAMPS_SHORT;
    }

  ASSERT(coef == psi->coef[ch] + NSAMPS_LONG);
  }
//}}}
