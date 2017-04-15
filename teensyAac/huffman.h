// huffman.h
#pragma once
#include "aaccommon.h"

int DecodeHuffmanScalar (const signed short* huffTab, const HuffInfo* huffTabInfo, unsigned int bitBuf, signed int* val);

void DecodeSectionData (cBitStream& bitStream, int winSequence, int numWinGrp, int maxSFB, unsigned char* sfbCodeBook);
int DecodeOneSymbol (cBitStream* bitStream, int huffTabIndex);

int DecodeOneScaleFactor (cBitStream& bitStream);
void DecodeScaleFactors (cBitStream& bitStream, int numWinGrp, int maxSFB, int globalGain,
                         unsigned char* sfbCodeBook, short* scaleFactors);

void DecodeSpectrumLong (cBitStream& bitStream, PSInfoBase* psi, int ch);
void DecodeSpectrumShort (cBitStream& bitStream, PSInfoBase* psi, int ch);
