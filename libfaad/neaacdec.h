#pragma once
//{{{
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
//}}}

#ifdef _WIN32
  #pragma pack(push, 8)
  #ifndef NEAACDECAPI
    #define NEAACDECAPI __cdecl
  #endif
#else
  #ifndef NEAACDECAPI
    #define NEAACDECAPI
  #endif
#endif

#define FAAD2_VERSION "2.7"

//{{{  object types for AAC
#define MAIN       1
#define LC         2
#define SSR        3
#define LTP        4
#define HE_AAC     5
#define ER_LC     17
#define ER_LTP    19
#define LD        23
//}}}
//{{{  header types
#define RAW        0
#define ADIF       1
#define ADTS       2
#define LATM       3
//}}}
//{{{  SBR signalling
#define NO_SBR           0
#define SBR_UPSAMPLED    1
#define SBR_DOWNSAMPLED  2
#define NO_SBR_UPSAMPLED 3
//}}}
//{{{  library output formats
#define FAAD_FMT_16BIT  1
//}}}
//{{{  Capabilities
#define LC_DEC_CAP           (1<<0) /* Can decode LC */
#define MAIN_DEC_CAP         (1<<1) /* Can decode MAIN */
#define LTP_DEC_CAP          (1<<2) /* Can decode LTP */
#define LD_DEC_CAP           (1<<3) /* Can decode LD */
#define ERROR_RESILIENCE_CAP (1<<4) /* Can decode ER */
//}}}
//{{{  Channel definitions
#define FRONT_CHANNEL_CENTER (1)
#define FRONT_CHANNEL_LEFT   (2)
#define FRONT_CHANNEL_RIGHT  (3)
#define SIDE_CHANNEL_LEFT    (4)
#define SIDE_CHANNEL_RIGHT   (5)
#define BACK_CHANNEL_LEFT    (6)
#define BACK_CHANNEL_RIGHT   (7)
#define BACK_CHANNEL_CENTER  (8)
#define LFE_CHANNEL          (9)
#define UNKNOWN_CHANNEL      (0)
//}}}

/* A decode call can eat up to FAAD_MIN_STREAMSIZE bytes per decoded channel,
   so at least so much bytes per channel should be available in this stream */
#define FAAD_MIN_STREAMSIZE 768 /* 6144 bits/channel */

typedef void* NeAACDecHandle;
//{{{
typedef struct mp4AudioSpecificConfig
{
    /* Audio Specific Info */
    unsigned char objectTypeIndex;
    unsigned char samplingFrequencyIndex;
    unsigned long samplingFrequency;
    unsigned char channelsConfiguration;

    /* GA Specific Info */
    unsigned char frameLengthFlag;
    unsigned char dependsOnCoreCoder;
    unsigned short coreCoderDelay;
    unsigned char extensionFlag;
    unsigned char aacSectionDataResilienceFlag;
    unsigned char aacScalefactorDataResilienceFlag;
    unsigned char aacSpectralDataResilienceFlag;
    unsigned char epConfig;

    char sbr_present_flag;
    char forceUpSampling;
    char downSampledSBR;
  } mp4AudioSpecificConfig;
//}}}
//{{{
typedef struct NeAACDecConfiguration
{
    unsigned char defObjectType;
    unsigned long defSampleRate;
    unsigned char outputFormat;
    unsigned char downMatrix;
    unsigned char useOldADTSFormat;
    unsigned char dontUpSampleImplicitSBR;
  } NeAACDecConfiguration, *NeAACDecConfigurationPtr;
//}}}
//{{{
typedef struct NeAACDecFrameInfo
{
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char error;
    unsigned long samplerate;

    /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
    unsigned char sbr;

    /* MPEG-4 ObjectType */
    unsigned char object_type;

    /* AAC header type; MP4 will be signalled as RAW also */
    unsigned char header_type;

    /* multichannel configuration */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char channel_position[64];

    /* PS: 0: off, 1: on */
    unsigned char ps;
  } NeAACDecFrameInfo;
//}}}

NeAACDecHandle NEAACDECAPI NeAACDecOpen();

unsigned long NEAACDECAPI NeAACDecGetCapabilities();

NeAACDecConfigurationPtr NEAACDECAPI NeAACDecGetCurrentConfiguration (NeAACDecHandle hDecoder);
unsigned char NEAACDECAPI NeAACDecSetConfiguration (NeAACDecHandle hDecoder, NeAACDecConfigurationPtr config);

/* Init the library based on info from the AAC file (ADTS/ADIF) */
long NEAACDECAPI NeAACDecInit (NeAACDecHandle hDecoder,
                               unsigned char *buffer, unsigned long buffer_size,
                               unsigned long *samplerate, unsigned char *channels);

/* Init the library using a DecoderSpecificInfo */
char NEAACDECAPI NeAACDecInit2 (NeAACDecHandle hDecoder,
                                unsigned char *pBuffer, unsigned long SizeOfDecoderSpecificInfo,
                                unsigned long *samplerate, unsigned char *channels);

void NEAACDECAPI NeAACDecPostSeekReset (NeAACDecHandle hDecoder, long frame);

void* NEAACDECAPI NeAACDecDecode (NeAACDecHandle hDecoder,
                                  NeAACDecFrameInfo *hInfo,
                                  unsigned char *buffer,
                                  unsigned long buffer_size);
void* NEAACDECAPI NeAACDecDecode2 (NeAACDecHandle hDecoder, NeAACDecFrameInfo *hInfo,
                                   unsigned char *buffer, unsigned long buffer_size,
                                   void **sample_buffer, unsigned long sample_buffer_size);

char NEAACDECAPI NeAACDecAudioSpecificConfig (unsigned char *pBuffer, unsigned long buffer_size,
                                              mp4AudioSpecificConfig *mp4ASC);

void NEAACDECAPI NeAACDecClose (NeAACDecHandle hDecoder);

#ifdef _WIN32
  #pragma pack(pop)
#endif

//{{{
#ifdef __cplusplus
}
#endif /* __cplusplus */
//}}}
