#pragma once
#include "aaccommon.h"

typedef struct _BitStreamInfo {
	unsigned char *bytePtr;
	unsigned int iCache;
	int cachedBits;
	int nBytes;
	} BitStreamInfo;

void SetBitstreamPointer (BitStreamInfo *bsi, int nBytes, unsigned char *buf);

unsigned int GetBits (BitStreamInfo *bsi, int nBits);
unsigned int GetBitsNoAdvance (BitStreamInfo *bsi, int nBits);

void AdvanceBitstream (BitStreamInfo *bsi, int nBits);
int CalcBitsUsed (BitStreamInfo *bsi, unsigned char *startBuf, int startOffset);

void ByteAlignBitstream (BitStreamInfo *bsi);
