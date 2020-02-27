#include "aaccommon.h"

//{{{
 /**************************************************************************************
 * Function:    UnpackADTSHeader
 *
 * Description: parse the ADTS frame header and initialize decoder state
 *
 * Inputs:      valid AACDecInfo struct
 *              double pointer to buffer with complete ADTS frame header (byte aligned)
 *                header size = 7 bytes, plus 2 if CRC
 *
 * Outputs:     filled in ADTS struct
 *              updated buffer pointer
 *              updated bit offset
 *              updated number of available bits
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * TODO:        test CRC
 *              verify that fixed fields don't change between frames
 **************************************************************************************/
int UnpackADTSHeader(AACDecInfo *aacDecInfo, unsigned char **buf, int *bitOffset, int *bitsAvail)
{
  int bitsUsed;
  PSInfoBase *psi;
  BitStreamInfo bsi;
  ADTSHeader *fhADTS;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  fhADTS = &(psi->fhADTS);

  /* init bitstream reader */
  SetBitstreamPointer(&bsi, (*bitsAvail + 7) >> 3, *buf);
  GetBits(&bsi, *bitOffset);

  /* verify that first 12 bits of header are syncword */
  if (GetBits(&bsi, 12) != 0x0fff) {
    return ERR_AAC_INVALID_ADTS_HEADER;
  }

  /* fixed fields - should not change from frame to frame */
  fhADTS->id =               GetBits(&bsi, 1);
  fhADTS->layer =            GetBits(&bsi, 2);
  fhADTS->protectBit =       GetBits(&bsi, 1);
  fhADTS->profile =          GetBits(&bsi, 2);
  fhADTS->sampRateIdx =      GetBits(&bsi, 4);
  fhADTS->privateBit =       GetBits(&bsi, 1);
  fhADTS->channelConfig =    GetBits(&bsi, 3);
  fhADTS->origCopy =         GetBits(&bsi, 1);
  fhADTS->home =             GetBits(&bsi, 1);

  /* variable fields - can change from frame to frame */
  fhADTS->copyBit =          GetBits(&bsi, 1);
  fhADTS->copyStart =        GetBits(&bsi, 1);
  fhADTS->frameLength =      GetBits(&bsi, 13);
  fhADTS->bufferFull =       GetBits(&bsi, 11);
  fhADTS->numRawDataBlocks = GetBits(&bsi, 2) + 1;

  /* note - MPEG4 spec, correction 1 changes how CRC is handled when protectBit == 0 and numRawDataBlocks > 1 */
  if (fhADTS->protectBit == 0)
    fhADTS->crcCheckWord = GetBits(&bsi, 16);

  /* byte align */
  ByteAlignBitstream(&bsi); /* should always be aligned anyway */

  /* check validity of header */
  if (fhADTS->layer != 0 || fhADTS->profile != AAC_PROFILE_LC ||
    fhADTS->sampRateIdx >= NUM_SAMPLE_RATES || fhADTS->channelConfig >= NUM_DEF_CHAN_MAPS)
    return ERR_AAC_INVALID_ADTS_HEADER;

  /* update codec info */
  psi->sampRateIdx = fhADTS->sampRateIdx;
  if (!psi->useImpChanMap)
    psi->nChans = channelMapTab[fhADTS->channelConfig];

  /* syntactic element fields will be read from bitstream for each element */
  aacDecInfo->prevBlockID = AAC_ID_INVALID;
  aacDecInfo->currBlockID = AAC_ID_INVALID;
  aacDecInfo->currInstTag = -1;

  /* fill in user-accessible data (TODO - calc bitrate, handle tricky channel config cases) */
  aacDecInfo->bitRate = 0;
  aacDecInfo->nChans = psi->nChans;
  aacDecInfo->sampRate = sampRateTab[psi->sampRateIdx];
  aacDecInfo->profile = fhADTS->profile;
  aacDecInfo->sbrEnabled = 0;
  aacDecInfo->adtsBlocksLeft = fhADTS->numRawDataBlocks;

  /* update bitstream reader */
  bitsUsed = CalcBitsUsed(&bsi, *buf, *bitOffset);
  *buf += (bitsUsed + *bitOffset) >> 3;
  *bitOffset = (bitsUsed + *bitOffset) & 0x07;
  *bitsAvail -= bitsUsed ;
  if (*bitsAvail < 0)
    return ERR_AAC_INDATA_UNDERFLOW;

  return ERR_AAC_NONE;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    GetADTSChannelMapping
 *
 * Description: determine the number of channels from implicit mapping rules
 *
 * Inputs:      valid AACDecInfo struct
 *              pointer to start of raw_data_block
 *              bit offset
 *              bits available
 *
 * Outputs:     updated number of channels
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       calculates total number of channels using rules in 14496-3, 4.5.1.2.1
 *              does not attempt to deduce speaker geometry
 **************************************************************************************/
int GetADTSChannelMapping(AACDecInfo *aacDecInfo, unsigned char *buf, int bitOffset, int bitsAvail)
{
  int ch, nChans, elementChans, err;
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  nChans = 0;
  do {
    /* parse next syntactic element */
    err = DecodeNextElement(aacDecInfo, &buf, &bitOffset, &bitsAvail);
    if (err)
      return err;

    elementChans = elementNumChans[aacDecInfo->currBlockID];
    nChans += elementChans;

    for (ch = 0; ch < elementChans; ch++) {
      err = DecodeNoiselessData(aacDecInfo, &buf, &bitOffset, &bitsAvail, ch);
      if (err)
        return err;
    }
  } while (aacDecInfo->currBlockID != AAC_ID_END);

  if (nChans <= 0)
    return ERR_AAC_CHANNEL_MAP;

  /* update number of channels in codec state and user-accessible info structs */
  psi->nChans = nChans;
  aacDecInfo->nChans = psi->nChans;
  psi->useImpChanMap = 1;

  return ERR_AAC_NONE;
}
//}}}

//{{{
/**************************************************************************************
 * Function:    SetRawBlockParams
 *
 * Description: set internal state variables for decoding a stream of raw data blocks
 *
 * Inputs:      valid AACDecInfo struct
 *              flag indicating source of parameters (from previous headers or passed
 *                explicitly by caller)
 *              number of channels
 *              sample rate
 *              profile ID
 *
 * Outputs:     updated state variables in aacDecInfo
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       if copyLast == 1, then psi->nChans, psi->sampRateIdx, and
 *                aacDecInfo->profile are not changed (it's assumed that we already
 *                set them, such as by a previous call to UnpackADTSHeader())
 *              if copyLast == 0, then the parameters we passed in are used instead
 **************************************************************************************/
int SetRawBlockParams(AACDecInfo *aacDecInfo, int copyLast, int nChans, int sampRate, int profile)
{
  int idx;
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  if (!copyLast) {
    aacDecInfo->profile = profile;
    psi->nChans = nChans;
    for (idx = 0; idx < NUM_SAMPLE_RATES; idx++) {
      if (sampRate == sampRateTab[idx]) {
        psi->sampRateIdx = idx;
        break;
      }
    }
    if (idx == NUM_SAMPLE_RATES)
      return ERR_AAC_INVALID_FRAME;
  }
  aacDecInfo->nChans = psi->nChans;
  aacDecInfo->sampRate = sampRateTab[psi->sampRateIdx];

  /* check validity of header */
  if (psi->sampRateIdx >= NUM_SAMPLE_RATES || psi->sampRateIdx < 0 || aacDecInfo->profile != AAC_PROFILE_LC)
    return ERR_AAC_RAWBLOCK_PARAMS;

  return ERR_AAC_NONE;
}
//}}}
//{{{
/**************************************************************************************
 * Function:    PrepareRawBlock
 *
 * Description: reset per-block state variables for raw blocks (no ADTS headers)
 *
 * Inputs:      valid AACDecInfo struct
 *
 * Outputs:     updated state variables in aacDecInfo
 *
 * Return:      0 if successful, error code (< 0) if error
 **************************************************************************************/
int PrepareRawBlock(AACDecInfo *aacDecInfo)
{
//  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
//  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  /* syntactic element fields will be read from bitstream for each element */
  aacDecInfo->prevBlockID = AAC_ID_INVALID;
  aacDecInfo->currBlockID = AAC_ID_INVALID;
  aacDecInfo->currInstTag = -1;

  /* fill in user-accessible data */
  aacDecInfo->bitRate = 0;
  aacDecInfo->sbrEnabled = 0;

  return ERR_AAC_NONE;
}

//}}}

//{{{
/**************************************************************************************
 * Function:    FlushCodec
 *
 * Description: flush internal codec state (after seeking, for example)
 *
 * Inputs:      valid AACDecInfo struct
 *
 * Outputs:     updated state variables in aacDecInfo
 *
 * Return:      0 if successful, error code (< 0) if error
 *
 * Notes:       only need to clear data which is persistent between frames
 *                (such as overlap buffer)
 **************************************************************************************/
int FlushCodec(AACDecInfo *aacDecInfo)
{
  PSInfoBase *psi;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return ERR_AAC_NULL_POINTER;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

  ClearBuffer(psi->overlap, AAC_MAX_NCHANS * AAC_MAX_NSAMPS * sizeof(int));
  ClearBuffer(psi->prevWinShape, AAC_MAX_NCHANS * sizeof(int));

  return ERR_AAC_NONE;
}
//}}}
