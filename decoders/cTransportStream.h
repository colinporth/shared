// cTransportStream.h - transport stream parser
#pragma once
//{{{  includes
#include <string>
#include <map>
#include <time.h>

#include "huffDecoder.h"
#include "dvbSubtitle.h"
//}}}
//{{{  macros
#define HILO(x) (x##_hi << 8 | x##_lo)

#define MjdToEpochTime(x) (unsigned int)((((x##_hi << 8) | x##_lo) - 40587) * 86400)

#define BcdTimeToSeconds(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                               (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                                     ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))
//}}}
//{{{  dvb const, struct
//{{{  pid const
#define PID_PAT   0x00   /* Program Association Table */
#define PID_CAT   0x01   /* Conditional Access Table */
#define PID_NIT   0x10   /* Network Information Table */
#define PID_BAT   0x11   /* Bouquet Association Table */
#define PID_SDT   0x11   /* Service Description Table */
#define PID_EIT   0x12   /* Event Information Table */
#define PID_RST   0x13   /* Running Status Table */
#define PID_TDT   0x14   /* Time Date Table */
#define PID_TOT   0x14   /* Time Offset Table */
#define PID_STF   0x14   /* Stuffing Table */
#define PID_SYN   0x15   /* Network sync */
//}}}
//{{{  tid const
#define TID_PAT          0x00   /* Program Association Section */
#define TID_CAT          0x01   /* Conditional Access Section */
#define TID_PMT          0x02   /* Conditional Access Section */
#define TID_EIT          0x12   /* Event Information Section */
#define TID_NIT_ACT      0x40   /* Network Information Section - actual */
#define TID_NIT_OTH      0x41   /* Network Information Section - other */
#define TID_SDT_ACT      0x42   /* Service Description Section - actual */
#define TID_SDT_OTH      0x46   /* Service Description Section - other */
#define TID_BAT          0x4A   /* Bouquet Association Section */
#define TID_EIT_ACT      0x4E   /* Event Information Section - actual */
#define TID_EIT_OTH      0x4F   /* Event Information Section - other */
#define TID_EIT_ACT_SCH  0x50   /* Event Information Section - actual, schedule  */
#define TID_EIT_OTH_SCH  0x60   /* Event Information Section - other, schedule */
#define TID_TDT          0x70   /* Time Date Section */
#define TID_RST          0x71   /* Running Status Section */
#define TID_ST           0x72   /* Stuffing Section */
#define TID_TOT          0x73   /* Time Offset Section */
//}}}
//{{{  serviceType const
#define kServiceTypeTV            0x01
#define kServiceTypeRadio         0x02
#define kServiceTypeAdvancedSDTV  0x16
#define kServiceTypeAdvancedHDTV  0x19
//}}}

//{{{  descr const
#define DESCR_VIDEO_STREAM          0x02
#define DESCR_AUDIO_STREAM          0x03
#define DESCR_HIERARCHY             0x04
#define DESCR_REGISTRATION          0x05
#define DESCR_DATA_STREAM_ALIGN     0x06
#define DESCR_TARGET_BACKGRID       0x07
#define DESCR_VIDEO_WINDOW          0x08
#define DESCR_CA                    0x09
#define DESCR_ISO_639_LANGUAGE      0x0A
#define DESCR_SYSTEM_CLOCK          0x0B
#define DESCR_MULTIPLEX_BUFFER_UTIL 0x0C
#define DESCR_COPYRIGHT             0x0D
#define DESCR_MAXIMUM_BITRATE       0x0E
#define DESCR_PRIVATE_DATA_IND      0x0F

#define DESCR_SMOOTHING_BUFFER      0x10
#define DESCR_STD                   0x11
#define DESCR_IBP                   0x12

#define DESCR_NW_NAME               0x40
#define DESCR_SERVICE_LIST          0x41
#define DESCR_STUFFING              0x42
#define DESCR_SAT_DEL_SYS           0x43
#define DESCR_CABLE_DEL_SYS         0x44
#define DESCR_VBI_DATA              0x45
#define DESCR_VBI_TELETEXT          0x46
#define DESCR_BOUQUET_NAME          0x47
#define DESCR_SERVICE               0x48
#define DESCR_COUNTRY_AVAIL         0x49
#define DESCR_LINKAGE               0x4A
#define DESCR_NVOD_REF              0x4B
#define DESCR_TIME_SHIFTED_SERVICE  0x4C
#define DESCR_SHORT_EVENT           0x4D
#define DESCR_EXTENDED_EVENT        0x4E
#define DESCR_TIME_SHIFTED_EVENT    0x4F

#define DESCR_COMPONENT             0x50
#define DESCR_MOSAIC                0x51
#define DESCR_STREAM_ID             0x52
#define DESCR_CA_IDENT              0x53
#define DESCR_CONTENT               0x54
#define DESCR_PARENTAL_RATING       0x55
#define DESCR_TELETEXT              0x56
#define DESCR_TELEPHONE             0x57
#define DESCR_LOCAL_TIME_OFF        0x58
#define DESCR_SUBTITLING            0x59
#define DESCR_TERR_DEL_SYS          0x5A
#define DESCR_ML_NW_NAME            0x5B
#define DESCR_ML_BQ_NAME            0x5C
#define DESCR_ML_SERVICE_NAME       0x5D
#define DESCR_ML_COMPONENT          0x5E
#define DESCR_PRIV_DATA_SPEC        0x5F

#define DESCR_SERVICE_MOVE          0x60
#define DESCR_SHORT_SMOOTH_BUF      0x61
#define DESCR_FREQUENCY_LIST        0x62
#define DESCR_PARTIAL_TP_STREAM     0x63
#define DESCR_DATA_BROADCAST        0x64
#define DESCR_CA_SYSTEM             0x65
#define DESCR_DATA_BROADCAST_ID     0x66
#define DESCR_TRANSPORT_STREAM      0x67
#define DESCR_DSNG                  0x68
#define DESCR_PDC                   0x69
#define DESCR_AC3                   0x6A
#define DESCR_ANCILLARY_DATA        0x6B
#define DESCR_CELL_LIST             0x6C
#define DESCR_CELL_FREQ_LINK        0x6D
#define DESCR_ANNOUNCEMENT_SUPPORT  0x6E
//}}}

//{{{  pat struct
#define PAT_LEN 8
typedef struct {
  uint8_t table_id                  :8;

  uint8_t section_length_hi         :4;
  uint8_t dummy1                    :2;
  uint8_t dummy                     :1;
  uint8_t section_syntax_indicator  :1;

  uint8_t section_length_lo         :8;
  uint8_t transport_stream_id_hi    :8;
  uint8_t transport_stream_id_lo    :8;

  uint8_t current_next_indicator    :1;
  uint8_t version_number            :5;
  uint8_t dummy2                    :2;

  uint8_t section_number            :8;
  uint8_t last_section_number       :8;
  } pat_t;

#define PAT_PROG_LEN 4
typedef struct {
  uint8_t program_number_hi         :8;
  uint8_t program_number_lo         :8;

  uint8_t network_pid_hi            :5;
  uint8_t                           :3;
  uint8_t network_pid_lo            :8;
  /* or program_map_pid (if prog_num=0)*/
  } pat_prog_t;
//}}}
//{{{  pmt struct
#define PMT_LEN 12
typedef struct {
   unsigned char table_id                  :8;

   uint8_t section_length_hi         :4;
   uint8_t                           :2;
   uint8_t dummy                     :1; // has to be 0
   uint8_t section_syntax_indicator  :1;
   uint8_t section_length_lo         :8;

   uint8_t program_number_hi         :8;
   uint8_t program_number_lo         :8;
   uint8_t current_next_indicator    :1;
   uint8_t version_number            :5;
   uint8_t                           :2;
   uint8_t section_number            :8;
   uint8_t last_section_number       :8;
   uint8_t PCR_PID_hi                :5;
   uint8_t                           :3;
   uint8_t PCR_PID_lo                :8;
   uint8_t program_info_length_hi    :4;
   uint8_t                           :4;
   uint8_t program_info_length_lo    :8;
   //descrs
  } pmt_t;

#define PMT_INFO_LEN 5
typedef struct {
   uint8_t stream_type        :8;
   uint8_t elementary_PID_hi  :5;
   uint8_t                    :3;
   uint8_t elementary_PID_lo  :8;
   uint8_t ES_info_length_hi  :4;
   uint8_t                    :4;
   uint8_t ES_info_length_lo  :8;
   // descrs
  } pmt_info_t;
//}}}
//{{{  nit struct
#define NIT_LEN 10
typedef struct {
  uint8_t table_id                     :8;

  uint8_t section_length_hi         :4;
  uint8_t                           :3;
  uint8_t section_syntax_indicator  :1;
  uint8_t section_length_lo         :8;

  uint8_t network_id_hi             :8;
  uint8_t network_id_lo             :8;
  uint8_t current_next_indicator    :1;
  uint8_t version_number            :5;
  uint8_t                           :2;
  uint8_t section_number            :8;
  uint8_t last_section_number       :8;
  uint8_t network_descr_length_hi   :4;
  uint8_t                           :4;
  uint8_t network_descr_length_lo   :8;
  /* descrs */
  } nit_t;

#define SIZE_NIT_MID 2
typedef struct {                                 // after descrs
  uint8_t transport_stream_loop_length_hi  :4;
  uint8_t                                  :4;
  uint8_t transport_stream_loop_length_lo  :8;
  } nit_mid_t;

#define SIZE_NIT_END 4
struct nit_end_struct {
  long CRC;
  };

#define NIT_TS_LEN 6
typedef struct {
  uint8_t transport_stream_id_hi      :8;
  uint8_t transport_stream_id_lo      :8;
  uint8_t original_network_id_hi      :8;
  uint8_t original_network_id_lo      :8;
  uint8_t transport_descrs_length_hi  :4;
  uint8_t                             :4;
  uint8_t transport_descrs_length_lo  :8;
  /* descrs  */
  } nit_ts_t;
//}}}
//{{{  eit struct
#define EIT_LEN 14
typedef struct {
  uint8_t table_id                    :8;

  uint8_t section_length_hi           :4;
  uint8_t                             :3;
  uint8_t section_syntax_indicator    :1;
  uint8_t section_length_lo           :8;

  uint8_t service_id_hi               :8;
  uint8_t service_id_lo               :8;
  uint8_t current_next_indicator      :1;
  uint8_t version_number              :5;
  uint8_t                             :2;
  uint8_t section_number              :8;
  uint8_t last_section_number         :8;
  uint8_t transport_stream_id_hi      :8;
  uint8_t transport_stream_id_lo      :8;
  uint8_t original_network_id_hi      :8;
  uint8_t original_network_id_lo      :8;
  uint8_t segment_last_section_number :8;
  uint8_t segment_last_table_id       :8;
  } eit_t;

#define EIT_EVENT_LEN 12
typedef struct {
  uint8_t event_id_hi                 :8;
  uint8_t event_id_lo                 :8;
  uint8_t mjd_hi                      :8;
  uint8_t mjd_lo                      :8;
  uint8_t start_time_h                :8;
  uint8_t start_time_m                :8;
  uint8_t start_time_s                :8;
  uint8_t duration_h                  :8;
  uint8_t duration_m                  :8;
  uint8_t duration_s                  :8;
  uint8_t descrs_loop_length_hi       :4;
  uint8_t free_ca_mode                :1;
  uint8_t running_status              :3;
  uint8_t descrs_loop_length_lo       :8;
  } eit_event_t;
//}}}
//{{{  sdt struct
#define SDT_LEN 11
typedef struct {
  uint8_t table_id                    :8;
  uint8_t section_length_hi           :4;
  uint8_t                             :3;
  uint8_t section_syntax_indicator    :1;
  uint8_t section_length_lo           :8;
  uint8_t transport_stream_id_hi      :8;
  uint8_t transport_stream_id_lo      :8;
  uint8_t current_next_indicator      :1;
  uint8_t version_number              :5;
  uint8_t                             :2;
  uint8_t section_number              :8;
  uint8_t last_section_number         :8;
  uint8_t original_network_id_hi      :8;
  uint8_t original_network_id_lo      :8;
  uint8_t                             :8;
  } sdt_t;

#define GetSDTTransportStreamId(x) (HILO(((sdt_t*)x)->transport_stream_id))
#define GetSDTOriginalNetworkId(x) (HILO(((sdt_t*)x)->original_network_id))

#define SDT_DESCR_LEN 5
typedef struct {
  uint8_t service_id_hi                :8;
  uint8_t service_id_lo                :8;
  uint8_t eit_present_following_flag   :1;
  uint8_t eit_schedule_flag            :1;
  uint8_t                              :6;
  uint8_t descrs_loop_length_hi        :4;
  uint8_t free_ca_mode                 :1;
  uint8_t running_status               :3;
  uint8_t descrs_loop_length_lo        :8;
  } sdt_descr_t;
//}}}
//{{{  tdt struct
#define TDT_LEN 8
typedef struct {
  uint8_t table_id                  :8;
  uint8_t section_length_hi         :4;
  uint8_t                           :3;
  uint8_t section_syntax_indicator  :1;
  uint8_t section_length_lo         :8;
  uint8_t utc_mjd_hi                :8;
  uint8_t utc_mjd_lo                :8;
  uint8_t utc_time_h                :8;
  uint8_t utc_time_m                :8;
  uint8_t utc_time_s                :8;
  } tdt_t;
//}}}

//{{{
typedef struct descr_gen_struct {
  uint8_t descr_tag        :8;
  uint8_t descr_length     :8;
  } descr_gen_t;

#define CastGenericDescr(x) ((descr_gen_t *)(x))

#define GetDescrTag(x) (((descr_gen_t *) x)->descr_tag)
#define GetDescrLength(x) (((descr_gen_t *) x)->descr_length+2)
//}}}

//{{{  0x40 network_name_desciptor
#define DESCR_NETWORK_NAME_LEN 2

typedef struct descr_network_name_struct {
  uint8_t descr_tag     :8;
  uint8_t descr_length  :8;
  } descr_network_name_t;

#define CastNetworkNameDescr(x) ((descr_network_name_t *)(x))
#define CastServiceListDescrLoop(x) ((descr_service_list_loop_t *)(x))
//}}}
//{{{  0x41 service_list_descr
#define DESCR_SERVICE_LIST_LEN 2

typedef struct descr_service_list_struct {
  uint8_t descr_tag     :8;
  uint8_t descr_length  :8;
  } descr_service_list_t;

#define CastServiceListDescr(x) ((descr_service_list_t *)(x))


#define DESCR_SERVICE_LIST_LOOP_LEN 3

typedef struct descr_service_list_loop_struct {
  uint8_t service_id_hi  :8;
  uint8_t service_id_lo  :8;
  uint8_t service_type   :8;
  } descr_service_list_loop_t;

#define CastServiceListDescrLoop(x) ((descr_service_list_loop_t *)(x))
//}}}
//{{{  0x47 bouquet_name_descr
#define DESCR_BOUQUET_NAME_LEN 2

typedef struct descr_bouquet_name_struct {
  uint8_t descr_tag    :8;
  uint8_t descr_length :8;
  } descr_bouquet_name_t;

#define CastBouquetNameDescr(x) ((descr_bouquet_name_t *)(x))
//}}}
//{{{  0x48 service_descr
#define DESCR_SERVICE_LEN  4

typedef struct descr_service_struct {
  uint8_t descr_tag             :8;
  uint8_t descr_length          :8;
  uint8_t service_type          :8;
  uint8_t provider_name_length  :8;
  } descr_service_t;

#define CastServiceDescr(x) ((descr_service_t *)(x))
//}}}
//{{{  0x4D short_event_descr
#define DESCR_SHORT_EVENT_LEN 6

typedef struct descr_short_event_struct {
  uint8_t descr_tag         :8;
  uint8_t descr_length      :8;
  uint8_t lang_code1        :8;
  uint8_t lang_code2        :8;
  uint8_t lang_code3        :8;
  uint8_t event_name_length :8;
  } descr_short_event_t;

#define CastShortEventDescr(x) ((descr_short_event_t *)(x))
//}}}
//{{{  0x4E extended_event_descr
#define DESCR_EXTENDED_EVENT_LEN 7

typedef struct descr_extended_event_struct {
  uint8_t descr_tag          :8;
  uint8_t descr_length       :8;
  /* TBD */
  uint8_t last_descr_number  :4;
  uint8_t descr_number       :4;
  uint8_t lang_code1         :8;
  uint8_t lang_code2         :8;
  uint8_t lang_code3         :8;
  uint8_t length_of_items    :8;
  } descr_extended_event_t;

#define CastExtendedEventDescr(x) ((descr_extended_event_t *)(x))

#define ITEM_EXTENDED_EVENT_LEN 1

typedef struct item_extended_event_struct {
  uint8_t item_description_length               :8;
  } item_extended_event_t;

#define CastExtendedEventItem(x) ((item_extended_event_t *)(x))
//}}}

//{{{  0x50 component_descr
#define DESCR_COMPONENT_LEN  8

typedef struct descr_component_struct {
  uint8_t descr_tag       :8;
  uint8_t descr_length    :8;
  uint8_t stream_content  :4;
  uint8_t reserved        :4;
  uint8_t component_type  :8;
  uint8_t component_tag   :8;
  uint8_t lang_code1      :8;
  uint8_t lang_code2      :8;
  uint8_t lang_code3      :8;
  } descr_component_t;

#define CastComponentDescr(x) ((descr_component_t *)(x))
//}}}
//{{{  0x54 content_descr
#define DESCR_CONTENT_LEN 2

typedef struct descr_content_struct {
  uint8_t descr_tag     :8;
  uint8_t descr_length  :8;
  } descr_content_t;

#define CastContentDescr(x) ((descr_content_t *)(x))

typedef struct nibble_content_struct {
  uint8_t user_nibble_2           :4;
  uint8_t user_nibble_1           :4;
  uint8_t content_nibble_level_2  :4;
  uint8_t content_nibble_level_1  :4;
  } nibble_content_t;

#define CastContentNibble(x) ((nibble_content_t *)(x))
//}}}
//{{{  0x59 subtitling_descr
#define DESCR_SUBTITLING_LEN 2

typedef struct descr_subtitling_struct {
  uint8_t descr_tag      :8;
  uint8_t descr_length   :8;
  } descr_subtitling_t;

#define CastSubtitlingDescr(x) ((descr_subtitling_t *)(x))

#define ITEM_SUBTITLING_LEN 8

typedef struct item_subtitling_struct {
  uint8_t lang_code1             :8;
  uint8_t lang_code2             :8;
  uint8_t lang_code3             :8;
  uint8_t subtitling_type        :8;
  uint8_t composition_page_id_hi :8;
  uint8_t composition_page_id_lo :8;
  uint8_t ancillary_page_id_hi   :8;
  uint8_t ancillary_page_id_lo   :8;
  } item_subtitling_t;

#define CastSubtitlingItem(x) ((item_subtitling_t *)(x))
//}}}
//{{{  0x5A terrestrial_delivery_system_descr
#define DESCR_TERRESTRIAL_DELIVERY_SYSTEM_LEN XX

typedef struct descr_terrestrial_delivery_struct {
  uint8_t descr_tag      :8;
  uint8_t descr_length   :8;

  uint8_t frequency1            :8;
  uint8_t frequency2            :8;
  uint8_t frequency3            :8;
  uint8_t frequency4            :8;

  uint8_t reserved1             :5;
  uint8_t bandwidth             :3;

  uint8_t code_rate_HP          :3;
  uint8_t hierarchy             :3;
  uint8_t constellation         :2;
  uint8_t other_frequency_flag  :1;
  uint8_t transmission_mode     :2;
  uint8_t guard_interval        :2;
  uint8_t code_rate_LP          :3;

  uint8_t reserver2             :8;
  uint8_t reserver3             :8;
  uint8_t reserver4             :8;
  uint8_t reserver5             :8;
  } descr_terrestrial_delivery_system_t;

#define CastTerrestrialDeliverySystemDescr(x) ((descr_terrestrial_delivery_system_t *)(x))
//}}}

//{{{  0x62 frequency_list_descr
#define DESCR_FREQUENCY_LIST_LEN XX

typedef struct descr_frequency_list_struct {
  uint8_t descr_tag      :8;
  uint8_t descr_length   :8;
  /* TBD */
  } descr_frequency_list_t;

#define CastFrequencyListDescr(x) ((descr_frequency_list_t *)(x))
//}}}

//{{{
// CRC32 lookup table for polynomial 0x04c11db7
static unsigned long crcTable[256] = {
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
  0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
  0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
  0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
  0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
  0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
  0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
  0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
  0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
  0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
  0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
  0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
  0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
  0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
  0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
  0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
  0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};
//}}}
//}}}

//{{{
class cPidInfo {
public:
  //{{{
  cPidInfo (int pid, bool isSection) : mPid(pid),  mIsSection(isSection) {

    switch (pid) {
      case PID_PAT: mInfo = "Pat "; break;
      case PID_CAT: mInfo = "Cat "; break;
      case PID_SDT: mInfo = "Sdt "; break;
      case PID_NIT: mInfo = "Nit "; break;
      case PID_EIT: mInfo = "Eit "; break;
      case PID_RST: mInfo = "Rst "; break;
      case PID_TDT: mInfo = "Tdt "; break;
      case PID_SYN: mInfo = "Syn "; break;
      default:;
      }
    }
  //}}}
  ~cPidInfo() {}

  //{{{
  void print() {
    printf ("- pid:%4d sid:%5d stream:%2d - packets:%6d disCon:%d repCon:%d  \n",
            mPid,  mSid, mStreamType, mTotal, mDisContinuity, mRepeatContinuity);
    }
  //}}}

  int mPid;
  int mSid = -1;
  bool mIsSection;

  int mStreamType = 0;
  uint64_t mPts = 0;
  uint64_t mDts = 0;
  uint64_t mPcr = 0;

  int mTotal = 0;
  int mContinuity = -1;
  int mDisContinuity = 0;
  int mRepeatContinuity = 0;

  int mSectionLength = 0;

  // content buffer
  int mBufSize = 0;
  uint8_t* mBuffer = nullptr;
  uint8_t* mBufPtr = nullptr;

  std::string mInfo;
  };
//}}}
//{{{
class cEpgItem {
public:
  cEpgItem() : mStartTime(0), mDuration(0) {}
  //{{{
  cEpgItem (time_t startTime, int duration, std::string title, std::string shortDescription)
      : mStartTime(startTime), mDuration(duration) {
    mTitle = title;
    mShortDescription = shortDescription;
    }
  //}}}
  ~cEpgItem() {}

  //{{{
  std::string getNameStr() {
    return mTitle;
    }
  //}}}
  //{{{
  std::string getStartTimeStr() {
    struct tm time = *localtime (&mStartTime);
    char* timeStr = asctime (&time);
    timeStr[24] = 0;
    return std::string (timeStr);
    }
  //}}}

  //{{{
  void set (time_t startTime, int duration, std::string title, std::string shortDescription) {

    mDuration = duration;
    mStartTime = startTime;

    mTitle = title;
    mShortDescription = shortDescription;
    }
  //}}}

  //{{{
  void print (std::string prefix) {

    struct tm time = *localtime (&mStartTime);
    char* timeStr = asctime (&time);
    timeStr[24] = 0;

    printf ("%s\n", (prefix + timeStr + " " + std::to_string (mDuration/60) + " " + mTitle).c_str());
    }
  //}}}

  // vars
  time_t mStartTime;
  int mDuration;

  std::string mTitle;
  std::string mShortDescription;
  };
//}}}
//{{{
class cService {
public:
  //{{{
  cService (int sid, int tsid, int onid, int type, std::string name) : mSid(sid), mTsid(tsid), mOnid(onid), mType(type) {

    mName = name;
    mAudPids[0] = -1;
    mAudPids[1] = -1;
    }
  //}}}
  ~cService() {}

  //  gets
  int getSid() const { return mSid; }
  int getTsid() const { return mTsid; }
  int getOnid() const { return mOnid; }
  int getType() const { return mType; }
  //{{{
  std::string getTypeStr() {
    switch (mType) {
      case kServiceTypeTV :           return "TV";
      case kServiceTypeRadio :        return "Radio";
      case kServiceTypeAdvancedSDTV : return "SDTV";
      case kServiceTypeAdvancedHDTV : return "HDTV";
      default : return "none";
      }
    }
  //}}}

  int getVidPid() const { return mVidPid; }
  int getAudPid() const { return mAudPids[0]; }
  int getSubPid() const { return mSubPid; }
  int getPcrPid() const { return mPcrPid; }
  int getProgramPid() const { return mProgramPid; }

  cEpgItem* getNow() { return &mNow; }
  std::string getName() { return mName; }

  //  sets
  void setVidPid (const int pid) { mVidPid = pid; }
  void setAudPid (const int pid) {
    if (mNumAudPids == 0)
      mAudPids[mNumAudPids++] = pid;
    else if (mAudPids[0] != pid)
      mAudPids[1] = pid;
    }
  void setSubPid (const int pid) { mSubPid = pid; }
  void setPcrPid (const int pid) { mPcrPid = pid; }
  void setProgramPid (const int pid) { mProgramPid = pid; }

  //{{{
  bool setNow (time_t startTime, int duration, std::string str1, std::string str2) {

    bool nowChanged = (startTime != mNow.mStartTime);

    mNow.set (startTime, duration, str1, str2);

    return nowChanged;
    }
  //}}}
  //{{{
  void setEpg (time_t startTime, int duration, std::string str1, std::string str2) {

    mEpgItemMap.insert (tEpgItemMap::value_type (startTime, cEpgItem(startTime, duration, str1, str2)));
    }
  //}}}
  //{{{
  void print() {
    printf ("- sid:%d tsid:%d onid:%d - prog:%d v:%d a:%d sub:%d pcr:%d %s <%s>\n",
            mSid, mTsid, mOnid, mProgramPid, mVidPid, mNumAudPids, mSubPid, mPcrPid, getTypeStr().c_str(), mName.c_str());
    mNow.print ("  - ");

    for (auto epgItem : mEpgItemMap)
      epgItem.second.print ("    - ");
    }
  //}}}

private:
  int mSid;
  int mTsid;
  int mOnid;
  int mType;

  int mVidPid = -1;
  int mNumAudPids = 0;
  int mAudPids[2];
  int mSubPid = -1;
  int mPcrPid = -1;
  int mProgramPid = -1;

  std::string mName;

  cEpgItem mNow;

  typedef std::map<time_t,cEpgItem> tEpgItemMap;  // startTime, cEpgItem
  tEpgItemMap mEpgItemMap;
  };
//}}}
//{{{  map typedefs
typedef std::map<int,cPidInfo> tPidInfoMap;  // pid inserts <pid,cPidInfo> into mPidInfoMap
typedef std::map<int,int>      tProgramMap;  // PAT inserts <pid,sid>'s into mProgramMap
                                             //     - pids parsed as programPid PMTs
typedef std::map<int,cService> tServiceMap;  // SDT inserts <sid,cService> into mServiceMap
                                             //     - sets cService ServiceType,Name
                                             // PMT - sets cService stream pids
                                             // EIT - adds cService Now,Epg events
//}}}

class cTransportStream {
public:
  cTransportStream() {}
  virtual ~cTransportStream() {}

  int getPackets() { return mPackets; }
  int getDiscontinuity() { return mDiscontinuity; }

  //{{{
  void demux (uint8_t* tsPtr, uint8_t* tsEnd, bool skipped) {

    if (skipped)
      //{{{  reset pid continuity, buffers
      for (auto pidInfo : mPidInfoMap) {
        pidInfo.second.mContinuity = -1;
        pidInfo.second.mBufPtr = nullptr;
        }
      //}}}

    // while full packet
    int lostSync = 0;
    while (tsPtr+188 <= tsEnd) {
      if (*tsPtr++ != 0x47)
        lostSync++;
      else {
        if (lostSync) {
          //{{{  lostSync warning
          printf ("%d demux ****** resynced bytes:%d ******\n", mPackets, lostSync);
          lostSync = 0;
          }
          //}}}

        // full packet with start syncCode
        if ((tsPtr+187 == tsEnd) || (*(tsPtr+187) == 0x47)) {
          // last packet or succeeded by syncCode
          //{{{  parse ts packet
          bool payStart = ((*tsPtr) & 0x40) != 0;
          uint16_t pid = ((*tsPtr & 0x1F) << 8) | *(tsPtr+1);
          bool adaption = (*(tsPtr+2) & 0x20) != 0;
          uint8_t headerBytes = adaption ? (4 + *(tsPtr+3)) : 3;
          uint8_t continuity = *(tsPtr+2) & 0x0F;
          //{{{  unused bits
          //bool tei = *tsPtr & 0x80;
          //bool payload = *(tsPtr+2) & 0x10;
          //if ((adaption && (*(tsPtr+4) & 0x80)) discontinuity = true;
          //}}}

          bool hasPcr = adaption && (*(tsPtr+4) & 0x10);
          uint64_t pcr = hasPcr ? parsePcr (tsPtr) : 0;

          pidPacket (pid, tsPtr-1);

          tsPtr += headerBytes;
          auto tsFrameBytesLeft = 187 - headerBytes;

          auto isSection = (pid == PID_PAT) || (pid == PID_SDT) ||
                           (pid == PID_EIT) || (pid == PID_TDT) ||
                           (mProgramMap.find (pid) != mProgramMap.end());

          // find or create pidInfo
          auto pidInfoIt = mPidInfoMap.find (pid);
          if (pidInfoIt == mPidInfoMap.end()) {
            // new pid, insert new cPidInfo, get pidInfoIt iterator
            auto insertPair = mPidInfoMap.insert (tPidInfoMap::value_type (pid, cPidInfo(pid, isSection)));
            pidInfoIt = insertPair.first;
            }

          // test continuity, reset buffers if fail
          if ((pid != 0x1FFF) &&
              (pidInfoIt->second.mContinuity >= 0) &&
              (pidInfoIt->second.mContinuity != ((continuity-1) & 0x0F))) {
            // test continuity
            if (pidInfoIt->second.mContinuity == continuity) // strange case of bbc subtitles
              pidInfoIt->second.mRepeatContinuity++;
            else {
              mDiscontinuity++;
              pidInfoIt->second.mDisContinuity++;
              }
            pidInfoIt->second.mBufPtr = nullptr;
            }
          pidInfoIt->second.mContinuity = continuity;
          pidInfoIt->second.mTotal++;

          // use ts pcr
          if (hasPcr)
            pidInfoIt->second.mPcr = pcr;
          //}}}

          if (isSection) {
            //{{{  parse section pids
            if (payStart) {
              // parse sectionStart
              int pointerField = *tsPtr;
              if (pointerField && pidInfoIt->second.mBufPtr) {
                //{{{  payStart packet starts with end of lastSection
                // copy to end of buffer
                memcpy (pidInfoIt->second.mBufPtr, tsPtr+1, pointerField);

                // parse buffer if enough bytes for sectionLength
                pidInfoIt->second.mBufPtr += pointerField;
                if (pidInfoIt->second.mBufPtr - pidInfoIt->second.mBuffer >= pidInfoIt->second.mSectionLength)
                  parseSection (pid, pidInfoIt->second.mBuffer, 1);

                /// reset buffer
                pidInfoIt->second.mBufPtr = nullptr;
                }
                //}}}

              do {
                pidInfoIt->second.mSectionLength = (((*(tsPtr + pointerField + 2) & 0x0F) << 8) | *(tsPtr + pointerField + 3)) + 3;
                if (pointerField < 183 - pidInfoIt->second.mSectionLength) {
                  parseSection (pid, tsPtr + pointerField + 1, 3);
                  pointerField += pidInfoIt->second.mSectionLength;
                  }
                else {
                  // section straddles packets, start buffering
                  if (!pidInfoIt->second.mBuffer) {
                    pidInfoIt->second.mBufSize = 4096; // guess at biggest section size
                    pidInfoIt->second.mBuffer = (uint8_t*)malloc(pidInfoIt->second.mBufSize);
                    }
                  memcpy (pidInfoIt->second.mBuffer, tsPtr + pointerField + 1, 183 - pointerField);
                  pidInfoIt->second.mBufPtr = pidInfoIt->second.mBuffer + 183 - pointerField;
                  break;
                  }
                } while (*(tsPtr + pointerField + 1) != 0xFF);
              }

            else if (pidInfoIt->second.mBufPtr) {
              // add to buffered section
              if (pidInfoIt->second.mBufPtr + tsFrameBytesLeft > pidInfoIt->second.mBuffer + pidInfoIt->second.mBufSize)
                printf ("%d sectionBuffer overflow > 4096 %d\n",
                        mPackets, (int)(pidInfoIt->second.mBufPtr - pidInfoIt->second.mBuffer));
              else {
                memcpy (pidInfoIt->second.mBufPtr, tsPtr, tsFrameBytesLeft);
                pidInfoIt->second.mBufPtr += tsFrameBytesLeft;
                }

              if ((pidInfoIt->second.mBufPtr - pidInfoIt->second.mBuffer) >= pidInfoIt->second.mSectionLength) {
                // enough bytes to parse buffered section
                parseSection (pid, pidInfoIt->second.mBuffer, 4);
                pidInfoIt->second.mBufPtr = 0;
                }
              }
            }
            //}}}
          else {
            if (payStart && !(*tsPtr) && !(*(tsPtr+1)) && (*(tsPtr+2) == 1) && (*(tsPtr+3) == 0xc0)) {
              //{{{  start audio pes
              if (pidInfoIt->second.mBufPtr)
                decodeAudPes (&pidInfoIt->second);

              //  start next audPES
              if (!pidInfoIt->second.mBuffer) {
                // first audPES, allocate buffer
                pidInfoIt->second.mBufSize = 10000;
                pidInfoIt->second.mBuffer= (uint8_t*)malloc (pidInfoIt->second.mBufSize);
                }

              pidInfoIt->second.mBufPtr = pidInfoIt->second.mBuffer;
              parseTimeStamps (tsPtr, pidInfoIt->second.mPts, pidInfoIt->second.mDts);

              int pesHeaderBytes = 9 + *(tsPtr+8);
              tsPtr += pesHeaderBytes;
              tsFrameBytesLeft -= pesHeaderBytes;
              }
              //}}}
            else if (payStart && !(*tsPtr) && !(*(tsPtr+1)) && (*(tsPtr+2) == 1) && (*(tsPtr+3) == 0xe0)) {
              //{{{  start video pes
              if (pidInfoIt->second.mBufPtr)
                decodeVidPes (&pidInfoIt->second);

              //  start next vidPES
              if (!pidInfoIt->second.mBuffer) {
                // first vidPES, allocate buffer
                pidInfoIt->second.mBufSize = 600000;
                pidInfoIt->second.mBuffer= (uint8_t*)malloc (pidInfoIt->second.mBufSize);
                }

              pidInfoIt->second.mBufPtr = pidInfoIt->second.mBuffer;
              parseTimeStamps (tsPtr, pidInfoIt->second.mPts, pidInfoIt->second.mDts);

              int pesHeaderBytes = 9 + *(tsPtr+8);
              tsPtr += pesHeaderBytes;
              tsFrameBytesLeft -= pesHeaderBytes;
              }
              //}}}
            if (pidInfoIt->second.mBufPtr) {
              //{{{  copy tsFrameBytesLeft bytes to buffer
              memcpy (pidInfoIt->second.mBufPtr, tsPtr, tsFrameBytesLeft);
              pidInfoIt->second.mBufPtr += tsFrameBytesLeft;

              if (pidInfoIt->second.mBufPtr > pidInfoIt->second.mBuffer+ pidInfoIt->second.mBufSize)
                printf ("%d demux *** PES overflow *** %d %d\n",
                        mPackets, int(pidInfoIt->second.mBufPtr - pidInfoIt->second.mBuffer), pidInfoIt->second.mBufSize);
              }
              //}}}
            }

          tsPtr += tsFrameBytesLeft;
          mPackets++;
          }
        }
      }
    }
  //}}}

  //{{{
  void printPids() {

    printf ("--- PidInfoMap -----\n");
    for (auto pidInfo : mPidInfoMap)
      pidInfo.second.print();
    }
  //}}}
  //{{{
  void printServices() {

    printf ("--- ServiceMap -----\n");
    for (auto service : mServiceMap)
      service.second.print();
    }
  //}}}
  //{{{
  void printPrograms() {

    printf ("--- ProgramMap -----\n");
    for (auto map : mProgramMap)
      printf ("- programPid:%d sid:%d\n", map.first, map.second);
    }
  //}}}

protected:
  virtual void pidPacket (int pid, uint8_t* ptr) {}
  virtual void decodeAudPes (cPidInfo* pidInfo) {}
  virtual void decodeVidPes (cPidInfo* pidInfo) {}
  virtual void startProgram (int vpid, int apid, std::string name, std::string startTime) {}

  tServiceMap mServiceMap;
  tPidInfoMap mPidInfoMap;
  std::string mTimeStr;
  std::string mNetworkNameStr;

private:
  //{{{
  void parseSection (int pid, uint8_t* buf, int tag) {

    switch (pid) {
      case PID_NIT: parseNit (buf); break;
      case PID_TDT: parseTdt (buf); break;
      case PID_PAT: parsePat (buf); break;
      case PID_SDT: parseSdt (buf); break;
      case PID_EIT: parseEit (buf, tag); break;
      default:      parsePmt (pid, buf); break;
      }
    }
  //}}}
  //{{{
  void parseNit (uint8_t* buf) {

    auto Nit = (nit_t*)buf;
    auto sectionLength = HILO(Nit->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  bad crc
      printf ("parseNIT - bad crc %d\n", sectionLength);
      return;
      }
      //}}}
    if ((Nit->table_id != TID_NIT_ACT) &&
        (Nit->table_id != TID_NIT_OTH) &&
        (Nit->table_id != TID_BAT)) {
      //{{{  wrong tid
      printf ("parseNIT - wrong TID %x\n", Nit->table_id);
      return;
      }
      //}}}

    auto networkId = HILO (Nit->network_id);

    auto ptr = buf + NIT_LEN;
    auto loopLength = HILO (Nit->network_descr_length);
    sectionLength -= NIT_LEN + 4;
    if (loopLength <= sectionLength) {
      if (sectionLength >= 0)
        parseDescrs ("parseNIT1", networkId, ptr, loopLength, Nit->table_id);
      sectionLength -= loopLength;

      ptr += loopLength;
      auto NitMid = (nit_mid_t*) ptr;
      loopLength = HILO (NitMid->transport_stream_loop_length);
      if ((sectionLength > 0) && (loopLength <= sectionLength)) {
        // iterate nitMids
        sectionLength -= SIZE_NIT_MID;
        ptr += SIZE_NIT_MID;

        while (loopLength > 0) {
          auto TSDesc = (nit_ts_t*)ptr;
          auto tsid = HILO (TSDesc->transport_stream_id);

          auto loop2Length = HILO (TSDesc->transport_descrs_length);
          ptr += NIT_TS_LEN;
          if (loop2Length <= loopLength)
            if (loopLength >= 0)
              parseDescrs ("parseNIT2", tsid, ptr, loop2Length, Nit->table_id);

          loopLength -= loop2Length + NIT_TS_LEN;
          sectionLength -= loop2Length + NIT_TS_LEN;
          ptr += loop2Length;
          }
        }
      }
    }
  //}}}
  //{{{
  void parseTdt (uint8_t* buf) {

    auto Tdt = (tdt_t*)buf;
    if (Tdt->table_id == TID_TDT) {
      mCurTime = MjdToEpochTime (Tdt->utc_mjd) + BcdTimeToSeconds (Tdt->utc_time);

      tm time = *localtime (&mCurTime);

      static const char wday_name[][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
      static const char mon_name[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
      mTimeStr = std::to_string (time.tm_hour) + ":" +
                 std::to_string (time.tm_min) + ":" +
                 std::to_string (time.tm_sec) + " " +
                 wday_name[time.tm_wday] + " " +
                 std::to_string (time.tm_mday) + " " +
                 mon_name[time.tm_mon] + " " +
                 std::to_string (1900 + time.tm_year);
      }
    }
  //}}}
  //{{{
  void parsePat (uint8_t* buf) {
  // PAT declares programPid,sid to mPogramMap, recognieses programPid PMT to declare service streams

    auto Pat = (pat_t*)buf;
    auto sectionLength = HILO(Pat->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  bad crc
      printf ("parsePAT - bad crc %d\n", sectionLength);
      return;
      }
      //}}}
    if (Pat->table_id != TID_PAT) {
      //{{{  wrong tid
      printf ("parsePAT - unexpected TID %x\n", Pat->table_id);
      return;
      }
      //}}}

    auto ptr = buf + PAT_LEN;
    sectionLength -= PAT_LEN + 4;
    while (sectionLength > 0) {
      auto patProgram = (pat_prog_t*)ptr;
      auto sid = HILO (patProgram->program_number);
      auto pid = HILO (patProgram->network_pid);
      if (mProgramMap.find (pid) == mProgramMap.end())
        mProgramMap.insert (tProgramMap::value_type (pid, sid));

      sectionLength -= PAT_PROG_LEN;
      ptr += PAT_PROG_LEN;
      }
    }
  //}}}
  //{{{
  void parseSdt (uint8_t* buf) {
  // SDT add new services to mServiceMap declaring serviceType, name

    auto Sdt = (sdt_t*)buf;
    auto sectionLength = HILO(Sdt->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  wrong crc
      printf ("parseSDT - bad crc %d\n", sectionLength);
      return;
      }
      //}}}
    if (Sdt->table_id == TID_SDT_OTH) // ignore other multiplex services for now
      return;
    if (Sdt->table_id != TID_SDT_ACT) {
      //{{{  wrong tid
      printf ("parseSDT - unexpected TID %x\n", Sdt->table_id);
      return;
      }
      //}}}

    //printf ("SDT - tsid:%d onid:%d\n", tsid, onid);

    auto ptr = buf + SDT_LEN;
    sectionLength -= SDT_LEN + 4;
    while (sectionLength > 0) {
      auto SdtDescr = (sdt_descr_t*)ptr;
      auto sid = HILO (SdtDescr->service_id);
      auto free_ca_mode = SdtDescr->free_ca_mode;
      //auto running_status = SdtDescr->running_status;
      auto loopLength = HILO (SdtDescr->descrs_loop_length);
      ptr += SDT_DESCR_LEN;

      auto DescrLength = 0;
      while ((DescrLength < loopLength) &&
             (GetDescrLength (ptr) > 0) &&
             (GetDescrLength (ptr) <= loopLength - DescrLength)) {

        switch (GetDescrTag (ptr)) {
          case DESCR_SERVICE: { // 0x48
            //{{{  service
            auto serviceType = CastServiceDescr(ptr)->service_type;
            if ((free_ca_mode == 0) &&
                ((serviceType == kServiceTypeTV)  ||
                 (serviceType == kServiceTypeRadio) ||
                 (serviceType == kServiceTypeAdvancedHDTV) ||
                 (serviceType == kServiceTypeAdvancedSDTV))) {

              auto it = mServiceMap.find (sid);
              if (it == mServiceMap.end()) {
                // new service
                auto tsid = HILO (Sdt->transport_stream_id);
                auto onid = HILO (Sdt->original_network_id);

                std::string nameStr = getDescrStr (
                  ptr + DESCR_SERVICE_LEN + CastServiceDescr(ptr)->provider_name_length + 1,
                  *((uint8_t*)(ptr + DESCR_SERVICE_LEN + CastServiceDescr(ptr)->provider_name_length)));

                // insert new cService, get serviceIt iterator
                auto pair = mServiceMap.insert (tServiceMap::value_type (sid, cService (sid, tsid, onid, serviceType, nameStr)));
                auto serviceIt = pair.first;
                printf ("SDT new cService tsid:%d sid:%d %s name<%s>\n",
                        tsid, sid, serviceIt->second.getTypeStr().c_str(), nameStr.c_str());
                }
              }

            break;
            }
            //}}}
          case DESCR_PRIV_DATA_SPEC:
          case DESCR_CA_IDENT:
          case DESCR_COUNTRY_AVAIL:
          case DESCR_DATA_BROADCAST:
          case 0x73: // default_authority
          case 0x7e: // FTA_content_management
          case 0x7f: // extension
            break;
          default:
            //{{{  other
            printf ("SDT - unexpected tag:%x\n", GetDescrTag(ptr));
            break;
            //}}}
          }

        DescrLength += GetDescrLength (ptr);
        ptr += GetDescrLength (ptr);
        }

      sectionLength -= loopLength + SDT_DESCR_LEN;
      }
    }
  //}}}
  //{{{
  void parsePmt (int pid, uint8_t* buf) {
  // PMT declares streams for a service

    auto pmt = (pmt_t*)buf;
    auto sectionLength = HILO(pmt->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  bad crc
      printf ("parsePMT - pid:%d bad crc %d\n", pid, sectionLength);
      return;
      }
      //}}}
    if (pmt->table_id != TID_PMT) {
      //{{{  wrong tid
      printf ("parsePMT - wrong TID %x\n", pmt->table_id);
      return;
      }
      //}}}

    auto sid = HILO (pmt->program_number);
    //printf ("PMT - pid:%d sid:%d\n", pid, sid);

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end()) {
      //{{{  service declared by SMT
      serviceIt->second.setProgramPid (pid);

      // point programPid to service by sid
      auto sectionIt = mPidInfoMap.find (pid);
      if (sectionIt != mPidInfoMap.end())
        sectionIt->second.mSid = sid;
      updatePidInfo (pid);

      serviceIt->second.setPcrPid (HILO (pmt->PCR_PID));

      auto ptr = buf + PMT_LEN;
      sectionLength -= 4;
      auto programInfoLength = HILO (pmt->program_info_length);

      auto streamLength = sectionLength - programInfoLength - PMT_LEN;
      if (streamLength >= 0)
        parseDescrs ("parsePMT1", sid, ptr, programInfoLength, pmt->table_id);

      ptr += programInfoLength;
      while (streamLength > 0) {
        auto pmtInfo = (pmt_info_t*)ptr;
        auto streamType = pmtInfo->stream_type;
        auto esPid = HILO (pmtInfo->elementary_PID);

        auto recognised = true;
        switch (streamType) {
          case 2:  serviceIt->second.setVidPid (esPid); break; // ISO 13818-2 video
          case 3:  serviceIt->second.setAudPid (esPid); break; // ISO 11172-3 audio
          case 4:  serviceIt->second.setAudPid (esPid); break; // ISO 13818-3 audio
          case 6:  serviceIt->second.setSubPid (esPid); break; // subtitle
          case 17: serviceIt->second.setAudPid (esPid); break; // HD aud
          case 27: serviceIt->second.setVidPid (esPid); break; // HD vid
          case 5:  recognised = false; break;
          case 11: recognised = false; break;
          case 13:
          default:
            printf ("parsePMT - unknown streamType:%d sid:%d esPid:%d\n", streamType, sid, esPid);
            recognised = false;
            break;
          }

        if (recognised) {
          // set sid for each stream pid
          auto sectionIt = mPidInfoMap.find (esPid);
          if (sectionIt != mPidInfoMap.end()) {
            sectionIt->second.mSid = sid;
            sectionIt->second.mStreamType = streamType;
            }
          updatePidInfo (esPid);
          }

        auto loopLength = HILO (pmtInfo->ES_info_length);
        parseDescrs ("parsePMT2", sid, ptr, loopLength, pmt->table_id);

        ptr += PMT_INFO_LEN;
        streamLength -= loopLength + PMT_INFO_LEN;
        ptr += loopLength;
        }
      }
      //}}}
    else if (pid == 32) {
      // simple tsFile with no SDT, pid 32 used by a single program tsFile, allocate service with sid
      printf ("parsePMT pid:32 - fake service for simple single program file sid:%d\n", sid);

      // insert new cService
      mServiceMap.insert (tServiceMap::value_type (sid, cService (sid, 0, 0, kServiceTypeTV, "tsFile")));
      }
    }
  //}}}
  //{{{
  void parseEit (uint8_t* buf, int tag) {

    auto Eit = (eit_t*)buf;
    auto sectionLength = HILO(Eit->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //printf ("%d parseEit len:%d tag:%d Bad CRC  \n", mPackets, sectionLength, tag);
      return;
      }

    auto tid = Eit->table_id;
    auto sid = HILO (Eit->service_id);
    auto now = (tid == TID_EIT_ACT);
    auto next = (tid == TID_EIT_OTH);
    auto epg = (tid == TID_EIT_ACT_SCH) || (tid == TID_EIT_OTH_SCH) ||
               (tid == TID_EIT_ACT_SCH+1) || (tid == TID_EIT_OTH_SCH+1);
    if (!now && !next && !epg) {
      //{{{  unexpected tid
      printf ("parseEIT - unexpected tid:%x\n", tid);
      return;
      }
      //}}}

    // parse Descrs
    auto ptr = buf + EIT_LEN;
    sectionLength -= EIT_LEN + 4;
    while (sectionLength > 0) {
      auto EitEvent = (eit_event_t*)ptr;
      auto loopLength = HILO (EitEvent->descrs_loop_length);
      if (loopLength > sectionLength - EIT_EVENT_LEN)
        return;
      ptr += EIT_EVENT_LEN;

      auto DescrLength = 0;
      while ((DescrLength < loopLength) &&
             (GetDescrLength (ptr) > 0) &&
             (GetDescrLength (ptr) <= loopLength - DescrLength)) {

        switch (GetDescrTag(ptr)) {
          case DESCR_SHORT_EVENT: {
            //{{{  shortEvent
            auto it = mServiceMap.find (sid);
            if (it != mServiceMap.end()) {
              // recognised service
              auto startTime = MjdToEpochTime (EitEvent->mjd) + BcdTimeToSeconds (EitEvent->start_time);
              auto duration = BcdTimeToSeconds (EitEvent->duration);
              auto running = (EitEvent->running_status == 0x04);

              auto title = huffDecode (ptr + DESCR_SHORT_EVENT_LEN, CastShortEventDescr(ptr)->event_name_length);
              std::string titleStr = title ?
                title : getDescrStr (ptr + DESCR_SHORT_EVENT_LEN, CastShortEventDescr(ptr)->event_name_length);

              auto shortDescription = huffDecode (
                ptr + DESCR_SHORT_EVENT_LEN + CastShortEventDescr(ptr)->event_name_length+1,
                size_t(ptr + DESCR_SHORT_EVENT_LEN + CastShortEventDescr(ptr)->event_name_length));
              std::string shortDescriptionStr = shortDescription ?
                shortDescription : getDescrStr (
                  ptr + DESCR_SHORT_EVENT_LEN + CastShortEventDescr(ptr)->event_name_length+1,
                  *((uint8_t*)(ptr + DESCR_SHORT_EVENT_LEN + CastShortEventDescr(ptr)->event_name_length)));

              if (now & running) {
                if (it->second.setNow (startTime, duration, titleStr, shortDescriptionStr)) {
                  updatePidInfo (it->second.getProgramPid());
                  updatePidInfo (it->second.getVidPid());
                  updatePidInfo (it->second.getAudPid());
                  updatePidInfo (it->second.getSubPid());
                  startProgram (it->second.getVidPid(), it->second.getAudPid(),
                                it->second.getNow()->getNameStr(), it->second.getNow()->getStartTimeStr());
                  }
                }
              else if (epg)
                it->second.setEpg (startTime, duration, titleStr, shortDescriptionStr);
              }

            break;
            }
            //}}}
          case DESCR_EXTENDED_EVENT: {
            //{{{  extendedEvent
            //0x4E extended_event_descr
            //#define DESCR_EXTENDED_EVENT_LEN 7
            //typedef struct descr_extended_event_struct {
            //  uint8_t descr_tag                         :8;
             // uint8_t descr_length                      :8;
            //  /* TBD */
            //  uint8_t last_descr_number                 :4;
            //  uint8_t descr_number                      :4;
            //  uint8_t lang_code1                             :8;
            //  uint8_t lang_code2                             :8;
            //  uint8_t lang_code3                             :8;
            //  uint8_t length_of_items                        :8;
            //  } descr_extended_event_t;
            //#define CastExtendedEventdescr(x) ((descr_extended_event_t *)(x))
            //#define ITEM_EXTENDED_EVENT_LEN 1
            //typedef struct item_extended_event_struct {
            //  uint8_t item_description_length               :8;
            //  } item_extended_event_t;
            //#define CastExtendedEventItem(x) ((item_extended_event_t *)(x))

            #ifdef EIT_EXTENDED_EVENT_DEBUG
              //{{{  print eit extended event
              printf ("EIT extendedEvent sid:%d descLen:%d lastDescNum:%d DescNum:%d item:%d\n",
                      sid, GetDescrLength (ptr),
                      CastExtendedEventDescr(ptr)->last_descr_number,
                      CastExtendedEventDescr(ptr)->descr_number,
                      CastExtendedEventDescr(ptr)->length_of_items);

              for (auto i = 0; i < GetDescrLength (ptr) -2; i++) {
                char c = *(ptr + 2 + i);
                if ((c >= 0x20) && (c <= 0x7F))
                  printf ("%c", c);
                else
                  printf("<%02x> ", c & 0xFF);
                }
              printf ("\n");
              //}}}
            #endif

            break;
            }
            //}}}
          case DESCR_COMPONENT:      // 0x50
          case DESCR_CA_IDENT:       // 0x53
          case DESCR_CONTENT:        // 0x54
          case 0x5f:                 // private_data
          case DESCR_ML_COMPONENT:   // 0x5e
          case DESCR_DATA_BROADCAST: // 0x64
          case 0x76:                 // content_identifier
          case 0x7e:                 // FTA_content_management
            //printf ("parseEIT - expected tag:%x %d %d\n", GetDescrTag(ptr), tid, sid);
            break;
          case 0x4a:                 // linkage
          case 0x89:                 // user_defined
          default:
            //printf ("parseEIT - unexpected tag:%x %d %d\n", GetDescrTag(ptr), tid, sid);
            break;
          }

        DescrLength += GetDescrLength (ptr);
        ptr += GetDescrLength (ptr);
        }

      sectionLength -= loopLength + EIT_EVENT_LEN;
      ptr += loopLength;
      }
    }
  //}}}

  //{{{
  uint64_t parsePcr (uint8_t* tsPtr) {
  // return 33 bits of pcr

    return ((*(tsPtr+5)<<25) |
            (*(tsPtr+6)<<17) |
            (*(tsPtr+7)<<9) |
            (*(tsPtr+8)<<1) |
            (*(tsPtr+9)>>7))  & 0x1FFFFFFFF;
    }
  //}}}
  //{{{
  uint64_t parseTimeStamp (uint8_t* tsPtr) {
  // return 33 bits of pts, dts

    return ((((*tsPtr) & 0x0E) << 29) |
            (*(tsPtr+1) << 22) |
           ((*(tsPtr+2) & 0xFE) << 14) |
            (*(tsPtr+3) << 7)  |
            (*(tsPtr+4) >> 1)) & 0x1FFFFFFFF;
    }
  //}}}
  //{{{
  void parseTimeStamps (uint8_t* tsPtr, uint64_t& pts, uint64_t& dts) {
    pts = ((*(tsPtr+8) >= 5) && (*(tsPtr+7) & 0x80)) ? parseTimeStamp (tsPtr+9) : 0;
    dts = ((*(tsPtr+8) >= 10) && (*(tsPtr+7) & 0x40)) ? parseTimeStamp (tsPtr+14) : 0;
    }
  //}}}
  //{{{
  std::string getDescrStr (uint8_t* buf, int len) {

    std::string str;
    for (auto i = 0; i < len; i++) {
      if (*buf == 0)
        break;
      if ((*buf >= ' ' && *buf <= '~') || (*buf == '\n') || (*buf >= 0xa0 && *buf <= 0xff))
        str += *buf;
      if (*buf == 0x8A)
        str += '\n';
      if ((*buf == 0x86 || *buf == 0x87))
        str += ' ';
      buf++;
      }

    return str;
    }
  //}}}
  //{{{
  //{{{
  void parseDescr (std::string sectionName, int key, uint8_t* buf, int tid) {
  //}}}

    switch (GetDescrTag(buf)) {
      case 0x25: // NIT2
        break;
      case DESCR_EXTENDED_EVENT: {
        //{{{  extended event
        std::string text = getDescrStr (
          buf + DESCR_EXTENDED_EVENT_LEN + CastExtendedEventDescr(buf)->length_of_items + 1,
          *((uint8_t*)(buf + DESCR_EXTENDED_EVENT_LEN + CastExtendedEventDescr(buf)->length_of_items)));

        printf ("extended event - %d %d %c%c%c %s\n",
                CastExtendedEventDescr(buf)->descr_number, CastExtendedEventDescr(buf)->last_descr_number,
                CastExtendedEventDescr(buf)->lang_code1,
                CastExtendedEventDescr(buf)->lang_code2,
                CastExtendedEventDescr(buf)->lang_code3, text.c_str());

        auto ptr = buf + DESCR_EXTENDED_EVENT_LEN;
        auto length = CastExtendedEventDescr(buf)->length_of_items;
        while ((length > 0) && (length < GetDescrLength (buf))) {
          text = getDescrStr (ptr + ITEM_EXTENDED_EVENT_LEN, CastExtendedEventItem(ptr)->item_description_length);
          std::string text2 = getDescrStr (
            ptr + ITEM_EXTENDED_EVENT_LEN + CastExtendedEventItem(ptr)->item_description_length + 1,
            *(uint8_t* )(ptr + ITEM_EXTENDED_EVENT_LEN + CastExtendedEventItem(ptr)->item_description_length));
          printf ("- %s %s\n", text.c_str(), text2.c_str());

          length -= ITEM_EXTENDED_EVENT_LEN + CastExtendedEventItem(ptr)->item_description_length +
                    *((uint8_t* )(ptr + ITEM_EXTENDED_EVENT_LEN +
                    CastExtendedEventItem(ptr)->item_description_length)) + 1;
          ptr += ITEM_EXTENDED_EVENT_LEN + CastExtendedEventItem(ptr)->item_description_length +
                 *((uint8_t* )(ptr + ITEM_EXTENDED_EVENT_LEN +
                 CastExtendedEventItem(ptr)->item_description_length)) + 1;
          }

        break;
        }
        //}}}
      case DESCR_COMPONENT: {
        //{{{  component
        std::string str = getDescrStr (buf + DESCR_COMPONENT_LEN, GetDescrLength (buf) - DESCR_COMPONENT_LEN);
        printf ("component %2d %2d %d %s\n",
                CastComponentDescr(buf)->stream_content,
                CastComponentDescr(buf)->component_type,
                CastComponentDescr(buf)->component_tag, str.c_str());
        break;
        }
        //}}}
      case DESCR_NW_NAME: {
        //{{{  networkName
        mNetworkNameStr = getDescrStr (buf + DESCR_BOUQUET_NAME_LEN, GetDescrLength (buf) - DESCR_BOUQUET_NAME_LEN);
        break;
        }
        //}}}
      case DESCR_TERR_DEL_SYS: {
        //{{{  terr del sys
        //auto tds = (descr_terrestrial_delivery_system_t*)buf;
        // ((tds->frequency1 << 24) | (tds->frequency2 << 16) |
        //  (tds->frequency3 << 8)  | tds->frequency4) / 100,
        //   tds->bandwidth)));
        break;
        }
        //}}}
      case DESCR_SERVICE_LIST: {
        //{{{  service list
        auto ptr = buf;
        auto length = GetDescrLength(buf);

        while (length > 0) {
          //auto sid = HILO (CastServiceListDescrLoop(ptr)->service_id);
          //auto serviceType = CastServiceListDescrLoop(ptr)->service_type;
          //printf ("serviceList sid:%5d type:%d\n", sid, serviceType);
          length -= DESCR_SERVICE_LIST_LEN;
          ptr += DESCR_SERVICE_LIST_LEN;
          }

        break;
        }
        //}}}
      //{{{  expected Descrs
      case DESCR_COUNTRY_AVAIL:
      case DESCR_CONTENT:
      case DESCR_CA_IDENT:
      case DESCR_ML_COMPONENT:
      case DESCR_DATA_BROADCAST:
      case DESCR_PRIV_DATA_SPEC:
      case DESCR_SYSTEM_CLOCK:
      case DESCR_MULTIPLEX_BUFFER_UTIL:
      case DESCR_MAXIMUM_BITRATE:
      case DESCR_SMOOTHING_BUFFER:
      case DESCR_FREQUENCY_LIST:  // NIT1
      case DESCR_LINKAGE:
      case 0x73:
      case 0x7F:                  // NIT2
      case 0x83:
      case 0xC3:
        break;
      //}}}
      default:
        printf ("parseDescr - %s unexpected descr:%02X\n", sectionName.c_str(), GetDescrTag(buf));
        break;
      }
    }
  //}}}
  //{{{
  void parseDescrs (std::string sectionName, int key, uint8_t* buf, int len, uint8_t tid) {

   auto ptr = buf;
   auto DescrLength = 0;

   while (DescrLength < len) {
     if ((GetDescrLength (ptr) <= 0) || (GetDescrLength (ptr) > len - DescrLength))
       return;

      parseDescr (sectionName, key, ptr, tid);

      DescrLength += GetDescrLength (ptr);
      ptr += GetDescrLength (ptr);
      }
    }
  //}}}

  //{{{
  void updatePidInfo (int pid) {
  // update pid cPidInfo UI text for speed, lock avoidance

    // cPidInfo from pid using mPidInfoMap
    auto pidInfoIt = mPidInfoMap.find (pid);
    if (pidInfoIt != mPidInfoMap.end()) {

      // cService from cPidInfo.sid using mServiceMap
      auto serviceIt = mServiceMap.find (pidInfoIt->second.mSid);
      if (serviceIt != mServiceMap.end()) {
        std::string str = serviceIt->second.getName() + serviceIt->second.getNow()->mTitle;
        if (pid == serviceIt->second.getVidPid())
          pidInfoIt->second.mInfo = "vid " + str;
        else if (pid == serviceIt->second.getAudPid())
          pidInfoIt->second.mInfo = "aud " + str;
        else if (pid == serviceIt->second.getSubPid())
          pidInfoIt->second.mInfo = "sub " + str;
        else if (pid == serviceIt->second.getProgramPid())
          pidInfoIt->second.mInfo = "pgm " + str;
        }
      }
    }
  //}}}
  //{{{
  bool crc32 (uint8_t* data, int len) {

    auto crc = 0xffffffff;
    for (auto i = 0; i < len; i++)
      crc = (crc << 8) ^ crcTable[((crc >> 24) ^ *data++) & 0xff];

    return crc != 0;
    }
  //}}}

  //{{{  private vars
  tProgramMap mProgramMap;

  int mPackets = 0;
  int mDiscontinuity = 0;

  time_t mCurTime;
  //}}}
  };
