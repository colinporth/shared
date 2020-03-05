// cAacdecoder.h
#pragma once
#include "iAudioDecoder.h"

// forward declarations for private data
struct sBitStream;
struct sProgConfigElement;
struct sPSInfoBase;
struct sPSInfoSBR;

class cAacDecoder : public iAudioDecoder {
public:
  //{{{  defines
  // according to spec (13818-7 section 8.2.2, 14496-3 section 4.5.3)
  // max size of input buffer =
  //    6144 bits =  768 bytes per SCE or CCE-I
  //   12288 bits = 1536 bytes per CPE
  //       0 bits =    0 bytes per CCE-D (uses bits from the SCE/CPE/CCE-I it is coupled to)
  #define AAC_MAX_NCHANS    2 /* set to default max number of channels  */
  #define MAX_NCHANS_ELEM   2 /* max number of channels in any single bitstream element (SCE,CPE,CCE,LFE) */

  #define AAC_MAX_NSAMPS    1024
  #define AAC_MAINBUF_SIZE  (768 * AAC_MAX_NCHANS)

  #define AAC_PROFILE_MP    0
  #define AAC_PROFILE_LC    1
  #define AAC_PROFILE_SSR   2
  #define AAC_NUM_PROFILES  3
  //}}}
  cAacDecoder();
  ~cAacDecoder();

  int cAacDecoder::getNumChannels() { return numChannels; }
  int cAacDecoder::getSampleRate() { return sampleRate * (sbrEnabled ? 2 : 1); }

  int decodeFrame (uint8_t* inbuf, int bytesLeft, float* outbuf, bool jumped);
  void flushCodec();

private:
  //{{{  private members
  void decodeSingleChannelElement (sBitStream* bsi);
  void decodeChannelPairElement (sBitStream* bsi);
  void decodeLFEChannelElement (sBitStream* bsi);
  void decodeDataStreamElement (sBitStream* bsi);
  void decodeProgramConfigElement (sProgConfigElement* pce, sBitStream* bsi);
  void decodeFillElement (sBitStream* bsi);
  bool decodeNextElement (uint8_t** buf, int* bitOffset, int* bitsAvail);
  bool decodeSbrBitstream (int chBase);

  void decodeNoiselessData (uint8_t** buf, int* bitOffset, int* bitsAvail, int channel);
  void dequantize (int channel);
  void applyStereoProcess();
  void applyPns (int channel);
  void applyTns (int channel);
  void imdct (int channel, int chOut, float* outbuf);
  void applySbr (int chBase, float* outbuf);

  bool unpackADTSHeader (uint8_t** buf, int* bitOffset, int* bitsAvail);
  bool getADTSChannelMapping (uint8_t* buf, int bitOffset, int bitsAvail);
  //}}}
  //{{{  private vars
  sPSInfoBase* psInfoBase;
  sPSInfoSBR* psInfoSBR;

  // raw decoded data
  void* rawSampleBuf [AAC_MAX_NCHANS];

  // fill data (can be used for processing SBR or other extensions)
  uint8_t* fillBuf;
  int fillCount;
  int fillExtType;

  // block information
  int prevBlockID;
  int currBlockID;
  int currInstTag;
  int sbDeinterleaveReqd [MAX_NCHANS_ELEM];
  int adtsBlocksLeft;

  //  info
  int bitRate;
  int numChannels;
  int sampleRate;
  int profile;
  int format;
  int sbrEnabled;
  int tnsUsed;
  int pnsUsed;
  //}}}
  };
