// sbr.h
#pragma once
#include "aaccommon.h"

//{{{
#ifdef __cplusplus
extern "C" {
#endif
//}}}
void CVKernel1 (int* XBuf, int* accBuf);
void CVKernel2 (int* XBuf, int* accBuf);
void QMFAnalysisConv (int* cTab, int* delay, int dIdx, int* uBuf);
void QMFSynthesisConv (int* cPtr, int* delay, int dIdx, short* outbuf, int nChans);
//{{{
#ifdef __cplusplus
}
#endif
//}}}

int CalcFreqTables (SBRHeader *sbrHeader, SBRFreq *sbrFreq, int sampRateIdx);
void AdjustHighFreq (PSInfoSBR *psi, SBRHeader *sbrHeader, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan, int ch);
void GenerateHighFreq (PSInfoSBR *psi, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan, int ch);

void DecodeSBREnvelope (cBitStream* bitStream, PSInfoSBR *psi, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan, int ch);
void DecodeSBRNoise (cBitStream* bitStream, PSInfoSBR *psi, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChan, int ch);
void UncoupleSBREnvelope (PSInfoSBR* psi, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChanR);
void UncoupleSBRNoise (PSInfoSBR* psi, SBRGrid *sbrGrid, SBRFreq *sbrFreq, SBRChan *sbrChanR);

int InvRNormalized (int r);
int RatioPowInv (int a, int b, int c);
int SqrtFix (int x, int fBitsIn, int *fBitsOut);

int QMFAnalysis (int* inbuf, int* delay, int* XBuf, int fBitsIn, int* delayIdx, int qmfaBands);
void QMFSynthesis (int* inbuf, int* delay, int* delayIdx, int qmfsBands, short* outbuf, int nChans);

int GetSampRateIdx (int sampRate);
int UnpackSBRHeader (cBitStream* bitStream, SBRHeader* sbrHeader);
void UnpackSBRSingleChannel (cBitStream* bitStream, PSInfoSBR *psi, int chOut);
void UnpackSBRChannelPair (cBitStream* bitStream, PSInfoSBR *psi, int chOut);
