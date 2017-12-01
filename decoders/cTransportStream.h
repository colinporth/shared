// cTransportStream.h - transport stream parser
#pragma once
//{{{  includes
#include <string>
#include <map>
#include <time.h>

#include "dvbSubtitle.h"
//}}}
//{{{  macros
#define HILO(x) (x##_hi << 8 | x##_lo)

#define MjdToEpochTime(x) (unsigned int)((((x##_hi << 8) | x##_lo) - 40587) * 86400)

#define BcdTimeToSeconds(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                               (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                                     ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))
//}}}
//{{{  const, struct
const int kBufSize = 4096;
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
//{{{  HD epg huffman decode
// const
#define START   '\0'
#define STOP    '\0'
#define ESCAPE  '\1'

// type
struct tHuffTable {
  unsigned int value;
  short bits;
  char next;
  };

// table constants
//{{{
struct tHuffTable huffTable1[] = {
    /*   51                             */
    { 0x00000000,  2,  84}, /*    0 'T' */
    { 0x40000000,  3,  66}, /*    1 'B' */
    { 0x80000000,  4,  67}, /*    2 'C' */
    { 0x90000000,  4,  73}, /*    3 'I' */
    { 0xd0000000,  4,  83}, /*    4 'S' */
    { 0x60000000,  5,  76}, /*    5 'L' */
    { 0x70000000,  5,  68}, /*    6 'D' */
    { 0x78000000,  5,  72}, /*    7 'H' */
    { 0xa0000000,  5,  82}, /*    8 'R' */
    { 0xa8000000,  5,  78}, /*    9 'N' */
    { 0xb0000000,  5,  69}, /*   10 'E' */
    { 0xc0000000,  5,  70}, /*   11 'F' */
    { 0xc8000000,  5,  65}, /*   12 'A' */
    { 0xe0000000,  5,  77}, /*   13 'M' */
    { 0xe8000000,  5,  80}, /*   14 'P' */
    { 0xf0000000,  5,  87}, /*   15 'W' */
    { 0x6c000000,  6,  81}, /*   16 'Q' */
    { 0xbc000000,  6,  71}, /*   17 'G' */
    { 0xf8000000,  6,  74}, /*   18 'J' */
    { 0x68000000,  7,  75}, /*   19 'K' */
    { 0xba000000,  7,  85}, /*   20 'U' */
    { 0xfc000000,  7,  79}, /*   21 'O' */
    { 0x6a000000,  8,  54}, /*   22 '6' */
    { 0x6b000000,  8,  46}, /*   23 '.' */
    { 0xb8000000,  8,  86}, /*   24 'V' */
    { 0xfe000000,  8,  89}, /*   25 'Y' */
    { 0xb9800000,  9,  50}, /*   26 '2' */
    { 0xff800000,  9,  88}, /*   27 'X' */
    { 0xb9000000, 10,  90}, /*   28 'Z' */
    { 0xff000000, 10,  56}, /*   29 '8' */
    { 0xb9400000, 11,  49}, /*   30 '1' */
    { 0xb9600000, 11,  51}, /*   31 '3' */
    { 0xff400000, 12,  52}, /*   32 '4' */
    { 0xff500000, 12,  39}, /*   33 '\'' */
    { 0xff700000, 12,  32}, /*   34 ' ' */
    { 0xff600000, 14,  53}, /*   35 '5' */
    { 0xff6c0000, 14,  48}, /*   36 '0' */
    { 0xff660000, 15, 109}, /*   37 'm' */
    { 0xff640000, 16,  99}, /*   38 'c' */
    { 0xff680000, 16,  57}, /*   39 '9' */
    { 0xff6a0000, 16,  97}, /*   40 'a' */
    { 0xff6b0000, 16, 100}, /*   41 'd' */
    { 0xff650000, 17, 115}, /*   42 's' */
    { 0xff658000, 17, 112}, /*   43 'p' */
    { 0xff690000, 17,  40}, /*   44 '(' */
    { 0xff698000, 18, 116}, /*   45 't' */
    { 0xff69c000, 19,  55}, /*   46 '7' */
    { 0xff69e000, 20,   1}, /*   47 '0x01' */
    { 0xff69f000, 20, 108}, /*   48 'l' */
    { 0x00000000,  1,   1}, /*   49 '0x01' */
    { 0x80000000,  1,   1}, /*   50 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   51 '0x01' */
    { 0x80000000,  1,   1}, /*   52 '0x01' */
    /*    0                             */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   53 '0x01' */
    { 0x80000000,  1,   1}, /*   54 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   55 '0x01' */
    { 0x80000000,  1,   1}, /*   56 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   57 '0x01' */
    { 0x80000000,  1,   1}, /*   58 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   59 '0x01' */
    { 0x80000000,  1,   1}, /*   60 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   61 '0x01' */
    { 0x80000000,  1,   1}, /*   62 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   63 '0x01' */
    { 0x80000000,  1,   1}, /*   64 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   65 '0x01' */
    { 0x80000000,  1,   1}, /*   66 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   67 '0x01' */
    { 0x80000000,  1,   1}, /*   68 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   69 '0x01' */
    { 0x80000000,  1,   1}, /*   70 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   71 '0x01' */
    { 0x80000000,  1,   1}, /*   72 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   73 '0x01' */
    { 0x80000000,  1,   1}, /*   74 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   75 '0x01' */
    { 0x80000000,  1,   1}, /*   76 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   77 '0x01' */
    { 0x80000000,  1,   1}, /*   78 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   79 '0x01' */
    { 0x80000000,  1,   1}, /*   80 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   81 '0x01' */
    { 0x80000000,  1,   1}, /*   82 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   83 '0x01' */
    { 0x80000000,  1,   1}, /*   84 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   85 '0x01' */
    { 0x80000000,  1,   1}, /*   86 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   87 '0x01' */
    { 0x80000000,  1,   1}, /*   88 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   89 '0x01' */
    { 0x80000000,  1,   1}, /*   90 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   91 '0x01' */
    { 0x80000000,  1,   1}, /*   92 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   93 '0x01' */
    { 0x80000000,  1,   1}, /*   94 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   95 '0x01' */
    { 0x80000000,  1,   1}, /*   96 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   97 '0x01' */
    { 0x80000000,  1,   1}, /*   98 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   99 '0x01' */
    { 0x80000000,  1,   1}, /*  100 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  101 '0x01' */
    { 0x80000000,  1,   1}, /*  102 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  103 '0x01' */
    { 0x80000000,  1,   1}, /*  104 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  105 '0x01' */
    { 0x80000000,  1,   1}, /*  106 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  107 '0x01' */
    { 0x80000000,  1,   1}, /*  108 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  109 '0x01' */
    { 0x80000000,  1,   1}, /*  110 '0x01' */
    /*   70                             */
    { 0x00000000,  4,  87}, /*  111 'W' */
    { 0x30000000,  4,  77}, /*  112 'M' */
    { 0x40000000,  4,  67}, /*  113 'C' */
    { 0x50000000,  4,  66}, /*  114 'B' */
    { 0x70000000,  4,  80}, /*  115 'P' */
    { 0x90000000,  4,  84}, /*  116 'T' */
    { 0xc0000000,  4,  78}, /*  117 'N' */
    { 0xf0000000,  4,  83}, /*  118 'S' */
    { 0x18000000,  5,  73}, /*  119 'I' */
    { 0x20000000,  5,  71}, /*  120 'G' */
    { 0x60000000,  5,  72}, /*  121 'H' */
    { 0x68000000,  5,  68}, /*  122 'D' */
    { 0x80000000,  5, 111}, /*  123 'o' */
    { 0x88000000,  5,  65}, /*  124 'A' */
    { 0xa0000000,  5, 116}, /*  125 't' */
    { 0xb0000000,  5,  97}, /*  126 'a' */
    { 0xb8000000,  5,  70}, /*  127 'F' */
    { 0xd0000000,  5,  76}, /*  128 'L' */
    { 0xd8000000,  5,  82}, /*  129 'R' */
    { 0x2c000000,  6,  85}, /*  130 'U' */
    { 0xac000000,  6,  79}, /*  131 'O' */
    { 0xe4000000,  6,  74}, /*  132 'J' */
    { 0xe8000000,  6,  69}, /*  133 'E' */
    { 0x10000000,  7, 102}, /*  134 'f' */
    { 0x12000000,  7,  81}, /*  135 'Q' */
    { 0x16000000,  7,  86}, /*  136 'V' */
    { 0x28000000,  7,   0}, /*  137 '0x00' */
    { 0x2a000000,  7, 119}, /*  138 'w' */
    { 0xe0000000,  7,  50}, /*  139 '2' */
    { 0xe2000000,  7,  75}, /*  140 'K' */
    { 0xec000000,  7,  89}, /*  141 'Y' */
    { 0xee000000,  7, 105}, /*  142 'i' */
    { 0x14000000,  8,  45}, /*  143 '-' */
    { 0xa9000000,  8,  49}, /*  144 '1' */
    { 0xa8000000,  9,  38}, /*  145 '&' */
    { 0xaa800000,  9,  88}, /*  146 'X' */
    { 0x15400000, 10, 114}, /*  147 'r' */
    { 0xa8800000, 10,  53}, /*  148 '5' */
    { 0xa8c00000, 10,  90}, /*  149 'Z' */
    { 0xaa400000, 10,  57}, /*  150 '9' */
    { 0xab400000, 10, 115}, /*  151 's' */
    { 0xab800000, 10,  52}, /*  152 '4' */
    { 0xabc00000, 10,  51}, /*  153 '3' */
    { 0x15000000, 11,  55}, /*  154 '7' */
    { 0x15800000, 11,  98}, /*  155 'b' */
    { 0x15c00000, 11, 121}, /*  156 'y' */
    { 0xaa000000, 11,  39}, /*  157 '\'' */
    { 0xab000000, 11,  54}, /*  158 '6' */
    { 0x15a00000, 12, 118}, /*  159 'v' */
    { 0x15b00000, 12, 100}, /*  160 'd' */
    { 0x15e00000, 12,  40}, /*  161 '(' */
    { 0xaa200000, 12,  32}, /*  162 ' ' */
    { 0xaa300000, 12,  48}, /*  163 '0' */
    { 0xab200000, 12, 110}, /*  164 'n' */
    { 0xab300000, 12,  56}, /*  165 '8' */
    { 0x15300000, 13, 103}, /*  166 'g' */
    { 0x15f00000, 13, 117}, /*  167 'u' */
    { 0x15200000, 14,  43}, /*  168 '+' */
    { 0x15240000, 14,  46}, /*  169 '.' */
    { 0x15280000, 14,   1}, /*  170 '0x01' */
    { 0x152c0000, 14, 108}, /*  171 'l' */
    { 0x153c0000, 14, 109}, /*  172 'm' */
    { 0x15f80000, 14, 112}, /*  173 'p' */
    { 0x15380000, 15,  92}, /*  174 '\\' */
    { 0x153a0000, 15,  47}, /*  175 '/' */
    { 0x15fe0000, 15, 101}, /*  176 'e' */
    { 0x15fd0000, 16,  34}, /*  177 '\"' */
    { 0x15fc8000, 17,  99}, /*  178 'c' */
    { 0x15fc0000, 18, 107}, /*  179 'k' */
    { 0x15fc4000, 18, 104}, /*  180 'h' */
    /*    7                             */
    { 0x80000000,  1,   0}, /*  181 '0x00' */
    { 0x40000000,  2,  32}, /*  182 ' ' */
    { 0x20000000,  3,  46}, /*  183 '.' */
    { 0x10000000,  4,  33}, /*  184 '!' */
    { 0x08000000,  5,  34}, /*  185 '\"' */
    { 0x00000000,  6,   1}, /*  186 '0x01' */
    { 0x04000000,  6,  58}, /*  187 ':' */
    /*    3                             */
    { 0x00000000,  1,  32}, /*  188 ' ' */
    { 0x80000000,  2,   1}, /*  189 '0x01' */
    { 0xc0000000,  2,  73}, /*  190 'I' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  191 '0x01' */
    { 0x80000000,  1,   1}, /*  192 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  193 '0x01' */
    { 0x80000000,  1,   1}, /*  194 '0x01' */
    /*    3                             */
    { 0x80000000,  1,  32}, /*  195 ' ' */
    { 0x00000000,  2,   1}, /*  196 '0x01' */
    { 0x40000000,  2,   0}, /*  197 '0x00' */
    /*    4                             */
    { 0x80000000,  1,  32}, /*  198 ' ' */
    { 0x40000000,  2,  66}, /*  199 'B' */
    { 0x00000000,  3,   1}, /*  200 '0x01' */
    { 0x20000000,  3,  46}, /*  201 '.' */
    /*   32                             */
    { 0x80000000,  1, 115}, /*  202 's' */
    { 0x00000000,  3, 109}, /*  203 'm' */
    { 0x40000000,  3,  67}, /*  204 'C' */
    { 0x20000000,  4, 116}, /*  205 't' */
    { 0x30000000,  4,  32}, /*  206 ' ' */
    { 0x60000000,  5, 100}, /*  207 'd' */
    { 0x70000000,  5, 118}, /*  208 'v' */
    { 0x6c000000,  6, 114}, /*  209 'r' */
    { 0x7c000000,  6,  65}, /*  210 'A' */
    { 0x6a000000,  7, 110}, /*  211 'n' */
    { 0x68000000,  8,  71}, /*  212 'G' */
    { 0x79000000,  8, 108}, /*  213 'l' */
    { 0x69800000,  9,  68}, /*  214 'D' */
    { 0x78000000,  9,  66}, /*  215 'B' */
    { 0x78800000,  9, 101}, /*  216 'e' */
    { 0x7a800000,  9, 105}, /*  217 'i' */
    { 0x7b000000,  9,  54}, /*  218 '6' */
    { 0x69000000, 10,  76}, /*  219 'L' */
    { 0x7a400000, 10,   0}, /*  220 '0x00' */
    { 0x7bc00000, 10, 119}, /*  221 'w' */
    { 0x69400000, 11,  79}, /*  222 'O' */
    { 0x7a000000, 11,  83}, /*  223 'S' */
    { 0x7a200000, 11,  69}, /*  224 'E' */
    { 0x7ba00000, 11,  78}, /*  225 'N' */
    { 0x7b900000, 12,  82}, /*  226 'R' */
    { 0x69600000, 13,  97}, /*  227 'a' */
    { 0x69680000, 13,  77}, /*  228 'M' */
    { 0x69700000, 13,  75}, /*  229 'K' */
    { 0x69780000, 13,  70}, /*  230 'F' */
    { 0x7b800000, 13,  48}, /*  231 '0' */
    { 0x7b880000, 14,   1}, /*  232 '0x01' */
    { 0x7b8c0000, 14,  99}, /*  233 'c' */
    /*   12                             */
    { 0x80000000,  1,  99}, /*  234 'c' */
    { 0x00000000,  3,  49}, /*  235 '1' */
    { 0x20000000,  4,  77}, /*  236 'M' */
    { 0x30000000,  4,  85}, /*  237 'U' */
    { 0x40000000,  4,  82}, /*  238 'R' */
    { 0x50000000,  4,  68}, /*  239 'D' */
    { 0x60000000,  4,  72}, /*  240 'H' */
    { 0x70000000,  5,  83}, /*  241 'S' */
    { 0x78000000,  6,  70}, /*  242 'F' */
    { 0x7c000000,  7,  71}, /*  243 'G' */
    { 0x7e000000,  8,   1}, /*  244 '0x01' */
    { 0x7f000000,  8,  89}, /*  245 'Y' */
    /*    3                             */
    { 0x80000000,  1,   0}, /*  246 '0x00' */
    { 0x00000000,  2,   1}, /*  247 '0x01' */
    { 0x40000000,  2,  32}, /*  248 ' ' */
    /*   11                             */
    { 0x00000000,  1,  42}, /*  249 '*' */
    { 0xa0000000,  3,  32}, /*  250 ' ' */
    { 0x80000000,  4, 100}, /*  251 'd' */
    { 0xc0000000,  4, 109}, /*  252 'm' */
    { 0xd0000000,  4, 116}, /*  253 't' */
    { 0xf0000000,  4, 115}, /*  254 's' */
    { 0x90000000,  5, 101}, /*  255 'e' */
    { 0xe0000000,  5, 103}, /*  256 'g' */
    { 0xe8000000,  5, 107}, /*  257 'k' */
    { 0x98000000,  6,   1}, /*  258 '0x01' */
    { 0x9c000000,  6, 121}, /*  259 'y' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  260 '0x01' */
    { 0x80000000,  1,  32}, /*  261 ' ' */
    /*    4                             */
    { 0x80000000,  1,  32}, /*  262 ' ' */
    { 0x40000000,  2,  48}, /*  263 '0' */
    { 0x00000000,  3,   1}, /*  264 '0x01' */
    { 0x20000000,  3,  46}, /*  265 '.' */
    /*   37                             */
    { 0xc0000000,  2,  32}, /*  266 ' ' */
    { 0x60000000,  3,  83}, /*  267 'S' */
    { 0x80000000,  3,  71}, /*  268 'G' */
    { 0xa0000000,  3,  79}, /*  269 'O' */
    { 0x30000000,  4,  84}, /*  270 'T' */
    { 0x40000000,  4,  85}, /*  271 'U' */
    { 0x00000000,  5,  69}, /*  272 'E' */
    { 0x10000000,  5,  68}, /*  273 'D' */
    { 0x08000000,  6, 109}, /*  274 'm' */
    { 0x18000000,  6,  48}, /*  275 '0' */
    { 0x1c000000,  6,  73}, /*  276 'I' */
    { 0x28000000,  6,  54}, /*  277 '6' */
    { 0x50000000,  6,  70}, /*  278 'F' */
    { 0x54000000,  6, 111}, /*  279 'o' */
    { 0x0c000000,  7,  76}, /*  280 'L' */
    { 0x0e000000,  7,  67}, /*  281 'C' */
    { 0x22000000,  7,  65}, /*  282 'A' */
    { 0x24000000,  7, 116}, /*  283 't' */
    { 0x26000000,  7,  89}, /*  284 'Y' */
    { 0x2e000000,  7,  50}, /*  285 '2' */
    { 0x58000000,  7,  66}, /*  286 'B' */
    { 0x5a000000,  7,  46}, /*  287 '.' */
    { 0x20000000,  8,  80}, /*  288 'P' */
    { 0x21000000,  8,  90}, /*  289 'Z' */
    { 0x5c000000,  8,  56}, /*  290 '8' */
    { 0x5d000000,  8, 105}, /*  291 'i' */
    { 0x5e000000,  8, 100}, /*  292 'd' */
    { 0x5f000000,  8,  72}, /*  293 'H' */
    { 0x2c800000,  9,  78}, /*  294 'N' */
    { 0x2d800000,  9,  82}, /*  295 'R' */
    { 0x2c000000, 10,  49}, /*  296 '1' */
    { 0x2c400000, 10,  87}, /*  297 'W' */
    { 0x2d200000, 11,  99}, /*  298 'c' */
    { 0x2d400000, 11,  97}, /*  299 'a' */
    { 0x2d600000, 11,  77}, /*  300 'M' */
    { 0x2d000000, 12,   1}, /*  301 '0x01' */
    { 0x2d100000, 12,  81}, /*  302 'Q' */
    /*   25                             */
    { 0x80000000,  1,  46}, /*  303 '.' */
    { 0x40000000,  2,   0}, /*  304 '0x00' */
    { 0x20000000,  4,  32}, /*  305 ' ' */
    { 0x00000000,  5,  73}, /*  306 'I' */
    { 0x08000000,  5,  84}, /*  307 'T' */
    { 0x10000000,  5,  67}, /*  308 'C' */
    { 0x30000000,  5, 112}, /*  309 'p' */
    { 0x38000000,  5,  48}, /*  310 '0' */
    { 0x1c000000,  6,  72}, /*  311 'H' */
    { 0x1a000000,  8,  87}, /*  312 'W' */
    { 0x18800000,  9,  83}, /*  313 'S' */
    { 0x1b000000,  9,  51}, /*  314 '3' */
    { 0x1b800000,  9,  66}, /*  315 'B' */
    { 0x18000000, 10,  49}, /*  316 '1' */
    { 0x18400000, 10,  77}, /*  317 'M' */
    { 0x19800000, 10,  99}, /*  318 'c' */
    { 0x19000000, 11, 116}, /*  319 't' */
    { 0x19200000, 11,  82}, /*  320 'R' */
    { 0x19400000, 11,  70}, /*  321 'F' */
    { 0x19c00000, 11,  69}, /*  322 'E' */
    { 0x19e00000, 11,  65}, /*  323 'A' */
    { 0x19600000, 13,   1}, /*  324 '0x01' */
    { 0x19680000, 13, 108}, /*  325 'l' */
    { 0x19700000, 13, 100}, /*  326 'd' */
    { 0x19780000, 13,  85}, /*  327 'U' */
    /*   18                             */
    { 0x00000000,  2,  49}, /*  328 '1' */
    { 0x80000000,  2,  55}, /*  329 '7' */
    { 0x40000000,  3,  52}, /*  330 '4' */
    { 0x60000000,  3,  50}, /*  331 '2' */
    { 0xc0000000,  3,  51}, /*  332 '3' */
    { 0xe0000000,  4,  53}, /*  333 '5' */
    { 0xf0000000,  6,  54}, /*  334 '6' */
    { 0xf8000000,  6,  67}, /*  335 'C' */
    { 0xf4000000,  7,  57}, /*  336 '9' */
    { 0xf6000000,  7,  32}, /*  337 ' ' */
    { 0xfc000000,  7,  56}, /*  338 '8' */
    { 0xff000000,  8,  85}, /*  339 'U' */
    { 0xfe000000, 10,  71}, /*  340 'G' */
    { 0xfe800000, 10,  48}, /*  341 '0' */
    { 0xfe400000, 11,   1}, /*  342 '0x01' */
    { 0xfe600000, 11,  87}, /*  343 'W' */
    { 0xfec00000, 11,  86}, /*  344 'V' */
    { 0xfee00000, 11,  83}, /*  345 'S' */
    /*   23                             */
    { 0x00000000,  2,  54}, /*  346 '6' */
    { 0x40000000,  2,  32}, /*  347 ' ' */
    { 0xc0000000,  2,  48}, /*  348 '0' */
    { 0x90000000,  4, 112}, /*  349 'p' */
    { 0xa0000000,  4,   0}, /*  350 '0x00' */
    { 0x80000000,  5,  49}, /*  351 '1' */
    { 0x88000000,  5,  97}, /*  352 'a' */
    { 0xb8000000,  5,  55}, /*  353 '7' */
    { 0xb0000000,  7,  45}, /*  354 '-' */
    { 0xb4000000,  7, 115}, /*  355 's' */
    { 0xb3000000,  8,  52}, /*  356 '4' */
    { 0xb6000000,  8, 116}, /*  357 't' */
    { 0xb7800000,  9,  37}, /*  358 '%' */
    { 0xb2000000, 10,  56}, /*  359 '8' */
    { 0xb2400000, 10,  58}, /*  360 ':' */
    { 0xb2800000, 10,  53}, /*  361 '5' */
    { 0xb2c00000, 10,  50}, /*  362 '2' */
    { 0xb7000000, 10,  47}, /*  363 '/' */
    { 0xb7600000, 11,  85}, /*  364 'U' */
    { 0xb7500000, 12,  44}, /*  365 ',' */
    { 0xb7400000, 13,  46}, /*  366 '.' */
    { 0xb7480000, 14,   1}, /*  367 '0x01' */
    { 0xb74c0000, 14, 108}, /*  368 'l' */
    /*   22                             */
    { 0x40000000,  2,   0}, /*  369 '0x00' */
    { 0x00000000,  3,  46}, /*  370 '.' */
    { 0xa0000000,  3,  48}, /*  371 '0' */
    { 0xe0000000,  3,  49}, /*  372 '1' */
    { 0x20000000,  4,  50}, /*  373 '2' */
    { 0x30000000,  4,  32}, /*  374 ' ' */
    { 0xd0000000,  4,  47}, /*  375 '/' */
    { 0x90000000,  5,  56}, /*  376 '8' */
    { 0xc0000000,  5,  51}, /*  377 '3' */
    { 0x80000000,  6,  53}, /*  378 '5' */
    { 0x84000000,  6, 115}, /*  379 's' */
    { 0x88000000,  6,  54}, /*  380 '6' */
    { 0x8c000000,  6,  58}, /*  381 ':' */
    { 0x98000000,  6,  39}, /*  382 '\'' */
    { 0xc8000000,  6,  88}, /*  383 'X' */
    { 0xcc000000,  6,  57}, /*  384 '9' */
    { 0x9e000000,  7,  52}, /*  385 '4' */
    { 0x9d000000,  8,  45}, /*  386 '-' */
    { 0x9c800000,  9,  55}, /*  387 '7' */
    { 0x9c000000, 10,  41}, /*  388 ')' */
    { 0x9c400000, 11,   1}, /*  389 '0x01' */
    { 0x9c600000, 11,  44}, /*  390 ',' */
    /*   18                             */
    { 0x00000000,  1,  48}, /*  391 '0' */
    { 0xc0000000,  2,  52}, /*  392 '4' */
    { 0xa0000000,  3,   0}, /*  393 '0x00' */
    { 0x90000000,  4,  32}, /*  394 ' ' */
    { 0x80000000,  5,  58}, /*  395 ':' */
    { 0x8a000000,  7,  53}, /*  396 '5' */
    { 0x8e000000,  7,  47}, /*  397 '/' */
    { 0x88000000,  8,  46}, /*  398 '.' */
    { 0x89000000,  8,  49}, /*  399 '1' */
    { 0x8c000000,  8,  87}, /*  400 'W' */
    { 0x8d800000,  9,  55}, /*  401 '7' */
    { 0x8d200000, 11,  51}, /*  402 '3' */
    { 0x8d600000, 11,  90}, /*  403 'Z' */
    { 0x8d000000, 12, 110}, /*  404 'n' */
    { 0x8d100000, 12,  54}, /*  405 '6' */
    { 0x8d500000, 12,  39}, /*  406 '\'' */
    { 0x8d400000, 13,   1}, /*  407 '0x01' */
    { 0x8d480000, 13, 115}, /*  408 's' */
    /*   19                             */
    { 0x00000000,  1,  32}, /*  409 ' ' */
    { 0x80000000,  2,   0}, /*  410 '0x00' */
    { 0xc0000000,  4, 114}, /*  411 'r' */
    { 0xd0000000,  4,  47}, /*  412 '/' */
    { 0xf0000000,  4,  66}, /*  413 'B' */
    { 0xe0000000,  5,  48}, /*  414 '0' */
    { 0xe8000000,  7,  58}, /*  415 ':' */
    { 0xec000000,  7,  45}, /*  416 '-' */
    { 0xea000000,  8,  49}, /*  417 '1' */
    { 0xeb000000,  8,  56}, /*  418 '8' */
    { 0xef000000,  8,  52}, /*  419 '4' */
    { 0xee000000, 10,  54}, /*  420 '6' */
    { 0xeec00000, 10,  57}, /*  421 '9' */
    { 0xee600000, 11, 116}, /*  422 't' */
    { 0xee800000, 11,  51}, /*  423 '3' */
    { 0xee400000, 12,   1}, /*  424 '0x01' */
    { 0xee500000, 12, 101}, /*  425 'e' */
    { 0xeea00000, 12,  55}, /*  426 '7' */
    { 0xeeb00000, 12,  53}, /*  427 '5' */
    /*   19                             */
    { 0x00000000,  1,   0}, /*  428 '0x00' */
    { 0xc0000000,  2,  32}, /*  429 ' ' */
    { 0x90000000,  4,  58}, /*  430 ':' */
    { 0xb0000000,  4,  47}, /*  431 '/' */
    { 0x88000000,  5,  56}, /*  432 '8' */
    { 0xa8000000,  5,  46}, /*  433 '.' */
    { 0x80000000,  6,  57}, /*  434 '9' */
    { 0xa0000000,  6,  48}, /*  435 '0' */
    { 0xa4000000,  7,  77}, /*  436 'M' */
    { 0x85000000,  8,  73}, /*  437 'I' */
    { 0x86000000,  8,  41}, /*  438 ')' */
    { 0xa6000000,  8,  82}, /*  439 'R' */
    { 0xa7000000,  8,  45}, /*  440 '-' */
    { 0x84000000,  9,  87}, /*  441 'W' */
    { 0x87000000,  9,  80}, /*  442 'P' */
    { 0x87800000,  9,  53}, /*  443 '5' */
    { 0x84c00000, 10,  50}, /*  444 '2' */
    { 0x84800000, 11,   1}, /*  445 '0x01' */
    { 0x84a00000, 11,  39}, /*  446 '\'' */
    /*   11                             */
    { 0x00000000,  1,   0}, /*  447 '0x00' */
    { 0xc0000000,  2,  32}, /*  448 ' ' */
    { 0xa0000000,  3,  48}, /*  449 '0' */
    { 0x90000000,  4,  47}, /*  450 '/' */
    { 0x84000000,  6,  45}, /*  451 '-' */
    { 0x8c000000,  6,  58}, /*  452 ':' */
    { 0x82000000,  7, 116}, /*  453 't' */
    { 0x88000000,  7,  51}, /*  454 '3' */
    { 0x8a000000,  7,  49}, /*  455 '1' */
    { 0x80000000,  8,   1}, /*  456 '0x01' */
    { 0x81000000,  8,  97}, /*  457 'a' */
    /*   16                             */
    { 0x00000000,  2,   0}, /*  458 '0x00' */
    { 0x40000000,  2,  32}, /*  459 ' ' */
    { 0x80000000,  2,  48}, /*  460 '0' */
    { 0xe0000000,  3,  58}, /*  461 ':' */
    { 0xc8000000,  5,  46}, /*  462 '.' */
    { 0xd8000000,  5, 105}, /*  463 'i' */
    { 0xc0000000,  6,  45}, /*  464 '-' */
    { 0xd4000000,  6,  97}, /*  465 'a' */
    { 0xc6000000,  7,  52}, /*  466 '4' */
    { 0xd0000000,  7,  56}, /*  467 '8' */
    { 0xd2000000,  7,  47}, /*  468 '/' */
    { 0xc4000000,  8,  54}, /*  469 '6' */
    { 0xc5800000,  9,  57}, /*  470 '9' */
    { 0xc5000000, 10,  51}, /*  471 '3' */
    { 0xc5400000, 11,   1}, /*  472 '0x01' */
    { 0xc5600000, 11, 116}, /*  473 't' */
    /*   11                             */
    { 0x80000000,  1,   0}, /*  474 '0x00' */
    { 0x40000000,  2,  32}, /*  475 ' ' */
    { 0x00000000,  3,  48}, /*  476 '0' */
    { 0x30000000,  4,  46}, /*  477 '.' */
    { 0x28000000,  5,  47}, /*  478 '/' */
    { 0x20000000,  7,  49}, /*  479 '1' */
    { 0x24000000,  7,  53}, /*  480 '5' */
    { 0x22000000,  8,   1}, /*  481 '0x01' */
    { 0x23000000,  8,  52}, /*  482 '4' */
    { 0x26000000,  8,  51}, /*  483 '3' */
    { 0x27000000,  8,  50}, /*  484 '2' */
    /*   15                             */
    { 0x80000000,  1,  32}, /*  485 ' ' */
    { 0x00000000,  2,  48}, /*  486 '0' */
    { 0x40000000,  3,  58}, /*  487 ':' */
    { 0x68000000,  5,   0}, /*  488 '0x00' */
    { 0x60000000,  6, 116}, /*  489 't' */
    { 0x64000000,  6, 112}, /*  490 'p' */
    { 0x74000000,  6,  56}, /*  491 '8' */
    { 0x78000000,  6,  46}, /*  492 '.' */
    { 0x7c000000,  6,  54}, /*  493 '6' */
    { 0x70000000,  7,  53}, /*  494 '5' */
    { 0x72000000,  8,  57}, /*  495 '9' */
    { 0x73000000,  9,  77}, /*  496 'M' */
    { 0x73800000, 10,  70}, /*  497 'F' */
    { 0x73c00000, 11,   1}, /*  498 '0x01' */
    { 0x73e00000, 11,  99}, /*  499 'c' */
    /*   14                             */
    { 0x00000000,  1,  49}, /*  500 '1' */
    { 0xc0000000,  2,   0}, /*  501 '0x00' */
    { 0x80000000,  4,  57}, /*  502 '9' */
    { 0xa0000000,  4,  46}, /*  503 '.' */
    { 0x98000000,  5,  48}, /*  504 '0' */
    { 0x90000000,  6,  32}, /*  505 ' ' */
    { 0x94000000,  6,  56}, /*  506 '8' */
    { 0xb0000000,  6,  55}, /*  507 '7' */
    { 0xb4000000,  6,  47}, /*  508 '/' */
    { 0xb8000000,  6,  54}, /*  509 '6' */
    { 0xbe000000,  7,  58}, /*  510 ':' */
    { 0xbd000000,  8,  52}, /*  511 '4' */
    { 0xbc000000,  9,   1}, /*  512 '0x01' */
    { 0xbc800000,  9,  51}, /*  513 '3' */
    /*    9                             */
    { 0x80000000,  1,  32}, /*  514 ' ' */
    { 0x00000000,  2,  48}, /*  515 '0' */
    { 0x60000000,  3,  46}, /*  516 '.' */
    { 0x40000000,  4,  50}, /*  517 '2' */
    { 0x50000000,  5,  49}, /*  518 '1' */
    { 0x5c000000,  6,  51}, /*  519 '3' */
    { 0x5a000000,  7,  67}, /*  520 'C' */
    { 0x58000000,  8,   1}, /*  521 '0x01' */
    { 0x59000000,  8,  84}, /*  522 'T' */
    /*    3                             */
    { 0x80000000,  1,  32}, /*  523 ' ' */
    { 0x00000000,  2,   1}, /*  524 '0x01' */
    { 0x40000000,  2,  46}, /*  525 '.' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  526 '0x01' */
    { 0x80000000,  1,   1}, /*  527 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  528 '0x01' */
    { 0x80000000,  1,   1}, /*  529 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  530 '0x01' */
    { 0x80000000,  1,   1}, /*  531 '0x01' */
    /*    5                             */
    { 0x80000000,  1,   0}, /*  532 '0x00' */
    { 0x40000000,  2,  32}, /*  533 ' ' */
    { 0x20000000,  3,  58}, /*  534 ':' */
    { 0x00000000,  4,   1}, /*  535 '0x01' */
    { 0x10000000,  4,  46}, /*  536 '.' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  537 '0x01' */
    { 0x80000000,  1,  72}, /*  538 'H' */
    /*   44                             */
    { 0x20000000,  3, 114}, /*  539 'r' */
    { 0x40000000,  3,  32}, /*  540 ' ' */
    { 0x80000000,  3, 108}, /*  541 'l' */
    { 0xc0000000,  3, 110}, /*  542 'n' */
    { 0x00000000,  4, 109}, /*  543 'm' */
    { 0x70000000,  4, 103}, /*  544 'g' */
    { 0xf0000000,  4, 100}, /*  545 'd' */
    { 0x10000000,  5, 119}, /*  546 'w' */
    { 0x60000000,  5,  84}, /*  547 'T' */
    { 0x68000000,  5,  99}, /*  548 'c' */
    { 0xa8000000,  5, 116}, /*  549 't' */
    { 0xb0000000,  5, 102}, /*  550 'f' */
    { 0xb8000000,  5, 105}, /*  551 'i' */
    { 0xe0000000,  5, 115}, /*  552 's' */
    { 0x18000000,  6, 117}, /*  553 'u' */
    { 0x1c000000,  6,   0}, /*  554 '0x00' */
    { 0xa4000000,  6,  82}, /*  555 'R' */
    { 0xe8000000,  6,  98}, /*  556 'b' */
    { 0xa2000000,  7, 118}, /*  557 'v' */
    { 0xec000000,  7, 112}, /*  558 'p' */
    { 0xa0000000,  8,  83}, /*  559 'S' */
    { 0xee000000,  8,  77}, /*  560 'M' */
    { 0xa1800000,  9,  80}, /*  561 'P' */
    { 0xef800000,  9,  46}, /*  562 '.' */
    { 0xa1000000, 11, 101}, /*  563 'e' */
    { 0xa1200000, 11,  66}, /*  564 'B' */
    { 0xa1400000, 11,  49}, /*  565 '1' */
    { 0xef600000, 11,  45}, /*  566 '-' */
    { 0xa1600000, 12, 107}, /*  567 'k' */
    { 0xa1700000, 12, 104}, /*  568 'h' */
    { 0xef000000, 12,  97}, /*  569 'a' */
    { 0xef400000, 12, 121}, /*  570 'y' */
    { 0xef500000, 12,  42}, /*  571 '*' */
    { 0xef100000, 13, 120}, /*  572 'x' */
    { 0xef180000, 13,  39}, /*  573 '\'' */
    { 0xef200000, 13,  78}, /*  574 'N' */
    { 0xef300000, 13,  50}, /*  575 '2' */
    { 0xef280000, 14,  58}, /*  576 ':' */
    { 0xef3c0000, 14, 122}, /*  577 'z' */
    { 0xef2c0000, 15,  76}, /*  578 'L' */
    { 0xef2e0000, 15,  70}, /*  579 'F' */
    { 0xef380000, 15,  68}, /*  580 'D' */
    { 0xef3a0000, 16,   1}, /*  581 '0x01' */
    { 0xef3b0000, 16, 113}, /*  582 'q' */
    /*   26                             */
    { 0x00000000,  2,  67}, /*  583 'C' */
    { 0x40000000,  2,  66}, /*  584 'B' */
    { 0xa0000000,  3, 114}, /*  585 'r' */
    { 0x90000000,  4, 105}, /*  586 'i' */
    { 0xc0000000,  4, 111}, /*  587 'o' */
    { 0xd0000000,  4, 117}, /*  588 'u' */
    { 0xe0000000,  4,  97}, /*  589 'a' */
    { 0xf0000000,  4, 101}, /*  590 'e' */
    { 0x88000000,  5, 108}, /*  591 'l' */
    { 0x80000000,  7,   0}, /*  592 '0x00' */
    { 0x84000000,  7, 121}, /*  593 'y' */
    { 0x82000000,  8,  79}, /*  594 'O' */
    { 0x86000000,  8,  51}, /*  595 '3' */
    { 0x83800000,  9,  65}, /*  596 'A' */
    { 0x87000000,  9,  83}, /*  597 'S' */
    { 0x83400000, 10,  58}, /*  598 ':' */
    { 0x83000000, 11,  46}, /*  599 '.' */
    { 0x83200000, 11, 119}, /*  600 'w' */
    { 0x87a00000, 11, 104}, /*  601 'h' */
    { 0x87c00000, 11,  42}, /*  602 '*' */
    { 0x87e00000, 11,  32}, /*  603 ' ' */
    { 0x87900000, 12,  82}, /*  604 'R' */
    { 0x87800000, 13,  39}, /*  605 '\'' */
    { 0x878c0000, 14,  84}, /*  606 'T' */
    { 0x87880000, 15,   1}, /*  607 '0x01' */
    { 0x878a0000, 15,  52}, /*  608 '4' */
    /*   31                             */
    { 0x00000000,  2, 111}, /*  609 'o' */
    { 0x40000000,  2,  32}, /*  610 ' ' */
    { 0x80000000,  3, 108}, /*  611 'l' */
    { 0xc0000000,  3, 104}, /*  612 'h' */
    { 0xa0000000,  4, 114}, /*  613 'r' */
    { 0xe0000000,  4,  97}, /*  614 'a' */
    { 0xb0000000,  5, 105}, /*  615 'i' */
    { 0xb8000000,  5, 101}, /*  616 'e' */
    { 0xf0000000,  6, 117}, /*  617 'u' */
    { 0xf4000000,  6,  66}, /*  618 'B' */
    { 0xf8000000,  7, 121}, /*  619 'y' */
    { 0xfc000000,  7,  33}, /*  620 '!' */
    { 0xfb000000,  8,  46}, /*  621 '.' */
    { 0xfa000000,  9, 119}, /*  622 'w' */
    { 0xfe000000,  9,   0}, /*  623 '0x00' */
    { 0xff000000,  9,  83}, /*  624 'S' */
    { 0xff800000,  9,  84}, /*  625 'T' */
    { 0xfac00000, 10,  50}, /*  626 '2' */
    { 0xfec00000, 10,  73}, /*  627 'I' */
    { 0xfa800000, 11,  52}, /*  628 '4' */
    { 0xfaa00000, 11,  42}, /*  629 '*' */
    { 0xfea00000, 11,  68}, /*  630 'D' */
    { 0xfe800000, 12,  85}, /*  631 'U' */
    { 0xfe900000, 13,  39}, /*  632 '\'' */
    { 0xfe980000, 14, 110}, /*  633 'n' */
    { 0xfe9c0000, 15, 122}, /*  634 'z' */
    { 0xfe9e0000, 17,  79}, /*  635 'O' */
    { 0xfe9e8000, 17,  69}, /*  636 'E' */
    { 0xfe9f0000, 17,  65}, /*  637 'A' */
    { 0xfe9f8000, 18,   1}, /*  638 '0x01' */
    { 0xfe9fc000, 18, 115}, /*  639 's' */
    /*   28                             */
    { 0x40000000,  2, 111}, /*  640 'o' */
    { 0x80000000,  2,  97}, /*  641 'a' */
    { 0x00000000,  3, 114}, /*  642 'r' */
    { 0xc0000000,  3, 101}, /*  643 'e' */
    { 0xe0000000,  3, 105}, /*  644 'i' */
    { 0x20000000,  5, 116}, /*  645 't' */
    { 0x38000000,  5, 117}, /*  646 'u' */
    { 0x2c000000,  6,  32}, /*  647 ' ' */
    { 0x2a000000,  7,  74}, /*  648 'J' */
    { 0x30000000,  7, 121}, /*  649 'y' */
    { 0x34000000,  7,   0}, /*  650 '0x00' */
    { 0x36000000,  7,  73}, /*  651 'I' */
    { 0x33000000,  8,  58}, /*  652 ':' */
    { 0x28800000,  9,  42}, /*  653 '*' */
    { 0x29000000,  9,  45}, /*  654 '-' */
    { 0x29800000,  9,  38}, /*  655 '&' */
    { 0x32000000,  9,  39}, /*  656 '\'' */
    { 0x28000000, 10,  65}, /*  657 'A' */
    { 0x28400000, 10, 104}, /*  658 'h' */
    { 0x32a00000, 11,  78}, /*  659 'N' */
    { 0x32c00000, 11,  86}, /*  660 'V' */
    { 0x32e00000, 12,  68}, /*  661 'D' */
    { 0x32f00000, 12, 119}, /*  662 'w' */
    { 0x32800000, 13,  79}, /*  663 'O' */
    { 0x32880000, 13,  69}, /*  664 'E' */
    { 0x32980000, 13, 100}, /*  665 'd' */
    { 0x32900000, 14,   1}, /*  666 '0x01' */
    { 0x32940000, 14,  84}, /*  667 'T' */
    /*   45                             */
    { 0x00000000,  2, 109}, /*  668 'm' */
    { 0x60000000,  3, 118}, /*  669 'v' */
    { 0xa0000000,  3, 110}, /*  670 'n' */
    { 0xe0000000,  3,  97}, /*  671 'a' */
    { 0x40000000,  4,  69}, /*  672 'E' */
    { 0x80000000,  4,   0}, /*  673 '0x00' */
    { 0xd0000000,  4, 120}, /*  674 'x' */
    { 0x98000000,  5, 100}, /*  675 'd' */
    { 0xc8000000,  5, 108}, /*  676 'l' */
    { 0x50000000,  6,  52}, /*  677 '4' */
    { 0x54000000,  6, 121}, /*  678 'y' */
    { 0x58000000,  6, 117}, /*  679 'u' */
    { 0x90000000,  6, 114}, /*  680 'r' */
    { 0xc0000000,  6, 105}, /*  681 'i' */
    { 0x5e000000,  7, 115}, /*  682 's' */
    { 0x94000000,  7,  70}, /*  683 'F' */
    { 0xc4000000,  7,  88}, /*  684 'X' */
    { 0x96000000,  8,  82}, /*  685 'R' */
    { 0xc7000000,  8,  32}, /*  686 ' ' */
    { 0x5d000000,  9, 103}, /*  687 'g' */
    { 0x97800000,  9,  58}, /*  688 ':' */
    { 0xc6000000,  9,  84}, /*  689 'T' */
    { 0xc6800000,  9,  39}, /*  690 '\'' */
    { 0x5c400000, 10,  99}, /*  691 'c' */
    { 0x5cc00000, 10, 113}, /*  692 'q' */
    { 0x5d800000, 10, 101}, /*  693 'e' */
    { 0x5dc00000, 10,  67}, /*  694 'C' */
    { 0x97000000, 10, 112}, /*  695 'p' */
    { 0x5c000000, 11,  45}, /*  696 '-' */
    { 0x5ca00000, 11,  90}, /*  697 'Z' */
    { 0x97600000, 11, 116}, /*  698 't' */
    { 0x5c200000, 12,  83}, /*  699 'S' */
    { 0x5c300000, 12,  46}, /*  700 '.' */
    { 0x97500000, 12,  87}, /*  701 'W' */
    { 0x5c800000, 13,  33}, /*  702 '!' */
    { 0x97400000, 13, 111}, /*  703 'o' */
    { 0x5c880000, 14, 102}, /*  704 'f' */
    { 0x5c8c0000, 14,  85}, /*  705 'U' */
    { 0x5c900000, 14,  78}, /*  706 'N' */
    { 0x5c940000, 14,  77}, /*  707 'M' */
    { 0x5c980000, 14,  76}, /*  708 'L' */
    { 0x5c9c0000, 14,  65}, /*  709 'A' */
    { 0x974c0000, 14,  68}, /*  710 'D' */
    { 0x97480000, 15,   1}, /*  711 '0x01' */
    { 0x974a0000, 15, 119}, /*  712 'w' */
    /*   26                             */
    { 0x00000000,  2, 105}, /*  713 'i' */
    { 0x80000000,  2,  97}, /*  714 'a' */
    { 0x60000000,  3, 114}, /*  715 'r' */
    { 0xc0000000,  3, 117}, /*  716 'u' */
    { 0xe0000000,  3, 111}, /*  717 'o' */
    { 0x40000000,  4, 101}, /*  718 'e' */
    { 0x58000000,  5, 108}, /*  719 'l' */
    { 0x50000000,  7,  65}, /*  720 'A' */
    { 0x54000000,  7,  79}, /*  721 'O' */
    { 0x52000000,  8,  32}, /*  722 ' ' */
    { 0x53000000,  9, 104}, /*  723 'h' */
    { 0x53800000,  9, 116}, /*  724 't' */
    { 0x56800000,  9, 102}, /*  725 'f' */
    { 0x57800000,  9,  76}, /*  726 'L' */
    { 0x56400000, 10,   0}, /*  727 '0x00' */
    { 0x57000000, 11, 106}, /*  728 'j' */
    { 0x57200000, 11,  73}, /*  729 'I' */
    { 0x57400000, 11,  46}, /*  730 '.' */
    { 0x57600000, 11,  49}, /*  731 '1' */
    { 0x56000000, 12,  77}, /*  732 'M' */
    { 0x56200000, 12,  42}, /*  733 '*' */
    { 0x56300000, 12,  75}, /*  734 'K' */
    { 0x56100000, 13, 121}, /*  735 'y' */
    { 0x561c0000, 14,  72}, /*  736 'H' */
    { 0x56180000, 15,   1}, /*  737 '0x01' */
    { 0x561a0000, 15,  33}, /*  738 '!' */
    /*   25                             */
    { 0x80000000,  2, 114}, /*  739 'r' */
    { 0x20000000,  3,  77}, /*  740 'M' */
    { 0x40000000,  3,  97}, /*  741 'a' */
    { 0x60000000,  3, 111}, /*  742 'o' */
    { 0xc0000000,  3, 105}, /*  743 'i' */
    { 0xe0000000,  3, 101}, /*  744 'e' */
    { 0x08000000,  5, 117}, /*  745 'u' */
    { 0x10000000,  5,  88}, /*  746 'X' */
    { 0x04000000,  6, 104}, /*  747 'h' */
    { 0x1c000000,  6, 108}, /*  748 'l' */
    { 0x02000000,  7, 121}, /*  749 'y' */
    { 0x18000000,  7, 119}, /*  750 'w' */
    { 0x00000000,  8,  58}, /*  751 ':' */
    { 0x1b000000,  8,  67}, /*  752 'C' */
    { 0x01800000,  9,   0}, /*  753 '0x00' */
    { 0x1a000000,  9,  45}, /*  754 '-' */
    { 0x1a800000, 10,  80}, /*  755 'P' */
    { 0x1ac00000, 10,  32}, /*  756 ' ' */
    { 0x01000000, 11,  39}, /*  757 '\'' */
    { 0x01400000, 11,  65}, /*  758 'A' */
    { 0x01200000, 12,  85}, /*  759 'U' */
    { 0x01600000, 12,  84}, /*  760 'T' */
    { 0x01700000, 12,  52}, /*  761 '4' */
    { 0x01300000, 13,   1}, /*  762 '0x01' */
    { 0x01380000, 13,  89}, /*  763 'Y' */
    /*   19                             */
    { 0x00000000,  1, 111}, /*  764 'o' */
    { 0x80000000,  3,  97}, /*  765 'a' */
    { 0xa0000000,  3, 105}, /*  766 'i' */
    { 0xc0000000,  3, 101}, /*  767 'e' */
    { 0xe0000000,  4, 117}, /*  768 'u' */
    { 0xf0000000,  5,  82}, /*  769 'R' */
    { 0xf8000000,  6,  65}, /*  770 'A' */
    { 0xfc000000,  7,  46}, /*  771 '.' */
    { 0xfe800000,  9, 121}, /*  772 'y' */
    { 0xff000000,  9,  83}, /*  773 'S' */
    { 0xff800000, 10,  69}, /*  774 'E' */
    { 0xffc00000, 10, 114}, /*  775 'r' */
    { 0xfe000000, 11,   0}, /*  776 '0x00' */
    { 0xfe400000, 11,  76}, /*  777 'L' */
    { 0xfe600000, 11,  77}, /*  778 'M' */
    { 0xfe300000, 12, 119}, /*  779 'w' */
    { 0xfe280000, 13,  68}, /*  780 'D' */
    { 0xfe200000, 14,   1}, /*  781 '0x01' */
    { 0xfe240000, 14,  73}, /*  782 'I' */
    /*   41                             */
    { 0x00000000,  1,  84}, /*  783 'T' */
    { 0x80000000,  3, 115}, /*  784 's' */
    { 0xa0000000,  3, 110}, /*  785 'n' */
    { 0xd0000000,  4, 116}, /*  786 't' */
    { 0xc8000000,  5,  32}, /*  787 ' ' */
    { 0xe8000000,  5,  39}, /*  788 '\'' */
    { 0xf8000000,  5, 114}, /*  789 'r' */
    { 0xc0000000,  6,  73}, /*  790 'I' */
    { 0xc4000000,  6,   0}, /*  791 '0x00' */
    { 0xe4000000,  6, 109}, /*  792 'm' */
    { 0xe0000000,  7, 100}, /*  793 'd' */
    { 0xe2000000,  7,  78}, /*  794 'N' */
    { 0xf2000000,  7, 122}, /*  795 'z' */
    { 0xf4000000,  7,  46}, /*  796 '.' */
    { 0xf0000000,  8,  97}, /*  797 'a' */
    { 0xf1000000,  8,  89}, /*  798 'Y' */
    { 0xf6000000,  9,  83}, /*  799 'S' */
    { 0xf7000000,  9,  99}, /*  800 'c' */
    { 0xf6a00000, 11,  68}, /*  801 'D' */
    { 0xf6c00000, 11, 102}, /*  802 'f' */
    { 0xf7800000, 11, 108}, /*  803 'l' */
    { 0xf7e00000, 11, 121}, /*  804 'y' */
    { 0xf6800000, 12,  86}, /*  805 'V' */
    { 0xf6e00000, 12, 111}, /*  806 'o' */
    { 0xf7b00000, 12,  70}, /*  807 'F' */
    { 0xf6900000, 13,  44}, /*  808 ',' */
    { 0xf6980000, 13,  65}, /*  809 'A' */
    { 0xf6f00000, 13,  79}, /*  810 'O' */
    { 0xf7a80000, 13, 103}, /*  811 'g' */
    { 0xf7c00000, 13,  67}, /*  812 'C' */
    { 0xf7c80000, 13,  58}, /*  813 ':' */
    { 0xf7d80000, 13, 118}, /*  814 'v' */
    { 0xf6f80000, 14, 112}, /*  815 'p' */
    { 0xf6fc0000, 14,  69}, /*  816 'E' */
    { 0xf7a00000, 14,  66}, /*  817 'B' */
    { 0xf7d00000, 14, 107}, /*  818 'k' */
    { 0xf7d40000, 14,  98}, /*  819 'b' */
    { 0xf7a40000, 16,   1}, /*  820 '0x01' */
    { 0xf7a50000, 16,  82}, /*  821 'R' */
    { 0xf7a60000, 16,  76}, /*  822 'L' */
    { 0xf7a70000, 16,  71}, /*  823 'G' */
    /*   16                             */
    { 0x00000000,  2,  97}, /*  824 'a' */
    { 0x40000000,  2, 117}, /*  825 'u' */
    { 0xc0000000,  2, 101}, /*  826 'e' */
    { 0xa0000000,  3, 111}, /*  827 'o' */
    { 0x90000000,  4, 105}, /*  828 'i' */
    { 0x80000000,  5,  32}, /*  829 ' ' */
    { 0x88000000,  6,  75}, /*  830 'K' */
    { 0x8e000000,  7,   0}, /*  831 '0x00' */
    { 0x8c800000,  9, 115}, /*  832 's' */
    { 0x8d000000,  9,  70}, /*  833 'F' */
    { 0x8c000000, 10,  86}, /*  834 'V' */
    { 0x8c400000, 10,  39}, /*  835 '\'' */
    { 0x8dc00000, 10, 102}, /*  836 'f' */
    { 0x8da00000, 11,  71}, /*  837 'G' */
    { 0x8d800000, 12,   1}, /*  838 '0x01' */
    { 0x8d900000, 12,  68}, /*  839 'D' */
    /*   26                             */
    { 0x40000000,  2, 105}, /*  840 'i' */
    { 0xc0000000,  2, 121}, /*  841 'y' */
    { 0x20000000,  3, 101}, /*  842 'e' */
    { 0xa0000000,  3,  32}, /*  843 ' ' */
    { 0x00000000,  4,  97}, /*  844 'a' */
    { 0x80000000,  4, 111}, /*  845 'o' */
    { 0x10000000,  5,   0}, /*  846 '0x00' */
    { 0x18000000,  5, 114}, /*  847 'r' */
    { 0x94000000,  6, 116}, /*  848 't' */
    { 0x98000000,  6, 110}, /*  849 'n' */
    { 0x9c000000,  6,  83}, /*  850 'S' */
    { 0x93000000,  8,  71}, /*  851 'G' */
    { 0x90000000,  9,  45}, /*  852 '-' */
    { 0x91800000,  9,  79}, /*  853 'O' */
    { 0x92000000,  9, 104}, /*  854 'h' */
    { 0x92800000,  9, 119}, /*  855 'w' */
    { 0x90800000, 10,  49}, /*  856 '1' */
    { 0x90c00000, 10,  39}, /*  857 '\'' */
    { 0x91600000, 11, 117}, /*  858 'u' */
    { 0x91000000, 12,  84}, /*  859 'T' */
    { 0x91100000, 12,  78}, /*  860 'N' */
    { 0x91200000, 12,  58}, /*  861 ':' */
    { 0x91300000, 12,  46}, /*  862 '.' */
    { 0x91400000, 12,  44}, /*  863 ',' */
    { 0x91500000, 13,   1}, /*  864 '0x01' */
    { 0x91580000, 13, 108}, /*  865 'l' */
    /*   19                             */
    { 0x00000000,  2,  97}, /*  866 'a' */
    { 0x80000000,  2, 111}, /*  867 'o' */
    { 0xc0000000,  2, 105}, /*  868 'i' */
    { 0x40000000,  3, 101}, /*  869 'e' */
    { 0x70000000,  4, 117}, /*  870 'u' */
    { 0x68000000,  5,  75}, /*  871 'K' */
    { 0x60000000,  7, 108}, /*  872 'l' */
    { 0x64000000,  7,  65}, /*  873 'A' */
    { 0x66000000,  7,  32}, /*  874 ' ' */
    { 0x63000000,  8, 121}, /*  875 'y' */
    { 0x62000000, 10,  76}, /*  876 'L' */
    { 0x62400000, 10,  73}, /*  877 'I' */
    { 0x62800000, 11,  67}, /*  878 'C' */
    { 0x62a00000, 11,  46}, /*  879 '.' */
    { 0x62e00000, 11,   0}, /*  880 '0x00' */
    { 0x62d00000, 12,  39}, /*  881 '\'' */
    { 0x62c00000, 13,  69}, /*  882 'E' */
    { 0x62c80000, 14,   1}, /*  883 '0x01' */
    { 0x62cc0000, 14,  89}, /*  884 'Y' */
    /*   29                             */
    { 0x40000000,  2,  97}, /*  885 'a' */
    { 0x80000000,  2, 111}, /*  886 'o' */
    { 0x00000000,  3, 101}, /*  887 'e' */
    { 0xe0000000,  3, 105}, /*  888 'i' */
    { 0x20000000,  4,  84}, /*  889 'T' */
    { 0xc0000000,  4, 121}, /*  890 'y' */
    { 0xd0000000,  4, 117}, /*  891 'u' */
    { 0x30000000,  5,   0}, /*  892 '0x00' */
    { 0x3c000000,  6,  99}, /*  893 'c' */
    { 0x39000000,  8, 114}, /*  894 'r' */
    { 0x3a000000,  8,  69}, /*  895 'E' */
    { 0x3b800000,  9,  70}, /*  896 'F' */
    { 0x38400000, 10,  90}, /*  897 'Z' */
    { 0x38c00000, 10,  32}, /*  898 ' ' */
    { 0x3b000000, 10,  49}, /*  899 '1' */
    { 0x3b400000, 10,  73}, /*  900 'I' */
    { 0x38200000, 11, 104}, /*  901 'h' */
    { 0x38800000, 11,  67}, /*  902 'C' */
    { 0x38100000, 12,  81}, /*  903 'Q' */
    { 0x38a00000, 12,  75}, /*  904 'K' */
    { 0x38b00000, 13,  80}, /*  905 'P' */
    { 0x38000000, 14,  58}, /*  906 ':' */
    { 0x38040000, 14,  46}, /*  907 '.' */
    { 0x38080000, 14,  39}, /*  908 '\'' */
    { 0x38b80000, 14,  77}, /*  909 'M' */
    { 0x380c0000, 15,   1}, /*  910 '0x01' */
    { 0x380e0000, 15, 119}, /*  911 'w' */
    { 0x38bc0000, 15,  83}, /*  912 'S' */
    { 0x38be0000, 15,  82}, /*  913 'R' */
    /*   22                             */
    { 0x80000000,  1, 101}, /*  914 'e' */
    { 0x00000000,  2, 111}, /*  915 'o' */
    { 0x60000000,  3, 105}, /*  916 'i' */
    { 0x50000000,  4,  97}, /*  917 'a' */
    { 0x48000000,  5, 117}, /*  918 'u' */
    { 0x40000000,  6,  67}, /*  919 'C' */
    { 0x44000000,  8,  69}, /*  920 'E' */
    { 0x46000000,  8,  70}, /*  921 'F' */
    { 0x47000000,  9,  66}, /*  922 'B' */
    { 0x47800000,  9,  72}, /*  923 'H' */
    { 0x45800000, 10,  89}, /*  924 'Y' */
    { 0x45000000, 11,  71}, /*  925 'G' */
    { 0x45200000, 11,  39}, /*  926 '\'' */
    { 0x45600000, 11,  73}, /*  927 'I' */
    { 0x45c00000, 11,  65}, /*  928 'A' */
    { 0x45400000, 12,  77}, /*  929 'M' */
    { 0x45e00000, 12,  32}, /*  930 ' ' */
    { 0x45f00000, 12,   0}, /*  931 '0x00' */
    { 0x45500000, 13,  84}, /*  932 'T' */
    { 0x45580000, 14,  46}, /*  933 '.' */
    { 0x455c0000, 15,   1}, /*  934 '0x01' */
    { 0x455e0000, 15,  90}, /*  935 'Z' */
    /*   44                             */
    { 0x00000000,  3,  39}, /*  936 '\'' */
    { 0x40000000,  3, 102}, /*  937 'f' */
    { 0xc0000000,  3, 117}, /*  938 'u' */
    { 0xe0000000,  3, 110}, /*  939 'n' */
    { 0x20000000,  4,  77}, /*  940 'M' */
    { 0x30000000,  4, 108}, /*  941 'l' */
    { 0x90000000,  4, 109}, /*  942 'm' */
    { 0x68000000,  5, 114}, /*  943 'r' */
    { 0x70000000,  5, 100}, /*  944 'd' */
    { 0x80000000,  5, 112}, /*  945 'p' */
    { 0xa0000000,  5, 104}, /*  946 'h' */
    { 0xb0000000,  5,   0}, /*  947 '0x00' */
    { 0x64000000,  6,  83}, /*  948 'S' */
    { 0x78000000,  6, 122}, /*  949 'z' */
    { 0x7c000000,  6,  98}, /*  950 'b' */
    { 0x8c000000,  6, 118}, /*  951 'v' */
    { 0xa8000000,  6, 119}, /*  952 'w' */
    { 0xac000000,  6,  85}, /*  953 'U' */
    { 0xb8000000,  7,  84}, /*  954 'T' */
    { 0xba000000,  7,  79}, /*  955 'O' */
    { 0xbc000000,  7,  75}, /*  956 'K' */
    { 0x61000000,  8,  67}, /*  957 'C' */
    { 0x62000000,  8, 120}, /*  958 'x' */
    { 0x63000000,  8,  46}, /*  959 '.' */
    { 0x89000000,  8, 116}, /*  960 't' */
    { 0x8b000000,  8,  32}, /*  961 ' ' */
    { 0xbe000000,  8, 115}, /*  962 's' */
    { 0xbf000000,  8,  78}, /*  963 'N' */
    { 0x60800000,  9, 103}, /*  964 'g' */
    { 0x88000000,  9,  45}, /*  965 '-' */
    { 0x8a800000,  9,  97}, /*  966 'a' */
    { 0x88800000, 10, 105}, /*  967 'i' */
    { 0x88c00000, 10, 101}, /*  968 'e' */
    { 0x8a400000, 10, 111}, /*  969 'o' */
    { 0x60000000, 11,  65}, /*  970 'A' */
    { 0x60200000, 11, 106}, /*  971 'j' */
    { 0x60400000, 11,  99}, /*  972 'c' */
    { 0x8a000000, 11,  50}, /*  973 '2' */
    { 0x60700000, 12,  82}, /*  974 'R' */
    { 0x8a200000, 12,  80}, /*  975 'P' */
    { 0x8a300000, 12,  58}, /*  976 ':' */
    { 0x60600000, 13,  69}, /*  977 'E' */
    { 0x60680000, 14,   1}, /*  978 '0x01' */
    { 0x606c0000, 14,  76}, /*  979 'L' */
    /*   28                             */
    { 0x40000000,  2, 114}, /*  980 'r' */
    { 0x80000000,  2, 108}, /*  981 'l' */
    { 0x00000000,  3, 101}, /*  982 'e' */
    { 0x20000000,  3,  97}, /*  983 'a' */
    { 0xe0000000,  3, 111}, /*  984 'o' */
    { 0xd0000000,  4, 105}, /*  985 'i' */
    { 0xc0000000,  6,  68}, /*  986 'D' */
    { 0xc4000000,  6, 117}, /*  987 'u' */
    { 0xcc000000,  6, 104}, /*  988 'h' */
    { 0xc8000000,  8,  32}, /*  989 ' ' */
    { 0xca000000,  8,  50}, /*  990 '2' */
    { 0xc9000000,  9,  72}, /*  991 'H' */
    { 0xc9800000,  9,  77}, /*  992 'M' */
    { 0xcb000000,  9,  83}, /*  993 'S' */
    { 0xcb800000, 11,  58}, /*  994 ':' */
    { 0xcba00000, 11,  42}, /*  995 '*' */
    { 0xcbd00000, 12, 115}, /*  996 's' */
    { 0xcbc80000, 13,  73}, /*  997 'I' */
    { 0xcbe00000, 13,   0}, /*  998 '0x00' */
    { 0xcbe80000, 13,  71}, /*  999 'G' */
    { 0xcbf00000, 13,  39}, /* 1000 '\'' */
    { 0xcbf80000, 14, 121}, /* 1001 'y' */
    { 0xcbc00000, 15,  89}, /* 1002 'Y' */
    { 0xcbc20000, 15,  76}, /* 1003 'L' */
    { 0xcbc40000, 15,  67}, /* 1004 'C' */
    { 0xcbc60000, 15,   1}, /* 1005 '0x01' */
    { 0xcbfc0000, 15,  79}, /* 1006 'O' */
    { 0xcbfe0000, 15,  46}, /* 1007 '.' */
    /*    8                             */
    { 0x80000000,  1, 117}, /* 1008 'u' */
    { 0x00000000,  3,  73}, /* 1009 'I' */
    { 0x20000000,  3,   0}, /* 1010 '0x00' */
    { 0x40000000,  3,  86}, /* 1011 'V' */
    { 0x70000000,  4,  32}, /* 1012 ' ' */
    { 0x68000000,  5,  67}, /* 1013 'C' */
    { 0x60000000,  6,   1}, /* 1014 '0x01' */
    { 0x64000000,  6,  39}, /* 1015 '\'' */
    /*   23                             */
    { 0x00000000,  2,  97}, /* 1016 'a' */
    { 0x40000000,  2, 111}, /* 1017 'o' */
    { 0xc0000000,  2, 101}, /* 1018 'e' */
    { 0x80000000,  3, 105}, /* 1019 'i' */
    { 0xb0000000,  4, 117}, /* 1020 'u' */
    { 0xa8000000,  5,  69}, /* 1021 'E' */
    { 0xa0000000,  6,  68}, /* 1022 'D' */
    { 0xa6000000,  7,   0}, /* 1023 '0x00' */
    { 0xa5000000,  8, 104}, /* 1024 'h' */
    { 0xa4000000, 10,  73}, /* 1025 'I' */
    { 0xa4800000, 10, 121}, /* 1026 'y' */
    { 0xa4c00000, 10, 110}, /* 1027 'n' */
    { 0xa4600000, 12,  32}, /* 1028 ' ' */
    { 0xa4400000, 13,  39}, /* 1029 '\'' */
    { 0xa4580000, 13,  83}, /* 1030 'S' */
    { 0xa4700000, 13,  78}, /* 1031 'N' */
    { 0xa4480000, 14,  66}, /* 1032 'B' */
    { 0xa44c0000, 14,  46}, /* 1033 '.' */
    { 0xa4500000, 14,  38}, /* 1034 '&' */
    { 0xa4780000, 14,  84}, /* 1035 'T' */
    { 0xa47c0000, 14,  49}, /* 1036 '1' */
    { 0xa4540000, 15,   1}, /* 1037 '0x01' */
    { 0xa4560000, 15,  67}, /* 1038 'C' */
    /*   39                             */
    { 0x20000000,  3, 111}, /* 1039 'o' */
    { 0x40000000,  3, 112}, /* 1040 'p' */
    { 0x60000000,  3, 117}, /* 1041 'u' */
    { 0xc0000000,  3, 104}, /* 1042 'h' */
    { 0xe0000000,  3, 116}, /* 1043 't' */
    { 0x10000000,  4,  97}, /* 1044 'a' */
    { 0x90000000,  4, 101}, /* 1045 'e' */
    { 0xb0000000,  4,  99}, /* 1046 'c' */
    { 0x00000000,  5, 110}, /* 1047 'n' */
    { 0x88000000,  5, 105}, /* 1048 'i' */
    { 0xa8000000,  5, 107}, /* 1049 'k' */
    { 0x0c000000,  6, 119}, /* 1050 'w' */
    { 0xa0000000,  6, 109}, /* 1051 'm' */
    { 0x0a000000,  7,  65}, /* 1052 'A' */
    { 0x84000000,  7, 108}, /* 1053 'l' */
    { 0xa4000000,  7, 113}, /* 1054 'q' */
    { 0xa6000000,  7,  77}, /* 1055 'M' */
    { 0x08000000,  8,  50}, /* 1056 '2' */
    { 0x09000000,  8,  80}, /* 1057 'P' */
    { 0x80000000,  8,  79}, /* 1058 'O' */
    { 0x82000000,  8,  73}, /* 1059 'I' */
    { 0x83000000,  8,  32}, /* 1060 ' ' */
    { 0x87000000,  8,   0}, /* 1061 '0x00' */
    { 0x81000000,  9, 121}, /* 1062 'y' */
    { 0x86000000,  9,  69}, /* 1063 'E' */
    { 0x81c00000, 10,  63}, /* 1064 '?' */
    { 0x86c00000, 10,  72}, /* 1065 'H' */
    { 0x81800000, 11,  66}, /* 1066 'B' */
    { 0x86800000, 11, 103}, /* 1067 'g' */
    { 0x81b00000, 12, 114}, /* 1068 'r' */
    { 0x86a00000, 12,  42}, /* 1069 '*' */
    { 0x86b00000, 13,  51}, /* 1070 '3' */
    { 0x86b80000, 13,  46}, /* 1071 '.' */
    { 0x81a00000, 14,  53}, /* 1072 '5' */
    { 0x81a80000, 14,  58}, /* 1073 ':' */
    { 0x81ac0000, 14,  49}, /* 1074 '1' */
    { 0x81a40000, 15,  67}, /* 1075 'C' */
    { 0x81a60000, 16,   1}, /* 1076 '0x01' */
    { 0x81a70000, 16,  89}, /* 1077 'Y' */
    /*   33                             */
    { 0x00000000,  1, 104}, /* 1078 'h' */
    { 0xa0000000,  3, 111}, /* 1079 'o' */
    { 0xe0000000,  3,  86}, /* 1080 'V' */
    { 0x80000000,  4, 101}, /* 1081 'e' */
    { 0x90000000,  4, 114}, /* 1082 'r' */
    { 0xc0000000,  5,  97}, /* 1083 'a' */
    { 0xd0000000,  5, 119}, /* 1084 'w' */
    { 0xcc000000,  6, 105}, /* 1085 'i' */
    { 0xda000000,  7,  79}, /* 1086 'O' */
    { 0xde000000,  7,  72}, /* 1087 'H' */
    { 0xc9000000,  8, 121}, /* 1088 'y' */
    { 0xca000000,  8,  77}, /* 1089 'M' */
    { 0xd8000000,  8,  46}, /* 1090 '.' */
    { 0xdc000000,  8, 117}, /* 1091 'u' */
    { 0xdd000000,  8,  87}, /* 1092 'W' */
    { 0xc8800000,  9,  80}, /* 1093 'P' */
    { 0xcb000000,  9,  58}, /* 1094 ':' */
    { 0xcb800000,  9,  52}, /* 1095 '4' */
    { 0xd9800000,  9,  73}, /* 1096 'I' */
    { 0xd9000000, 10,  32}, /* 1097 ' ' */
    { 0xd9400000, 10,   0}, /* 1098 '0x00' */
    { 0xc8400000, 11,  88}, /* 1099 'X' */
    { 0xc8600000, 11, 115}, /* 1100 's' */
    { 0xc8000000, 12,  84}, /* 1101 'T' */
    { 0xc8100000, 12,  83}, /* 1102 'S' */
    { 0xc8200000, 12,  66}, /* 1103 'B' */
    { 0xc8300000, 13,  85}, /* 1104 'U' */
    { 0xc8380000, 14,  65}, /* 1105 'A' */
    { 0xc83c0000, 16,  67}, /* 1106 'C' */
    { 0xc83d0000, 16,  42}, /* 1107 '*' */
    { 0xc83f0000, 16,  78}, /* 1108 'N' */
    { 0xc83e0000, 17,   1}, /* 1109 '0x01' */
    { 0xc83e8000, 17,  89}, /* 1110 'Y' */
    /*   23                             */
    { 0x00000000,  1, 110}, /* 1111 'n' */
    { 0x80000000,  2, 112}, /* 1112 'p' */
    { 0xd0000000,  4,  75}, /* 1113 'K' */
    { 0xf0000000,  4, 108}, /* 1114 'l' */
    { 0xc0000000,  5,  82}, /* 1115 'R' */
    { 0xe0000000,  5,  83}, /* 1116 'S' */
    { 0xcc000000,  6,  69}, /* 1117 'E' */
    { 0xec000000,  6, 115}, /* 1118 's' */
    { 0xca000000,  7, 103}, /* 1119 'g' */
    { 0xea000000,  7,  84}, /* 1120 'T' */
    { 0xc9000000,  8,  32}, /* 1121 ' ' */
    { 0xc8000000,  9,  45}, /* 1122 '-' */
    { 0xc8800000,  9, 114}, /* 1123 'r' */
    { 0xe8000000,  9,  50}, /* 1124 '2' */
    { 0xe8800000,  9, 109}, /* 1125 'm' */
    { 0xe9800000,  9,   0}, /* 1126 '0x00' */
    { 0xe9000000, 10,  46}, /* 1127 '.' */
    { 0xe9400000, 11,  99}, /* 1128 'c' */
    { 0xe9600000, 12, 107}, /* 1129 'k' */
    { 0xe9700000, 14,   1}, /* 1130 '0x01' */
    { 0xe9740000, 14, 122}, /* 1131 'z' */
    { 0xe9780000, 14, 116}, /* 1132 't' */
    { 0xe97c0000, 14,  66}, /* 1133 'B' */
    /*   21                             */
    { 0x80000000,  1,  32}, /* 1134 ' ' */
    { 0x00000000,  3,  58}, /* 1135 ':' */
    { 0x60000000,  3, 105}, /* 1136 'i' */
    { 0x20000000,  4, 101}, /* 1137 'e' */
    { 0x30000000,  4,  97}, /* 1138 'a' */
    { 0x40000000,  4,  51}, /* 1139 '3' */
    { 0x54000000,  6,  67}, /* 1140 'C' */
    { 0x5c000000,  6,   0}, /* 1141 '0x00' */
    { 0x50000000,  7,  39}, /* 1142 '\'' */
    { 0x52000000,  7,  52}, /* 1143 '4' */
    { 0x5a000000,  7, 111}, /* 1144 'o' */
    { 0x59000000,  8,  73}, /* 1145 'I' */
    { 0x58000000, 10, 115}, /* 1146 's' */
    { 0x58400000, 10,  68}, /* 1147 'D' */
    { 0x58800000, 10,  46}, /* 1148 '.' */
    { 0x58c00000, 11,  56}, /* 1149 '8' */
    { 0x58e80000, 13, 117}, /* 1150 'u' */
    { 0x58f00000, 13, 114}, /* 1151 'r' */
    { 0x58f80000, 13,  66}, /* 1152 'B' */
    { 0x58e00000, 14,   1}, /* 1153 '0x01' */
    { 0x58e40000, 14,  69}, /* 1154 'E' */
    /*   18                             */
    { 0x40000000,  2, 111}, /* 1155 'o' */
    { 0xc0000000,  2, 101}, /* 1156 'e' */
    { 0x20000000,  3, 104}, /* 1157 'h' */
    { 0x80000000,  3,  97}, /* 1158 'a' */
    { 0xa0000000,  3, 105}, /* 1159 'i' */
    { 0x00000000,  5,  46}, /* 1160 '.' */
    { 0x10000000,  5,  79}, /* 1161 'O' */
    { 0x18000000,  5, 114}, /* 1162 'r' */
    { 0x0c000000,  6, 121}, /* 1163 'y' */
    { 0x08000000,  7, 117}, /* 1164 'u' */
    { 0x0a000000,  8,   0}, /* 1165 '0x00' */
    { 0x0b800000,  9,  65}, /* 1166 'A' */
    { 0x0b200000, 11,  89}, /* 1167 'Y' */
    { 0x0b400000, 11,  84}, /* 1168 'T' */
    { 0x0b600000, 11,  32}, /* 1169 ' ' */
    { 0x0b000000, 12,  73}, /* 1170 'I' */
    { 0x0b100000, 13,   1}, /* 1171 '0x01' */
    { 0x0b180000, 13, 108}, /* 1172 'l' */
    /*   14                             */
    { 0x00000000,  2,   0}, /* 1173 '0x00' */
    { 0x80000000,  2,  32}, /* 1174 ' ' */
    { 0xc0000000,  2, 116}, /* 1175 't' */
    { 0x40000000,  3,  84}, /* 1176 'T' */
    { 0x70000000,  4,  99}, /* 1177 'c' */
    { 0x68000000,  5, 109}, /* 1178 'm' */
    { 0x64000000,  6,  85}, /* 1179 'U' */
    { 0x60000000,  8,  97}, /* 1180 'a' */
    { 0x61000000,  8,  88}, /* 1181 'X' */
    { 0x62000000,  8,  45}, /* 1182 '-' */
    { 0x63000000,  9, 120}, /* 1183 'x' */
    { 0x63c00000, 10,  57}, /* 1184 '9' */
    { 0x63800000, 11,   1}, /* 1185 '0x01' */
    { 0x63a00000, 11, 105}, /* 1186 'i' */
    /*   19                             */
    { 0x80000000,  1, 111}, /* 1187 'o' */
    { 0x40000000,  2, 101}, /* 1188 'e' */
    { 0x00000000,  3, 117}, /* 1189 'u' */
    { 0x30000000,  4,  32}, /* 1190 ' ' */
    { 0x20000000,  5, 118}, /* 1191 'v' */
    { 0x28000000,  6,  97}, /* 1192 'a' */
    { 0x2e000000,  8,  80}, /* 1193 'P' */
    { 0x2f000000,  8,  39}, /* 1194 '\'' */
    { 0x2c000000,  9, 110}, /* 1195 'n' */
    { 0x2d800000,  9, 114}, /* 1196 'r' */
    { 0x2c800000, 10,  68}, /* 1197 'D' */
    { 0x2cc00000, 11, 119}, /* 1198 'w' */
    { 0x2ce00000, 11, 115}, /* 1199 's' */
    { 0x2d000000, 11,  82}, /* 1200 'R' */
    { 0x2d200000, 11,  76}, /* 1201 'L' */
    { 0x2d400000, 11,   0}, /* 1202 '0x00' */
    { 0x2d600000, 12,  67}, /* 1203 'C' */
    { 0x2d700000, 13,   1}, /* 1204 '0x01' */
    { 0x2d780000, 13,  78}, /* 1205 'N' */
    /*   11                             */
    { 0x80000000,  1, 111}, /* 1206 'o' */
    { 0x00000000,  2,  97}, /* 1207 'a' */
    { 0x40000000,  3, 105}, /* 1208 'i' */
    { 0x60000000,  5,  79}, /* 1209 'O' */
    { 0x68000000,  5, 117}, /* 1210 'u' */
    { 0x70000000,  5, 101}, /* 1211 'e' */
    { 0x78000000,  6,  32}, /* 1212 ' ' */
    { 0x7e000000,  7,   0}, /* 1213 '0x00' */
    { 0x7d000000,  8,  58}, /* 1214 ':' */
    { 0x7c000000,  9,   1}, /* 1215 '0x01' */
    { 0x7c800000,  9,  45}, /* 1216 '-' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 1217 '0x01' */
    { 0x80000000,  1,   1}, /* 1218 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 1219 '0x01' */
    { 0x80000000,  1, 120}, /* 1220 'x' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 1221 '0x01' */
    { 0x80000000,  1,   1}, /* 1222 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 1223 '0x01' */
    { 0x80000000,  1,   1}, /* 1224 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 1225 '0x01' */
    { 0x80000000,  1,   1}, /* 1226 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 1227 '0x01' */
    { 0x80000000,  1,   1}, /* 1228 '0x01' */
    /*   39                             */
    { 0x20000000,  3, 114}, /* 1229 'r' */
    { 0x60000000,  3, 116}, /* 1230 't' */
    { 0x80000000,  3, 108}, /* 1231 'l' */
    { 0xc0000000,  3, 110}, /* 1232 'n' */
    { 0x10000000,  4, 109}, /* 1233 'm' */
    { 0x40000000,  4,  99}, /* 1234 'c' */
    { 0xa0000000,  4, 115}, /* 1235 's' */
    { 0xe0000000,  4, 121}, /* 1236 'y' */
    { 0xb0000000,  5,  32}, /* 1237 ' ' */
    { 0xb8000000,  5, 100}, /* 1238 'd' */
    { 0xf0000000,  5, 105}, /* 1239 'i' */
    { 0xf8000000,  5, 107}, /* 1240 'k' */
    { 0x08000000,  6,  98}, /* 1241 'b' */
    { 0x0c000000,  6,   0}, /* 1242 '0x00' */
    { 0x58000000,  6, 112}, /* 1243 'p' */
    { 0x5c000000,  6, 103}, /* 1244 'g' */
    { 0x00000000,  7, 101}, /* 1245 'e' */
    { 0x02000000,  7,  39}, /* 1246 '\'' */
    { 0x06000000,  7, 119}, /* 1247 'w' */
    { 0x52000000,  7, 117}, /* 1248 'u' */
    { 0x54000000,  7, 122}, /* 1249 'z' */
    { 0x56000000,  7, 118}, /* 1250 'v' */
    { 0x05000000,  8, 102}, /* 1251 'f' */
    { 0x51000000,  8, 104}, /* 1252 'h' */
    { 0x04800000,  9,  58}, /* 1253 ':' */
    { 0x04000000, 10,  33}, /* 1254 '!' */
    { 0x50000000, 10, 111}, /* 1255 'o' */
    { 0x50400000, 10, 120}, /* 1256 'x' */
    { 0x04400000, 11,  45}, /* 1257 '-' */
    { 0x04600000, 11,  97}, /* 1258 'a' */
    { 0x50a00000, 11,  46}, /* 1259 '.' */
    { 0x50c00000, 11,  78}, /* 1260 'N' */
    { 0x50e00000, 11,  44}, /* 1261 ',' */
    { 0x50800000, 12, 113}, /* 1262 'q' */
    { 0x50900000, 13, 106}, /* 1263 'j' */
    { 0x509c0000, 14,  63}, /* 1264 '?' */
    { 0x509a0000, 15,  74}, /* 1265 'J' */
    { 0x50980000, 16,   1}, /* 1266 '0x01' */
    { 0x50990000, 16,  85}, /* 1267 'U' */
    /*   24                             */
    { 0x00000000,  3, 114}, /* 1268 'r' */
    { 0x20000000,  3, 111}, /* 1269 'o' */
    { 0x40000000,  3, 101}, /* 1270 'e' */
    { 0x60000000,  3,  97}, /* 1271 'a' */
    { 0x80000000,  3, 105}, /* 1272 'i' */
    { 0xb0000000,  4, 117}, /* 1273 'u' */
    { 0xc0000000,  4, 121}, /* 1274 'y' */
    { 0xd0000000,  4, 108}, /* 1275 'l' */
    { 0xf0000000,  4,  32}, /* 1276 ' ' */
    { 0xa8000000,  5, 115}, /* 1277 's' */
    { 0xe0000000,  5,  98}, /* 1278 'b' */
    { 0xe8000000,  5,   0}, /* 1279 '0x00' */
    { 0xa0000000,  6, 104}, /* 1280 'h' */
    { 0xa4000000,  7,  51}, /* 1281 '3' */
    { 0xa7000000,  8,  39}, /* 1282 '\'' */
    { 0xa6400000, 10, 116}, /* 1283 't' */
    { 0xa6800000, 10, 106}, /* 1284 'j' */
    { 0xa6000000, 11, 110}, /* 1285 'n' */
    { 0xa6200000, 11, 100}, /* 1286 'd' */
    { 0xa6e00000, 11, 119}, /* 1287 'w' */
    { 0xa6d00000, 12, 109}, /* 1288 'm' */
    { 0xa6c80000, 13,  46}, /* 1289 '.' */
    { 0xa6c00000, 14,   1}, /* 1290 '0x01' */
    { 0xa6c40000, 14,  58}, /* 1291 ':' */
    /*   37                             */
    { 0x00000000,  2, 107}, /* 1292 'k' */
    { 0x40000000,  3, 111}, /* 1293 'o' */
    { 0x80000000,  3, 104}, /* 1294 'h' */
    { 0xc0000000,  3, 116}, /* 1295 't' */
    { 0xe0000000,  3, 101}, /* 1296 'e' */
    { 0x70000000,  4, 114}, /* 1297 'r' */
    { 0xa0000000,  5,  32}, /* 1298 ' ' */
    { 0xb0000000,  5, 105}, /* 1299 'i' */
    { 0xb8000000,  5,  97}, /* 1300 'a' */
    { 0x60000000,  6, 108}, /* 1301 'l' */
    { 0x64000000,  6, 121}, /* 1302 'y' */
    { 0x68000000,  6, 115}, /* 1303 's' */
    { 0x6c000000,  6,   0}, /* 1304 '0x00' */
    { 0xa8000000,  7,  99}, /* 1305 'c' */
    { 0xae000000,  7, 117}, /* 1306 'u' */
    { 0xaa000000,  8,  58}, /* 1307 ':' */
    { 0xab000000,  8,  80}, /* 1308 'P' */
    { 0xac800000,  9,  68}, /* 1309 'D' */
    { 0xad000000, 10,  71}, /* 1310 'G' */
    { 0xad800000, 10,  98}, /* 1311 'b' */
    { 0xac000000, 11,  76}, /* 1312 'L' */
    { 0xac200000, 11,  75}, /* 1313 'K' */
    { 0xac600000, 11,  65}, /* 1314 'A' */
    { 0xad400000, 11, 113}, /* 1315 'q' */
    { 0xadc00000, 11,  46}, /* 1316 '.' */
    { 0xade00000, 11,  67}, /* 1317 'C' */
    { 0xac400000, 12, 110}, /* 1318 'n' */
    { 0xac500000, 12,  39}, /* 1319 '\'' */
    { 0xad600000, 13,  66}, /* 1320 'B' */
    { 0xad680000, 13,  73}, /* 1321 'I' */
    { 0xad740000, 14, 102}, /* 1322 'f' */
    { 0xad780000, 14,  56}, /* 1323 '8' */
    { 0xad700000, 15,  77}, /* 1324 'M' */
    { 0xad720000, 15,   1}, /* 1325 '0x01' */
    { 0xad7e0000, 15,  70}, /* 1326 'F' */
    { 0xad7c0000, 16, 119}, /* 1327 'w' */
    { 0xad7d0000, 16,  81}, /* 1328 'Q' */
    /*   36                             */
    { 0xc0000000,  2,  32}, /* 1329 ' ' */
    { 0x20000000,  3, 101}, /* 1330 'e' */
    { 0x80000000,  3,   0}, /* 1331 '0x00' */
    { 0xa0000000,  3,  97}, /* 1332 'a' */
    { 0x10000000,  4, 121}, /* 1333 'y' */
    { 0x40000000,  4, 105}, /* 1334 'i' */
    { 0x60000000,  4, 115}, /* 1335 's' */
    { 0x00000000,  5, 111}, /* 1336 'o' */
    { 0x50000000,  5, 100}, /* 1337 'd' */
    { 0x0c000000,  6, 117}, /* 1338 'u' */
    { 0x58000000,  6, 114}, /* 1339 'r' */
    { 0x5c000000,  6, 108}, /* 1340 'l' */
    { 0x74000000,  6, 118}, /* 1341 'v' */
    { 0x78000000,  6, 103}, /* 1342 'g' */
    { 0x08000000,  7,  39}, /* 1343 '\'' */
    { 0x7e000000,  7,  46}, /* 1344 '.' */
    { 0x0a000000,  8,  58}, /* 1345 ':' */
    { 0x0b000000,  8, 104}, /* 1346 'h' */
    { 0x70000000,  8,  99}, /* 1347 'c' */
    { 0x72000000,  8, 110}, /* 1348 'n' */
    { 0x73000000,  8, 119}, /* 1349 'w' */
    { 0x71000000,  9,  63}, /* 1350 '?' */
    { 0x7c000000,  9,  33}, /* 1351 '!' */
    { 0x7c800000,  9,  45}, /* 1352 '-' */
    { 0x7d000000,  9, 102}, /* 1353 'f' */
    { 0x71c00000, 10, 109}, /* 1354 'm' */
    { 0x7d800000, 10,  44}, /* 1355 ',' */
    { 0x7dc00000, 11, 116}, /* 1356 't' */
    { 0x7de00000, 11,  98}, /* 1357 'b' */
    { 0x71900000, 12,  41}, /* 1358 ')' */
    { 0x71a00000, 12,  47}, /* 1359 '/' */
    { 0x71b00000, 12, 107}, /* 1360 'k' */
    { 0x71880000, 13, 112}, /* 1361 'p' */
    { 0x71840000, 14, 122}, /* 1362 'z' */
    { 0x71800000, 15,   1}, /* 1363 '0x01' */
    { 0x71820000, 15,  52}, /* 1364 '4' */
    /*   42                             */
    { 0x40000000,  2,  32}, /* 1365 ' ' */
    { 0x00000000,  3, 115}, /* 1366 's' */
    { 0xa0000000,  3, 114}, /* 1367 'r' */
    { 0x20000000,  4, 116}, /* 1368 't' */
    { 0x90000000,  4, 110}, /* 1369 'n' */
    { 0xc0000000,  4,   0}, /* 1370 '0x00' */
    { 0xe0000000,  4,  97}, /* 1371 'a' */
    { 0xf0000000,  4, 119}, /* 1372 'w' */
    { 0x80000000,  5, 108}, /* 1373 'l' */
    { 0xd8000000,  5, 101}, /* 1374 'e' */
    { 0x38000000,  6, 109}, /* 1375 'm' */
    { 0x88000000,  6,  99}, /* 1376 'c' */
    { 0x8c000000,  6, 100}, /* 1377 'd' */
    { 0x34000000,  7, 105}, /* 1378 'i' */
    { 0x36000000,  7, 112}, /* 1379 'p' */
    { 0x3c000000,  7,  98}, /* 1380 'b' */
    { 0xd0000000,  7, 118}, /* 1381 'v' */
    { 0xd6000000,  7, 121}, /* 1382 'y' */
    { 0x30000000,  8, 103}, /* 1383 'g' */
    { 0x31000000,  8, 102}, /* 1384 'f' */
    { 0x32000000,  8, 120}, /* 1385 'x' */
    { 0x3e000000,  8, 107}, /* 1386 'k' */
    { 0x3f000000,  8,  58}, /* 1387 ':' */
    { 0xd3000000,  8, 111}, /* 1388 'o' */
    { 0xd4000000,  8,  39}, /* 1389 '\'' */
    { 0x33800000,  9, 104}, /* 1390 'h' */
    { 0xd2800000,  9,  46}, /* 1391 '.' */
    { 0x33000000, 10,  80}, /* 1392 'P' */
    { 0x33400000, 10,  66}, /* 1393 'B' */
    { 0xd2000000, 10,  44}, /* 1394 ',' */
    { 0xd5000000, 10,  86}, /* 1395 'V' */
    { 0xd5400000, 10, 122}, /* 1396 'z' */
    { 0xd5c00000, 10, 106}, /* 1397 'j' */
    { 0xd2400000, 11,  52}, /* 1398 '4' */
    { 0xd2600000, 11,  63}, /* 1399 '?' */
    { 0xd5a00000, 11, 117}, /* 1400 'u' */
    { 0xd5900000, 12,  45}, /* 1401 '-' */
    { 0xd5880000, 13,  33}, /* 1402 '!' */
    { 0xd5840000, 14, 113}, /* 1403 'q' */
    { 0xd5820000, 15,  71}, /* 1404 'G' */
    { 0xd5800000, 16,   1}, /* 1405 '0x01' */
    { 0xd5810000, 16,  83}, /* 1406 'S' */
    /*   24                             */
    { 0x00000000,  1,  32}, /* 1407 ' ' */
    { 0xa0000000,  3, 111}, /* 1408 'o' */
    { 0x90000000,  4, 116}, /* 1409 't' */
    { 0xc0000000,  4,  97}, /* 1410 'a' */
    { 0xd0000000,  4, 105}, /* 1411 'i' */
    { 0xf0000000,  4, 101}, /* 1412 'e' */
    { 0x80000000,  5,  46}, /* 1413 '.' */
    { 0xe0000000,  5, 114}, /* 1414 'r' */
    { 0xe8000000,  5, 102}, /* 1415 'f' */
    { 0x88000000,  6,   0}, /* 1416 '0x00' */
    { 0x8d000000,  8, 121}, /* 1417 'y' */
    { 0x8f000000,  8, 117}, /* 1418 'u' */
    { 0x8c000000,  9,  39}, /* 1419 '\'' */
    { 0x8e800000,  9, 108}, /* 1420 'l' */
    { 0x8cc00000, 10, 110}, /* 1421 'n' */
    { 0x8e000000, 10, 103}, /* 1422 'g' */
    { 0x8c800000, 11,  99}, /* 1423 'c' */
    { 0x8e400000, 11,  45}, /* 1424 '-' */
    { 0x8ca00000, 12,  44}, /* 1425 ',' */
    { 0x8cb00000, 12, 115}, /* 1426 's' */
    { 0x8e700000, 12,  58}, /* 1427 ':' */
    { 0x8e680000, 13, 107}, /* 1428 'k' */
    { 0x8e600000, 14,   1}, /* 1429 '0x01' */
    { 0x8e640000, 14,  98}, /* 1430 'b' */
    /*   33                             */
    { 0x00000000,  2, 104}, /* 1431 'h' */
    { 0x80000000,  2,  32}, /* 1432 ' ' */
    { 0x40000000,  3,   0}, /* 1433 '0x00' */
    { 0x60000000,  3, 101}, /* 1434 'e' */
    { 0xc0000000,  4, 105}, /* 1435 'i' */
    { 0xe0000000,  5,  58}, /* 1436 ':' */
    { 0xe8000000,  5, 114}, /* 1437 'r' */
    { 0xf8000000,  5,  97}, /* 1438 'a' */
    { 0xd0000000,  6, 115}, /* 1439 's' */
    { 0xdc000000,  6, 108}, /* 1440 'l' */
    { 0xf4000000,  6, 117}, /* 1441 'u' */
    { 0xd6000000,  7,  98}, /* 1442 'b' */
    { 0xd8000000,  7, 103}, /* 1443 'g' */
    { 0xda000000,  7, 111}, /* 1444 'o' */
    { 0xf2000000,  7, 110}, /* 1445 'n' */
    { 0xd4000000,  8,  50}, /* 1446 '2' */
    { 0xf0000000,  8,  33}, /* 1447 '!' */
    { 0xf1800000,  9, 100}, /* 1448 'd' */
    { 0xd5000000, 10,  46}, /* 1449 '.' */
    { 0xd5400000, 10,  44}, /* 1450 ',' */
    { 0xd5800000, 10,  39}, /* 1451 '\'' */
    { 0xd5c00000, 10, 116}, /* 1452 't' */
    { 0xf1400000, 10, 121}, /* 1453 'y' */
    { 0xf1000000, 11, 119}, /* 1454 'w' */
    { 0xf1300000, 12, 109}, /* 1455 'm' */
    { 0xf12c0000, 14,  63}, /* 1456 '?' */
    { 0xf1200000, 15, 112}, /* 1457 'p' */
    { 0xf1220000, 15, 102}, /* 1458 'f' */
    { 0xf1240000, 15,  64}, /* 1459 '@' */
    { 0xf1260000, 15,  45}, /* 1460 '-' */
    { 0xf12a0000, 15,  59}, /* 1461 ';' */
    { 0xf1280000, 16,   1}, /* 1462 '0x01' */
    { 0xf1290000, 16, 122}, /* 1463 'z' */
    /*   31                             */
    { 0x00000000,  1, 101}, /* 1464 'e' */
    { 0xa0000000,  3, 111}, /* 1465 'o' */
    { 0x90000000,  4, 105}, /* 1466 'i' */
    { 0xc0000000,  4,  97}, /* 1467 'a' */
    { 0xe0000000,  4,  32}, /* 1468 ' ' */
    { 0xf0000000,  4, 116}, /* 1469 't' */
    { 0xd0000000,  5, 114}, /* 1470 'r' */
    { 0xd8000000,  5,   0}, /* 1471 '0x00' */
    { 0x80000000,  6,  98}, /* 1472 'b' */
    { 0x84000000,  6, 117}, /* 1473 'u' */
    { 0x88000000,  8, 119}, /* 1474 'w' */
    { 0x89000000,  8, 100}, /* 1475 'd' */
    { 0x8a000000,  8, 110}, /* 1476 'n' */
    { 0x8b000000,  8, 121}, /* 1477 'y' */
    { 0x8c000000,  8,  33}, /* 1478 '!' */
    { 0x8d000000,  8, 108}, /* 1479 'l' */
    { 0x8f000000,  8,  46}, /* 1480 '.' */
    { 0x8e000000,  9,  39}, /* 1481 '\'' */
    { 0x8e800000, 10, 115}, /* 1482 's' */
    { 0x8ec00000, 11, 109}, /* 1483 'm' */
    { 0x8ef00000, 12,  58}, /* 1484 ':' */
    { 0x8ee00000, 14, 102}, /* 1485 'f' */
    { 0x8ee40000, 14,  63}, /* 1486 '?' */
    { 0x8ee80000, 14,  99}, /* 1487 'c' */
    { 0x8eec0000, 16, 118}, /* 1488 'v' */
    { 0x8eed0000, 16, 113}, /* 1489 'q' */
    { 0x8eee0000, 16, 103}, /* 1490 'g' */
    { 0x8eef0000, 18, 104}, /* 1491 'h' */
    { 0x8eef4000, 18,   1}, /* 1492 '0x01' */
    { 0x8eef8000, 18,  44}, /* 1493 ',' */
    { 0x8eefc000, 18,  42}, /* 1494 '*' */
    /*   35                             */
    { 0x40000000,  2, 110}, /* 1495 'n' */
    { 0x00000000,  3,  99}, /* 1496 'c' */
    { 0x90000000,  4, 111}, /* 1497 'o' */
    { 0xa0000000,  4, 108}, /* 1498 'l' */
    { 0xc0000000,  4, 103}, /* 1499 'g' */
    { 0xd0000000,  4, 115}, /* 1500 's' */
    { 0xe0000000,  4, 116}, /* 1501 't' */
    { 0xf0000000,  4, 101}, /* 1502 'e' */
    { 0x28000000,  5,  97}, /* 1503 'a' */
    { 0x30000000,  5, 118}, /* 1504 'v' */
    { 0x80000000,  5, 114}, /* 1505 'r' */
    { 0x88000000,  5, 100}, /* 1506 'd' */
    { 0xb0000000,  5, 109}, /* 1507 'm' */
    { 0x20000000,  6, 112}, /* 1508 'p' */
    { 0x38000000,  6,  32}, /* 1509 ' ' */
    { 0xbc000000,  6, 102}, /* 1510 'f' */
    { 0x3c000000,  7, 122}, /* 1511 'z' */
    { 0x3e000000,  7,   0}, /* 1512 '0x00' */
    { 0xb8000000,  7,  98}, /* 1513 'b' */
    { 0xba000000,  7, 107}, /* 1514 'k' */
    { 0x25000000,  8,  45}, /* 1515 '-' */
    { 0x26000000,  8, 120}, /* 1516 'x' */
    { 0x24800000,  9,  39}, /* 1517 '\'' */
    { 0x27800000,  9, 113}, /* 1518 'q' */
    { 0x27000000, 10, 117}, /* 1519 'u' */
    { 0x27400000, 10, 105}, /* 1520 'i' */
    { 0x24200000, 11, 104}, /* 1521 'h' */
    { 0x24400000, 11,  58}, /* 1522 ':' */
    { 0x24600000, 11, 119}, /* 1523 'w' */
    { 0x24080000, 13,  44}, /* 1524 ',' */
    { 0x24100000, 13, 121}, /* 1525 'y' */
    { 0x24180000, 13,  47}, /* 1526 '/' */
    { 0x24000000, 14,  46}, /* 1527 '.' */
    { 0x24040000, 15,   1}, /* 1528 '0x01' */
    { 0x24060000, 15, 106}, /* 1529 'j' */
    /*    9                             */
    { 0x00000000,  1, 121}, /* 1530 'y' */
    { 0xc0000000,  2, 111}, /* 1531 'o' */
    { 0xa0000000,  3, 101}, /* 1532 'e' */
    { 0x90000000,  4,  97}, /* 1533 'a' */
    { 0x88000000,  5, 117}, /* 1534 'u' */
    { 0x84000000,  6, 105}, /* 1535 'i' */
    { 0x80000000,  7,   0}, /* 1536 '0x00' */
    { 0x82000000,  8,   1}, /* 1537 '0x01' */
    { 0x83000000,  8,  32}, /* 1538 ' ' */
    /*   33                             */
    { 0x00000000,  2,  32}, /* 1539 ' ' */
    { 0x80000000,  2, 101}, /* 1540 'e' */
    { 0x40000000,  3, 105}, /* 1541 'i' */
    { 0xc0000000,  3,   0}, /* 1542 '0x00' */
    { 0x60000000,  4, 121}, /* 1543 'y' */
    { 0x70000000,  4, 115}, /* 1544 's' */
    { 0xf0000000,  4, 102}, /* 1545 'f' */
    { 0xe4000000,  6,  97}, /* 1546 'a' */
    { 0xe8000000,  6, 108}, /* 1547 'l' */
    { 0xe2000000,  7,  58}, /* 1548 ':' */
    { 0xec000000,  7, 107}, /* 1549 'k' */
    { 0xe0000000,  8,  39}, /* 1550 '\'' */
    { 0xef000000,  8,  46}, /* 1551 '.' */
    { 0xe1800000,  9, 119}, /* 1552 'w' */
    { 0xee000000,  9, 111}, /* 1553 'o' */
    { 0xe1400000, 10, 104}, /* 1554 'h' */
    { 0xe1000000, 11,  98}, /* 1555 'b' */
    { 0xe1200000, 11,  44}, /* 1556 ',' */
    { 0xeee00000, 11, 110}, /* 1557 'n' */
    { 0xeea00000, 12,  63}, /* 1558 '?' */
    { 0xeec00000, 12, 109}, /* 1559 'm' */
    { 0xeed00000, 12,  33}, /* 1560 '!' */
    { 0xee900000, 13, 117}, /* 1561 'u' */
    { 0xee980000, 13,  99}, /* 1562 'c' */
    { 0xeeb00000, 13, 100}, /* 1563 'd' */
    { 0xeeb80000, 13, 116}, /* 1564 't' */
    { 0xee840000, 14, 106}, /* 1565 'j' */
    { 0xee880000, 14,  45}, /* 1566 '-' */
    { 0xee800000, 15, 112}, /* 1567 'p' */
    { 0xee820000, 15,  47}, /* 1568 '/' */
    { 0xee8e0000, 15,  83}, /* 1569 'S' */
    { 0xee8c0000, 16,   1}, /* 1570 '0x01' */
    { 0xee8d0000, 16, 114}, /* 1571 'r' */
    /*   36                             */
    { 0x40000000,  2, 101}, /* 1572 'e' */
    { 0x00000000,  3, 108}, /* 1573 'l' */
    { 0xa0000000,  3,  97}, /* 1574 'a' */
    { 0x30000000,  4, 121}, /* 1575 'y' */
    { 0x80000000,  4,   0}, /* 1576 '0x00' */
    { 0x90000000,  4, 100}, /* 1577 'd' */
    { 0xc0000000,  4, 111}, /* 1578 'o' */
    { 0xe0000000,  4, 105}, /* 1579 'i' */
    { 0xf0000000,  4,  32}, /* 1580 ' ' */
    { 0x20000000,  5, 117}, /* 1581 'u' */
    { 0xd0000000,  5, 115}, /* 1582 's' */
    { 0x28000000,  6, 116}, /* 1583 't' */
    { 0x2c000000,  6, 109}, /* 1584 'm' */
    { 0xda000000,  7, 107}, /* 1585 'k' */
    { 0xd8000000,  8, 102}, /* 1586 'f' */
    { 0xdc000000,  8,  98}, /* 1587 'b' */
    { 0xde000000,  8,  39}, /* 1588 '\'' */
    { 0xdf000000,  8,  99}, /* 1589 'c' */
    { 0xd9800000,  9, 118}, /* 1590 'v' */
    { 0xdd000000,  9,  58}, /* 1591 ':' */
    { 0xd9400000, 10,  46}, /* 1592 '.' */
    { 0xdd800000, 10, 119}, /* 1593 'w' */
    { 0xd9000000, 11, 122}, /* 1594 'z' */
    { 0xdde00000, 11, 112}, /* 1595 'p' */
    { 0xd9200000, 12, 104}, /* 1596 'h' */
    { 0xd9300000, 12,  42}, /* 1597 '*' */
    { 0xddc00000, 13, 103}, /* 1598 'g' */
    { 0xddc80000, 13,  44}, /* 1599 ',' */
    { 0xddd00000, 14, 114}, /* 1600 'r' */
    { 0xddd40000, 14, 110}, /* 1601 'n' */
    { 0xdddc0000, 14,  45}, /* 1602 '-' */
    { 0xddd80000, 15,  33}, /* 1603 '!' */
    { 0xdddb0000, 16,  63}, /* 1604 '?' */
    { 0xddda8000, 17,  67}, /* 1605 'C' */
    { 0xddda0000, 18,   1}, /* 1606 '0x01' */
    { 0xddda4000, 18, 106}, /* 1607 'j' */
    /*   25                             */
    { 0x80000000,  2, 101}, /* 1608 'e' */
    { 0x20000000,  3, 109}, /* 1609 'm' */
    { 0x60000000,  3,  32}, /* 1610 ' ' */
    { 0xe0000000,  3,  97}, /* 1611 'a' */
    { 0x00000000,  4, 105}, /* 1612 'i' */
    { 0x10000000,  4,   0}, /* 1613 '0x00' */
    { 0x40000000,  4, 121}, /* 1614 'y' */
    { 0xd0000000,  4, 112}, /* 1615 'p' */
    { 0x50000000,  5,  98}, /* 1616 'b' */
    { 0xc0000000,  5, 111}, /* 1617 'o' */
    { 0xc8000000,  6, 110}, /* 1618 'n' */
    { 0xcc000000,  6, 115}, /* 1619 's' */
    { 0x58000000,  7, 108}, /* 1620 'l' */
    { 0x5c000000,  7, 102}, /* 1621 'f' */
    { 0x5a000000,  8,  58}, /* 1622 ':' */
    { 0x5e000000,  8,  52}, /* 1623 '4' */
    { 0x5b000000,  9, 104}, /* 1624 'h' */
    { 0x5f000000,  9, 119}, /* 1625 'w' */
    { 0x5bc00000, 10,  39}, /* 1626 '\'' */
    { 0x5f800000, 10, 114}, /* 1627 'r' */
    { 0x5fc00000, 10, 117}, /* 1628 'u' */
    { 0x5ba00000, 11,  46}, /* 1629 '.' */
    { 0x5b900000, 12, 107}, /* 1630 'k' */
    { 0x5b800000, 13,   1}, /* 1631 '0x01' */
    { 0x5b880000, 13, 100}, /* 1632 'd' */
    /*   40                             */
    { 0x00000000,  3, 105}, /* 1633 'i' */
    { 0x80000000,  3, 103}, /* 1634 'g' */
    { 0xa0000000,  3,  32}, /* 1635 ' ' */
    { 0xc0000000,  3, 100}, /* 1636 'd' */
    { 0x30000000,  4,  97}, /* 1637 'a' */
    { 0x40000000,  4, 115}, /* 1638 's' */
    { 0x60000000,  4, 101}, /* 1639 'e' */
    { 0xe0000000,  4,   0}, /* 1640 '0x00' */
    { 0xf0000000,  4, 116}, /* 1641 't' */
    { 0x70000000,  5,  99}, /* 1642 'c' */
    { 0x78000000,  5, 110}, /* 1643 'n' */
    { 0x28000000,  6, 121}, /* 1644 'y' */
    { 0x50000000,  6,  39}, /* 1645 '\'' */
    { 0x54000000,  6, 107}, /* 1646 'k' */
    { 0x5c000000,  6, 111}, /* 1647 'o' */
    { 0x20000000,  7, 114}, /* 1648 'r' */
    { 0x26000000,  7, 102}, /* 1649 'f' */
    { 0x2c000000,  7, 117}, /* 1650 'u' */
    { 0x2e000000,  7, 106}, /* 1651 'j' */
    { 0x22000000,  8, 118}, /* 1652 'v' */
    { 0x24000000,  8,  45}, /* 1653 '-' */
    { 0x25000000,  8,  46}, /* 1654 '.' */
    { 0x58000000,  8, 108}, /* 1655 'l' */
    { 0x5a000000,  8, 120}, /* 1656 'x' */
    { 0x5b000000,  8,  58}, /* 1657 ':' */
    { 0x23800000,  9,  44}, /* 1658 ',' */
    { 0x59800000,  9, 109}, /* 1659 'm' */
    { 0x23000000, 10,  33}, /* 1660 '!' */
    { 0x23400000, 11, 122}, /* 1661 'z' */
    { 0x59200000, 11,  63}, /* 1662 '?' */
    { 0x59400000, 11, 104}, /* 1663 'h' */
    { 0x59600000, 11,  98}, /* 1664 'b' */
    { 0x23600000, 12,  66}, /* 1665 'B' */
    { 0x23700000, 12,  42}, /* 1666 '*' */
    { 0x59000000, 12, 119}, /* 1667 'w' */
    { 0x59180000, 13, 113}, /* 1668 'q' */
    { 0x59140000, 14, 112}, /* 1669 'p' */
    { 0x59100000, 15,  59}, /* 1670 ';' */
    { 0x59120000, 16,  47}, /* 1671 '/' */
    { 0x59130000, 16,   1}, /* 1672 '0x01' */
    /*   42                             */
    { 0x00000000,  2, 114}, /* 1673 'r' */
    { 0xc0000000,  3, 110}, /* 1674 'n' */
    { 0x40000000,  4, 102}, /* 1675 'f' */
    { 0x50000000,  4,  32}, /* 1676 ' ' */
    { 0x60000000,  4, 119}, /* 1677 'w' */
    { 0x80000000,  4, 111}, /* 1678 'o' */
    { 0xb0000000,  4, 117}, /* 1679 'u' */
    { 0x78000000,  5, 116}, /* 1680 't' */
    { 0x90000000,  5,  99}, /* 1681 'c' */
    { 0xa0000000,  5, 112}, /* 1682 'p' */
    { 0xa8000000,  5, 100}, /* 1683 'd' */
    { 0xe0000000,  5, 109}, /* 1684 'm' */
    { 0xf0000000,  5, 108}, /* 1685 'l' */
    { 0x70000000,  6,  97}, /* 1686 'a' */
    { 0x74000000,  6,  98}, /* 1687 'b' */
    { 0x98000000,  6, 121}, /* 1688 'y' */
    { 0x9c000000,  6,   0}, /* 1689 '0x00' */
    { 0xe8000000,  6, 115}, /* 1690 's' */
    { 0xec000000,  6, 107}, /* 1691 'k' */
    { 0xfa000000,  7, 118}, /* 1692 'v' */
    { 0xfc000000,  7, 103}, /* 1693 'g' */
    { 0xff000000,  8, 105}, /* 1694 'i' */
    { 0xf8800000,  9, 104}, /* 1695 'h' */
    { 0xf9000000,  9,  33}, /* 1696 '!' */
    { 0xfe000000,  9, 101}, /* 1697 'e' */
    { 0xfe800000,  9, 106}, /* 1698 'j' */
    { 0xf9800000, 10,  39}, /* 1699 '\'' */
    { 0xf8200000, 11,  63}, /* 1700 '?' */
    { 0xf8400000, 11,  58}, /* 1701 ':' */
    { 0xf9c00000, 11, 122}, /* 1702 'z' */
    { 0xf9e00000, 11, 120}, /* 1703 'x' */
    { 0xf8000000, 12,  74}, /* 1704 'J' */
    { 0xf8600000, 12,  46}, /* 1705 '.' */
    { 0xf8700000, 12,  45}, /* 1706 '-' */
    { 0xf8100000, 14,  52}, /* 1707 '4' */
    { 0xf8180000, 14,  44}, /* 1708 ',' */
    { 0xf81c0000, 14,  71}, /* 1709 'G' */
    { 0xf8160000, 15,  41}, /* 1710 ')' */
    { 0xf8140000, 16,  83}, /* 1711 'S' */
    { 0xf8150000, 17,  68}, /* 1712 'D' */
    { 0xf8158000, 18,   1}, /* 1713 '0x01' */
    { 0xf815c000, 18, 113}, /* 1714 'q' */
    /*   28                             */
    { 0x00000000,  2, 101}, /* 1715 'e' */
    { 0x40000000,  3,   0}, /* 1716 '0x00' */
    { 0x80000000,  3, 105}, /* 1717 'i' */
    { 0xc0000000,  3, 111}, /* 1718 'o' */
    { 0x60000000,  4, 115}, /* 1719 's' */
    { 0xa0000000,  4,  32}, /* 1720 ' ' */
    { 0xe0000000,  4, 112}, /* 1721 'p' */
    { 0x70000000,  5, 108}, /* 1722 'l' */
    { 0x78000000,  5, 114}, /* 1723 'r' */
    { 0xb0000000,  5, 104}, /* 1724 'h' */
    { 0xf0000000,  5,  97}, /* 1725 'a' */
    { 0xb8000000,  6, 116}, /* 1726 't' */
    { 0xbc000000,  6,  39}, /* 1727 '\'' */
    { 0xf8000000,  7, 100}, /* 1728 'd' */
    { 0xfa000000,  7, 109}, /* 1729 'm' */
    { 0xfc000000,  7, 121}, /* 1730 'y' */
    { 0xfe800000,  9,  58}, /* 1731 ':' */
    { 0xff800000,  9,  33}, /* 1732 '!' */
    { 0xff000000, 10, 119}, /* 1733 'w' */
    { 0xff400000, 10, 117}, /* 1734 'u' */
    { 0xfe000000, 11,  98}, /* 1735 'b' */
    { 0xfe400000, 11,  45}, /* 1736 '-' */
    { 0xfe600000, 11,  46}, /* 1737 '.' */
    { 0xfe300000, 12, 110}, /* 1738 'n' */
    { 0xfe280000, 13, 107}, /* 1739 'k' */
    { 0xfe240000, 14,  44}, /* 1740 ',' */
    { 0xfe200000, 15,   1}, /* 1741 '0x01' */
    { 0xfe220000, 15,  99}, /* 1742 'c' */
    /*    5                             */
    { 0x80000000,  1, 117}, /* 1743 'u' */
    { 0x40000000,  2,   0}, /* 1744 '0x00' */
    { 0x20000000,  3,  58}, /* 1745 ':' */
    { 0x00000000,  4,   1}, /* 1746 '0x01' */
    { 0x10000000,  4,  39}, /* 1747 '\'' */
    /*   40                             */
    { 0x00000000,  3,  32}, /* 1748 ' ' */
    { 0x60000000,  3, 105}, /* 1749 'i' */
    { 0xa0000000,  3, 101}, /* 1750 'e' */
    { 0x30000000,  4, 121}, /* 1751 'y' */
    { 0x40000000,  4, 100}, /* 1752 'd' */
    { 0x80000000,  4, 115}, /* 1753 's' */
    { 0x90000000,  4, 116}, /* 1754 't' */
    { 0xc0000000,  4,  97}, /* 1755 'a' */
    { 0xd0000000,  4,   0}, /* 1756 '0x00' */
    { 0xf0000000,  4, 111}, /* 1757 'o' */
    { 0x58000000,  5, 110}, /* 1758 'n' */
    { 0xe8000000,  5, 108}, /* 1759 'l' */
    { 0x24000000,  6, 107}, /* 1760 'k' */
    { 0x28000000,  6, 114}, /* 1761 'r' */
    { 0x2c000000,  6, 109}, /* 1762 'm' */
    { 0x54000000,  6, 117}, /* 1763 'u' */
    { 0xe4000000,  6, 103}, /* 1764 'g' */
    { 0x20000000,  7,  39}, /* 1765 '\'' */
    { 0x52000000,  7,  99}, /* 1766 'c' */
    { 0xe2000000,  7,  58}, /* 1767 ':' */
    { 0x22000000,  8, 102}, /* 1768 'f' */
    { 0x23000000,  8,  46}, /* 1769 '.' */
    { 0x51000000,  8,  98}, /* 1770 'b' */
    { 0xe0000000,  8, 118}, /* 1771 'v' */
    { 0x50000000,  9,  44}, /* 1772 ',' */
    { 0x50800000,  9, 112}, /* 1773 'p' */
    { 0xe1000000,  9, 119}, /* 1774 'w' */
    { 0xe1c00000, 10, 106}, /* 1775 'j' */
    { 0xe1800000, 11,  45}, /* 1776 '-' */
    { 0xe1a00000, 12, 104}, /* 1777 'h' */
    { 0xe1b00000, 13,  71}, /* 1778 'G' */
    { 0xe1b80000, 14, 113}, /* 1779 'q' */
    { 0xe1be0000, 15,  83}, /* 1780 'S' */
    { 0xe1bc0000, 16,  33}, /* 1781 '!' */
    { 0xe1bd0000, 18,  42}, /* 1782 '*' */
    { 0xe1bd8000, 18,  84}, /* 1783 'T' */
    { 0xe1bd4000, 19,   1}, /* 1784 '0x01' */
    { 0xe1bd6000, 19,  69}, /* 1785 'E' */
    { 0xe1bdc000, 19,  49}, /* 1786 '1' */
    { 0xe1bde000, 19,  47}, /* 1787 '/' */
    /*   37                             */
    { 0x80000000,  2,  32}, /* 1788 ' ' */
    { 0xc0000000,  2,   0}, /* 1789 '0x00' */
    { 0x60000000,  3, 116}, /* 1790 't' */
    { 0x00000000,  4, 115}, /* 1791 's' */
    { 0x20000000,  4, 105}, /* 1792 'i' */
    { 0x30000000,  4, 104}, /* 1793 'h' */
    { 0x18000000,  5,  59}, /* 1794 ';' */
    { 0x50000000,  5, 101}, /* 1795 'e' */
    { 0x44000000,  6, 111}, /* 1796 'o' */
    { 0x4c000000,  6,  99}, /* 1797 'c' */
    { 0x58000000,  6,  58}, /* 1798 ':' */
    { 0x10000000,  7,  46}, /* 1799 '.' */
    { 0x12000000,  7,  33}, /* 1800 '!' */
    { 0x16000000,  7, 121}, /* 1801 'y' */
    { 0x40000000,  7, 112}, /* 1802 'p' */
    { 0x48000000,  7,  97}, /* 1803 'a' */
    { 0x5e000000,  7, 117}, /* 1804 'u' */
    { 0x14000000,  8,  44}, /* 1805 ',' */
    { 0x15000000,  8, 102}, /* 1806 'f' */
    { 0x43000000,  8,  39}, /* 1807 '\'' */
    { 0x4b000000,  8, 110}, /* 1808 'n' */
    { 0x5c000000,  8, 108}, /* 1809 'l' */
    { 0x5d000000,  8, 114}, /* 1810 'r' */
    { 0x42800000,  9, 107}, /* 1811 'k' */
    { 0x4a000000,  9, 100}, /* 1812 'd' */
    { 0x42400000, 10, 109}, /* 1813 'm' */
    { 0x4ac00000, 10,  98}, /* 1814 'b' */
    { 0x42000000, 11,  63}, /* 1815 '?' */
    { 0x42200000, 11, 119}, /* 1816 'w' */
    { 0x4a800000, 11, 103}, /* 1817 'g' */
    { 0x4aa00000, 12, 113}, /* 1818 'q' */
    { 0x4ab40000, 14,  69}, /* 1819 'E' */
    { 0x4ab80000, 14,  45}, /* 1820 '-' */
    { 0x4ab00000, 15,   1}, /* 1821 '0x01' */
    { 0x4ab20000, 15,  41}, /* 1822 ')' */
    { 0x4abc0000, 15,  87}, /* 1823 'W' */
    { 0x4abe0000, 15,  49}, /* 1824 '1' */
    /*   39                             */
    { 0x00000000,  3, 105}, /* 1825 'i' */
    { 0x60000000,  3,   0}, /* 1826 '0x00' */
    { 0x80000000,  3,  32}, /* 1827 ' ' */
    { 0xe0000000,  3, 104}, /* 1828 'h' */
    { 0x20000000,  4,  97}, /* 1829 'a' */
    { 0x40000000,  4, 114}, /* 1830 'r' */
    { 0xa0000000,  4, 115}, /* 1831 's' */
    { 0xb0000000,  4, 111}, /* 1832 'o' */
    { 0xd0000000,  4, 101}, /* 1833 'e' */
    { 0x38000000,  5, 116}, /* 1834 't' */
    { 0x50000000,  5, 121}, /* 1835 'y' */
    { 0xc0000000,  5, 117}, /* 1836 'u' */
    { 0x58000000,  6, 109}, /* 1837 'm' */
    { 0xc8000000,  6,  99}, /* 1838 'c' */
    { 0xcc000000,  6, 108}, /* 1839 'l' */
    { 0x30000000,  7,  39}, /* 1840 '\'' */
    { 0x34000000,  7,  58}, /* 1841 ':' */
    { 0x32000000,  8, 119}, /* 1842 'w' */
    { 0x36000000,  8,  33}, /* 1843 '!' */
    { 0x5c000000,  8,  46}, /* 1844 '.' */
    { 0x5d000000,  8,  98}, /* 1845 'b' */
    { 0x5e000000,  8,  69}, /* 1846 'E' */
    { 0x5f000000,  8, 102}, /* 1847 'f' */
    { 0x33000000,  9,  63}, /* 1848 '?' */
    { 0x37000000,  9, 110}, /* 1849 'n' */
    { 0x33800000, 10, 122}, /* 1850 'z' */
    { 0x37800000, 10, 100}, /* 1851 'd' */
    { 0x33e00000, 11,  44}, /* 1852 ',' */
    { 0x37c00000, 11,  80}, /* 1853 'P' */
    { 0x33c00000, 12, 118}, /* 1854 'v' */
    { 0x33d00000, 12,  45}, /* 1855 '-' */
    { 0x37e00000, 12,  41}, /* 1856 ')' */
    { 0x37f00000, 13, 103}, /* 1857 'g' */
    { 0x37f80000, 14,   1}, /* 1858 '0x01' */
    { 0x37fc0000, 15,  83}, /* 1859 'S' */
    { 0x37ff0000, 16,  52}, /* 1860 '4' */
    { 0x37fe0000, 17, 107}, /* 1861 'k' */
    { 0x37fe8000, 18, 106}, /* 1862 'j' */
    { 0x37fec000, 18, 112}, /* 1863 'p' */
    /*   34                             */
    { 0x00000000,  2, 114}, /* 1864 'r' */
    { 0x80000000,  3, 115}, /* 1865 's' */
    { 0xe0000000,  3, 110}, /* 1866 'n' */
    { 0x40000000,  4, 101}, /* 1867 'e' */
    { 0x50000000,  4, 109}, /* 1868 'm' */
    { 0xc0000000,  4, 116}, /* 1869 't' */
    { 0x60000000,  5,  99}, /* 1870 'c' */
    { 0x68000000,  5, 103}, /* 1871 'g' */
    { 0x70000000,  5,  98}, /* 1872 'b' */
    { 0xa0000000,  5, 112}, /* 1873 'p' */
    { 0xa8000000,  5, 105}, /* 1874 'i' */
    { 0xb0000000,  5, 108}, /* 1875 'l' */
    { 0xd0000000,  5, 100}, /* 1876 'd' */
    { 0xd8000000,  5,  97}, /* 1877 'a' */
    { 0x78000000,  6,   0}, /* 1878 '0x00' */
    { 0xb8000000,  6, 121}, /* 1879 'y' */
    { 0x7c000000,  7, 122}, /* 1880 'z' */
    { 0xbc000000,  7,  32}, /* 1881 ' ' */
    { 0x7f000000,  8,  39}, /* 1882 '\'' */
    { 0xbe000000,  8,  45}, /* 1883 '-' */
    { 0x7e000000,  9, 107}, /* 1884 'k' */
    { 0xbf800000,  9,  58}, /* 1885 ':' */
    { 0x7e800000, 10, 102}, /* 1886 'f' */
    { 0x7ec00000, 10,  44}, /* 1887 ',' */
    { 0xbf000000, 10, 119}, /* 1888 'w' */
    { 0xbf400000, 12, 118}, /* 1889 'v' */
    { 0xbf500000, 12, 120}, /* 1890 'x' */
    { 0xbf700000, 12, 111}, /* 1891 'o' */
    { 0xbf600000, 13, 106}, /* 1892 'j' */
    { 0xbf680000, 14, 117}, /* 1893 'u' */
    { 0xbf6c0000, 15,  46}, /* 1894 '.' */
    { 0xbf6f0000, 16, 104}, /* 1895 'h' */
    { 0xbf6e0000, 17,  63}, /* 1896 '?' */
    { 0xbf6e8000, 17,   1}, /* 1897 '0x01' */
    /*   11                             */
    { 0x80000000,  1, 101}, /* 1898 'e' */
    { 0x40000000,  2, 105}, /* 1899 'i' */
    { 0x20000000,  3,  97}, /* 1900 'a' */
    { 0x10000000,  4, 111}, /* 1901 'o' */
    { 0x00000000,  5,  32}, /* 1902 ' ' */
    { 0x0c000000,  6,   0}, /* 1903 '0x00' */
    { 0x08000000,  7, 121}, /* 1904 'y' */
    { 0x0b000000,  8, 115}, /* 1905 's' */
    { 0x0a800000,  9, 114}, /* 1906 'r' */
    { 0x0a000000, 10,   1}, /* 1907 '0x01' */
    { 0x0a400000, 10,  46}, /* 1908 '.' */
    /*   31                             */
    { 0x00000000,  1, 115}, /* 1909 's' */
    { 0x80000000,  3,  32}, /* 1910 ' ' */
    { 0xc0000000,  3,   0}, /* 1911 '0x00' */
    { 0xb0000000,  4, 105}, /* 1912 'i' */
    { 0xe0000000,  4, 111}, /* 1913 'o' */
    { 0xa0000000,  5,  97}, /* 1914 'a' */
    { 0xf0000000,  5, 110}, /* 1915 'n' */
    { 0xf8000000,  5, 101}, /* 1916 'e' */
    { 0xae000000,  7, 121}, /* 1917 'y' */
    { 0xa8000000,  8, 109}, /* 1918 'm' */
    { 0xab000000,  8, 100}, /* 1919 'd' */
    { 0xad000000,  8, 108}, /* 1920 'l' */
    { 0xa9800000,  9,  98}, /* 1921 'b' */
    { 0xaa000000,  9, 107}, /* 1922 'k' */
    { 0xaa800000,  9, 114}, /* 1923 'r' */
    { 0xa9000000, 10, 106}, /* 1924 'j' */
    { 0xac400000, 10,  44}, /* 1925 ',' */
    { 0xacc00000, 10, 104}, /* 1926 'h' */
    { 0xa9600000, 11,  45}, /* 1927 '-' */
    { 0xac000000, 11,  99}, /* 1928 'c' */
    { 0xac200000, 11, 102}, /* 1929 'f' */
    { 0xaca00000, 11, 112}, /* 1930 'p' */
    { 0xa9400000, 12, 103}, /* 1931 'g' */
    { 0xac800000, 12, 116}, /* 1932 't' */
    { 0xa9500000, 13,  46}, /* 1933 '.' */
    { 0xa9580000, 13,  58}, /* 1934 ':' */
    { 0xac980000, 13, 113}, /* 1935 'q' */
    { 0xac940000, 14,  39}, /* 1936 '\'' */
    { 0xac920000, 15,  63}, /* 1937 '?' */
    { 0xac900000, 16,   1}, /* 1938 '0x01' */
    { 0xac910000, 16,  66}, /* 1939 'B' */
    /*   20                             */
    { 0x00000000,  2, 112}, /* 1940 'p' */
    { 0x80000000,  2,  32}, /* 1941 ' ' */
    { 0xc0000000,  2, 116}, /* 1942 't' */
    { 0x60000000,  4,   0}, /* 1943 '0x00' */
    { 0x40000000,  5, 111}, /* 1944 'o' */
    { 0x50000000,  5,  99}, /* 1945 'c' */
    { 0x70000000,  5, 105}, /* 1946 'i' */
    { 0x78000000,  5, 109}, /* 1947 'm' */
    { 0x48000000,  6, 101}, /* 1948 'e' */
    { 0x58000000,  6, 121}, /* 1949 'y' */
    { 0x4c000000,  7, 117}, /* 1950 'u' */
    { 0x4e000000,  7, 102}, /* 1951 'f' */
    { 0x5e000000,  7,  44}, /* 1952 ',' */
    { 0x5c000000,  9, 103}, /* 1953 'g' */
    { 0x5c800000,  9,  97}, /* 1954 'a' */
    { 0x5d800000,  9,  57}, /* 1955 '9' */
    { 0x5d400000, 10,  39}, /* 1956 '\'' */
    { 0x5d200000, 11, 120}, /* 1957 'x' */
    { 0x5d000000, 12,   1}, /* 1958 '0x01' */
    { 0x5d100000, 12, 115}, /* 1959 's' */
    /*   36                             */
    { 0x00000000,  1,  32}, /* 1960 ' ' */
    { 0xc0000000,  2,   0}, /* 1961 '0x00' */
    { 0x88000000,  5, 111}, /* 1962 'o' */
    { 0x90000000,  5, 115}, /* 1963 's' */
    { 0xa0000000,  5,  97}, /* 1964 'a' */
    { 0xb0000000,  5, 108}, /* 1965 'l' */
    { 0xb8000000,  5,  58}, /* 1966 ':' */
    { 0x98000000,  6, 100}, /* 1967 'd' */
    { 0x82000000,  7, 110}, /* 1968 'n' */
    { 0x84000000,  7, 116}, /* 1969 't' */
    { 0xa8000000,  7,  39}, /* 1970 '\'' */
    { 0xaa000000,  7,  98}, /* 1971 'b' */
    { 0xae000000,  7,  46}, /* 1972 '.' */
    { 0x80000000,  8, 105}, /* 1973 'i' */
    { 0x86000000,  8,  44}, /* 1974 ',' */
    { 0x87000000,  8, 112}, /* 1975 'p' */
    { 0x9c000000,  8, 109}, /* 1976 'm' */
    { 0x9e000000,  8,  99}, /* 1977 'c' */
    { 0xac000000,  8, 119}, /* 1978 'w' */
    { 0xad000000,  8, 101}, /* 1979 'e' */
    { 0x81000000,  9,  63}, /* 1980 '?' */
    { 0x81800000,  9, 102}, /* 1981 'f' */
    { 0x9d000000,  9, 114}, /* 1982 'r' */
    { 0x9d800000,  9, 103}, /* 1983 'g' */
    { 0x9f000000, 10, 122}, /* 1984 'z' */
    { 0x9f800000, 10,  45}, /* 1985 '-' */
    { 0x9fc00000, 10,  84}, /* 1986 'T' */
    { 0x9f400000, 12,  50}, /* 1987 '2' */
    { 0x9f600000, 12,  33}, /* 1988 '!' */
    { 0x9f700000, 12, 107}, /* 1989 'k' */
    { 0x9f500000, 13, 118}, /* 1990 'v' */
    { 0x9f580000, 14, 121}, /* 1991 'y' */
    { 0x9f5c0000, 15, 104}, /* 1992 'h' */
    { 0x9f5f0000, 16, 106}, /* 1993 'j' */
    { 0x9f5e0000, 17,   1}, /* 1994 '0x01' */
    { 0x9f5e8000, 17,  41}, /* 1995 ')' */
    /*   21                             */
    { 0x00000000,  2, 122}, /* 1996 'z' */
    { 0x40000000,  2,   0}, /* 1997 '0x00' */
    { 0xa0000000,  3, 105}, /* 1998 'i' */
    { 0x80000000,  4, 121}, /* 1999 'y' */
    { 0x90000000,  4, 101}, /* 2000 'e' */
    { 0xc0000000,  4, 119}, /* 2001 'w' */
    { 0xd0000000,  4,  32}, /* 2002 ' ' */
    { 0xe0000000,  4, 108}, /* 2003 'l' */
    { 0xf0000000,  5,  97}, /* 2004 'a' */
    { 0xf8000000,  6, 111}, /* 2005 'o' */
    { 0xfc000000,  8, 109}, /* 2006 'm' */
    { 0xfd000000,  8,  58}, /* 2007 ':' */
    { 0xff000000,  8,  99}, /* 2008 'c' */
    { 0xfe000000,  9,  44}, /* 2009 ',' */
    { 0xfec00000, 10,  98}, /* 2010 'b' */
    { 0xfe800000, 11, 117}, /* 2011 'u' */
    { 0xfeb00000, 12,  33}, /* 2012 '!' */
    { 0xfea00000, 14,   1}, /* 2013 '0x01' */
    { 0xfea40000, 14, 116}, /* 2014 't' */
    { 0xfea80000, 14, 104}, /* 2015 'h' */
    { 0xfeac0000, 14,  63}, /* 2016 '?' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2017 '0x01' */
    { 0x80000000,  1,   1}, /* 2018 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2019 '0x01' */
    { 0x80000000,  1,   1}, /* 2020 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2021 '0x01' */
    { 0x80000000,  1,   1}, /* 2022 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2023 '0x01' */
    { 0x80000000,  1,   1}, /* 2024 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2025 '0x01' */
    { 0x80000000,  1,   1}  /* 2026 '0x01' */
};

unsigned huffIndex1[] = {
           0, /*   0 */
          51, /*   1 */
          53, /*   2 */
          53, /*   3 */
          55, /*   4 */
          57, /*   5 */
          59, /*   6 */
          61, /*   7 */
          63, /*   8 */
          65, /*   9 */
          67, /*  10 */
          69, /*  11 */
          71, /*  12 */
          73, /*  13 */
          75, /*  14 */
          77, /*  15 */
          79, /*  16 */
          81, /*  17 */
          83, /*  18 */
          85, /*  19 */
          87, /*  20 */
          89, /*  21 */
          91, /*  22 */
          93, /*  23 */
          95, /*  24 */
          97, /*  25 */
          99, /*  26 */
         101, /*  27 */
         103, /*  28 */
         105, /*  29 */
         107, /*  30 */
         109, /*  31 */
         111, /*  32 */
         181, /*  33 */
         188, /*  34 */
         191, /*  35 */
         193, /*  36 */
         195, /*  37 */
         198, /*  38 */
         202, /*  39 */
         234, /*  40 */
         246, /*  41 */
         249, /*  42 */
         260, /*  43 */
         262, /*  44 */
         266, /*  45 */
         303, /*  46 */
         328, /*  47 */
         346, /*  48 */
         369, /*  49 */
         391, /*  50 */
         409, /*  51 */
         428, /*  52 */
         447, /*  53 */
         458, /*  54 */
         474, /*  55 */
         485, /*  56 */
         500, /*  57 */
         514, /*  58 */
         523, /*  59 */
         526, /*  60 */
         528, /*  61 */
         530, /*  62 */
         532, /*  63 */
         537, /*  64 */
         539, /*  65 */
         583, /*  66 */
         609, /*  67 */
         640, /*  68 */
         668, /*  69 */
         713, /*  70 */
         739, /*  71 */
         764, /*  72 */
         783, /*  73 */
         824, /*  74 */
         840, /*  75 */
         866, /*  76 */
         885, /*  77 */
         914, /*  78 */
         936, /*  79 */
         980, /*  80 */
        1008, /*  81 */
        1016, /*  82 */
        1039, /*  83 */
        1078, /*  84 */
        1111, /*  85 */
        1134, /*  86 */
        1155, /*  87 */
        1173, /*  88 */
        1187, /*  89 */
        1206, /*  90 */
        1217, /*  91 */
        1219, /*  92 */
        1221, /*  93 */
        1223, /*  94 */
        1225, /*  95 */
        1227, /*  96 */
        1229, /*  97 */
        1268, /*  98 */
        1292, /*  99 */
        1329, /* 100 */
        1365, /* 101 */
        1407, /* 102 */
        1431, /* 103 */
        1464, /* 104 */
        1495, /* 105 */
        1530, /* 106 */
        1539, /* 107 */
        1572, /* 108 */
        1608, /* 109 */
        1633, /* 110 */
        1673, /* 111 */
        1715, /* 112 */
        1743, /* 113 */
        1748, /* 114 */
        1788, /* 115 */
        1825, /* 116 */
        1864, /* 117 */
        1898, /* 118 */
        1909, /* 119 */
        1940, /* 120 */
        1960, /* 121 */
        1996, /* 122 */
        2017, /* 123 */
        2019, /* 124 */
        2021, /* 125 */
        2023, /* 126 */
        2025, /* 127 */
        2027  /* 128 */
};
//}}}
//{{{
struct tHuffTable huffTable2[] = {
    /*   51                             */
    { 0x40000000,  3,  65}, /*    0 'A' */
    { 0x80000000,  3,  67}, /*    1 'C' */
    { 0xe0000000,  3,  84}, /*    2 'T' */
    { 0x10000000,  4,  74}, /*    3 'J' */
    { 0x30000000,  4,  68}, /*    4 'D' */
    { 0xa0000000,  4,  83}, /*    5 'S' */
    { 0x00000000,  5,  72}, /*    6 'H' */
    { 0x20000000,  5,  73}, /*    7 'I' */
    { 0x28000000,  5,  82}, /*    8 'R' */
    { 0x68000000,  5,  70}, /*    9 'F' */
    { 0x70000000,  5,  46}, /*   10 '.' */
    { 0x78000000,  5,  87}, /*   11 'W' */
    { 0xb8000000,  5,  77}, /*   12 'M' */
    { 0xc0000000,  5,  66}, /*   13 'B' */
    { 0xc8000000,  5,  80}, /*   14 'P' */
    { 0xd8000000,  5,  78}, /*   15 'N' */
    { 0x08000000,  6,  79}, /*   16 'O' */
    { 0x64000000,  6,  91}, /*   17 '[' */
    { 0xb4000000,  6,  76}, /*   18 'L' */
    { 0xd4000000,  6,  69}, /*   19 'E' */
    { 0x0c000000,  7,  75}, /*   20 'K' */
    { 0xd0000000,  7,  89}, /*   21 'Y' */
    { 0xd2000000,  7,  71}, /*   22 'G' */
    { 0x0e000000,  8,  50}, /*   23 '2' */
    { 0x60000000,  8, 112}, /*   24 'p' */
    { 0x61000000,  8,  98}, /*   25 'b' */
    { 0x62000000,  8,  85}, /*   26 'U' */
    { 0x63000000,  8,  40}, /*   27 '(' */
    { 0xb0000000,  8,  49}, /*   28 '1' */
    { 0xb3000000,  8,  86}, /*   29 'V' */
    { 0x0f000000,  9,  81}, /*   30 'Q' */
    { 0xb1000000,  9,  51}, /*   31 '3' */
    { 0x0f800000, 10,  57}, /*   32 '9' */
    { 0x0fc00000, 10,  56}, /*   33 '8' */
    { 0xb1800000, 10,  54}, /*   34 '6' */
    { 0xb1c00000, 10,  53}, /*   35 '5' */
    { 0xb2000000, 10,  90}, /*   36 'Z' */
    { 0xb2400000, 10,  55}, /*   37 '7' */
    { 0xb2800000, 10,  52}, /*   38 '4' */
    { 0xb2e00000, 12,  88}, /*   39 'X' */
    { 0xb2f00000, 12,  32}, /*   40 ' ' */
    { 0xb2c80000, 13, 119}, /*   41 'w' */
    { 0xb2d00000, 13,  39}, /*   42 '\'' */
    { 0xb2d80000, 13,  34}, /*   43 '\"' */
    { 0xb2c00000, 14, 116}, /*   44 't' */
    { 0xb2c40000, 15,  97}, /*   45 'a' */
    { 0xb2c60000, 16,  96}, /*   46 '`' */
    { 0xb2c70000, 17,   1}, /*   47 '0x01' */
    { 0xb2c78000, 17, 109}, /*   48 'm' */
    { 0x00000000,  1,   1}, /*   49 '0x01' */
    { 0x80000000,  1,   1}, /*   50 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   51 '0x01' */
    { 0x80000000,  1,   1}, /*   52 '0x01' */
    /*    0                             */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   53 '0x01' */
    { 0x80000000,  1,   1}, /*   54 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   55 '0x01' */
    { 0x80000000,  1,   1}, /*   56 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   57 '0x01' */
    { 0x80000000,  1,   1}, /*   58 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   59 '0x01' */
    { 0x80000000,  1,   1}, /*   60 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   61 '0x01' */
    { 0x80000000,  1,   1}, /*   62 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   63 '0x01' */
    { 0x80000000,  1,   1}, /*   64 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   65 '0x01' */
    { 0x80000000,  1,   1}, /*   66 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   67 '0x01' */
    { 0x80000000,  1,   1}, /*   68 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   69 '0x01' */
    { 0x80000000,  1,   1}, /*   70 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   71 '0x01' */
    { 0x80000000,  1,   1}, /*   72 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   73 '0x01' */
    { 0x80000000,  1,   1}, /*   74 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   75 '0x01' */
    { 0x80000000,  1,   1}, /*   76 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   77 '0x01' */
    { 0x80000000,  1,   1}, /*   78 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   79 '0x01' */
    { 0x80000000,  1,   1}, /*   80 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   81 '0x01' */
    { 0x80000000,  1,   1}, /*   82 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   83 '0x01' */
    { 0x80000000,  1,   1}, /*   84 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   85 '0x01' */
    { 0x80000000,  1,   1}, /*   86 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   87 '0x01' */
    { 0x80000000,  1,   1}, /*   88 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   89 '0x01' */
    { 0x80000000,  1,   1}, /*   90 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   91 '0x01' */
    { 0x80000000,  1,   1}, /*   92 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   93 '0x01' */
    { 0x80000000,  1,   1}, /*   94 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   95 '0x01' */
    { 0x80000000,  1,   1}, /*   96 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   97 '0x01' */
    { 0x80000000,  1,   1}, /*   98 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*   99 '0x01' */
    { 0x80000000,  1,   1}, /*  100 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  101 '0x01' */
    { 0x80000000,  1,   1}, /*  102 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  103 '0x01' */
    { 0x80000000,  1,   1}, /*  104 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  105 '0x01' */
    { 0x80000000,  1,   1}, /*  106 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  107 '0x01' */
    { 0x80000000,  1,   1}, /*  108 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  109 '0x01' */
    { 0x80000000,  1,   1}, /*  110 '0x01' */
    /*   80                             */
    { 0x40000000,  3,  97}, /*  111 'a' */
    { 0x80000000,  3, 116}, /*  112 't' */
    { 0x10000000,  4, 111}, /*  113 'o' */
    { 0x20000000,  4, 115}, /*  114 's' */
    { 0x30000000,  5, 100}, /*  115 'd' */
    { 0x60000000,  5,  91}, /*  116 '[' */
    { 0x78000000,  5, 112}, /*  117 'p' */
    { 0xa8000000,  5,  98}, /*  118 'b' */
    { 0xc8000000,  5,  99}, /*  119 'c' */
    { 0xd0000000,  5, 104}, /*  120 'h' */
    { 0xe0000000,  5, 119}, /*  121 'w' */
    { 0xe8000000,  5, 105}, /*  122 'i' */
    { 0xf8000000,  5, 102}, /*  123 'f' */
    { 0x00000000,  6,  65}, /*  124 'A' */
    { 0x0c000000,  6,  77}, /*  125 'M' */
    { 0x3c000000,  6, 101}, /*  126 'e' */
    { 0x70000000,  6,  66}, /*  127 'B' */
    { 0x74000000,  6,  67}, /*  128 'C' */
    { 0xa0000000,  6,  84}, /*  129 'T' */
    { 0xb4000000,  6,  83}, /*  130 'S' */
    { 0xb8000000,  6, 103}, /*  131 'g' */
    { 0xc0000000,  6, 114}, /*  132 'r' */
    { 0xd8000000,  6, 110}, /*  133 'n' */
    { 0xdc000000,  6, 108}, /*  134 'l' */
    { 0xf4000000,  6, 109}, /*  135 'm' */
    { 0x06000000,  7, 118}, /*  136 'v' */
    { 0x08000000,  7,  71}, /*  137 'G' */
    { 0x0a000000,  7,  78}, /*  138 'N' */
    { 0x3a000000,  7, 121}, /*  139 'y' */
    { 0x6a000000,  7,  72}, /*  140 'H' */
    { 0x6e000000,  7,  76}, /*  141 'L' */
    { 0xa4000000,  7,  74}, /*  142 'J' */
    { 0xa6000000,  7,  70}, /*  143 'F' */
    { 0xb2000000,  7,  82}, /*  144 'R' */
    { 0xbc000000,  7, 117}, /*  145 'u' */
    { 0xc4000000,  7,  68}, /*  146 'D' */
    { 0xc6000000,  7,  87}, /*  147 'W' */
    { 0xf2000000,  7,  80}, /*  148 'P' */
    { 0x04000000,  8, 107}, /*  149 'k' */
    { 0x05000000,  8,  79}, /*  150 'O' */
    { 0x68000000,  8,  45}, /*  151 '-' */
    { 0x6d000000,  8,  49}, /*  152 '1' */
    { 0xb1000000,  8,  75}, /*  153 'K' */
    { 0xbe000000,  8, 106}, /*  154 'j' */
    { 0xf0000000,  8,  73}, /*  155 'I' */
    { 0xf1000000,  8,  69}, /*  156 'E' */
    { 0x39000000,  9, 113}, /*  157 'q' */
    { 0x39800000,  9,  85}, /*  158 'U' */
    { 0x69000000,  9,  86}, /*  159 'V' */
    { 0x6c000000,  9,  89}, /*  160 'Y' */
    { 0x6c800000,  9,  32}, /*  161 ' ' */
    { 0xb0800000,  9,  50}, /*  162 '2' */
    { 0xbf000000,  9,   0}, /*  163 '0x00' */
    { 0x38000000, 10,  51}, /*  164 '3' */
    { 0x38400000, 10,  56}, /*  165 '8' */
    { 0x38800000, 10,  54}, /*  166 '6' */
    { 0x69c00000, 10,  53}, /*  167 '5' */
    { 0xb0000000, 10,  40}, /*  168 '(' */
    { 0xbf800000, 10,  55}, /*  169 '7' */
    { 0x38c00000, 11,  48}, /*  170 '0' */
    { 0x69800000, 11,  39}, /*  171 '\'' */
    { 0x69a00000, 11,  57}, /*  172 '9' */
    { 0xb0400000, 11,  90}, /*  173 'Z' */
    { 0xbfc00000, 11,  52}, /*  174 '4' */
    { 0xbfe00000, 11,  81}, /*  175 'Q' */
    { 0x38f00000, 12,  88}, /*  176 'X' */
    { 0xb0600000, 13,   1}, /*  177 '0x01' */
    { 0xb0680000, 13,  46}, /*  178 '.' */
    { 0xb0700000, 13,  38}, /*  179 '&' */
    { 0x38e00000, 14,  92}, /*  180 '\\' */
    { 0x38e80000, 14,  64}, /*  181 '@' */
    { 0x38ec0000, 14,  96}, /*  182 '`' */
    { 0xb0780000, 14,  34}, /*  183 '\"' */
    { 0xb07c0000, 14, 122}, /*  184 'z' */
    { 0x38e60000, 15,  36}, /*  185 '$' */
    { 0x38e40000, 16,  43}, /*  186 '+' */
    { 0x38e58000, 17, 120}, /*  187 'x' */
    { 0x38e54000, 18,  93}, /*  188 ']' */
    { 0x38e50000, 19,  47}, /*  189 '/' */
    { 0x38e52000, 19,  63}, /*  190 '?' */
    /*   13                             */
    { 0x80000000,  1,  32}, /*  191 ' ' */
    { 0x40000000,  2,   0}, /*  192 '0x00' */
    { 0x20000000,  3,  46}, /*  193 '.' */
    { 0x00000000,  4,  58}, /*  194 ':' */
    { 0x18000000,  5,  91}, /*  195 '[' */
    { 0x12000000,  7,  34}, /*  196 '\"' */
    { 0x14000000,  7,  47}, /*  197 '/' */
    { 0x16000000,  7,  33}, /*  198 '!' */
    { 0x10000000,  8,  41}, /*  199 ')' */
    { 0x11000000,  9,  39}, /*  200 '\'' */
    { 0x11c00000, 10,  63}, /*  201 '?' */
    { 0x11800000, 11,   1}, /*  202 '0x01' */
    { 0x11a00000, 11,  93}, /*  203 ']' */
    /*   36                             */
    { 0xc0000000,  2,  32}, /*  204 ' ' */
    { 0x20000000,  3,  46}, /*  205 '.' */
    { 0x00000000,  4, 112}, /*  206 'p' */
    { 0x50000000,  4,  66}, /*  207 'B' */
    { 0x70000000,  4,  84}, /*  208 'T' */
    { 0x80000000,  4, 105}, /*  209 'i' */
    { 0x10000000,  5, 102}, /*  210 'f' */
    { 0x18000000,  5,  87}, /*  211 'W' */
    { 0x40000000,  5,  83}, /*  212 'S' */
    { 0x60000000,  5, 116}, /*  213 't' */
    { 0x68000000,  5,  67}, /*  214 'C' */
    { 0xa0000000,  5,   0}, /*  215 '0x00' */
    { 0xb8000000,  5,  44}, /*  216 ',' */
    { 0x48000000,  6,  74}, /*  217 'J' */
    { 0x90000000,  6, 109}, /*  218 'm' */
    { 0xa8000000,  6, 110}, /*  219 'n' */
    { 0xac000000,  6,  73}, /*  220 'I' */
    { 0x4c000000,  7,  69}, /*  221 'E' */
    { 0x4e000000,  7,  68}, /*  222 'D' */
    { 0x94000000,  7, 119}, /*  223 'w' */
    { 0x96000000,  7, 103}, /*  224 'g' */
    { 0x98000000,  7,  98}, /*  225 'b' */
    { 0x9a000000,  7,  76}, /*  226 'L' */
    { 0x9c000000,  7,  45}, /*  227 '-' */
    { 0xb0000000,  7,  99}, /*  228 'c' */
    { 0xb2000000,  7,  72}, /*  229 'H' */
    { 0x9e000000,  8,  80}, /*  230 'P' */
    { 0xb4000000,  8, 114}, /*  231 'r' */
    { 0xb7000000,  8,  75}, /*  232 'K' */
    { 0x9f800000,  9, 108}, /*  233 'l' */
    { 0xb5000000,  9,  89}, /*  234 'Y' */
    { 0xb5800000,  9,  81}, /*  235 'Q' */
    { 0xb6000000,  9,  71}, /*  236 'G' */
    { 0xb6800000,  9,  65}, /*  237 'A' */
    { 0x9f000000, 10,   1}, /*  238 '0x01' */
    { 0x9f400000, 10,  97}, /*  239 'a' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  240 '0x01' */
    { 0x80000000,  1,   1}, /*  241 '0x01' */
    /*    8                             */
    { 0x00000000,  1,  49}, /*  242 '1' */
    { 0xc0000000,  2,  51}, /*  243 '3' */
    { 0x80000000,  3,  52}, /*  244 '4' */
    { 0xb0000000,  4,  50}, /*  245 '2' */
    { 0xa8000000,  5,  55}, /*  246 '7' */
    { 0xa4000000,  6,  53}, /*  247 '5' */
    { 0xa0000000,  7,   1}, /*  248 '0x01' */
    { 0xa2000000,  7,  57}, /*  249 '9' */
    /*    3                             */
    { 0x80000000,  1,  32}, /*  250 ' ' */
    { 0x00000000,  2,   1}, /*  251 '0x01' */
    { 0x40000000,  2,  44}, /*  252 ',' */
    /*   12                             */
    { 0x80000000,  1,  32}, /*  253 ' ' */
    { 0x40000000,  2, 119}, /*  254 'w' */
    { 0x20000000,  3,  66}, /*  255 'B' */
    { 0x00000000,  4,  69}, /*  256 'E' */
    { 0x10000000,  6,  50}, /*  257 '2' */
    { 0x18000000,  6,  65}, /*  258 'A' */
    { 0x1c000000,  6,  82}, /*  259 'R' */
    { 0x14000000,  8,  79}, /*  260 'O' */
    { 0x15000000,  8,  52}, /*  261 '4' */
    { 0x17000000,  8,  74}, /*  262 'J' */
    { 0x16000000,  9,   1}, /*  263 '0x01' */
    { 0x16800000,  9,  80}, /*  264 'P' */
    /*   71                             */
    { 0x80000000,  1, 115}, /*  265 's' */
    { 0x20000000,  3, 116}, /*  266 't' */
    { 0x40000000,  3,  32}, /*  267 ' ' */
    { 0x00000000,  4, 108}, /*  268 'l' */
    { 0x68000000,  5, 114}, /*  269 'r' */
    { 0x10000000,  6, 110}, /*  270 'n' */
    { 0x14000000,  6,  46}, /*  271 '.' */
    { 0x18000000,  6,  67}, /*  272 'C' */
    { 0x60000000,  6,  66}, /*  273 'B' */
    { 0x74000000,  6,  65}, /*  274 'A' */
    { 0x70000000,  7, 100}, /*  275 'd' */
    { 0x78000000,  7, 118}, /*  276 'v' */
    { 0x1c000000,  8,  83}, /*  277 'S' */
    { 0x1f000000,  8, 112}, /*  278 'p' */
    { 0x65000000,  8,  68}, /*  279 'D' */
    { 0x7b000000,  8, 105}, /*  280 'i' */
    { 0x7c000000,  8,  99}, /*  281 'c' */
    { 0x7d000000,  8, 109}, /*  282 'm' */
    { 0x7f000000,  8,  44}, /*  283 ',' */
    { 0x1d000000,  9, 102}, /*  284 'f' */
    { 0x1d800000,  9, 103}, /*  285 'g' */
    { 0x64000000,  9,  70}, /*  286 'F' */
    { 0x64800000,  9, 104}, /*  287 'h' */
    { 0x66800000,  9,  72}, /*  288 'H' */
    { 0x67000000,  9,  78}, /*  289 'N' */
    { 0x72800000,  9,  82}, /*  290 'R' */
    { 0x73000000,  9,   0}, /*  291 '0x00' */
    { 0x73800000,  9,  84}, /*  292 'T' */
    { 0x7a800000,  9,  71}, /*  293 'G' */
    { 0x7e800000,  9,  76}, /*  294 'L' */
    { 0x1e000000, 10, 111}, /*  295 'o' */
    { 0x1e400000, 10,  75}, /*  296 'K' */
    { 0x1ec00000, 10,  97}, /*  297 'a' */
    { 0x66400000, 10, 117}, /*  298 'u' */
    { 0x67c00000, 10,  79}, /*  299 'O' */
    { 0x72400000, 10,  73}, /*  300 'I' */
    { 0x7a000000, 10, 119}, /*  301 'w' */
    { 0x7a400000, 10,  98}, /*  302 'b' */
    { 0x7e400000, 10, 101}, /*  303 'e' */
    { 0x1ea00000, 11,  63}, /*  304 '?' */
    { 0x66000000, 11,  69}, /*  305 'E' */
    { 0x66200000, 11,  55}, /*  306 '7' */
    { 0x72000000, 11,  80}, /*  307 'P' */
    { 0x1e900000, 12,  87}, /*  308 'W' */
    { 0x67900000, 12,  58}, /*  309 ':' */
    { 0x67a00000, 12,  33}, /*  310 '!' */
    { 0x72300000, 12,  74}, /*  311 'J' */
    { 0x7e100000, 12, 113}, /*  312 'q' */
    { 0x7e300000, 12,  77}, /*  313 'M' */
    { 0x1e880000, 13,  86}, /*  314 'V' */
    { 0x67880000, 13,  57}, /*  315 '9' */
    { 0x67b80000, 13, 121}, /*  316 'y' */
    { 0x72200000, 13,  56}, /*  317 '8' */
    { 0x72280000, 13,  53}, /*  318 '5' */
    { 0x7e000000, 13,  54}, /*  319 '6' */
    { 0x7e280000, 13, 107}, /*  320 'k' */
    { 0x1e800000, 14,  50}, /*  321 '2' */
    { 0x1e840000, 14,  48}, /*  322 '0' */
    { 0x67840000, 14,  89}, /*  323 'Y' */
    { 0x67b00000, 14,  41}, /*  324 ')' */
    { 0x7e080000, 14, 106}, /*  325 'j' */
    { 0x7e0c0000, 14,  81}, /*  326 'Q' */
    { 0x67800000, 15,  45}, /*  327 '-' */
    { 0x67820000, 15,  39}, /*  328 '\'' */
    { 0x67b60000, 15, 122}, /*  329 'z' */
    { 0x7e200000, 15,  88}, /*  330 'X' */
    { 0x7e220000, 15,  85}, /*  331 'U' */
    { 0x7e240000, 15,  52}, /*  332 '4' */
    { 0x7e260000, 15,  51}, /*  333 '3' */
    { 0x67b40000, 16,   1}, /*  334 '0x01' */
    { 0x67b50000, 16,  49}, /*  335 '1' */
    /*   55                             */
    { 0x40000000,  2,  49}, /*  336 '1' */
    { 0x00000000,  3,  80}, /*  337 'P' */
    { 0xa0000000,  3, 116}, /*  338 't' */
    { 0x80000000,  4,  50}, /*  339 '2' */
    { 0xd0000000,  4,  53}, /*  340 '5' */
    { 0xe0000000,  4,  78}, /*  341 'N' */
    { 0x38000000,  5,  84}, /*  342 'T' */
    { 0x90000000,  5, 112}, /*  343 'p' */
    { 0xf8000000,  5,  99}, /*  344 'c' */
    { 0x24000000,  6,  97}, /*  345 'a' */
    { 0x28000000,  6,  83}, /*  346 'S' */
    { 0x30000000,  6,  82}, /*  347 'R' */
    { 0x9c000000,  6, 101}, /*  348 'e' */
    { 0xf0000000,  6,  74}, /*  349 'J' */
    { 0xf4000000,  6,  65}, /*  350 'A' */
    { 0x2c000000,  7,  68}, /*  351 'D' */
    { 0x36000000,  7,  75}, /*  352 'K' */
    { 0x98000000,  7, 118}, /*  353 'v' */
    { 0x9a000000,  7, 115}, /*  354 's' */
    { 0xc0000000,  7,  98}, /*  355 'b' */
    { 0xc4000000,  7,  71}, /*  356 'G' */
    { 0xc6000000,  7,  56}, /*  357 '8' */
    { 0xc8000000,  7,  77}, /*  358 'M' */
    { 0xca000000,  7,  72}, /*  359 'H' */
    { 0xcc000000,  7,  67}, /*  360 'C' */
    { 0x20000000,  8, 109}, /*  361 'm' */
    { 0x22000000,  8, 111}, /*  362 'o' */
    { 0x23000000,  8,  69}, /*  363 'E' */
    { 0x2e000000,  8,  87}, /*  364 'W' */
    { 0xc3000000,  8, 103}, /*  365 'g' */
    { 0xce000000,  8,  76}, /*  366 'L' */
    { 0x21000000,  9, 100}, /*  367 'd' */
    { 0x2f800000,  9,  85}, /*  368 'U' */
    { 0x34000000,  9,  70}, /*  369 'F' */
    { 0x35000000,  9, 102}, /*  370 'f' */
    { 0xc2000000,  9, 119}, /*  371 'w' */
    { 0xc2800000,  9,  66}, /*  372 'B' */
    { 0xcf800000,  9, 110}, /*  373 'n' */
    { 0x21800000, 10, 108}, /*  374 'l' */
    { 0x21c00000, 10,  57}, /*  375 '9' */
    { 0x2f000000, 10,  52}, /*  376 '4' */
    { 0x2f400000, 10,  73}, /*  377 'I' */
    { 0x34800000, 10,  51}, /*  378 '3' */
    { 0x35c00000, 10, 104}, /*  379 'h' */
    { 0xcf400000, 10, 105}, /*  380 'i' */
    { 0x34c00000, 11,  90}, /*  381 'Z' */
    { 0x34e00000, 11,  86}, /*  382 'V' */
    { 0x35800000, 11,  32}, /*  383 ' ' */
    { 0xcf000000, 11, 107}, /*  384 'k' */
    { 0x35b00000, 12,  79}, /*  385 'O' */
    { 0xcf200000, 12,  39}, /*  386 '\'' */
    { 0x35a00000, 13,   1}, /*  387 '0x01' */
    { 0x35a80000, 13, 117}, /*  388 'u' */
    { 0xcf300000, 13,  88}, /*  389 'X' */
    { 0xcf380000, 13,  55}, /*  390 '7' */
    /*   12                             */
    { 0x00000000,  1,  32}, /*  391 ' ' */
    { 0xc0000000,  2,  46}, /*  392 '.' */
    { 0xa0000000,  3,   0}, /*  393 '0x00' */
    { 0x90000000,  4,  44}, /*  394 ',' */
    { 0x80000000,  5,  58}, /*  395 ':' */
    { 0x8c000000,  6,  59}, /*  396 ';' */
    { 0x8a000000,  7,  33}, /*  397 '!' */
    { 0x89000000,  8,  40}, /*  398 '(' */
    { 0x88000000, 10,   1}, /*  399 '0x01' */
    { 0x88400000, 10, 111}, /*  400 'o' */
    { 0x88800000, 10,  63}, /*  401 '?' */
    { 0x88c00000, 10,  41}, /*  402 ')' */
    /*   13                             */
    { 0x00000000,  1,  42}, /*  403 '*' */
    { 0x80000000,  3, 115}, /*  404 's' */
    { 0xa0000000,  3,  32}, /*  405 ' ' */
    { 0xc0000000,  4, 109}, /*  406 'm' */
    { 0xe0000000,  4, 116}, /*  407 't' */
    { 0xd0000000,  5, 103}, /*  408 'g' */
    { 0xd8000000,  5, 107}, /*  409 'k' */
    { 0xf8000000,  5, 100}, /*  410 'd' */
    { 0xf4000000,  6, 121}, /*  411 'y' */
    { 0xf2000000,  7, 101}, /*  412 'e' */
    { 0xf0000000,  8, 105}, /*  413 'i' */
    { 0xf1000000,  9,   1}, /*  414 '0x01' */
    { 0xf1800000,  9, 110}, /*  415 'n' */
    /*    3                             */
    { 0x80000000,  1, 110}, /*  416 'n' */
    { 0x00000000,  2,   1}, /*  417 '0x01' */
    { 0x40000000,  2,  32}, /*  418 ' ' */
    /*   20                             */
    { 0x80000000,  1,  32}, /*  419 ' ' */
    { 0x40000000,  2,  83}, /*  420 'S' */
    { 0x20000000,  3,  48}, /*  421 '0' */
    { 0x10000000,  4,  65}, /*  422 'A' */
    { 0x00000000,  5,  53}, /*  423 '5' */
    { 0x0c000000,  7,  98}, /*  424 'b' */
    { 0x0e000000,  7,  51}, /*  425 '3' */
    { 0x08000000,  8,  50}, /*  426 '2' */
    { 0x09000000,  8,  34}, /*  427 '\"' */
    { 0x0b000000,  8,  49}, /*  428 '1' */
    { 0x0a000000, 10,  81}, /*  429 'Q' */
    { 0x0a800000, 10,  39}, /*  430 '\'' */
    { 0x0ae00000, 11,  52}, /*  431 '4' */
    { 0x0a400000, 12,  84}, /*  432 'T' */
    { 0x0a500000, 12,  66}, /*  433 'B' */
    { 0x0a600000, 12,  55}, /*  434 '7' */
    { 0x0a700000, 12,  54}, /*  435 '6' */
    { 0x0ac00000, 12,   0}, /*  436 '0x00' */
    { 0x0ad00000, 13,   1}, /*  437 '0x01' */
    { 0x0ad80000, 13, 105}, /*  438 'i' */
    /*   64                             */
    { 0x00000000,  2,  32}, /*  439 ' ' */
    { 0x40000000,  4, 116}, /*  440 't' */
    { 0x50000000,  4,  98}, /*  441 'b' */
    { 0x60000000,  4, 119}, /*  442 'w' */
    { 0x70000000,  4, 117}, /*  443 'u' */
    { 0x90000000,  4, 111}, /*  444 'o' */
    { 0xa0000000,  4, 115}, /*  445 's' */
    { 0xb0000000,  4, 102}, /*  446 'f' */
    { 0x80000000,  5,  99}, /*  447 'c' */
    { 0xd8000000,  5, 108}, /*  448 'l' */
    { 0xe8000000,  5, 100}, /*  449 'd' */
    { 0x88000000,  6,  57}, /*  450 '9' */
    { 0xc0000000,  6, 104}, /*  451 'h' */
    { 0xc8000000,  6,  49}, /*  452 '1' */
    { 0xcc000000,  6, 121}, /*  453 'y' */
    { 0xd4000000,  6, 114}, /*  454 'r' */
    { 0xe0000000,  6,  97}, /*  455 'a' */
    { 0xf0000000,  6, 109}, /*  456 'm' */
    { 0xf8000000,  6, 112}, /*  457 'p' */
    { 0x8c000000,  7,  83}, /*  458 'S' */
    { 0xd0000000,  7, 101}, /*  459 'e' */
    { 0xd2000000,  7, 105}, /*  460 'i' */
    { 0xf6000000,  7, 110}, /*  461 'n' */
    { 0x8e000000,  8,  67}, /*  462 'C' */
    { 0xc5000000,  8,  87}, /*  463 'W' */
    { 0xc7000000,  8, 103}, /*  464 'g' */
    { 0xe5000000,  8,  74}, /*  465 'J' */
    { 0xe6000000,  8,  68}, /*  466 'D' */
    { 0xf5000000,  8,  50}, /*  467 '2' */
    { 0xfc000000,  8,  55}, /*  468 '7' */
    { 0xfe000000,  8,  71}, /*  469 'G' */
    { 0xff000000,  8,  79}, /*  470 'O' */
    { 0x8f800000,  9,  72}, /*  471 'H' */
    { 0xc4000000,  9,  65}, /*  472 'A' */
    { 0xc6000000,  9,  54}, /*  473 '6' */
    { 0xc6800000,  9,  66}, /*  474 'B' */
    { 0xe7800000,  9,  77}, /*  475 'M' */
    { 0xf4000000,  9,  69}, /*  476 'E' */
    { 0xf4800000,  9,  76}, /*  477 'L' */
    { 0xfd000000,  9,  85}, /*  478 'U' */
    { 0xfd800000,  9, 107}, /*  479 'k' */
    { 0x8f000000, 10,  70}, /*  480 'F' */
    { 0xc4800000, 10, 106}, /*  481 'j' */
    { 0xc4c00000, 10,  80}, /*  482 'P' */
    { 0xe4400000, 10, 113}, /*  483 'q' */
    { 0xe4800000, 10,  53}, /*  484 '5' */
    { 0xe4c00000, 10,  84}, /*  485 'T' */
    { 0xe7400000, 10,  73}, /*  486 'I' */
    { 0x8f600000, 11,  75}, /*  487 'K' */
    { 0xe4000000, 11, 118}, /*  488 'v' */
    { 0xe4200000, 11,  90}, /*  489 'Z' */
    { 0xe7200000, 11,  78}, /*  490 'N' */
    { 0x8f500000, 12,  82}, /*  491 'R' */
    { 0xe7100000, 12,  89}, /*  492 'Y' */
    { 0x8f480000, 13,  48}, /*  493 '0' */
    { 0x8f400000, 14,  52}, /*  494 '4' */
    { 0x8f440000, 14, 122}, /*  495 'z' */
    { 0xe7040000, 14,  86}, /*  496 'V' */
    { 0xe7080000, 14,  51}, /*  497 '3' */
    { 0xe70c0000, 14,  56}, /*  498 '8' */
    { 0xe7020000, 15,  81}, /*  499 'Q' */
    { 0xe7000000, 16,  39}, /*  500 '\'' */
    { 0xe7010000, 17,   1}, /*  501 '0x01' */
    { 0xe7018000, 17, 120}, /*  502 'x' */
    /*   65                             */
    { 0x80000000,  1,  32}, /*  503 ' ' */
    { 0x40000000,  2,   0}, /*  504 '0x00' */
    { 0x30000000,  4,  46}, /*  505 '.' */
    { 0x10000000,  5, 105}, /*  506 'i' */
    { 0x20000000,  5,  48}, /*  507 '0' */
    { 0x28000000,  5,  99}, /*  508 'c' */
    { 0x04000000,  6, 117}, /*  509 'u' */
    { 0x00000000,  7,  97}, /*  510 'a' */
    { 0x02000000,  7,  91}, /*  511 '[' */
    { 0x1a000000,  7,  51}, /*  512 '3' */
    { 0x08000000,  8,  52}, /*  513 '4' */
    { 0x0e000000,  8,  72}, /*  514 'H' */
    { 0x18000000,  8,  83}, /*  515 'S' */
    { 0x19000000,  8,  87}, /*  516 'W' */
    { 0x1c000000,  8, 111}, /*  517 'o' */
    { 0x1e000000,  8,  49}, /*  518 '1' */
    { 0x0a000000,  9,  53}, /*  519 '5' */
    { 0x0a800000,  9,  76}, /*  520 'L' */
    { 0x0b800000,  9, 112}, /*  521 'p' */
    { 0x0c000000,  9,  84}, /*  522 'T' */
    { 0x0c800000,  9,  65}, /*  523 'A' */
    { 0x0d000000,  9,  77}, /*  524 'M' */
    { 0x0f000000,  9,  67}, /*  525 'C' */
    { 0x0f800000,  9,  50}, /*  526 '2' */
    { 0x1d800000,  9,  68}, /*  527 'D' */
    { 0x1f000000,  9,  66}, /*  528 'B' */
    { 0x09000000, 10,  78}, /*  529 'N' */
    { 0x09800000, 10, 116}, /*  530 't' */
    { 0x09c00000, 10,  74}, /*  531 'J' */
    { 0x0b400000, 10,  82}, /*  532 'R' */
    { 0x0dc00000, 10,  80}, /*  533 'P' */
    { 0x1d400000, 10, 115}, /*  534 's' */
    { 0x1fc00000, 10,  73}, /*  535 'I' */
    { 0x0b000000, 11, 114}, /*  536 'r' */
    { 0x0d800000, 11,  86}, /*  537 'V' */
    { 0x1d000000, 11, 119}, /*  538 'w' */
    { 0x1d200000, 11,  70}, /*  539 'F' */
    { 0x1fa00000, 11,  71}, /*  540 'G' */
    { 0x09400000, 12,  69}, /*  541 'E' */
    { 0x09500000, 12,  58}, /*  542 ':' */
    { 0x0b200000, 12, 104}, /*  543 'h' */
    { 0x0da00000, 12,  44}, /*  544 ',' */
    { 0x1f800000, 12,  39}, /*  545 '\'' */
    { 0x09680000, 13,  98}, /*  546 'b' */
    { 0x09700000, 13,  75}, /*  547 'K' */
    { 0x09780000, 13,  89}, /*  548 'Y' */
    { 0x0b380000, 13,  79}, /*  549 'O' */
    { 0x0db00000, 13,  45}, /*  550 '-' */
    { 0x1f900000, 13, 102}, /*  551 'f' */
    { 0x1f980000, 13,  40}, /*  552 '(' */
    { 0x0b300000, 14,  34}, /*  553 '\"' */
    { 0x0db80000, 14, 121}, /*  554 'y' */
    { 0x09600000, 15,  63}, /*  555 '?' */
    { 0x09620000, 15, 109}, /*  556 'm' */
    { 0x09640000, 15,  81}, /*  557 'Q' */
    { 0x0dbc0000, 15,  42}, /*  558 '*' */
    { 0x0dbe0000, 15,  38}, /*  559 '&' */
    { 0x09660000, 16,  85}, /*  560 'U' */
    { 0x09670000, 16,  59}, /*  561 ';' */
    { 0x0b340000, 16,  56}, /*  562 '8' */
    { 0x0b350000, 16,  54}, /*  563 '6' */
    { 0x0b370000, 16, 107}, /*  564 'k' */
    { 0x0b360000, 17, 100}, /*  565 'd' */
    { 0x0b368000, 18,   1}, /*  566 '0x01' */
    { 0x0b36c000, 18, 110}, /*  567 'n' */
    /*   51                             */
    { 0x40000000,  2,  99}, /*  568 'c' */
    { 0xc0000000,  3,  49}, /*  569 '1' */
    { 0xe0000000,  3, 101}, /*  570 'e' */
    { 0x00000000,  4,  53}, /*  571 '5' */
    { 0x20000000,  4,  56}, /*  572 '8' */
    { 0x10000000,  5,  84}, /*  573 'T' */
    { 0x18000000,  5, 102}, /*  574 'f' */
    { 0x30000000,  5,  66}, /*  575 'B' */
    { 0x38000000,  5,  50}, /*  576 '2' */
    { 0x88000000,  5,  51}, /*  577 '3' */
    { 0x90000000,  5,  55}, /*  578 '7' */
    { 0xa0000000,  5,  54}, /*  579 '6' */
    { 0xb0000000,  5,  97}, /*  580 'a' */
    { 0xbc000000,  6,  52}, /*  581 '4' */
    { 0x80000000,  7,  70}, /*  582 'F' */
    { 0x84000000,  7, 115}, /*  583 's' */
    { 0x86000000,  7,  77}, /*  584 'M' */
    { 0x98000000,  7,  72}, /*  585 'H' */
    { 0x9c000000,  7,  68}, /*  586 'D' */
    { 0x9e000000,  7,  65}, /*  587 'A' */
    { 0xaa000000,  7,  83}, /*  588 'S' */
    { 0x83000000,  8, 109}, /*  589 'm' */
    { 0x9a000000,  8,  87}, /*  590 'W' */
    { 0xa8000000,  8,  71}, /*  591 'G' */
    { 0xa9000000,  8,  85}, /*  592 'U' */
    { 0xac000000,  8, 100}, /*  593 'd' */
    { 0xad000000,  8,  79}, /*  594 'O' */
    { 0xae000000,  8,  78}, /*  595 'N' */
    { 0xb9000000,  8,  67}, /*  596 'C' */
    { 0xbb000000,  8,  80}, /*  597 'P' */
    { 0x9b000000,  9,  76}, /*  598 'L' */
    { 0xaf000000,  9,  32}, /*  599 ' ' */
    { 0xaf800000,  9,  73}, /*  600 'I' */
    { 0xb8000000,  9,  69}, /*  601 'E' */
    { 0xb8800000,  9,  82}, /*  602 'R' */
    { 0xba000000,  9,  75}, /*  603 'K' */
    { 0xba800000,  9, 116}, /*  604 't' */
    { 0x82400000, 10,  74}, /*  605 'J' */
    { 0x82c00000, 10,  57}, /*  606 '9' */
    { 0x82000000, 11, 118}, /*  607 'v' */
    { 0x82200000, 11, 112}, /*  608 'p' */
    { 0x82800000, 11, 104}, /*  609 'h' */
    { 0x9ba00000, 11, 111}, /*  610 'o' */
    { 0x9bc00000, 11,  81}, /*  611 'Q' */
    { 0x9be00000, 11,  48}, /*  612 '0' */
    { 0x82a00000, 12, 108}, /*  613 'l' */
    { 0x82b00000, 12, 105}, /*  614 'i' */
    { 0x9b800000, 12,  86}, /*  615 'V' */
    { 0x9b980000, 13, 121}, /*  616 'y' */
    { 0x9b900000, 14,   1}, /*  617 '0x01' */
    { 0x9b940000, 14, 103}, /*  618 'g' */
    /*   35                             */
    { 0x00000000,  1,  48}, /*  619 '0' */
    { 0xe0000000,  3,  32}, /*  620 ' ' */
    { 0x90000000,  4,  97}, /*  621 'a' */
    { 0xb0000000,  4, 112}, /*  622 'p' */
    { 0x88000000,  5, 115}, /*  623 's' */
    { 0xc0000000,  5,  46}, /*  624 '.' */
    { 0xc8000000,  5,  56}, /*  625 '8' */
    { 0xd8000000,  5,  44}, /*  626 ',' */
    { 0x80000000,  6,  52}, /*  627 '4' */
    { 0x84000000,  6, 116}, /*  628 't' */
    { 0xa4000000,  6,  53}, /*  629 '5' */
    { 0xd0000000,  6,  54}, /*  630 '6' */
    { 0xa0000000,  7,  51}, /*  631 '3' */
    { 0xa2000000,  7,  55}, /*  632 '7' */
    { 0xaa000000,  7,  93}, /*  633 ']' */
    { 0xac000000,  7,  45}, /*  634 '-' */
    { 0xae000000,  7,  49}, /*  635 '1' */
    { 0xa8000000,  8,  41}, /*  636 ')' */
    { 0xa9000000,  8,  47}, /*  637 '/' */
    { 0xd4000000,  8,   0}, /*  638 '0x00' */
    { 0xd5000000,  8,  57}, /*  639 '9' */
    { 0xd7000000,  8,  50}, /*  640 '2' */
    { 0xd6000000,  9,  37}, /*  641 '%' */
    { 0xd6800000, 10,  58}, /*  642 ':' */
    { 0xd6d00000, 12, 102}, /*  643 'f' */
    { 0xd6f00000, 12, 109}, /*  644 'm' */
    { 0xd6e00000, 13, 121}, /*  645 'y' */
    { 0xd6c40000, 14, 108}, /*  646 'l' */
    { 0xd6c80000, 14,  59}, /*  647 ';' */
    { 0xd6cc0000, 14,  39}, /*  648 '\'' */
    { 0xd6e80000, 14, 107}, /*  649 'k' */
    { 0xd6ec0000, 14,  33}, /*  650 '!' */
    { 0xd6c00000, 15,  67}, /*  651 'C' */
    { 0xd6c20000, 16,   1}, /*  652 '0x01' */
    { 0xd6c30000, 16,  74}, /*  653 'J' */
    /*   38                             */
    { 0x00000000,  2,  57}, /*  654 '9' */
    { 0x80000000,  3,  49}, /*  655 '1' */
    { 0xe0000000,  3,  48}, /*  656 '0' */
    { 0x50000000,  4,  32}, /*  657 ' ' */
    { 0x70000000,  4,  50}, /*  658 '2' */
    { 0xb0000000,  4,  46}, /*  659 '.' */
    { 0xc0000000,  4,  53}, /*  660 '5' */
    { 0x40000000,  5,  54}, /*  661 '6' */
    { 0x48000000,  5,  56}, /*  662 '8' */
    { 0x68000000,  5,  47}, /*  663 '/' */
    { 0xa0000000,  5,  93}, /*  664 ']' */
    { 0xd0000000,  5,  51}, /*  665 '3' */
    { 0x64000000,  6,  55}, /*  666 '7' */
    { 0xd8000000,  6,  52}, /*  667 '4' */
    { 0x60000000,  7,   0}, /*  668 '0x00' */
    { 0x62000000,  7,  45}, /*  669 '-' */
    { 0xa8000000,  7,  41}, /*  670 ')' */
    { 0xac000000,  7,  58}, /*  671 ':' */
    { 0xae000000,  7, 115}, /*  672 's' */
    { 0xdc000000,  7,  44}, /*  673 ',' */
    { 0xde000000,  7, 120}, /*  674 'x' */
    { 0xaa000000,  8,  39}, /*  675 '\'' */
    { 0xab000000,  9,  88}, /*  676 'X' */
    { 0xabc00000, 11, 116}, /*  677 't' */
    { 0xabe00000, 11,  82}, /*  678 'R' */
    { 0xabb00000, 12,  59}, /*  679 ';' */
    { 0xab800000, 13, 112}, /*  680 'p' */
    { 0xab880000, 13, 109}, /*  681 'm' */
    { 0xab900000, 13,  33}, /*  682 '!' */
    { 0xaba80000, 13,  38}, /*  683 '&' */
    { 0xab980000, 14, 101}, /*  684 'e' */
    { 0xab9c0000, 15,  98}, /*  685 'b' */
    { 0xab9e0000, 15,  97}, /*  686 'a' */
    { 0xaba00000, 15,  68}, /*  687 'D' */
    { 0xaba20000, 15,  67}, /*  688 'C' */
    { 0xaba40000, 15,  37}, /*  689 '%' */
    { 0xaba60000, 16,   1}, /*  690 '0x01' */
    { 0xaba70000, 16, 111}, /*  691 'o' */
    /*   32                             */
    { 0xc0000000,  2,  48}, /*  692 '0' */
    { 0x00000000,  3,  32}, /*  693 ' ' */
    { 0x40000000,  3,  46}, /*  694 '.' */
    { 0x80000000,  3,  53}, /*  695 '5' */
    { 0x30000000,  4,  47}, /*  696 '/' */
    { 0xb0000000,  4,  44}, /*  697 ',' */
    { 0x20000000,  5,  93}, /*  698 ']' */
    { 0x28000000,  5, 112}, /*  699 'p' */
    { 0x68000000,  5,  49}, /*  700 '1' */
    { 0xa8000000,  5,  52}, /*  701 '4' */
    { 0x60000000,  6,  54}, /*  702 '6' */
    { 0x64000000,  6,  50}, /*  703 '2' */
    { 0x70000000,  6,  58}, /*  704 ':' */
    { 0x74000000,  6,  45}, /*  705 '-' */
    { 0x7c000000,  6,  41}, /*  706 ')' */
    { 0xa0000000,  7,   0}, /*  707 '0x00' */
    { 0xa4000000,  7,  56}, /*  708 '8' */
    { 0xa6000000,  7,  57}, /*  709 '9' */
    { 0x78000000,  8,  68}, /*  710 'D' */
    { 0x79000000,  8,  51}, /*  711 '3' */
    { 0x7a000000,  8, 116}, /*  712 't' */
    { 0x7b000000,  8,  55}, /*  713 '7' */
    { 0xa2000000,  8, 110}, /*  714 'n' */
    { 0xa3000000,  9,  97}, /*  715 'a' */
    { 0xa3800000, 10,  39}, /*  716 '\'' */
    { 0xa3e00000, 11,  59}, /*  717 ';' */
    { 0xa3c00000, 12, 115}, /*  718 's' */
    { 0xa3d00000, 13,  34}, /*  719 '\"' */
    { 0xa3d80000, 15,   1}, /*  720 '0x01' */
    { 0xa3da0000, 15, 105}, /*  721 'i' */
    { 0xa3dc0000, 15,  87}, /*  722 'W' */
    { 0xa3de0000, 15,  76}, /*  723 'L' */
    /*   36                             */
    { 0x00000000,  2,  32}, /*  724 ' ' */
    { 0x80000000,  2,  48}, /*  725 '0' */
    { 0xc0000000,  3,  46}, /*  726 '.' */
    { 0xe0000000,  4,  47}, /*  727 '/' */
    { 0x40000000,  5,  50}, /*  728 '2' */
    { 0x48000000,  5,  49}, /*  729 '1' */
    { 0x50000000,  5,  41}, /*  730 ')' */
    { 0x60000000,  5,  58}, /*  731 ':' */
    { 0x70000000,  5,  45}, /*  732 '-' */
    { 0xf8000000,  5,  93}, /*  733 ']' */
    { 0x58000000,  6,  68}, /*  734 'D' */
    { 0x68000000,  6,  52}, /*  735 '4' */
    { 0x6c000000,  6,   0}, /*  736 '0x00' */
    { 0x7c000000,  6,  53}, /*  737 '5' */
    { 0xf0000000,  6,  44}, /*  738 ',' */
    { 0x5c000000,  7,  55}, /*  739 '7' */
    { 0x5e000000,  7,  51}, /*  740 '3' */
    { 0x7a000000,  7,  54}, /*  741 '6' */
    { 0xf6000000,  7, 116}, /*  742 't' */
    { 0x79000000,  8,  66}, /*  743 'B' */
    { 0xf4000000,  8,  56}, /*  744 '8' */
    { 0xf5000000,  9,  57}, /*  745 '9' */
    { 0x78400000, 10,  59}, /*  746 ';' */
    { 0xf5800000, 10, 114}, /*  747 'r' */
    { 0xf5c00000, 10, 115}, /*  748 's' */
    { 0x78000000, 11, 110}, /*  749 'n' */
    { 0x78a00000, 11,  98}, /*  750 'b' */
    { 0x78c00000, 11,  39}, /*  751 '\'' */
    { 0x78e00000, 11,  65}, /*  752 'A' */
    { 0x78200000, 12, 112}, /*  753 'p' */
    { 0x78300000, 12, 101}, /*  754 'e' */
    { 0x78800000, 13,  97}, /*  755 'a' */
    { 0x78880000, 13,  38}, /*  756 '&' */
    { 0x78900000, 13,  37}, /*  757 '%' */
    { 0x78980000, 14,   1}, /*  758 '0x01' */
    { 0x789c0000, 14, 107}, /*  759 'k' */
    /*   35                             */
    { 0x40000000,  2,  32}, /*  760 ' ' */
    { 0x00000000,  3,  52}, /*  761 '4' */
    { 0x80000000,  3,  46}, /*  762 '.' */
    { 0xc0000000,  3,  48}, /*  763 '0' */
    { 0x20000000,  4,  47}, /*  764 '/' */
    { 0xa0000000,  4,  53}, /*  765 '5' */
    { 0xb0000000,  4,  45}, /*  766 '-' */
    { 0x30000000,  5,  49}, /*  767 '1' */
    { 0xe8000000,  5,  93}, /*  768 ']' */
    { 0xf0000000,  5,  44}, /*  769 ',' */
    { 0x38000000,  6,  50}, /*  770 '2' */
    { 0xe4000000,  6,  56}, /*  771 '8' */
    { 0xf8000000,  6,  41}, /*  772 ')' */
    { 0x3c000000,  7,  58}, /*  773 ':' */
    { 0x3e000000,  7,  39}, /*  774 '\'' */
    { 0xe0000000,  7, 116}, /*  775 't' */
    { 0xe2000000,  7,  51}, /*  776 '3' */
    { 0xfe000000,  7,   0}, /*  777 '0x00' */
    { 0xfd000000,  8,  54}, /*  778 '6' */
    { 0xfc000000, 10,  57}, /*  779 '9' */
    { 0xfc800000, 10,  55}, /*  780 '7' */
    { 0xfc600000, 11,  67}, /*  781 'C' */
    { 0xfcc00000, 11,  59}, /*  782 ';' */
    { 0xfc500000, 12, 120}, /*  783 'x' */
    { 0xfce80000, 13, 109}, /*  784 'm' */
    { 0xfcf00000, 13,  73}, /*  785 'I' */
    { 0xfc400000, 14, 102}, /*  786 'f' */
    { 0xfc440000, 14, 101}, /*  787 'e' */
    { 0xfc480000, 14,  98}, /*  788 'b' */
    { 0xfc4c0000, 14,  76}, /*  789 'L' */
    { 0xfce00000, 14,  37}, /*  790 '%' */
    { 0xfcf80000, 14, 112}, /*  791 'p' */
    { 0xfcfc0000, 14,  99}, /*  792 'c' */
    { 0xfce40000, 15,   1}, /*  793 '0x01' */
    { 0xfce60000, 15, 105}, /*  794 'i' */
    /*   32                             */
    { 0x00000000,  2,  48}, /*  795 '0' */
    { 0x80000000,  2,  32}, /*  796 ' ' */
    { 0x40000000,  3,  46}, /*  797 '.' */
    { 0x70000000,  4, 112}, /*  798 'p' */
    { 0xc0000000,  4,  53}, /*  799 '5' */
    { 0x60000000,  5,  47}, /*  800 '/' */
    { 0xd0000000,  5,  97}, /*  801 'a' */
    { 0xd8000000,  5,  45}, /*  802 '-' */
    { 0xe8000000,  5,  54}, /*  803 '6' */
    { 0x68000000,  6,  51}, /*  804 '3' */
    { 0xf0000000,  6,  50}, /*  805 '2' */
    { 0xf8000000,  6,  56}, /*  806 '8' */
    { 0xfc000000,  6,  93}, /*  807 ']' */
    { 0x6c000000,  7,  58}, /*  808 ':' */
    { 0xe0000000,  7,  41}, /*  809 ')' */
    { 0xe2000000,  7, 115}, /*  810 's' */
    { 0xe6000000,  7,  44}, /*  811 ',' */
    { 0xf4000000,  7,  55}, /*  812 '7' */
    { 0xf6000000,  7,  57}, /*  813 '9' */
    { 0x6e000000,  8,  52}, /*  814 '4' */
    { 0xe4000000,  8,   0}, /*  815 '0x00' */
    { 0xe5000000,  8, 116}, /*  816 't' */
    { 0x6f800000, 10,  99}, /*  817 'c' */
    { 0x6fc00000, 10,  49}, /*  818 '1' */
    { 0x6f200000, 11,  59}, /*  819 ';' */
    { 0x6f600000, 11, 109}, /*  820 'm' */
    { 0x6f000000, 12, 101}, /*  821 'e' */
    { 0x6f100000, 12,  39}, /*  822 '\'' */
    { 0x6f500000, 12, 107}, /*  823 'k' */
    { 0x6f480000, 13, 108}, /*  824 'l' */
    { 0x6f400000, 14,   1}, /*  825 '0x01' */
    { 0x6f440000, 14, 102}, /*  826 'f' */
    /*   33                             */
    { 0x00000000,  2,  32}, /*  827 ' ' */
    { 0x80000000,  2,  46}, /*  828 '.' */
    { 0xe0000000,  3,  48}, /*  829 '0' */
    { 0x50000000,  4,  93}, /*  830 ']' */
    { 0x48000000,  5,  49}, /*  831 '1' */
    { 0x60000000,  5,  55}, /*  832 '7' */
    { 0x70000000,  5,  41}, /*  833 ')' */
    { 0xc0000000,  5,  44}, /*  834 ',' */
    { 0xc8000000,  5,  58}, /*  835 ':' */
    { 0xd0000000,  5,  47}, /*  836 '/' */
    { 0x44000000,  6,  45}, /*  837 '-' */
    { 0x68000000,  6,  53}, /*  838 '5' */
    { 0x6c000000,  6,  52}, /*  839 '4' */
    { 0x7c000000,  6,  56}, /*  840 '8' */
    { 0xdc000000,  6, 116}, /*  841 't' */
    { 0x40000000,  7,  54}, /*  842 '6' */
    { 0x78000000,  7,  51}, /*  843 '3' */
    { 0x7a000000,  7,  50}, /*  844 '2' */
    { 0xda000000,  7,  57}, /*  845 '9' */
    { 0x42000000,  8,   0}, /*  846 '0x00' */
    { 0x43000000,  8,  43}, /*  847 '+' */
    { 0xd8000000,  8,  39}, /*  848 '\'' */
    { 0xd9400000, 10,  97}, /*  849 'a' */
    { 0xd9800000, 10,  63}, /*  850 '?' */
    { 0xd9000000, 11, 109}, /*  851 'm' */
    { 0xd9200000, 11, 101}, /*  852 'e' */
    { 0xd9c00000, 11,  59}, /*  853 ';' */
    { 0xd9e00000, 13, 102}, /*  854 'f' */
    { 0xd9e80000, 13,  98}, /*  855 'b' */
    { 0xd9f00000, 13,  77}, /*  856 'M' */
    { 0xd9f80000, 14,  34}, /*  857 '\"' */
    { 0xd9fc0000, 15,   1}, /*  858 '0x01' */
    { 0xd9fe0000, 15, 105}, /*  859 'i' */
    /*   31                             */
    { 0xc0000000,  2,  46}, /*  860 '.' */
    { 0x20000000,  3,  32}, /*  861 ' ' */
    { 0x60000000,  3,  48}, /*  862 '0' */
    { 0xa0000000,  3,  45}, /*  863 '-' */
    { 0x00000000,  4,  56}, /*  864 '8' */
    { 0x40000000,  4,  55}, /*  865 '7' */
    { 0x90000000,  4,  93}, /*  866 ']' */
    { 0x50000000,  5,  47}, /*  867 '/' */
    { 0x14000000,  6,  54}, /*  868 '6' */
    { 0x18000000,  6,  50}, /*  869 '2' */
    { 0x1c000000,  6,  49}, /*  870 '1' */
    { 0x5c000000,  6, 116}, /*  871 't' */
    { 0x80000000,  6,  57}, /*  872 '9' */
    { 0x88000000,  6,  41}, /*  873 ')' */
    { 0x8c000000,  6,  53}, /*  874 '5' */
    { 0x12000000,  7,  51}, /*  875 '3' */
    { 0x5a000000,  7,  44}, /*  876 ',' */
    { 0x84000000,  7,  97}, /*  877 'a' */
    { 0x86000000,  7,  52}, /*  878 '4' */
    { 0x10000000,  8,   0}, /*  879 '0x00' */
    { 0x58000000,  8,  58}, /*  880 ':' */
    { 0x59000000,  8, 112}, /*  881 'p' */
    { 0x11400000, 10,  82}, /*  882 'R' */
    { 0x11800000, 10,  59}, /*  883 ';' */
    { 0x11000000, 11,  39}, /*  884 '\'' */
    { 0x11e00000, 11, 109}, /*  885 'm' */
    { 0x11300000, 12, 102}, /*  886 'f' */
    { 0x11c00000, 12,  65}, /*  887 'A' */
    { 0x11d00000, 12,  63}, /*  888 '?' */
    { 0x11200000, 13,   1}, /*  889 '0x01' */
    { 0x11280000, 13, 115}, /*  890 's' */
    /*   31                             */
    { 0x40000000,  2,  32}, /*  891 ' ' */
    { 0x00000000,  3,  52}, /*  892 '4' */
    { 0xc0000000,  3,  46}, /*  893 '.' */
    { 0x30000000,  4,  48}, /*  894 '0' */
    { 0x80000000,  4,  57}, /*  895 '9' */
    { 0x90000000,  4,  55}, /*  896 '7' */
    { 0xb0000000,  4,  56}, /*  897 '8' */
    { 0xe0000000,  4,  49}, /*  898 '1' */
    { 0x20000000,  5,  47}, /*  899 '/' */
    { 0xa0000000,  5,  51}, /*  900 '3' */
    { 0xf0000000,  5,  53}, /*  901 '5' */
    { 0xa8000000,  6,  41}, /*  902 ')' */
    { 0xf8000000,  6,  54}, /*  903 '6' */
    { 0xfc000000,  6,  93}, /*  904 ']' */
    { 0x28000000,  7,  58}, /*  905 ':' */
    { 0x2a000000,  7,  44}, /*  906 ',' */
    { 0xae000000,  7, 116}, /*  907 't' */
    { 0x2c000000,  8, 112}, /*  908 'p' */
    { 0x2d000000,  8,  45}, /*  909 '-' */
    { 0x2f000000,  8,  97}, /*  910 'a' */
    { 0xac000000,  8,  99}, /*  911 'c' */
    { 0xad000000,  8,  50}, /*  912 '2' */
    { 0x2e800000,  9,   0}, /*  913 '0x00' */
    { 0x2e400000, 10,  59}, /*  914 ';' */
    { 0x2e000000, 12, 108}, /*  915 'l' */
    { 0x2e100000, 12,  39}, /*  916 '\'' */
    { 0x2e200000, 13, 102}, /*  917 'f' */
    { 0x2e280000, 13,  68}, /*  918 'D' */
    { 0x2e300000, 13,  65}, /*  919 'A' */
    { 0x2e380000, 14,   1}, /*  920 '0x01' */
    { 0x2e3c0000, 14, 105}, /*  921 'i' */
    /*   28                             */
    { 0x00000000,  3,  53}, /*  922 '5' */
    { 0x20000000,  3,  93}, /*  923 ']' */
    { 0xe0000000,  3,  57}, /*  924 '9' */
    { 0x50000000,  4,  48}, /*  925 '0' */
    { 0x60000000,  4,  46}, /*  926 '.' */
    { 0x70000000,  4,  45}, /*  927 '-' */
    { 0x80000000,  4,  52}, /*  928 '4' */
    { 0x90000000,  4,  56}, /*  929 '8' */
    { 0xb0000000,  4,  54}, /*  930 '6' */
    { 0xc0000000,  4,  32}, /*  931 ' ' */
    { 0xd0000000,  4,  55}, /*  932 '7' */
    { 0x40000000,  5,  50}, /*  933 '2' */
    { 0xa0000000,  5,  51}, /*  934 '3' */
    { 0x48000000,  6,  49}, /*  935 '1' */
    { 0x4c000000,  6,  47}, /*  936 '/' */
    { 0xa8000000,  6, 116}, /*  937 't' */
    { 0xae000000,  7,  41}, /*  938 ')' */
    { 0xac000000,  8,  58}, /*  939 ':' */
    { 0xad000000,  9,  44}, /*  940 ',' */
    { 0xad800000, 11, 112}, /*  941 'p' */
    { 0xada00000, 11,  59}, /*  942 ';' */
    { 0xade00000, 11,   0}, /*  943 '0x00' */
    { 0xadc00000, 13, 110}, /*  944 'n' */
    { 0xadc80000, 13, 109}, /*  945 'm' */
    { 0xadd00000, 13,  97}, /*  946 'a' */
    { 0xaddc0000, 14, 101}, /*  947 'e' */
    { 0xadd80000, 15,   1}, /*  948 '0x01' */
    { 0xadda0000, 15, 107}, /*  949 'k' */
    /*    9                             */
    { 0x80000000,  1,  32}, /*  950 ' ' */
    { 0x40000000,  2,  48}, /*  951 '0' */
    { 0x20000000,  3,  51}, /*  952 '3' */
    { 0x08000000,  5,  49}, /*  953 '1' */
    { 0x10000000,  5,  84}, /*  954 'T' */
    { 0x18000000,  5,  67}, /*  955 'C' */
    { 0x04000000,  6,  52}, /*  956 '4' */
    { 0x00000000,  7,   1}, /*  957 '0x01' */
    { 0x02000000,  7,  53}, /*  958 '5' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  959 '0x01' */
    { 0x80000000,  1,  32}, /*  960 ' ' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  961 '0x01' */
    { 0x80000000,  1,   1}, /*  962 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  963 '0x01' */
    { 0x80000000,  1,   1}, /*  964 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /*  965 '0x01' */
    { 0x80000000,  1,   1}, /*  966 '0x01' */
    /*   12                             */
    { 0x80000000,  1,  32}, /*  967 ' ' */
    { 0x40000000,  2,   0}, /*  968 '0x00' */
    { 0x00000000,  3,  58}, /*  969 ':' */
    { 0x20000000,  5,  33}, /*  970 '!' */
    { 0x30000000,  5,  91}, /*  971 '[' */
    { 0x38000000,  5,  46}, /*  972 '.' */
    { 0x28000000,  6,  59}, /*  973 ';' */
    { 0x2c000000,  7,  39}, /*  974 '\'' */
    { 0x2f000000,  8,  44}, /*  975 ',' */
    { 0x2e800000,  9,  47}, /*  976 '/' */
    { 0x2e000000, 10,   1}, /*  977 '0x01' */
    { 0x2e400000, 10,  81}, /*  978 'Q' */
    /*    6                             */
    { 0x00000000,  1, 107}, /*  979 'k' */
    { 0x80000000,  2,  32}, /*  980 ' ' */
    { 0xe0000000,  3,  84}, /*  981 'T' */
    { 0xd0000000,  4,  98}, /*  982 'b' */
    { 0xc0000000,  5,   1}, /*  983 '0x01' */
    { 0xc8000000,  5,  72}, /*  984 'H' */
    /*   58                             */
    { 0x40000000,  2,  32}, /*  985 ' ' */
    { 0xc0000000,  3,  68}, /*  986 'D' */
    { 0xe0000000,  3, 110}, /*  987 'n' */
    { 0x00000000,  4, 115}, /*  988 's' */
    { 0x10000000,  4, 109}, /*  989 'm' */
    { 0x20000000,  4, 100}, /*  990 'd' */
    { 0x80000000,  4, 114}, /*  991 'r' */
    { 0xb0000000,  4, 108}, /*  992 'l' */
    { 0x30000000,  5,  99}, /*  993 'c' */
    { 0x90000000,  5, 117}, /*  994 'u' */
    { 0xa0000000,  5, 103}, /*  995 'g' */
    { 0x3c000000,  6,  98}, /*  996 'b' */
    { 0x9c000000,  6, 116}, /*  997 't' */
    { 0xac000000,  6, 102}, /*  998 'f' */
    { 0xa8000000,  7, 119}, /*  999 'w' */
    { 0xaa000000,  7, 105}, /* 1000 'i' */
    { 0x99000000,  8, 118}, /* 1001 'v' */
    { 0x9b000000,  8, 112}, /* 1002 'p' */
    { 0x39000000,  9, 104}, /* 1003 'h' */
    { 0x3a000000,  9,  46}, /* 1004 '.' */
    { 0x3b800000,  9,  66}, /* 1005 'B' */
    { 0x9a000000,  9, 113}, /* 1006 'q' */
    { 0x38000000, 10,  67}, /* 1007 'C' */
    { 0x38800000, 10,  44}, /* 1008 ',' */
    { 0x39800000, 10, 121}, /* 1009 'y' */
    { 0x3a800000, 10,  83}, /* 1010 'S' */
    { 0x3b000000, 10, 107}, /* 1011 'k' */
    { 0x3b400000, 10,  84}, /* 1012 'T' */
    { 0x98400000, 10,  82}, /* 1013 'R' */
    { 0x98800000, 10,  70}, /* 1014 'F' */
    { 0x98c00000, 10, 122}, /* 1015 'z' */
    { 0x9ac00000, 10,  97}, /* 1016 'a' */
    { 0x38400000, 11,  80}, /* 1017 'P' */
    { 0x38c00000, 11,  45}, /* 1018 '-' */
    { 0x3ac00000, 11,  65}, /* 1019 'A' */
    { 0x3ae00000, 11,  73}, /* 1020 'I' */
    { 0x98200000, 11, 101}, /* 1021 'e' */
    { 0x9a800000, 11,  78}, /* 1022 'N' */
    { 0x9aa00000, 11, 120}, /* 1023 'x' */
    { 0x38600000, 12,  88}, /* 1024 'X' */
    { 0x38700000, 12,  75}, /* 1025 'K' */
    { 0x38e00000, 12,  51}, /* 1026 '3' */
    { 0x38f00000, 12,  38}, /* 1027 '&' */
    { 0x39d00000, 12,  77}, /* 1028 'M' */
    { 0x39f00000, 12,  89}, /* 1029 'Y' */
    { 0x39c00000, 13,  76}, /* 1030 'L' */
    { 0x39c80000, 13,  87}, /* 1031 'W' */
    { 0x39e00000, 13,  42}, /* 1032 '*' */
    { 0x98000000, 13, 111}, /* 1033 'o' */
    { 0x98080000, 13,  49}, /* 1034 '1' */
    { 0x98100000, 13,  39}, /* 1035 '\'' */
    { 0x98180000, 13,  58}, /* 1036 ':' */
    { 0x39e80000, 14, 106}, /* 1037 'j' */
    { 0x39ec0000, 15,  71}, /* 1038 'G' */
    { 0x39ee0000, 16,  79}, /* 1039 'O' */
    { 0x39ef0000, 17,  52}, /* 1040 '4' */
    { 0x39ef8000, 18,   1}, /* 1041 '0x01' */
    { 0x39efc000, 18,  69}, /* 1042 'E' */
    /*   40                             */
    { 0x00000000,  2,  67}, /* 1043 'C' */
    { 0x40000000,  3,  97}, /* 1044 'a' */
    { 0xa0000000,  3, 114}, /* 1045 'r' */
    { 0xc0000000,  3, 101}, /* 1046 'e' */
    { 0xe0000000,  3,  66}, /* 1047 'B' */
    { 0x70000000,  4, 105}, /* 1048 'i' */
    { 0x80000000,  4, 117}, /* 1049 'u' */
    { 0x90000000,  4, 111}, /* 1050 'o' */
    { 0x68000000,  5, 108}, /* 1051 'l' */
    { 0x60000000,  8,  84}, /* 1052 'T' */
    { 0x61000000,  8,  46}, /* 1053 '.' */
    { 0x63000000,  8, 121}, /* 1054 'y' */
    { 0x64000000,  8,  73}, /* 1055 'I' */
    { 0x66000000,  8, 104}, /* 1056 'h' */
    { 0x67000000,  8,  32}, /* 1057 ' ' */
    { 0x62000000,  9,  65}, /* 1058 'A' */
    { 0x62800000, 10,  79}, /* 1059 'O' */
    { 0x62c00000, 11,  58}, /* 1060 ':' */
    { 0x65000000, 11,  38}, /* 1061 '&' */
    { 0x65200000, 11,  51}, /* 1062 '3' */
    { 0x65400000, 11, 106}, /* 1063 'j' */
    { 0x65600000, 11,  77}, /* 1064 'M' */
    { 0x65c00000, 11,  68}, /* 1065 'D' */
    { 0x62f00000, 12,  87}, /* 1066 'W' */
    { 0x65800000, 12,  42}, /* 1067 '*' */
    { 0x65a00000, 12,  44}, /* 1068 ',' */
    { 0x65b00000, 12,  49}, /* 1069 '1' */
    { 0x65e00000, 12,   1}, /* 1070 '0x01' */
    { 0x65f00000, 12,  80}, /* 1071 'P' */
    { 0x62e00000, 13,  69}, /* 1072 'E' */
    { 0x65980000, 13,  82}, /* 1073 'R' */
    { 0x62ec0000, 14, 119}, /* 1074 'w' */
    { 0x62e80000, 15,  45}, /* 1075 '-' */
    { 0x65920000, 15,  85}, /* 1076 'U' */
    { 0x65940000, 15,  83}, /* 1077 'S' */
    { 0x65960000, 15,  70}, /* 1078 'F' */
    { 0x62ea0000, 16,  88}, /* 1079 'X' */
    { 0x62eb0000, 16,  86}, /* 1080 'V' */
    { 0x65900000, 16,  81}, /* 1081 'Q' */
    { 0x65910000, 16,  52}, /* 1082 '4' */
    /*   47                             */
    { 0x00000000,  2, 104}, /* 1083 'h' */
    { 0x60000000,  3,  66}, /* 1084 'B' */
    { 0x80000000,  3,  97}, /* 1085 'a' */
    { 0xc0000000,  3, 111}, /* 1086 'o' */
    { 0x50000000,  4, 114}, /* 1087 'r' */
    { 0xa0000000,  4,  46}, /* 1088 '.' */
    { 0xe0000000,  4,  32}, /* 1089 ' ' */
    { 0xf0000000,  4, 108}, /* 1090 'l' */
    { 0x40000000,  5,  39}, /* 1091 '\'' */
    { 0xb8000000,  5, 105}, /* 1092 'i' */
    { 0x4c000000,  6, 101}, /* 1093 'e' */
    { 0xb4000000,  6, 117}, /* 1094 'u' */
    { 0x4a000000,  7, 121}, /* 1095 'y' */
    { 0xb2000000,  7,  44}, /* 1096 ',' */
    { 0x49000000,  8,  73}, /* 1097 'I' */
    { 0x48000000,  9,  65}, /* 1098 'A' */
    { 0xb0000000, 10,  67}, /* 1099 'C' */
    { 0xb0400000, 10,  68}, /* 1100 'D' */
    { 0xb0c00000, 10,   0}, /* 1101 '0x00' */
    { 0xb1000000, 10,  83}, /* 1102 'S' */
    { 0xb1400000, 10,  84}, /* 1103 'T' */
    { 0xb1c00000, 10,  71}, /* 1104 'G' */
    { 0x48800000, 11,  42}, /* 1105 '*' */
    { 0x48e00000, 11, 119}, /* 1106 'w' */
    { 0xb1a00000, 11,  74}, /* 1107 'J' */
    { 0x48a00000, 12,  82}, /* 1108 'R' */
    { 0x48c00000, 12,  50}, /* 1109 '2' */
    { 0x48d00000, 12,  85}, /* 1110 'U' */
    { 0xb0800000, 12,  63}, /* 1111 '?' */
    { 0xb0a00000, 12,  79}, /* 1112 'O' */
    { 0xb0b00000, 12,  72}, /* 1113 'H' */
    { 0xb1800000, 12,  69}, /* 1114 'E' */
    { 0x48b00000, 13, 122}, /* 1115 'z' */
    { 0xb0980000, 13,  45}, /* 1116 '-' */
    { 0xb1980000, 13, 115}, /* 1117 's' */
    { 0x48bc0000, 14,  49}, /* 1118 '1' */
    { 0xb0940000, 14,  33}, /* 1119 '!' */
    { 0xb1900000, 14,  80}, /* 1120 'P' */
    { 0x48ba0000, 15, 110}, /* 1121 'n' */
    { 0xb0900000, 15,  75}, /* 1122 'K' */
    { 0xb0920000, 15,  55}, /* 1123 '7' */
    { 0xb1960000, 15,  52}, /* 1124 '4' */
    { 0x48b80000, 16,  58}, /* 1125 ':' */
    { 0xb1940000, 16,  70}, /* 1126 'F' */
    { 0xb1950000, 16,  41}, /* 1127 ')' */
    { 0x48b90000, 17,   1}, /* 1128 '0x01' */
    { 0x48b98000, 17,  98}, /* 1129 'b' */
    /*   47                             */
    { 0x00000000,  2,  97}, /* 1130 'a' */
    { 0x40000000,  2,  44}, /* 1131 ',' */
    { 0x80000000,  3, 114}, /* 1132 'r' */
    { 0xa0000000,  3, 111}, /* 1133 'o' */
    { 0xc0000000,  3, 101}, /* 1134 'e' */
    { 0xe0000000,  4, 105}, /* 1135 'i' */
    { 0xfc000000,  6, 117}, /* 1136 'u' */
    { 0xf2000000,  7,  32}, /* 1137 ' ' */
    { 0xf6000000,  7,  39}, /* 1138 '\'' */
    { 0xf0000000,  8,  46}, /* 1139 '.' */
    { 0xf1000000,  8,  93}, /* 1140 ']' */
    { 0xfa000000,  8, 121}, /* 1141 'y' */
    { 0xf5000000,  9, 119}, /* 1142 'w' */
    { 0xf5800000,  9,  87}, /* 1143 'W' */
    { 0xf8800000,  9,  78}, /* 1144 'N' */
    { 0xf9000000,  9,  67}, /* 1145 'C' */
    { 0xfb800000,  9,  74}, /* 1146 'J' */
    { 0xf4800000, 10, 104}, /* 1147 'h' */
    { 0xf8000000, 10, 116}, /* 1148 't' */
    { 0xf9c00000, 10,  73}, /* 1149 'I' */
    { 0xfb400000, 10,  58}, /* 1150 ':' */
    { 0xf4200000, 11,  77}, /* 1151 'M' */
    { 0xf4400000, 11,  38}, /* 1152 '&' */
    { 0xf4600000, 11,  86}, /* 1153 'V' */
    { 0xf8400000, 11,  45}, /* 1154 '-' */
    { 0xf8600000, 11,  71}, /* 1155 'G' */
    { 0xf9800000, 11,  79}, /* 1156 'O' */
    { 0xf9a00000, 11,  65}, /* 1157 'A' */
    { 0xfb200000, 11,  83}, /* 1158 'S' */
    { 0xf4000000, 12,  70}, /* 1159 'F' */
    { 0xf4c00000, 12,  42}, /* 1160 '*' */
    { 0xf4d00000, 12, 115}, /* 1161 's' */
    { 0xf4f00000, 12, 100}, /* 1162 'd' */
    { 0xfb000000, 12, 118}, /* 1163 'v' */
    { 0xf4e00000, 13, 109}, /* 1164 'm' */
    { 0xf4e80000, 13, 106}, /* 1165 'j' */
    { 0xfb100000, 13,   1}, /* 1166 '0x01' */
    { 0xfb180000, 13,  84}, /* 1167 'T' */
    { 0xf4180000, 14,  57}, /* 1168 '9' */
    { 0xf4100000, 15,  41}, /* 1169 ')' */
    { 0xf4160000, 15,  69}, /* 1170 'E' */
    { 0xf41c0000, 15,  56}, /* 1171 '8' */
    { 0xf41e0000, 15,  55}, /* 1172 '7' */
    { 0xf4120000, 16,  85}, /* 1173 'U' */
    { 0xf4130000, 16,  82}, /* 1174 'R' */
    { 0xf4140000, 16,  66}, /* 1175 'B' */
    { 0xf4150000, 16,  52}, /* 1176 '4' */
    /*   54                             */
    { 0x00000000,  3, 112}, /* 1177 'p' */
    { 0x40000000,  3,  97}, /* 1178 'a' */
    { 0x60000000,  3, 110}, /* 1179 'n' */
    { 0xc0000000,  3, 108}, /* 1180 'l' */
    { 0x90000000,  4, 109}, /* 1181 'm' */
    { 0xa0000000,  4, 120}, /* 1182 'x' */
    { 0xe0000000,  4, 118}, /* 1183 'v' */
    { 0xf0000000,  4, 100}, /* 1184 'd' */
    { 0x20000000,  5, 115}, /* 1185 's' */
    { 0x30000000,  5, 114}, /* 1186 'r' */
    { 0x38000000,  5, 117}, /* 1187 'u' */
    { 0x80000000,  5,  69}, /* 1188 'E' */
    { 0x88000000,  5,  32}, /* 1189 ' ' */
    { 0x28000000,  6,  46}, /* 1190 '.' */
    { 0x2c000000,  6, 105}, /* 1191 'i' */
    { 0xb4000000,  6,  58}, /* 1192 ':' */
    { 0xb0000000,  7, 121}, /* 1193 'y' */
    { 0xb2000000,  7, 116}, /* 1194 't' */
    { 0xb8000000,  7, 103}, /* 1195 'g' */
    { 0xbc000000,  7,  52}, /* 1196 '4' */
    { 0xbb000000,  9, 119}, /* 1197 'w' */
    { 0xbe000000,  9,  99}, /* 1198 'c' */
    { 0xbf000000,  9,  98}, /* 1199 'b' */
    { 0xba800000, 10,  82}, /* 1200 'R' */
    { 0xbac00000, 10,  70}, /* 1201 'F' */
    { 0xbbc00000, 10,  67}, /* 1202 'C' */
    { 0xba200000, 11, 107}, /* 1203 'k' */
    { 0xba400000, 11, 102}, /* 1204 'f' */
    { 0xba600000, 11, 111}, /* 1205 'o' */
    { 0xbb800000, 11,  85}, /* 1206 'U' */
    { 0xbba00000, 11,  76}, /* 1207 'L' */
    { 0xbea00000, 11, 101}, /* 1208 'e' */
    { 0xbec00000, 11,  78}, /* 1209 'N' */
    { 0xbf800000, 11, 104}, /* 1210 'h' */
    { 0xbfa00000, 11,  73}, /* 1211 'I' */
    { 0xbfe00000, 11,  68}, /* 1212 'D' */
    { 0xbe800000, 12,  77}, /* 1213 'M' */
    { 0xbef00000, 12,  39}, /* 1214 '\'' */
    { 0xbfc00000, 12,  50}, /* 1215 '2' */
    { 0xbfd00000, 12,  45}, /* 1216 '-' */
    { 0xba180000, 13, 113}, /* 1217 'q' */
    { 0xbe900000, 13,  79}, /* 1218 'O' */
    { 0xbe980000, 13,  65}, /* 1219 'A' */
    { 0xbee80000, 13,  90}, /* 1220 'Z' */
    { 0xba000000, 14,  87}, /* 1221 'W' */
    { 0xba040000, 14,  83}, /* 1222 'S' */
    { 0xba080000, 14,  72}, /* 1223 'H' */
    { 0xba0c0000, 14,  57}, /* 1224 '9' */
    { 0xba100000, 14,  41}, /* 1225 ')' */
    { 0xbee00000, 14,  84}, /* 1226 'T' */
    { 0xbee40000, 14,  44}, /* 1227 ',' */
    { 0xba140000, 15,  80}, /* 1228 'P' */
    { 0xba160000, 16,   1}, /* 1229 '0x01' */
    { 0xba170000, 16, 122}, /* 1230 'z' */
    /*   36                             */
    { 0x00000000,  2, 111}, /* 1231 'o' */
    { 0x80000000,  2, 114}, /* 1232 'r' */
    { 0x40000000,  3, 101}, /* 1233 'e' */
    { 0xc0000000,  3, 105}, /* 1234 'i' */
    { 0xe0000000,  3,  97}, /* 1235 'a' */
    { 0x60000000,  4, 108}, /* 1236 'l' */
    { 0x70000000,  5, 117}, /* 1237 'u' */
    { 0x78000000,  7,  32}, /* 1238 ' ' */
    { 0x7a000000,  7,  79}, /* 1239 'O' */
    { 0x7d000000,  8,  65}, /* 1240 'A' */
    { 0x7f000000,  8,  66}, /* 1241 'B' */
    { 0x7e000000,  9, 102}, /* 1242 'f' */
    { 0x7c000000, 10,  46}, /* 1243 '.' */
    { 0x7cc00000, 10,  76}, /* 1244 'L' */
    { 0x7c400000, 11,  77}, /* 1245 'M' */
    { 0x7c600000, 11,  69}, /* 1246 'E' */
    { 0x7ca00000, 11,  84}, /* 1247 'T' */
    { 0x7ea00000, 11,  67}, /* 1248 'C' */
    { 0x7ec00000, 11,  87}, /* 1249 'W' */
    { 0x7c800000, 12,  58}, /* 1250 ':' */
    { 0x7e800000, 12,  85}, /* 1251 'U' */
    { 0x7e900000, 12,  39}, /* 1252 '\'' */
    { 0x7ef00000, 12,  49}, /* 1253 '1' */
    { 0x7c900000, 13, 121}, /* 1254 'y' */
    { 0x7c980000, 13,  44}, /* 1255 ',' */
    { 0x7ee00000, 14,  45}, /* 1256 '-' */
    { 0x7ee80000, 14,  73}, /* 1257 'I' */
    { 0x7ee60000, 15, 104}, /* 1258 'h' */
    { 0x7ee40000, 16,  42}, /* 1259 '*' */
    { 0x7ee50000, 16,  75}, /* 1260 'K' */
    { 0x7eec0000, 16,  70}, /* 1261 'F' */
    { 0x7eed0000, 16,   1}, /* 1262 '0x01' */
    { 0x7eee0000, 17,  88}, /* 1263 'X' */
    { 0x7eee8000, 17,  82}, /* 1264 'R' */
    { 0x7eef0000, 17,  59}, /* 1265 ';' */
    { 0x7eef8000, 17,  52}, /* 1266 '4' */
    /*   37                             */
    { 0x00000000,  2, 114}, /* 1267 'r' */
    { 0x40000000,  2,  97}, /* 1268 'a' */
    { 0xa0000000,  3, 111}, /* 1269 'o' */
    { 0xc0000000,  3, 101}, /* 1270 'e' */
    { 0x90000000,  4, 117}, /* 1271 'u' */
    { 0xf0000000,  4, 105}, /* 1272 'i' */
    { 0x88000000,  5, 110}, /* 1273 'n' */
    { 0xe8000000,  5, 108}, /* 1274 'l' */
    { 0x80000000,  6,  88}, /* 1275 'X' */
    { 0xe0000000,  7, 121}, /* 1276 'y' */
    { 0xe2000000,  7,  32}, /* 1277 ' ' */
    { 0xe6000000,  7, 119}, /* 1278 'w' */
    { 0x85000000,  8,  52}, /* 1279 '4' */
    { 0x87000000,  8,  80}, /* 1280 'P' */
    { 0xe5000000,  8, 104}, /* 1281 'h' */
    { 0x84000000,  9,  45}, /* 1282 '-' */
    { 0x86000000,  9,  67}, /* 1283 'C' */
    { 0xe4800000,  9,  77}, /* 1284 'M' */
    { 0x86800000, 10,  66}, /* 1285 'B' */
    { 0xe4000000, 10,  46}, /* 1286 '.' */
    { 0xe4400000, 10,  73}, /* 1287 'I' */
    { 0x84800000, 11,  59}, /* 1288 ';' */
    { 0x84a00000, 11,  44}, /* 1289 ',' */
    { 0x84c00000, 11,  65}, /* 1290 'A' */
    { 0x84e00000, 11,  78}, /* 1291 'N' */
    { 0x86e00000, 11,  58}, /* 1292 ':' */
    { 0x86c00000, 13,  79}, /* 1293 'O' */
    { 0x86c80000, 13,  76}, /* 1294 'L' */
    { 0x86d80000, 14,  98}, /* 1295 'b' */
    { 0x86d00000, 15,  75}, /* 1296 'K' */
    { 0x86d20000, 15,  50}, /* 1297 '2' */
    { 0x86d40000, 15,  49}, /* 1298 '1' */
    { 0x86d60000, 15,  39}, /* 1299 '\'' */
    { 0x86dc0000, 16,   1}, /* 1300 '0x01' */
    { 0x86dd0000, 16, 109}, /* 1301 'm' */
    { 0x86de0000, 16,  84}, /* 1302 'T' */
    { 0x86df0000, 16,  83}, /* 1303 'S' */
    /*   32                             */
    { 0x00000000,  2, 101}, /* 1304 'e' */
    { 0x40000000,  2,  97}, /* 1305 'a' */
    { 0x80000000,  2, 111}, /* 1306 'o' */
    { 0xc0000000,  3, 105}, /* 1307 'i' */
    { 0xf0000000,  4, 117}, /* 1308 'u' */
    { 0xe0000000,  5,  82}, /* 1309 'R' */
    { 0xe8000000,  7,  80}, /* 1310 'P' */
    { 0xec000000,  7, 121}, /* 1311 'y' */
    { 0xeb000000,  8,  73}, /* 1312 'I' */
    { 0xee000000,  9,  81}, /* 1313 'Q' */
    { 0xee800000,  9,  77}, /* 1314 'M' */
    { 0xef000000,  9,  65}, /* 1315 'A' */
    { 0xef800000,  9,  32}, /* 1316 ' ' */
    { 0xea000000, 10,  83}, /* 1317 'S' */
    { 0xea400000, 10,  46}, /* 1318 '.' */
    { 0xeaa00000, 11,  71}, /* 1319 'G' */
    { 0xeac00000, 11,  41}, /* 1320 ')' */
    { 0xea900000, 12,  69}, /* 1321 'E' */
    { 0xeae00000, 13,  79}, /* 1322 'O' */
    { 0xeae80000, 13,  76}, /* 1323 'L' */
    { 0xea800000, 14,  49}, /* 1324 '1' */
    { 0xea840000, 14,  38}, /* 1325 '&' */
    { 0xeaf00000, 14,  72}, /* 1326 'H' */
    { 0xeaf40000, 14, 119}, /* 1327 'w' */
    { 0xeaf80000, 14, 118}, /* 1328 'v' */
    { 0xea880000, 15,  88}, /* 1329 'X' */
    { 0xea8a0000, 15,  87}, /* 1330 'W' */
    { 0xea8c0000, 15,  68}, /* 1331 'D' */
    { 0xeafc0000, 15, 115}, /* 1332 's' */
    { 0xeafe0000, 15,  70}, /* 1333 'F' */
    { 0xea8e0000, 16,   1}, /* 1334 '0x01' */
    { 0xea8f0000, 16, 114}, /* 1335 'r' */
    /*   52                             */
    { 0x00000000,  1, 110}, /* 1336 'n' */
    { 0xc0000000,  3, 116}, /* 1337 't' */
    { 0x90000000,  4, 115}, /* 1338 's' */
    { 0xf0000000,  4, 114}, /* 1339 'r' */
    { 0x80000000,  5,  84}, /* 1340 'T' */
    { 0xa8000000,  5,  97}, /* 1341 'a' */
    { 0xe0000000,  5,  32}, /* 1342 ' ' */
    { 0x88000000,  6,  99}, /* 1343 'c' */
    { 0xa4000000,  6, 109}, /* 1344 'm' */
    { 0xb0000000,  6, 122}, /* 1345 'z' */
    { 0xb4000000,  6,  46}, /* 1346 '.' */
    { 0xbc000000,  6, 100}, /* 1347 'd' */
    { 0xec000000,  6,  73}, /* 1348 'I' */
    { 0x8e000000,  7,  39}, /* 1349 '\'' */
    { 0xba000000,  7, 108}, /* 1350 'l' */
    { 0xa0000000,  8,  65}, /* 1351 'A' */
    { 0xa1000000,  8, 118}, /* 1352 'v' */
    { 0xb9000000,  8,  86}, /* 1353 'V' */
    { 0xe9000000,  8, 102}, /* 1354 'f' */
    { 0xeb000000,  8, 111}, /* 1355 'o' */
    { 0x8c800000,  9,  67}, /* 1356 'C' */
    { 0x8d000000,  9,  80}, /* 1357 'P' */
    { 0xa2000000,  9, 119}, /* 1358 'w' */
    { 0xa3000000,  9,  58}, /* 1359 ':' */
    { 0xb8000000,  9, 112}, /* 1360 'p' */
    { 0xe8000000,  9,  82}, /* 1361 'R' */
    { 0xe8800000,  9,  44}, /* 1362 ',' */
    { 0xea800000,  9,  89}, /* 1363 'Y' */
    { 0x8c000000, 10,  69}, /* 1364 'E' */
    { 0x8c400000, 10,  54}, /* 1365 '6' */
    { 0x8dc00000, 10, 113}, /* 1366 'q' */
    { 0xa2c00000, 10, 121}, /* 1367 'y' */
    { 0xa3800000, 10,  77}, /* 1368 'M' */
    { 0xea000000, 10, 103}, /* 1369 'g' */
    { 0xea400000, 10,  68}, /* 1370 'D' */
    { 0x8d800000, 11, 101}, /* 1371 'e' */
    { 0x8da00000, 11,  53}, /* 1372 '5' */
    { 0xa2800000, 11,  83}, /* 1373 'S' */
    { 0xa2a00000, 11,  57}, /* 1374 '9' */
    { 0xa3e00000, 11,  70}, /* 1375 'F' */
    { 0xb8800000, 11,  98}, /* 1376 'b' */
    { 0xb8c00000, 11,  45}, /* 1377 '-' */
    { 0xb8e00000, 11,  76}, /* 1378 'L' */
    { 0xa3c00000, 12,  66}, /* 1379 'B' */
    { 0xa3d00000, 12,  41}, /* 1380 ')' */
    { 0xb8a00000, 13,  78}, /* 1381 'N' */
    { 0xb8a80000, 13,  81}, /* 1382 'Q' */
    { 0xb8b00000, 13, 107}, /* 1383 'k' */
    { 0xb8bc0000, 14,  79}, /* 1384 'O' */
    { 0xb8ba0000, 15,  63}, /* 1385 '?' */
    { 0xb8b80000, 16,   1}, /* 1386 '0x01' */
    { 0xb8b90000, 16, 104}, /* 1387 'h' */
    /*   29                             */
    { 0x00000000,  2, 101}, /* 1388 'e' */
    { 0x80000000,  2,  97}, /* 1389 'a' */
    { 0xc0000000,  2, 111}, /* 1390 'o' */
    { 0x60000000,  3, 117}, /* 1391 'u' */
    { 0x50000000,  4, 105}, /* 1392 'i' */
    { 0x40000000,  5,  32}, /* 1393 ' ' */
    { 0x4c000000,  7,  68}, /* 1394 'D' */
    { 0x4e000000,  7,  46}, /* 1395 '.' */
    { 0x49000000,  8, 114}, /* 1396 'r' */
    { 0x4b000000,  9, 115}, /* 1397 's' */
    { 0x48000000, 10,  77}, /* 1398 'M' */
    { 0x48800000, 10,  74}, /* 1399 'J' */
    { 0x48c00000, 10,  44}, /* 1400 ',' */
    { 0x4a000000, 10,  66}, /* 1401 'B' */
    { 0x4a400000, 10,  45}, /* 1402 '-' */
    { 0x4a800000, 10,  86}, /* 1403 'V' */
    { 0x4b800000, 10,  75}, /* 1404 'K' */
    { 0x4bc00000, 10,  84}, /* 1405 'T' */
    { 0x48400000, 11,  67}, /* 1406 'C' */
    { 0x4ac00000, 11, 110}, /* 1407 'n' */
    { 0x48600000, 12,  33}, /* 1408 '!' */
    { 0x48700000, 13, 119}, /* 1409 'w' */
    { 0x48780000, 13,  82}, /* 1410 'R' */
    { 0x4ae00000, 13,  70}, /* 1411 'F' */
    { 0x4af00000, 13,  55}, /* 1412 '7' */
    { 0x4af80000, 13,  39}, /* 1413 '\'' */
    { 0x4ae80000, 14,  71}, /* 1414 'G' */
    { 0x4aec0000, 15,   1}, /* 1415 '0x01' */
    { 0x4aee0000, 15,  76}, /* 1416 'L' */
    /*   40                             */
    { 0x00000000,  2, 105}, /* 1417 'i' */
    { 0x40000000,  2,  97}, /* 1418 'a' */
    { 0x80000000,  2, 101}, /* 1419 'e' */
    { 0xf0000000,  4, 121}, /* 1420 'y' */
    { 0xc0000000,  5, 110}, /* 1421 'n' */
    { 0xc8000000,  5, 111}, /* 1422 'o' */
    { 0xd8000000,  5, 114}, /* 1423 'r' */
    { 0xe8000000,  5,  32}, /* 1424 ' ' */
    { 0xd0000000,  6,  39}, /* 1425 '\'' */
    { 0xe0000000,  6, 117}, /* 1426 'u' */
    { 0xd6000000,  7,  46}, /* 1427 '.' */
    { 0xe6000000,  7, 108}, /* 1428 'l' */
    { 0xe5000000,  8, 104}, /* 1429 'h' */
    { 0xd4000000,  9,  84}, /* 1430 'T' */
    { 0xd5800000,  9,  71}, /* 1431 'G' */
    { 0xe4800000,  9,  44}, /* 1432 ',' */
    { 0xd5000000, 10, 119}, /* 1433 'w' */
    { 0xd4800000, 11,  53}, /* 1434 '5' */
    { 0xd4a00000, 11,  52}, /* 1435 '4' */
    { 0xd4c00000, 11,  65}, /* 1436 'A' */
    { 0xd5400000, 11,  51}, /* 1437 '3' */
    { 0xe4000000, 11,  50}, /* 1438 '2' */
    { 0xe4600000, 11,  77}, /* 1439 'M' */
    { 0xd4e00000, 12,  45}, /* 1440 '-' */
    { 0xd5600000, 12,  58}, /* 1441 ':' */
    { 0xd5700000, 12,  33}, /* 1442 '!' */
    { 0xe4200000, 12,  41}, /* 1443 ')' */
    { 0xe4500000, 12,  83}, /* 1444 'S' */
    { 0xd4f00000, 13,  86}, /* 1445 'V' */
    { 0xe4300000, 13,  54}, /* 1446 '6' */
    { 0xe4380000, 13,  49}, /* 1447 '1' */
    { 0xd4fc0000, 14,  76}, /* 1448 'L' */
    { 0xe4400000, 14,  73}, /* 1449 'I' */
    { 0xe4440000, 14,  63}, /* 1450 '?' */
    { 0xd4f80000, 15,   1}, /* 1451 '0x01' */
    { 0xd4fa0000, 15, 118}, /* 1452 'v' */
    { 0xe4480000, 15, 107}, /* 1453 'k' */
    { 0xe44a0000, 15,  89}, /* 1454 'Y' */
    { 0xe44c0000, 15,  78}, /* 1455 'N' */
    { 0xe44e0000, 15,  69}, /* 1456 'E' */
    /*   36                             */
    { 0x00000000,  2, 105}, /* 1457 'i' */
    { 0x80000000,  2, 111}, /* 1458 'o' */
    { 0x60000000,  3,  93}, /* 1459 ']' */
    { 0xc0000000,  3, 101}, /* 1460 'e' */
    { 0xe0000000,  3,  97}, /* 1461 'a' */
    { 0x40000000,  4, 117}, /* 1462 'u' */
    { 0x50000000,  6, 108}, /* 1463 'l' */
    { 0x54000000,  6,  65}, /* 1464 'A' */
    { 0x5c000000,  6, 121}, /* 1465 'y' */
    { 0x5b000000,  8,  32}, /* 1466 ' ' */
    { 0x58000000,  9,  73}, /* 1467 'I' */
    { 0x58800000,  9,  82}, /* 1468 'R' */
    { 0x5a800000,  9,  44}, /* 1469 ',' */
    { 0x59000000, 10,  79}, /* 1470 'O' */
    { 0x59400000, 10,  39}, /* 1471 '\'' */
    { 0x59a00000, 11,  46}, /* 1472 '.' */
    { 0x59c00000, 11,  67}, /* 1473 'C' */
    { 0x59e00000, 11,  89}, /* 1474 'Y' */
    { 0x5a200000, 11,  70}, /* 1475 'F' */
    { 0x5a400000, 11,  76}, /* 1476 'L' */
    { 0x59900000, 12, 116}, /* 1477 't' */
    { 0x5a100000, 12,  71}, /* 1478 'G' */
    { 0x5a700000, 12,  83}, /* 1479 'S' */
    { 0x59880000, 13, 104}, /* 1480 'h' */
    { 0x5a080000, 13,  87}, /* 1481 'W' */
    { 0x5a600000, 13,  74}, /* 1482 'J' */
    { 0x59800000, 14,  69}, /* 1483 'E' */
    { 0x5a040000, 14,  34}, /* 1484 '\"' */
    { 0x5a6c0000, 14,   0}, /* 1485 '0x00' */
    { 0x59840000, 15,  84}, /* 1486 'T' */
    { 0x59860000, 15,  80}, /* 1487 'P' */
    { 0x5a000000, 15,  68}, /* 1488 'D' */
    { 0x5a020000, 15,  55}, /* 1489 '7' */
    { 0x5a6a0000, 15, 106}, /* 1490 'j' */
    { 0x5a680000, 16,   1}, /* 1491 '0x01' */
    { 0x5a690000, 16,  85}, /* 1492 'U' */
    /*   47                             */
    { 0x00000000,  2, 111}, /* 1493 'o' */
    { 0xc0000000,  2,  97}, /* 1494 'a' */
    { 0x60000000,  3, 101}, /* 1495 'e' */
    { 0xa0000000,  3, 105}, /* 1496 'i' */
    { 0x50000000,  4,  99}, /* 1497 'c' */
    { 0x90000000,  4, 117}, /* 1498 'u' */
    { 0x48000000,  5, 121}, /* 1499 'y' */
    { 0x88000000,  5, 114}, /* 1500 'r' */
    { 0x80000000,  6,  83}, /* 1501 'S' */
    { 0x84000000,  6,  80}, /* 1502 'P' */
    { 0x42000000,  7,  32}, /* 1503 ' ' */
    { 0x41000000,  8,  67}, /* 1504 'C' */
    { 0x46000000,  8,  70}, /* 1505 'F' */
    { 0x40000000,  9,  49}, /* 1506 '1' */
    { 0x44000000,  9, 104}, /* 1507 'h' */
    { 0x45000000,  9,  73}, /* 1508 'I' */
    { 0x47000000,  9,  84}, /* 1509 'T' */
    { 0x47800000,  9,  88}, /* 1510 'X' */
    { 0x40800000, 10,  65}, /* 1511 'A' */
    { 0x44800000, 10, 122}, /* 1512 'z' */
    { 0x44c00000, 11, 102}, /* 1513 'f' */
    { 0x45c00000, 11,  46}, /* 1514 '.' */
    { 0x40c00000, 12,  90}, /* 1515 'Z' */
    { 0x40d00000, 12,  75}, /* 1516 'K' */
    { 0x40e00000, 12,  66}, /* 1517 'B' */
    { 0x44f00000, 12, 115}, /* 1518 's' */
    { 0x45800000, 12,  82}, /* 1519 'R' */
    { 0x45900000, 12,  87}, /* 1520 'W' */
    { 0x45a00000, 12,  79}, /* 1521 'O' */
    { 0x40f00000, 13,  44}, /* 1522 ',' */
    { 0x40f80000, 13,  39}, /* 1523 '\'' */
    { 0x45b80000, 13,  68}, /* 1524 'D' */
    { 0x45e00000, 13,  52}, /* 1525 '4' */
    { 0x45f00000, 13,  69}, /* 1526 'E' */
    { 0x44e00000, 14, 109}, /* 1527 'm' */
    { 0x44e40000, 14,  74}, /* 1528 'J' */
    { 0x44e80000, 14,   1}, /* 1529 '0x01' */
    { 0x45e80000, 14, 108}, /* 1530 'l' */
    { 0x45ec0000, 14,  45}, /* 1531 '-' */
    { 0x45f80000, 14,  41}, /* 1532 ')' */
    { 0x45fc0000, 14, 119}, /* 1533 'w' */
    { 0x44ec0000, 15, 116}, /* 1534 't' */
    { 0x44ee0000, 15,  86}, /* 1535 'V' */
    { 0x45b00000, 15,  81}, /* 1536 'Q' */
    { 0x45b20000, 15,  78}, /* 1537 'N' */
    { 0x45b40000, 15,  54}, /* 1538 '6' */
    { 0x45b60000, 15,  50}, /* 1539 '2' */
    /*   35                             */
    { 0x00000000,  2,  97}, /* 1540 'a' */
    { 0x80000000,  2, 111}, /* 1541 'o' */
    { 0xc0000000,  2, 101}, /* 1542 'e' */
    { 0x60000000,  3, 105}, /* 1543 'i' */
    { 0x50000000,  5, 117}, /* 1544 'u' */
    { 0x58000000,  5,  69}, /* 1545 'E' */
    { 0x48000000,  6,  65}, /* 1546 'A' */
    { 0x44000000,  7, 121}, /* 1547 'y' */
    { 0x4c000000,  7, 103}, /* 1548 'g' */
    { 0x41000000,  8,  66}, /* 1549 'B' */
    { 0x42000000,  8,  73}, /* 1550 'I' */
    { 0x46000000,  8,  70}, /* 1551 'F' */
    { 0x47000000,  8,  32}, /* 1552 ' ' */
    { 0x40000000,  9,  41}, /* 1553 ')' */
    { 0x40800000,  9,  39}, /* 1554 '\'' */
    { 0x43800000,  9,  89}, /* 1555 'Y' */
    { 0x4e000000,  9,  76}, /* 1556 'L' */
    { 0x4e800000,  9,  72}, /* 1557 'H' */
    { 0x43000000, 10,  67}, /* 1558 'C' */
    { 0x4f000000, 10,  87}, /* 1559 'W' */
    { 0x4f800000, 10,  78}, /* 1560 'N' */
    { 0x4fc00000, 10,  84}, /* 1561 'T' */
    { 0x43500000, 12,  77}, /* 1562 'M' */
    { 0x43700000, 12,  79}, /* 1563 'O' */
    { 0x4f400000, 12,  44}, /* 1564 ',' */
    { 0x4f500000, 12,  74}, /* 1565 'J' */
    { 0x4f700000, 12, 104}, /* 1566 'h' */
    { 0x43400000, 13,  68}, /* 1567 'D' */
    { 0x43480000, 13,  46}, /* 1568 '.' */
    { 0x4f600000, 13,  88}, /* 1569 'X' */
    { 0x4f680000, 13,   1}, /* 1570 '0x01' */
    { 0x43600000, 14, 107}, /* 1571 'k' */
    { 0x43640000, 14,  90}, /* 1572 'Z' */
    { 0x43680000, 14,  81}, /* 1573 'Q' */
    { 0x436c0000, 14,  71}, /* 1574 'G' */
    /*   59                             */
    { 0xe0000000,  3, 110}, /* 1575 'n' */
    { 0x00000000,  4, 115}, /* 1576 's' */
    { 0x10000000,  4,  78}, /* 1577 'N' */
    { 0x30000000,  4, 122}, /* 1578 'z' */
    { 0x40000000,  4, 114}, /* 1579 'r' */
    { 0x80000000,  4, 117}, /* 1580 'u' */
    { 0xa0000000,  4, 112}, /* 1581 'p' */
    { 0xb0000000,  4, 108}, /* 1582 'l' */
    { 0xd0000000,  4,  39}, /* 1583 '\'' */
    { 0x28000000,  5,  85}, /* 1584 'U' */
    { 0x58000000,  5, 104}, /* 1585 'h' */
    { 0x70000000,  5, 102}, /* 1586 'f' */
    { 0x78000000,  5, 119}, /* 1587 'w' */
    { 0x90000000,  5,  46}, /* 1588 '.' */
    { 0xc0000000,  5,  32}, /* 1589 ' ' */
    { 0x20000000,  6, 111}, /* 1590 'o' */
    { 0x24000000,  6,  74}, /* 1591 'J' */
    { 0x50000000,  6,  98}, /* 1592 'b' */
    { 0x54000000,  6, 109}, /* 1593 'm' */
    { 0x64000000,  6,  72}, /* 1594 'H' */
    { 0x68000000,  6,  79}, /* 1595 'O' */
    { 0x6c000000,  6, 118}, /* 1596 'v' */
    { 0x9c000000,  6,  99}, /* 1597 'c' */
    { 0xcc000000,  6, 120}, /* 1598 'x' */
    { 0x9a000000,  7, 100}, /* 1599 'd' */
    { 0xc8000000,  7,  97}, /* 1600 'a' */
    { 0x61000000,  8, 116}, /* 1601 't' */
    { 0x62000000,  8, 107}, /* 1602 'k' */
    { 0x98000000,  8, 103}, /* 1603 'g' */
    { 0xca000000,  8,  82}, /* 1604 'R' */
    { 0x60000000,  9,  75}, /* 1605 'K' */
    { 0x99000000,  9, 105}, /* 1606 'i' */
    { 0xcb800000,  9,  86}, /* 1607 'V' */
    { 0x60c00000, 10,  50}, /* 1608 '2' */
    { 0x63400000, 10,  83}, /* 1609 'S' */
    { 0x63800000, 10,  66}, /* 1610 'B' */
    { 0x99800000, 10,  87}, /* 1611 'W' */
    { 0x99c00000, 10,  45}, /* 1612 '-' */
    { 0x60800000, 11,  44}, /* 1613 ',' */
    { 0x63000000, 11, 106}, /* 1614 'j' */
    { 0x63200000, 11,  76}, /* 1615 'L' */
    { 0x63e00000, 11,  73}, /* 1616 'I' */
    { 0xcb200000, 11,  65}, /* 1617 'A' */
    { 0xcb600000, 11,  67}, /* 1618 'C' */
    { 0x60a00000, 12,  58}, /* 1619 ':' */
    { 0x63c00000, 12, 101}, /* 1620 'e' */
    { 0xcb000000, 12, 121}, /* 1621 'y' */
    { 0xcb100000, 12,  47}, /* 1622 '/' */
    { 0xcb400000, 12,  77}, /* 1623 'M' */
    { 0x60b00000, 13,  33}, /* 1624 '!' */
    { 0x63d80000, 13,  80}, /* 1625 'P' */
    { 0xcb580000, 13,  70}, /* 1626 'F' */
    { 0x60b80000, 14,  69}, /* 1627 'E' */
    { 0x60bc0000, 14,  68}, /* 1628 'D' */
    { 0x63d00000, 14,  56}, /* 1629 '8' */
    { 0x63d40000, 14,  52}, /* 1630 '4' */
    { 0xcb540000, 14,  84}, /* 1631 'T' */
    { 0xcb500000, 15,   1}, /* 1632 '0x01' */
    { 0xcb520000, 15, 113}, /* 1633 'q' */
    /*   39                             */
    { 0xc0000000,  2,  97}, /* 1634 'a' */
    { 0x00000000,  3, 105}, /* 1635 'i' */
    { 0x40000000,  3, 101}, /* 1636 'e' */
    { 0x60000000,  3, 108}, /* 1637 'l' */
    { 0x80000000,  3, 111}, /* 1638 'o' */
    { 0xa0000000,  3, 114}, /* 1639 'r' */
    { 0x20000000,  4, 104}, /* 1640 'h' */
    { 0x3c000000,  6, 117}, /* 1641 'u' */
    { 0x34000000,  7,  67}, /* 1642 'C' */
    { 0x38000000,  7,  46}, /* 1643 '.' */
    { 0x3a000000,  7,  32}, /* 1644 ' ' */
    { 0x30000000,  8,  66}, /* 1645 'B' */
    { 0x32000000,  8,  68}, /* 1646 'D' */
    { 0x37000000,  8, 115}, /* 1647 's' */
    { 0x31800000,  9,  79}, /* 1648 'O' */
    { 0x33800000,  9,  44}, /* 1649 ',' */
    { 0x31400000, 10, 121}, /* 1650 'y' */
    { 0x33000000, 10,  77}, /* 1651 'M' */
    { 0x36000000, 10,  69}, /* 1652 'E' */
    { 0x36400000, 10,  39}, /* 1653 '\'' */
    { 0x36c00000, 10,  51}, /* 1654 '3' */
    { 0x31200000, 11,  84}, /* 1655 'T' */
    { 0x31000000, 12,  76}, /* 1656 'L' */
    { 0x31100000, 12,  71}, /* 1657 'G' */
    { 0x33400000, 12,  42}, /* 1658 '*' */
    { 0x33600000, 12,  65}, /* 1659 'A' */
    { 0x33700000, 12,  83}, /* 1660 'S' */
    { 0x36800000, 12, 119}, /* 1661 'w' */
    { 0x36900000, 12,  70}, /* 1662 'F' */
    { 0x33580000, 13,  74}, /* 1663 'J' */
    { 0x36a00000, 13, 102}, /* 1664 'f' */
    { 0x36a80000, 13,  82}, /* 1665 'R' */
    { 0x36b80000, 13, 116}, /* 1666 't' */
    { 0x33500000, 14,  86}, /* 1667 'V' */
    { 0x36b00000, 14,  89}, /* 1668 'Y' */
    { 0x36b40000, 14,  73}, /* 1669 'I' */
    { 0x33560000, 15,  38}, /* 1670 '&' */
    { 0x33540000, 16,   1}, /* 1671 '0x01' */
    { 0x33550000, 16,  41}, /* 1672 ')' */
    /*   13                             */
    { 0x80000000,  1, 117}, /* 1673 'u' */
    { 0x00000000,  2,  86}, /* 1674 'V' */
    { 0x60000000,  3,  32}, /* 1675 ' ' */
    { 0x50000000,  4,  46}, /* 1676 '.' */
    { 0x40000000,  5,  97}, /* 1677 'a' */
    { 0x48000000,  7, 119}, /* 1678 'w' */
    { 0x4c000000,  7,  69}, /* 1679 'E' */
    { 0x4e000000,  7,  67}, /* 1680 'C' */
    { 0x4b000000,  8,  38}, /* 1681 '&' */
    { 0x4a800000,  9,  39}, /* 1682 '\'' */
    { 0x4a000000, 10,  84}, /* 1683 'T' */
    { 0x4a400000, 11,   1}, /* 1684 '0x01' */
    { 0x4a600000, 11, 115}, /* 1685 's' */
    /*   37                             */
    { 0x40000000,  2,  97}, /* 1686 'a' */
    { 0xc0000000,  2, 111}, /* 1687 'o' */
    { 0x80000000,  3, 105}, /* 1688 'i' */
    { 0xa0000000,  3, 101}, /* 1689 'e' */
    { 0x00000000,  4, 112}, /* 1690 'p' */
    { 0x30000000,  4, 117}, /* 1691 'u' */
    { 0x18000000,  5,  69}, /* 1692 'E' */
    { 0x20000000,  5, 104}, /* 1693 'h' */
    { 0x10000000,  6,  32}, /* 1694 ' ' */
    { 0x28000000,  6, 121}, /* 1695 'y' */
    { 0x14000000,  8,  68}, /* 1696 'D' */
    { 0x16000000,  8,  46}, /* 1697 '.' */
    { 0x2c000000,  8,  84}, /* 1698 'T' */
    { 0x2e000000,  8,  83}, /* 1699 'S' */
    { 0x15000000,  9,  70}, /* 1700 'F' */
    { 0x2d000000,  9,  66}, /* 1701 'B' */
    { 0x2d800000,  9, 110}, /* 1702 'n' */
    { 0x2f800000,  9,  65}, /* 1703 'A' */
    { 0x15800000, 10, 119}, /* 1704 'w' */
    { 0x17000000, 10,  78}, /* 1705 'N' */
    { 0x17400000, 10,  38}, /* 1706 '&' */
    { 0x17800000, 10,  86}, /* 1707 'V' */
    { 0x17c00000, 10,  72}, /* 1708 'H' */
    { 0x2f400000, 10,  39}, /* 1709 '\'' */
    { 0x15c00000, 11, 116}, /* 1710 't' */
    { 0x15e00000, 11,  73}, /* 1711 'I' */
    { 0x2f000000, 12,  67}, /* 1712 'C' */
    { 0x2f200000, 12,  79}, /* 1713 'O' */
    { 0x2f100000, 14,  44}, /* 1714 ',' */
    { 0x2f140000, 14, 115}, /* 1715 's' */
    { 0x2f180000, 14,  85}, /* 1716 'U' */
    { 0x2f1c0000, 14,  77}, /* 1717 'M' */
    { 0x2f300000, 14,  45}, /* 1718 '-' */
    { 0x2f340000, 14,   1}, /* 1719 '0x01' */
    { 0x2f3c0000, 14,  58}, /* 1720 ':' */
    { 0x2f380000, 15,  50}, /* 1721 '2' */
    { 0x2f3a0000, 15,  82}, /* 1722 'R' */
    /*   59                             */
    { 0x80000000,  1,  93}, /* 1723 ']' */
    { 0x00000000,  4,  97}, /* 1724 'a' */
    { 0x20000000,  4, 104}, /* 1725 'h' */
    { 0x70000000,  4, 116}, /* 1726 't' */
    { 0x10000000,  5, 112}, /* 1727 'p' */
    { 0x18000000,  5,  44}, /* 1728 ',' */
    { 0x30000000,  5,  76}, /* 1729 'L' */
    { 0x40000000,  5, 105}, /* 1730 'i' */
    { 0x48000000,  5, 117}, /* 1731 'u' */
    { 0x50000000,  5, 111}, /* 1732 'o' */
    { 0x58000000,  5,  99}, /* 1733 'c' */
    { 0x60000000,  5, 101}, /* 1734 'e' */
    { 0x3c000000,  6, 107}, /* 1735 'k' */
    { 0x38000000,  7, 119}, /* 1736 'w' */
    { 0x6a000000,  7,  32}, /* 1737 ' ' */
    { 0x6e000000,  7, 109}, /* 1738 'm' */
    { 0x3a000000,  8, 113}, /* 1739 'q' */
    { 0x68000000,  8,  77}, /* 1740 'M' */
    { 0x69000000,  8, 110}, /* 1741 'n' */
    { 0x6c000000,  8, 108}, /* 1742 'l' */
    { 0x3b000000,  9,  80}, /* 1743 'P' */
    { 0x6d800000,  9, 121}, /* 1744 'y' */
    { 0x3b800000, 10,  65}, /* 1745 'A' */
    { 0x6d200000, 11,  46}, /* 1746 '.' */
    { 0x3bd00000, 12, 114}, /* 1747 'r' */
    { 0x3be00000, 12,  83}, /* 1748 'S' */
    { 0x3bf00000, 12,  87}, /* 1749 'W' */
    { 0x6d000000, 12,  67}, /* 1750 'C' */
    { 0x6d500000, 12,  69}, /* 1751 'E' */
    { 0x6d600000, 12, 118}, /* 1752 'v' */
    { 0x6d180000, 13,   1}, /* 1753 '0x01' */
    { 0x6d400000, 13,  73}, /* 1754 'I' */
    { 0x6d780000, 13, 103}, /* 1755 'g' */
    { 0x3bc00000, 14,  42}, /* 1756 '*' */
    { 0x3bc80000, 14,  52}, /* 1757 '4' */
    { 0x3bcc0000, 14,  49}, /* 1758 '1' */
    { 0x6d100000, 14,  79}, /* 1759 'O' */
    { 0x6d480000, 14,   0}, /* 1760 '0x00' */
    { 0x6d700000, 14, 122}, /* 1761 'z' */
    { 0x6d160000, 15,  66}, /* 1762 'B' */
    { 0x6d740000, 15,  72}, /* 1763 'H' */
    { 0x6d760000, 15,  84}, /* 1764 'T' */
    { 0x3bc40000, 16,  71}, /* 1765 'G' */
    { 0x3bc60000, 16, 125}, /* 1766 '}' */
    { 0x3bc70000, 16,  68}, /* 1767 'D' */
    { 0x6d140000, 16,  45}, /* 1768 '-' */
    { 0x6d4c0000, 16,  51}, /* 1769 '3' */
    { 0x6d4d0000, 16,  50}, /* 1770 '2' */
    { 0x3bc50000, 17,  39}, /* 1771 '\'' */
    { 0x3bc58000, 17,  63}, /* 1772 '?' */
    { 0x6d4e8000, 17, 115}, /* 1773 's' */
    { 0x6d4f0000, 17, 106}, /* 1774 'j' */
    { 0x6d4f8000, 17,  98}, /* 1775 'b' */
    { 0x6d150000, 18,  82}, /* 1776 'R' */
    { 0x6d154000, 18,  75}, /* 1777 'K' */
    { 0x6d158000, 18,  74}, /* 1778 'J' */
    { 0x6d15c000, 18,  70}, /* 1779 'F' */
    { 0x6d4e0000, 18,  58}, /* 1780 ':' */
    { 0x6d4e4000, 18,  41}, /* 1781 ')' */
    /*   44                             */
    { 0x00000000,  1, 104}, /* 1782 'h' */
    { 0x80000000,  3, 111}, /* 1783 'o' */
    { 0xa0000000,  4,  86}, /* 1784 'V' */
    { 0xb0000000,  4, 119}, /* 1785 'w' */
    { 0xc0000000,  4, 114}, /* 1786 'r' */
    { 0xf0000000,  4, 101}, /* 1787 'e' */
    { 0xd0000000,  5,  97}, /* 1788 'a' */
    { 0xd8000000,  5, 105}, /* 1789 'i' */
    { 0xe0000000,  5, 117}, /* 1790 'u' */
    { 0xe8000000,  7,  72}, /* 1791 'H' */
    { 0xec000000,  7,  87}, /* 1792 'W' */
    { 0xea000000,  8,  32}, /* 1793 ' ' */
    { 0xeb000000,  8, 121}, /* 1794 'y' */
    { 0xee800000,  9,  77}, /* 1795 'M' */
    { 0xef800000,  9, 120}, /* 1796 'x' */
    { 0xee000000, 10,  83}, /* 1797 'S' */
    { 0xee400000, 11,  65}, /* 1798 'A' */
    { 0xef200000, 11, 115}, /* 1799 's' */
    { 0xef600000, 11,  74}, /* 1800 'J' */
    { 0xee700000, 12,  88}, /* 1801 'X' */
    { 0xef000000, 12,  46}, /* 1802 '.' */
    { 0xee680000, 13,  45}, /* 1803 '-' */
    { 0xef180000, 13,  76}, /* 1804 'L' */
    { 0xef400000, 13,  67}, /* 1805 'C' */
    { 0xef580000, 13,  99}, /* 1806 'c' */
    { 0xee600000, 14,  84}, /* 1807 'T' */
    { 0xee640000, 14,  85}, /* 1808 'U' */
    { 0xef140000, 14,  52}, /* 1809 '4' */
    { 0xef480000, 14,  79}, /* 1810 'O' */
    { 0xef120000, 15,  71}, /* 1811 'G' */
    { 0xef4c0000, 15,  69}, /* 1812 'E' */
    { 0xef4e0000, 15,  44}, /* 1813 ',' */
    { 0xef540000, 15,  39}, /* 1814 '\'' */
    { 0xef110000, 16,  59}, /* 1815 ';' */
    { 0xef500000, 16,  80}, /* 1816 'P' */
    { 0xef510000, 16,  49}, /* 1817 '1' */
    { 0xef520000, 16,   1}, /* 1818 '0x01' */
    { 0xef570000, 16,  68}, /* 1819 'D' */
    { 0xef100000, 17,  58}, /* 1820 ':' */
    { 0xef108000, 17,  42}, /* 1821 '*' */
    { 0xef530000, 17,  82}, /* 1822 'R' */
    { 0xef538000, 17,  78}, /* 1823 'N' */
    { 0xef560000, 17,  73}, /* 1824 'I' */
    { 0xef568000, 17,  66}, /* 1825 'B' */
    /*   45                             */
    { 0x00000000,  2,  75}, /* 1826 'K' */
    { 0x80000000,  2, 110}, /* 1827 'n' */
    { 0x60000000,  3,  83}, /* 1828 'S' */
    { 0xc0000000,  3, 112}, /* 1829 'p' */
    { 0xf0000000,  4, 108}, /* 1830 'l' */
    { 0x50000000,  5, 115}, /* 1831 's' */
    { 0x58000000,  5, 114}, /* 1832 'r' */
    { 0xe8000000,  5,  82}, /* 1833 'R' */
    { 0x40000000,  6, 103}, /* 1834 'g' */
    { 0xe0000000,  6,  32}, /* 1835 ' ' */
    { 0x4e000000,  7,  46}, /* 1836 '.' */
    { 0xe4000000,  7, 109}, /* 1837 'm' */
    { 0x44000000,  8, 107}, /* 1838 'k' */
    { 0x45000000,  8, 116}, /* 1839 't' */
    { 0x46000000,  8,  69}, /* 1840 'E' */
    { 0x47000000,  8,  45}, /* 1841 '-' */
    { 0x48000000,  8,  70}, /* 1842 'F' */
    { 0x4c000000,  8,  50}, /* 1843 '2' */
    { 0xe6000000,  8,  99}, /* 1844 'c' */
    { 0xe7000000,  8,  78}, /* 1845 'N' */
    { 0x49000000,  9, 102}, /* 1846 'f' */
    { 0x49800000,  9,  56}, /* 1847 '8' */
    { 0x4a000000,  9,  44}, /* 1848 ',' */
    { 0x4b800000,  9,  90}, /* 1849 'Z' */
    { 0x4a800000, 10, 104}, /* 1850 'h' */
    { 0x4d000000, 10, 105}, /* 1851 'i' */
    { 0x4d400000, 10, 119}, /* 1852 'w' */
    { 0x4d800000, 10,  97}, /* 1853 'a' */
    { 0x4ac00000, 11,  98}, /* 1854 'b' */
    { 0x4ae00000, 11,  68}, /* 1855 'D' */
    { 0x4dc00000, 11,  33}, /* 1856 '!' */
    { 0x4b000000, 12, 101}, /* 1857 'e' */
    { 0x4b100000, 12,  86}, /* 1858 'V' */
    { 0x4b200000, 12,  80}, /* 1859 'P' */
    { 0x4b300000, 12,  73}, /* 1860 'I' */
    { 0x4b400000, 12,  66}, /* 1861 'B' */
    { 0x4b500000, 12,  65}, /* 1862 'A' */
    { 0x4df00000, 12, 100}, /* 1863 'd' */
    { 0x4b600000, 13,  72}, /* 1864 'H' */
    { 0x4b680000, 13,  67}, /* 1865 'C' */
    { 0x4b700000, 13,  58}, /* 1866 ':' */
    { 0x4b780000, 13,  41}, /* 1867 ')' */
    { 0x4de80000, 13, 122}, /* 1868 'z' */
    { 0x4de00000, 14,   1}, /* 1869 '0x01' */
    { 0x4de40000, 14,  84}, /* 1870 'T' */
    /*   38                             */
    { 0x40000000,  2,  32}, /* 1871 ' ' */
    { 0xc0000000,  2, 105}, /* 1872 'i' */
    { 0x00000000,  3,  46}, /* 1873 '.' */
    { 0x20000000,  3,  97}, /* 1874 'a' */
    { 0xa0000000,  3, 101}, /* 1875 'e' */
    { 0x88000000,  5,  67}, /* 1876 'C' */
    { 0x98000000,  5, 111}, /* 1877 'o' */
    { 0x82000000,  7,  70}, /* 1878 'F' */
    { 0x86000000,  7,  73}, /* 1879 'I' */
    { 0x81000000,  8,  49}, /* 1880 '1' */
    { 0x85000000,  8,  52}, /* 1881 '4' */
    { 0x90000000,  8, 114}, /* 1882 'r' */
    { 0x92000000,  8,  69}, /* 1883 'E' */
    { 0x93000000,  8, 115}, /* 1884 's' */
    { 0x95000000,  8,  39}, /* 1885 '\'' */
    { 0x96000000,  8,  58}, /* 1886 ':' */
    { 0x97000000,  8, 108}, /* 1887 'l' */
    { 0x80800000,  9,  47}, /* 1888 '/' */
    { 0x84800000,  9,  45}, /* 1889 '-' */
    { 0x91000000,  9,  68}, /* 1890 'D' */
    { 0x91800000,  9, 117}, /* 1891 'u' */
    { 0x94800000,  9,  44}, /* 1892 ',' */
    { 0x80000000, 10,  54}, /* 1893 '6' */
    { 0x80400000, 10,  50}, /* 1894 '2' */
    { 0x84000000, 10,  53}, /* 1895 '5' */
    { 0x94000000, 10,  59}, /* 1896 ';' */
    { 0x94400000, 11,   1}, /* 1897 '0x01' */
    { 0x84400000, 12,  55}, /* 1898 '7' */
    { 0x94600000, 12,  51}, /* 1899 '3' */
    { 0x84500000, 13,  57}, /* 1900 '9' */
    { 0x84580000, 13, 121}, /* 1901 'y' */
    { 0x84600000, 13,  80}, /* 1902 'P' */
    { 0x84680000, 13,  72}, /* 1903 'H' */
    { 0x84700000, 13,  65}, /* 1904 'A' */
    { 0x84780000, 13,  56}, /* 1905 '8' */
    { 0x94780000, 13,  87}, /* 1906 'W' */
    { 0x94700000, 14, 102}, /* 1907 'f' */
    { 0x94740000, 14,  66}, /* 1908 'B' */
    /*   30                             */
    { 0x00000000,  2, 104}, /* 1909 'h' */
    { 0x80000000,  2, 105}, /* 1910 'i' */
    { 0x60000000,  3,  97}, /* 1911 'a' */
    { 0xc0000000,  3, 111}, /* 1912 'o' */
    { 0xe0000000,  3, 101}, /* 1913 'e' */
    { 0x40000000,  4, 114}, /* 1914 'r' */
    { 0x58000000,  5,  79}, /* 1915 'O' */
    { 0x52000000,  7,  32}, /* 1916 ' ' */
    { 0x54000000,  7, 121}, /* 1917 'y' */
    { 0x50000000,  9,  66}, /* 1918 'B' */
    { 0x50800000,  9,  46}, /* 1919 '.' */
    { 0x56000000,  9,  73}, /* 1920 'I' */
    { 0x57000000,  9,  87}, /* 1921 'W' */
    { 0x57800000,  9,  65}, /* 1922 'A' */
    { 0x51000000, 10,  39}, /* 1923 '\'' */
    { 0x51400000, 10,  77}, /* 1924 'M' */
    { 0x51c00000, 10,  84}, /* 1925 'T' */
    { 0x51800000, 11,  50}, /* 1926 '2' */
    { 0x51a00000, 11,  58}, /* 1927 ':' */
    { 0x56800000, 11,  72}, /* 1928 'H' */
    { 0x56c00000, 11,  86}, /* 1929 'V' */
    { 0x56a00000, 12, 108}, /* 1930 'l' */
    { 0x56e00000, 12, 115}, /* 1931 's' */
    { 0x56f00000, 12,  44}, /* 1932 ',' */
    { 0x56b80000, 13, 117}, /* 1933 'u' */
    { 0x56b00000, 15,  53}, /* 1934 '5' */
    { 0x56b20000, 15,  41}, /* 1935 ')' */
    { 0x56b60000, 15,  69}, /* 1936 'E' */
    { 0x56b40000, 16,   1}, /* 1937 '0x01' */
    { 0x56b50000, 16, 109}, /* 1938 'm' */
    /*   24                             */
    { 0x80000000,  1,  32}, /* 1939 ' ' */
    { 0x00000000,  3,  97}, /* 1940 'a' */
    { 0x20000000,  4, 122}, /* 1941 'z' */
    { 0x30000000,  4, 109}, /* 1942 'm' */
    { 0x70000000,  4, 116}, /* 1943 't' */
    { 0x48000000,  5,  85}, /* 1944 'U' */
    { 0x50000000,  5,  45}, /* 1945 '-' */
    { 0x58000000,  5, 101}, /* 1946 'e' */
    { 0x60000000,  6, 117}, /* 1947 'u' */
    { 0x64000000,  6,  73}, /* 1948 'I' */
    { 0x6c000000,  6,  44}, /* 1949 ',' */
    { 0x44000000,  7,  86}, /* 1950 'V' */
    { 0x68000000,  7,  53}, /* 1951 '5' */
    { 0x6a000000,  7,  46}, /* 1952 '.' */
    { 0x40000000,  8,  83}, /* 1953 'S' */
    { 0x42000000,  8,  58}, /* 1954 ':' */
    { 0x46000000,  8,  99}, /* 1955 'c' */
    { 0x47000000,  8, 105}, /* 1956 'i' */
    { 0x41000000,  9,  57}, /* 1957 '9' */
    { 0x41800000,  9,  39}, /* 1958 '\'' */
    { 0x43800000,  9,  88}, /* 1959 'X' */
    { 0x43000000, 10,  41}, /* 1960 ')' */
    { 0x43400000, 11,   1}, /* 1961 '0x01' */
    { 0x43600000, 11, 121}, /* 1962 'y' */
    /*   24                             */
    { 0x80000000,  1, 111}, /* 1963 'o' */
    { 0x00000000,  2, 101}, /* 1964 'e' */
    { 0x40000000,  3, 117}, /* 1965 'u' */
    { 0x60000000,  5,  97}, /* 1966 'a' */
    { 0x70000000,  5, 118}, /* 1967 'v' */
    { 0x78000000,  5,  32}, /* 1968 ' ' */
    { 0x68000000,  7, 110}, /* 1969 'n' */
    { 0x6a000000,  8, 114}, /* 1970 'r' */
    { 0x6d000000,  8,  79}, /* 1971 'O' */
    { 0x6f000000,  8, 105}, /* 1972 'i' */
    { 0x6b800000,  9,  78}, /* 1973 'N' */
    { 0x6c000000,  9,  76}, /* 1974 'L' */
    { 0x6c800000,  9, 115}, /* 1975 's' */
    { 0x6e800000,  9, 109}, /* 1976 'm' */
    { 0x6b000000, 10,  46}, /* 1977 '.' */
    { 0x6b400000, 10,  77}, /* 1978 'M' */
    { 0x6e400000, 10,  80}, /* 1979 'P' */
    { 0x6e100000, 12,  82}, /* 1980 'R' */
    { 0x6e200000, 12,  50}, /* 1981 '2' */
    { 0x6e000000, 13,  45}, /* 1982 '-' */
    { 0x6e300000, 13,  67}, /* 1983 'C' */
    { 0x6e380000, 13,  44}, /* 1984 ',' */
    { 0x6e080000, 14,   1}, /* 1985 '0x01' */
    { 0x6e0c0000, 14, 100}, /* 1986 'd' */
    /*   18                             */
    { 0x40000000,  2,  97}, /* 1987 'a' */
    { 0x80000000,  2, 101}, /* 1988 'e' */
    { 0xc0000000,  2, 111}, /* 1989 'o' */
    { 0x20000000,  4, 122}, /* 1990 'z' */
    { 0x30000000,  4, 105}, /* 1991 'i' */
    { 0x08000000,  5,  90}, /* 1992 'Z' */
    { 0x10000000,  5,  32}, /* 1993 ' ' */
    { 0x18000000,  5, 117}, /* 1994 'u' */
    { 0x04000000,  7,  79}, /* 1995 'O' */
    { 0x06000000,  7,  46}, /* 1996 '.' */
    { 0x00000000,  8, 121}, /* 1997 'y' */
    { 0x01000000,  8,  52}, /* 1998 '4' */
    { 0x02000000,  8,  44}, /* 1999 ',' */
    { 0x03000000, 10, 102}, /* 2000 'f' */
    { 0x03400000, 10,  45}, /* 2001 '-' */
    { 0x03800000, 10,   0}, /* 2002 '0x00' */
    { 0x03c00000, 11,   1}, /* 2003 '0x01' */
    { 0x03e00000, 11, 108}, /* 2004 'l' */
    /*   34                             */
    { 0x80000000,  1,  83}, /* 2005 'S' */
    { 0x40000000,  2,  65}, /* 2006 'A' */
    { 0x00000000,  4,  50}, /* 2007 '2' */
    { 0x20000000,  4,  82}, /* 2008 'R' */
    { 0x30000000,  4,  49}, /* 2009 '1' */
    { 0x14000000,  6, 110}, /* 2010 'n' */
    { 0x18000000,  6, 109}, /* 2011 'm' */
    { 0x1e000000,  7, 108}, /* 2012 'l' */
    { 0x13000000,  8, 114}, /* 2013 'r' */
    { 0x1d000000,  8,  98}, /* 2014 'b' */
    { 0x10800000,  9,  67}, /* 2015 'C' */
    { 0x11000000,  9, 102}, /* 2016 'f' */
    { 0x12800000,  9,  77}, /* 2017 'M' */
    { 0x1c800000,  9,  99}, /* 2018 'c' */
    { 0x10400000, 10,   0}, /* 2019 '0x00' */
    { 0x11c00000, 10,  75}, /* 2020 'K' */
    { 0x12000000, 10,  72}, /* 2021 'H' */
    { 0x1c400000, 10,  84}, /* 2022 'T' */
    { 0x10000000, 11,  74}, /* 2023 'J' */
    { 0x10200000, 11,  66}, /* 2024 'B' */
    { 0x11a00000, 11,  90}, /* 2025 'Z' */
    { 0x12600000, 11,  80}, /* 2026 'P' */
    { 0x1c000000, 11,  76}, /* 2027 'L' */
    { 0x11800000, 12,  70}, /* 2028 'F' */
    { 0x12500000, 12,  73}, /* 2029 'I' */
    { 0x1c300000, 12,  78}, /* 2030 'N' */
    { 0x11900000, 13, 115}, /* 2031 's' */
    { 0x11980000, 13,  86}, /* 2032 'V' */
    { 0x12400000, 13,  85}, /* 2033 'U' */
    { 0x12480000, 13,  71}, /* 2034 'G' */
    { 0x1c280000, 13,  68}, /* 2035 'D' */
    { 0x1c200000, 14,  79}, /* 2036 'O' */
    { 0x1c240000, 15,   1}, /* 2037 '0x01' */
    { 0x1c260000, 15,  87}, /* 2038 'W' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2039 '0x01' */
    { 0x80000000,  1, 120}, /* 2040 'x' */
    /*    9                             */
    { 0x80000000,  1,   0}, /* 2041 '0x00' */
    { 0x40000000,  2,  32}, /* 2042 ' ' */
    { 0x20000000,  3,  46}, /* 2043 '.' */
    { 0x10000000,  4,  91}, /* 2044 '[' */
    { 0x08000000,  5,  58}, /* 2045 ':' */
    { 0x04000000,  6,  44}, /* 2046 ',' */
    { 0x02000000,  7,  59}, /* 2047 ';' */
    { 0x00000000,  8,   1}, /* 2048 '0x01' */
    { 0x01000000,  8,  93}, /* 2049 ']' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2050 '0x01' */
    { 0x80000000,  1,   1}, /* 2051 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 2052 '0x01' */
    { 0x80000000,  1,   1}, /* 2053 '0x01' */
    /*   31                             */
    { 0x20000000,  3, 119}, /* 2054 'w' */
    { 0x60000000,  3,  72}, /* 2055 'H' */
    { 0xc0000000,  3,  80}, /* 2056 'P' */
    { 0xe0000000,  3,  70}, /* 2057 'F' */
    { 0x50000000,  4, 110}, /* 2058 'n' */
    { 0xb0000000,  4,  68}, /* 2059 'D' */
    { 0x08000000,  5, 115}, /* 2060 's' */
    { 0x10000000,  5,  65}, /* 2061 'A' */
    { 0x48000000,  5, 116}, /* 2062 't' */
    { 0x80000000,  5, 103}, /* 2063 'g' */
    { 0x98000000,  5,  99}, /* 2064 'c' */
    { 0x00000000,  6,  87}, /* 2065 'W' */
    { 0x04000000,  6, 114}, /* 2066 'r' */
    { 0x18000000,  6, 101}, /* 2067 'e' */
    { 0x1c000000,  6,  67}, /* 2068 'C' */
    { 0x8c000000,  6, 108}, /* 2069 'l' */
    { 0x90000000,  6, 104}, /* 2070 'h' */
    { 0x94000000,  6,  71}, /* 2071 'G' */
    { 0xa4000000,  6,  66}, /* 2072 'B' */
    { 0xa8000000,  6,  84}, /* 2073 'T' */
    { 0xac000000,  6,  83}, /* 2074 'S' */
    { 0x40000000,  7, 111}, /* 2075 'o' */
    { 0x42000000,  7, 100}, /* 2076 'd' */
    { 0x44000000,  7,  98}, /* 2077 'b' */
    { 0x46000000,  7,  82}, /* 2078 'R' */
    { 0x88000000,  7,  79}, /* 2079 'O' */
    { 0x8a000000,  7,  76}, /* 2080 'L' */
    { 0xa2000000,  7, 102}, /* 2081 'f' */
    { 0xa0000000,  8,  74}, /* 2082 'J' */
    { 0xa1000000,  9,   1}, /* 2083 '0x01' */
    { 0xa1800000,  9,  77}, /* 2084 'M' */
    /*   51                             */
    { 0x40000000,  2, 110}, /* 2085 'n' */
    { 0x20000000,  3, 114}, /* 2086 'r' */
    { 0xa0000000,  3, 116}, /* 2087 't' */
    { 0x00000000,  4, 109}, /* 2088 'm' */
    { 0x90000000,  4, 115}, /* 2089 's' */
    { 0xe0000000,  4,  32}, /* 2090 ' ' */
    { 0xf0000000,  4, 108}, /* 2091 'l' */
    { 0x18000000,  5, 100}, /* 2092 'd' */
    { 0x80000000,  5, 105}, /* 2093 'i' */
    { 0xc0000000,  5, 121}, /* 2094 'y' */
    { 0xd0000000,  5,  99}, /* 2095 'c' */
    { 0x10000000,  6, 112}, /* 2096 'p' */
    { 0x88000000,  6, 117}, /* 2097 'u' */
    { 0x8c000000,  6, 118}, /* 2098 'v' */
    { 0xcc000000,  6, 103}, /* 2099 'g' */
    { 0xd8000000,  6,  98}, /* 2100 'b' */
    { 0xdc000000,  6, 107}, /* 2101 'k' */
    { 0x14000000,  7, 119}, /* 2102 'w' */
    { 0x17000000,  8, 122}, /* 2103 'z' */
    { 0xc8000000,  8,  46}, /* 2104 '.' */
    { 0xcb000000,  8, 102}, /* 2105 'f' */
    { 0x16000000,  9,  44}, /* 2106 ',' */
    { 0x16800000,  9,  39}, /* 2107 '\'' */
    { 0xc9800000,  9, 101}, /* 2108 'e' */
    { 0xca800000,  9, 104}, /* 2109 'h' */
    { 0xca400000, 10, 120}, /* 2110 'x' */
    { 0xc9000000, 11,  97}, /* 2111 'a' */
    { 0xc9200000, 11,  45}, /* 2112 '-' */
    { 0xc9600000, 11, 106}, /* 2113 'j' */
    { 0xca000000, 11,  58}, /* 2114 ':' */
    { 0xca200000, 11, 111}, /* 2115 'o' */
    { 0xc9400000, 12, 113}, /* 2116 'q' */
    { 0xc9500000, 14,  33}, /* 2117 '!' */
    { 0xc95c0000, 14,  63}, /* 2118 '?' */
    { 0xc9540000, 16,   1}, /* 2119 '0x01' */
    { 0xc9560000, 16,  59}, /* 2120 ';' */
    { 0xc9570000, 16,  41}, /* 2121 ')' */
    { 0xc9590000, 16,  47}, /* 2122 '/' */
    { 0xc95b0000, 16,  64}, /* 2123 '@' */
    { 0xc9550000, 17,  74}, /* 2124 'J' */
    { 0xc9558000, 17,  93}, /* 2125 ']' */
    { 0xc95a0000, 17,  78}, /* 2126 'N' */
    { 0xc95a8000, 17,  76}, /* 2127 'L' */
    { 0xc9580000, 18,  82}, /* 2128 'R' */
    { 0xc9584000, 18,  83}, /* 2129 'S' */
    { 0xc9588000, 18,  86}, /* 2130 'V' */
    { 0xc958c000, 20,  91}, /* 2131 '[' */
    { 0xc958e000, 20,  80}, /* 2132 'P' */
    { 0xc958f000, 20,   0}, /* 2133 '0x00' */
    { 0xc958d000, 21,  87}, /* 2134 'W' */
    { 0xc958d800, 21,  49}, /* 2135 '1' */
    /*   39                             */
    { 0x00000000,  2, 101}, /* 2136 'e' */
    { 0x40000000,  3, 117}, /* 2137 'u' */
    { 0x60000000,  3,  97}, /* 2138 'a' */
    { 0x80000000,  3, 121}, /* 2139 'y' */
    { 0xa0000000,  3, 111}, /* 2140 'o' */
    { 0xc0000000,  4, 108}, /* 2141 'l' */
    { 0xe0000000,  4, 114}, /* 2142 'r' */
    { 0xf0000000,  4, 105}, /* 2143 'i' */
    { 0xd0000000,  6, 115}, /* 2144 's' */
    { 0xd8000000,  6,  32}, /* 2145 ' ' */
    { 0xdc000000,  6,  98}, /* 2146 'b' */
    { 0xd5000000,  8,  99}, /* 2147 'c' */
    { 0xd4000000,  9, 106}, /* 2148 'j' */
    { 0xd6000000,  9,  44}, /* 2149 ',' */
    { 0xd6800000,  9,  46}, /* 2150 '.' */
    { 0xd4c00000, 10,  39}, /* 2151 '\'' */
    { 0xd7c00000, 10, 116}, /* 2152 't' */
    { 0xd4a00000, 11,  58}, /* 2153 ':' */
    { 0xd7000000, 11, 119}, /* 2154 'w' */
    { 0xd7200000, 11, 100}, /* 2155 'd' */
    { 0xd7400000, 11, 104}, /* 2156 'h' */
    { 0xd7600000, 11,  38}, /* 2157 '&' */
    { 0xd7a00000, 11,  45}, /* 2158 '-' */
    { 0xd7800000, 12, 109}, /* 2159 'm' */
    { 0xd7900000, 12, 110}, /* 2160 'n' */
    { 0xd4800000, 13,  63}, /* 2161 '?' */
    { 0xd4900000, 13, 118}, /* 2162 'v' */
    { 0xd4880000, 14, 102}, /* 2163 'f' */
    { 0xd49c0000, 14, 112}, /* 2164 'p' */
    { 0xd48e0000, 15,  59}, /* 2165 ';' */
    { 0xd49a0000, 15,  68}, /* 2166 'D' */
    { 0xd48c0000, 16,  47}, /* 2167 '/' */
    { 0xd4990000, 16,  64}, /* 2168 '@' */
    { 0xd48d0000, 17,  88}, /* 2169 'X' */
    { 0xd48d8000, 17,  34}, /* 2170 '\"' */
    { 0xd4988000, 17, 107}, /* 2171 'k' */
    { 0xd4980000, 18,   0}, /* 2172 '0x00' */
    { 0xd4984000, 19,   1}, /* 2173 '0x01' */
    { 0xd4986000, 19,  33}, /* 2174 '!' */
    /*   57                             */
    { 0x00000000,  2, 111}, /* 2175 'o' */
    { 0x60000000,  3,  97}, /* 2176 'a' */
    { 0x80000000,  3, 101}, /* 2177 'e' */
    { 0xc0000000,  3, 104}, /* 2178 'h' */
    { 0x40000000,  4, 105}, /* 2179 'i' */
    { 0x50000000,  4, 108}, /* 2180 'l' */
    { 0xb0000000,  4, 107}, /* 2181 'k' */
    { 0xf0000000,  4, 116}, /* 2182 't' */
    { 0xa0000000,  5, 117}, /* 2183 'u' */
    { 0xa8000000,  5, 114}, /* 2184 'r' */
    { 0xe0000000,  5,  32}, /* 2185 ' ' */
    { 0xe8000000,  7, 121}, /* 2186 'y' */
    { 0xea000000,  7,  99}, /* 2187 'c' */
    { 0xed000000,  8, 115}, /* 2188 's' */
    { 0xee000000,  8,  46}, /* 2189 '.' */
    { 0xef800000,  9,  44}, /* 2190 ',' */
    { 0xec400000, 10,  71}, /* 2191 'G' */
    { 0xec200000, 11, 110}, /* 2192 'n' */
    { 0xecc00000, 11,  68}, /* 2193 'D' */
    { 0xef000000, 11,  75}, /* 2194 'K' */
    { 0xef600000, 11,  67}, /* 2195 'C' */
    { 0xec000000, 12,  58}, /* 2196 ':' */
    { 0xec900000, 12,  45}, /* 2197 '-' */
    { 0xeca00000, 12,  65}, /* 2198 'A' */
    { 0xece00000, 12,  76}, /* 2199 'L' */
    { 0xecf00000, 12,  39}, /* 2200 '\'' */
    { 0xef200000, 12, 100}, /* 2201 'd' */
    { 0xef400000, 12, 113}, /* 2202 'q' */
    { 0xef500000, 12,  73}, /* 2203 'I' */
    { 0xec800000, 13,  78}, /* 2204 'N' */
    { 0xec880000, 13, 122}, /* 2205 'z' */
    { 0xef380000, 13,  70}, /* 2206 'F' */
    { 0xec140000, 14, 119}, /* 2207 'w' */
    { 0xec1c0000, 14,  69}, /* 2208 'E' */
    { 0xecb00000, 14,  63}, /* 2209 '?' */
    { 0xef300000, 14,  77}, /* 2210 'M' */
    { 0xef340000, 14,  83}, /* 2211 'S' */
    { 0xec100000, 15,  98}, /* 2212 'b' */
    { 0xec120000, 15,   1}, /* 2213 '0x01' */
    { 0xec180000, 15,  33}, /* 2214 '!' */
    { 0xecb40000, 15,  81}, /* 2215 'Q' */
    { 0xecb60000, 15,  80}, /* 2216 'P' */
    { 0xecb80000, 15,  59}, /* 2217 ';' */
    { 0xecbc0000, 15,  66}, /* 2218 'B' */
    { 0xec1b0000, 16,  86}, /* 2219 'V' */
    { 0xecbb0000, 16, 109}, /* 2220 'm' */
    { 0xecbe0000, 16,  47}, /* 2221 '/' */
    { 0xec1a8000, 17,  41}, /* 2222 ')' */
    { 0xecba0000, 17,  85}, /* 2223 'U' */
    { 0xecbf0000, 17,  87}, /* 2224 'W' */
    { 0xec1a0000, 18, 112}, /* 2225 'p' */
    { 0xec1a4000, 18,  72}, /* 2226 'H' */
    { 0xecba8000, 18,   0}, /* 2227 '0x00' */
    { 0xecbf8000, 18,  82}, /* 2228 'R' */
    { 0xecbfc000, 18,  84}, /* 2229 'T' */
    { 0xecbac000, 19,  93}, /* 2230 ']' */
    { 0xecbae000, 19,  91}, /* 2231 '[' */
    /*   46                             */
    { 0x00000000,  1,  32}, /* 2232 ' ' */
    { 0xa0000000,  3, 101}, /* 2233 'e' */
    { 0xc0000000,  4,  97}, /* 2234 'a' */
    { 0xe0000000,  4, 105}, /* 2235 'i' */
    { 0x88000000,  5,  46}, /* 2236 '.' */
    { 0x98000000,  5, 111}, /* 2237 'o' */
    { 0xd8000000,  5, 114}, /* 2238 'r' */
    { 0xf8000000,  5, 115}, /* 2239 's' */
    { 0x84000000,  6, 100}, /* 2240 'd' */
    { 0x94000000,  6, 108}, /* 2241 'l' */
    { 0xd0000000,  6, 117}, /* 2242 'u' */
    { 0xf4000000,  6, 121}, /* 2243 'y' */
    { 0x92000000,  7,  45}, /* 2244 '-' */
    { 0xd6000000,  7, 118}, /* 2245 'v' */
    { 0xf0000000,  7, 103}, /* 2246 'g' */
    { 0xf2000000,  7,  44}, /* 2247 ',' */
    { 0x80000000,  8, 104}, /* 2248 'h' */
    { 0x81000000,  8,  58}, /* 2249 ':' */
    { 0x82000000,  8, 109}, /* 2250 'm' */
    { 0x90000000,  8, 119}, /* 2251 'w' */
    { 0x91000000,  8, 110}, /* 2252 'n' */
    { 0xd5000000,  8,  39}, /* 2253 '\'' */
    { 0x83800000,  9, 102}, /* 2254 'f' */
    { 0xd4400000, 10,  63}, /* 2255 '?' */
    { 0xd4800000, 10,  98}, /* 2256 'b' */
    { 0xd4c00000, 10,  99}, /* 2257 'c' */
    { 0x83000000, 11,  33}, /* 2258 '!' */
    { 0xd4000000, 11, 112}, /* 2259 'p' */
    { 0xd4200000, 11, 116}, /* 2260 't' */
    { 0x83200000, 12,   0}, /* 2261 '0x00' */
    { 0x83300000, 12,  41}, /* 2262 ')' */
    { 0x83400000, 12, 107}, /* 2263 'k' */
    { 0x83500000, 12,  59}, /* 2264 ';' */
    { 0x83700000, 12,  47}, /* 2265 '/' */
    { 0x83680000, 14,  34}, /* 2266 '\"' */
    { 0x836c0000, 14, 106}, /* 2267 'j' */
    { 0x83600000, 15, 122}, /* 2268 'z' */
    { 0x83660000, 15, 113}, /* 2269 'q' */
    { 0x83630000, 16,  64}, /* 2270 '@' */
    { 0x83640000, 16,   1}, /* 2271 '0x01' */
    { 0x83628000, 17,  67}, /* 2272 'C' */
    { 0x83650000, 17,  93}, /* 2273 ']' */
    { 0x83620000, 18,  90}, /* 2274 'Z' */
    { 0x83624000, 18,  91}, /* 2275 '[' */
    { 0x83658000, 18,  84}, /* 2276 'T' */
    { 0x8365c000, 18,  96}, /* 2277 '`' */
    /*   61                             */
    { 0x80000000,  2,  32}, /* 2278 ' ' */
    { 0x40000000,  3, 115}, /* 2279 's' */
    { 0xc0000000,  3, 114}, /* 2280 'r' */
    { 0x00000000,  4, 100}, /* 2281 'd' */
    { 0x30000000,  4,  97}, /* 2282 'a' */
    { 0xf0000000,  4, 110}, /* 2283 'n' */
    { 0x10000000,  5,  99}, /* 2284 'c' */
    { 0x60000000,  5, 101}, /* 2285 'e' */
    { 0x68000000,  5, 119}, /* 2286 'w' */
    { 0x78000000,  5, 116}, /* 2287 't' */
    { 0xe0000000,  5, 108}, /* 2288 'l' */
    { 0x18000000,  6, 120}, /* 2289 'x' */
    { 0x20000000,  6, 118}, /* 2290 'v' */
    { 0x24000000,  6, 105}, /* 2291 'i' */
    { 0x28000000,  6, 121}, /* 2292 'y' */
    { 0x70000000,  6, 109}, /* 2293 'm' */
    { 0xe8000000,  6,  46}, /* 2294 '.' */
    { 0x1c000000,  7, 102}, /* 2295 'f' */
    { 0x2e000000,  7,  98}, /* 2296 'b' */
    { 0x74000000,  7,  44}, /* 2297 ',' */
    { 0xec000000,  7, 112}, /* 2298 'p' */
    { 0x1e000000,  8,  45}, /* 2299 '-' */
    { 0x1f000000,  8, 104}, /* 2300 'h' */
    { 0x2c000000,  8, 107}, /* 2301 'k' */
    { 0x76000000,  8,  39}, /* 2302 '\'' */
    { 0xee000000,  8, 103}, /* 2303 'g' */
    { 0xef000000,  8, 111}, /* 2304 'o' */
    { 0x2d800000,  9,  58}, /* 2305 ':' */
    { 0x2d000000, 10,  41}, /* 2306 ')' */
    { 0x2d400000, 10, 113}, /* 2307 'q' */
    { 0x77400000, 10,  63}, /* 2308 '?' */
    { 0x77800000, 10, 117}, /* 2309 'u' */
    { 0x77000000, 11, 122}, /* 2310 'z' */
    { 0x77c00000, 11,  33}, /* 2311 '!' */
    { 0x77e00000, 12,   0}, /* 2312 '0x00' */
    { 0x77f00000, 12, 106}, /* 2313 'j' */
    { 0x77300000, 13,  47}, /* 2314 '/' */
    { 0x77240000, 14,  52}, /* 2315 '4' */
    { 0x77280000, 14,  66}, /* 2316 'B' */
    { 0x77380000, 14,  93}, /* 2317 ']' */
    { 0x773c0000, 14,  59}, /* 2318 ';' */
    { 0x77200000, 15,  34}, /* 2319 '\"' */
    { 0x77230000, 16,  68}, /* 2320 'D' */
    { 0x772c0000, 16,   1}, /* 2321 '0x01' */
    { 0x772e0000, 16,  64}, /* 2322 '@' */
    { 0x77220000, 17,  84}, /* 2323 'T' */
    { 0x77228000, 17,  67}, /* 2324 'C' */
    { 0x772d8000, 17,  91}, /* 2325 '[' */
    { 0x772f8000, 17,  76}, /* 2326 'L' */
    { 0x772d0000, 18,  86}, /* 2327 'V' */
    { 0x772d4000, 18,  71}, /* 2328 'G' */
    { 0x772f0000, 18,  49}, /* 2329 '1' */
    { 0x772f5000, 20,  69}, /* 2330 'E' */
    { 0x772f4000, 21,  50}, /* 2331 '2' */
    { 0x772f4800, 21,  78}, /* 2332 'N' */
    { 0x772f6000, 21,  70}, /* 2333 'F' */
    { 0x772f6800, 21,  65}, /* 2334 'A' */
    { 0x772f7000, 22,  92}, /* 2335 '\\' */
    { 0x772f7400, 22,  80}, /* 2336 'P' */
    { 0x772f7800, 22,  77}, /* 2337 'M' */
    { 0x772f7c00, 22,  72}, /* 2338 'H' */
    /*   35                             */
    { 0x00000000,  2, 111}, /* 2339 'o' */
    { 0x80000000,  2,  32}, /* 2340 ' ' */
    { 0x40000000,  3, 105}, /* 2341 'i' */
    { 0xc0000000,  3, 114}, /* 2342 'r' */
    { 0x70000000,  4,  97}, /* 2343 'a' */
    { 0xe0000000,  4, 101}, /* 2344 'e' */
    { 0x60000000,  5, 116}, /* 2345 't' */
    { 0xf0000000,  5, 117}, /* 2346 'u' */
    { 0xf8000000,  5, 102}, /* 2347 'f' */
    { 0x6c000000,  6, 108}, /* 2348 'l' */
    { 0x68000000,  8, 115}, /* 2349 's' */
    { 0x69000000,  8, 121}, /* 2350 'y' */
    { 0x6b000000,  9,  46}, /* 2351 '.' */
    { 0x6a000000, 10,  63}, /* 2352 '?' */
    { 0x6ac00000, 10,  44}, /* 2353 ',' */
    { 0x6bc00000, 10,  45}, /* 2354 '-' */
    { 0x6a600000, 11,  58}, /* 2355 ':' */
    { 0x6a800000, 11,  39}, /* 2356 '\'' */
    { 0x6aa00000, 11, 103}, /* 2357 'g' */
    { 0x6a400000, 12, 109}, /* 2358 'm' */
    { 0x6b800000, 12,   1}, /* 2359 '0x01' */
    { 0x6ba00000, 12,  98}, /* 2360 'b' */
    { 0x6bb00000, 12, 110}, /* 2361 'n' */
    { 0x6a580000, 13,  99}, /* 2362 'c' */
    { 0x6b900000, 13,  33}, /* 2363 '!' */
    { 0x6b980000, 14,  41}, /* 2364 ')' */
    { 0x6b9c0000, 14, 119}, /* 2365 'w' */
    { 0x6a520000, 15, 112}, /* 2366 'p' */
    { 0x6a540000, 15,  47}, /* 2367 '/' */
    { 0x6a500000, 16, 104}, /* 2368 'h' */
    { 0x6a560000, 16,  59}, /* 2369 ';' */
    { 0x6a510000, 17,   0}, /* 2370 '0x00' */
    { 0x6a518000, 17, 100}, /* 2371 'd' */
    { 0x6a570000, 17, 107}, /* 2372 'k' */
    { 0x6a578000, 17, 118}, /* 2373 'v' */
    /*   40                             */
    { 0xc0000000,  2,  32}, /* 2374 ' ' */
    { 0x00000000,  3,  97}, /* 2375 'a' */
    { 0x40000000,  3, 104}, /* 2376 'h' */
    { 0xa0000000,  3, 101}, /* 2377 'e' */
    { 0x30000000,  4, 117}, /* 2378 'u' */
    { 0x60000000,  4, 114}, /* 2379 'r' */
    { 0x80000000,  4, 105}, /* 2380 'i' */
    { 0x70000000,  5, 108}, /* 2381 'l' */
    { 0x90000000,  5, 115}, /* 2382 's' */
    { 0x98000000,  5, 111}, /* 2383 'o' */
    { 0x24000000,  6,  44}, /* 2384 ',' */
    { 0x28000000,  6, 110}, /* 2385 'n' */
    { 0x78000000,  6, 103}, /* 2386 'g' */
    { 0x7c000000,  6,  46}, /* 2387 '.' */
    { 0x2c000000,  7, 121}, /* 2388 'y' */
    { 0x21000000,  8,  39}, /* 2389 '\'' */
    { 0x22000000,  8,  45}, /* 2390 '-' */
    { 0x23000000,  8,  58}, /* 2391 ':' */
    { 0x20000000,  9, 100}, /* 2392 'd' */
    { 0x2e800000,  9,  98}, /* 2393 'b' */
    { 0x2f000000,  9, 116}, /* 2394 't' */
    { 0x2f800000,  9, 119}, /* 2395 'w' */
    { 0x20800000, 10,  63}, /* 2396 '?' */
    { 0x20c00000, 10, 109}, /* 2397 'm' */
    { 0x2e200000, 11,  33}, /* 2398 '!' */
    { 0x2e600000, 11, 102}, /* 2399 'f' */
    { 0x2e100000, 12,  59}, /* 2400 ';' */
    { 0x2e500000, 12,   0}, /* 2401 '0x00' */
    { 0x2e000000, 13, 107}, /* 2402 'k' */
    { 0x2e400000, 13, 112}, /* 2403 'p' */
    { 0x2e080000, 14,  41}, /* 2404 ')' */
    { 0x2e0c0000, 14,  34}, /* 2405 '\"' */
    { 0x2e4a0000, 15,  99}, /* 2406 'c' */
    { 0x2e4e0000, 15,  47}, /* 2407 '/' */
    { 0x2e480000, 16,   1}, /* 2408 '0x01' */
    { 0x2e4c0000, 16,  93}, /* 2409 ']' */
    { 0x2e4d0000, 16, 122}, /* 2410 'z' */
    { 0x2e490000, 17,  96}, /* 2411 '`' */
    { 0x2e498000, 18, 118}, /* 2412 'v' */
    { 0x2e49c000, 18,  64}, /* 2413 '@' */
    /*   41                             */
    { 0x00000000,  1, 101}, /* 2414 'e' */
    { 0x80000000,  3, 111}, /* 2415 'o' */
    { 0xa0000000,  3, 105}, /* 2416 'i' */
    { 0xc0000000,  3,  32}, /* 2417 ' ' */
    { 0xf0000000,  4,  97}, /* 2418 'a' */
    { 0xe4000000,  6, 114}, /* 2419 'r' */
    { 0xec000000,  6, 116}, /* 2420 't' */
    { 0xe1000000,  8, 121}, /* 2421 'y' */
    { 0xe3000000,  8, 108}, /* 2422 'l' */
    { 0xe8000000,  8,  46}, /* 2423 '.' */
    { 0xe9000000,  8, 110}, /* 2424 'n' */
    { 0xeb000000,  8, 117}, /* 2425 'u' */
    { 0xe0000000,  9, 100}, /* 2426 'd' */
    { 0xe2000000,  9, 115}, /* 2427 's' */
    { 0xea000000,  9,  44}, /* 2428 ',' */
    { 0xe0c00000, 10, 119}, /* 2429 'w' */
    { 0xe2800000, 10,  39}, /* 2430 '\'' */
    { 0xe2c00000, 10,  45}, /* 2431 '-' */
    { 0xeaa00000, 11, 109}, /* 2432 'm' */
    { 0xeac00000, 11,  58}, /* 2433 ':' */
    { 0xeae00000, 11,  98}, /* 2434 'b' */
    { 0xe0900000, 12,  99}, /* 2435 'c' */
    { 0xe0b00000, 12,  63}, /* 2436 '?' */
    { 0xea800000, 12,  33}, /* 2437 '!' */
    { 0xe0800000, 13,  41}, /* 2438 ')' */
    { 0xe0a00000, 13, 104}, /* 2439 'h' */
    { 0xe0a80000, 13, 107}, /* 2440 'k' */
    { 0xea980000, 13, 102}, /* 2441 'f' */
    { 0xea940000, 14, 103}, /* 2442 'g' */
    { 0xe0880000, 15, 112}, /* 2443 'p' */
    { 0xe08a0000, 15,  59}, /* 2444 ';' */
    { 0xe08c0000, 15,  47}, /* 2445 '/' */
    { 0xe08e0000, 15,   0}, /* 2446 '0x00' */
    { 0xea920000, 15, 118}, /* 2447 'v' */
    { 0xea900000, 16, 113}, /* 2448 'q' */
    { 0xea910000, 17,   1}, /* 2449 '0x01' */
    { 0xea918000, 18,  34}, /* 2450 '\"' */
    { 0xea91c000, 20, 122}, /* 2451 'z' */
    { 0xea91d000, 20, 106}, /* 2452 'j' */
    { 0xea91e000, 20,  93}, /* 2453 ']' */
    { 0xea91f000, 20,  42}, /* 2454 '*' */
    /*   44                             */
    { 0x80000000,  2, 110}, /* 2455 'n' */
    { 0x00000000,  3, 116}, /* 2456 't' */
    { 0x40000000,  3, 115}, /* 2457 's' */
    { 0x30000000,  4, 108}, /* 2458 'l' */
    { 0xc0000000,  4, 111}, /* 2459 'o' */
    { 0xd0000000,  4,  99}, /* 2460 'c' */
    { 0xf0000000,  4, 101}, /* 2461 'e' */
    { 0x20000000,  5,  97}, /* 2462 'a' */
    { 0x60000000,  5, 109}, /* 2463 'm' */
    { 0x68000000,  5, 100}, /* 2464 'd' */
    { 0x70000000,  5, 118}, /* 2465 'v' */
    { 0xe0000000,  5, 103}, /* 2466 'g' */
    { 0xe8000000,  5, 114}, /* 2467 'r' */
    { 0x28000000,  6, 112}, /* 2468 'p' */
    { 0x78000000,  6, 102}, /* 2469 'f' */
    { 0x2c000000,  7, 122}, /* 2470 'z' */
    { 0x7c000000,  7,  32}, /* 2471 ' ' */
    { 0x2f000000,  8,  98}, /* 2472 'b' */
    { 0x7e000000,  8, 107}, /* 2473 'k' */
    { 0x2e000000,  9,  45}, /* 2474 '-' */
    { 0x2e800000,  9, 120}, /* 2475 'x' */
    { 0x7f000000, 10, 117}, /* 2476 'u' */
    { 0x7f800000, 10, 113}, /* 2477 'q' */
    { 0x7f400000, 11,  46}, /* 2478 '.' */
    { 0x7fc00000, 11,  44}, /* 2479 ',' */
    { 0x7f700000, 12, 119}, /* 2480 'w' */
    { 0x7ff00000, 12,  39}, /* 2481 '\'' */
    { 0x7f680000, 13, 105}, /* 2482 'i' */
    { 0x7fe80000, 13, 106}, /* 2483 'j' */
    { 0x7f640000, 14,  58}, /* 2484 ':' */
    { 0x7fe00000, 14, 104}, /* 2485 'h' */
    { 0x7f600000, 15,  47}, /* 2486 '/' */
    { 0x7fe60000, 15, 121}, /* 2487 'y' */
    { 0x7f630000, 16,  63}, /* 2488 '?' */
    { 0x7fe40000, 16,  80}, /* 2489 'P' */
    { 0x7f628000, 17,  82}, /* 2490 'R' */
    { 0x7fe58000, 17,  33}, /* 2491 '!' */
    { 0x7fe50000, 18,  41}, /* 2492 ')' */
    { 0x7fe54000, 18,  83}, /* 2493 'S' */
    { 0x7f620000, 19,   0}, /* 2494 '0x00' */
    { 0x7f622000, 19,  67}, /* 2495 'C' */
    { 0x7f624000, 19,   1}, /* 2496 '0x01' */
    { 0x7f626000, 20,  68}, /* 2497 'D' */
    { 0x7f627000, 20,  59}, /* 2498 ';' */
    /*   14                             */
    { 0x00000000,  1, 111}, /* 2499 'o' */
    { 0xc0000000,  2, 117}, /* 2500 'u' */
    { 0xa0000000,  3,  97}, /* 2501 'a' */
    { 0x90000000,  4, 101}, /* 2502 'e' */
    { 0x80000000,  5, 105}, /* 2503 'i' */
    { 0x8c000000,  6, 121}, /* 2504 'y' */
    { 0x8a000000,  7,  32}, /* 2505 ' ' */
    { 0x89000000,  8,  46}, /* 2506 '.' */
    { 0x88000000,  9,  39}, /* 2507 '\'' */
    { 0x88c00000, 10, 116}, /* 2508 't' */
    { 0x88800000, 11, 110}, /* 2509 'n' */
    { 0x88b00000, 12, 115}, /* 2510 's' */
    { 0x88a00000, 13,   1}, /* 2511 '0x01' */
    { 0x88a80000, 13, 104}, /* 2512 'h' */
    /*   40                             */
    { 0x80000000,  2,  32}, /* 2513 ' ' */
    { 0xc0000000,  2, 101}, /* 2514 'e' */
    { 0x00000000,  3, 115}, /* 2515 's' */
    { 0x60000000,  3, 105}, /* 2516 'i' */
    { 0x20000000,  4,  46}, /* 2517 '.' */
    { 0x48000000,  5, 121}, /* 2518 'y' */
    { 0x30000000,  6,  39}, /* 2519 '\'' */
    { 0x34000000,  6,  97}, /* 2520 'a' */
    { 0x38000000,  6, 112}, /* 2521 'p' */
    { 0x3c000000,  6,  44}, /* 2522 ',' */
    { 0x50000000,  6, 108}, /* 2523 'l' */
    { 0x54000000,  6, 102}, /* 2524 'f' */
    { 0x58000000,  6, 110}, /* 2525 'n' */
    { 0x40000000,  7,  47}, /* 2526 '/' */
    { 0x44000000,  7,  45}, /* 2527 '-' */
    { 0x46000000,  7, 111}, /* 2528 'o' */
    { 0x5d000000,  8,  58}, /* 2529 ':' */
    { 0x43800000,  9,  98}, /* 2530 'b' */
    { 0x5c000000,  9, 119}, /* 2531 'w' */
    { 0x5c800000,  9, 109}, /* 2532 'm' */
    { 0x5e000000,  9, 104}, /* 2533 'h' */
    { 0x5f000000,  9, 117}, /* 2534 'u' */
    { 0x5f800000,  9, 107}, /* 2535 'k' */
    { 0x42000000, 10, 114}, /* 2536 'r' */
    { 0x42400000, 10,   1}, /* 2537 '0x01' */
    { 0x42800000, 10,  63}, /* 2538 '?' */
    { 0x43000000, 10, 116}, /* 2539 't' */
    { 0x43400000, 10, 103}, /* 2540 'g' */
    { 0x5ec00000, 10, 100}, /* 2541 'd' */
    { 0x42c00000, 11, 106}, /* 2542 'j' */
    { 0x42e00000, 12,  41}, /* 2543 ')' */
    { 0x5e800000, 12,  59}, /* 2544 ';' */
    { 0x5e900000, 12,  99}, /* 2545 'c' */
    { 0x5ea00000, 12,  83}, /* 2546 'S' */
    { 0x42f00000, 13, 118}, /* 2547 'v' */
    { 0x42f80000, 13,  82}, /* 2548 'R' */
    { 0x5eb00000, 13,  33}, /* 2549 '!' */
    { 0x5eb80000, 14,  64}, /* 2550 '@' */
    { 0x5ebc0000, 15,  34}, /* 2551 '\"' */
    { 0x5ebe0000, 15,   0}, /* 2552 '0x00' */
    /*   43                             */
    { 0x40000000,  3,  97}, /* 2553 'a' */
    { 0x60000000,  3, 105}, /* 2554 'i' */
    { 0x80000000,  3, 108}, /* 2555 'l' */
    { 0xc0000000,  3, 101}, /* 2556 'e' */
    { 0xe0000000,  3,  32}, /* 2557 ' ' */
    { 0x00000000,  4, 117}, /* 2558 'u' */
    { 0x10000000,  4, 100}, /* 2559 'd' */
    { 0x20000000,  4, 121}, /* 2560 'y' */
    { 0xb0000000,  4, 111}, /* 2561 'o' */
    { 0xa0000000,  5, 115}, /* 2562 's' */
    { 0x30000000,  6,  46}, /* 2563 '.' */
    { 0x38000000,  6, 116}, /* 2564 't' */
    { 0x34000000,  7, 118}, /* 2565 'v' */
    { 0x3c000000,  7, 102}, /* 2566 'f' */
    { 0xa8000000,  7, 109}, /* 2567 'm' */
    { 0xaa000000,  7, 107}, /* 2568 'k' */
    { 0xac000000,  7, 112}, /* 2569 'p' */
    { 0x37000000,  8,  99}, /* 2570 'c' */
    { 0x3e000000,  8,  45}, /* 2571 '-' */
    { 0xaf000000,  8,  44}, /* 2572 ',' */
    { 0x36800000,  9,  58}, /* 2573 ':' */
    { 0x3f800000,  9,  98}, /* 2574 'b' */
    { 0xae000000,  9,  39}, /* 2575 '\'' */
    { 0x36000000, 10, 114}, /* 2576 'r' */
    { 0x36400000, 10, 104}, /* 2577 'h' */
    { 0x3f000000, 10, 110}, /* 2578 'n' */
    { 0x3f400000, 10, 103}, /* 2579 'g' */
    { 0xaec00000, 10, 119}, /* 2580 'w' */
    { 0xae800000, 11,  63}, /* 2581 '?' */
    { 0xaea00000, 13,  33}, /* 2582 '!' */
    { 0xaeb00000, 13, 122}, /* 2583 'z' */
    { 0xaeac0000, 14,  47}, /* 2584 '/' */
    { 0xaea80000, 15,  59}, /* 2585 ';' */
    { 0xaeb80000, 15,  69}, /* 2586 'E' */
    { 0xaeba0000, 15,  42}, /* 2587 '*' */
    { 0xaebe0000, 15,   0}, /* 2588 '0x00' */
    { 0xaeab0000, 16,   1}, /* 2589 '0x01' */
    { 0xaebc0000, 16,  41}, /* 2590 ')' */
    { 0xaeaa0000, 17,  64}, /* 2591 '@' */
    { 0xaeaa8000, 17, 106}, /* 2592 'j' */
    { 0xaebd0000, 17,  34}, /* 2593 '\"' */
    { 0xaebd8000, 18,  91}, /* 2594 '[' */
    { 0xaebdc000, 18,  93}, /* 2595 ']' */
    /*   40                             */
    { 0x00000000,  2,  97}, /* 2596 'a' */
    { 0x40000000,  2, 101}, /* 2597 'e' */
    { 0xe0000000,  3,  32}, /* 2598 ' ' */
    { 0x90000000,  4, 112}, /* 2599 'p' */
    { 0xb0000000,  4, 111}, /* 2600 'o' */
    { 0xc0000000,  4, 105}, /* 2601 'i' */
    { 0x80000000,  5,  46}, /* 2602 '.' */
    { 0x88000000,  5, 115}, /* 2603 's' */
    { 0xd0000000,  5, 117}, /* 2604 'u' */
    { 0xd8000000,  5, 109}, /* 2605 'm' */
    { 0xa4000000,  6, 121}, /* 2606 'y' */
    { 0xac000000,  6,  98}, /* 2607 'b' */
    { 0xa0000000,  7,  44}, /* 2608 ',' */
    { 0xa8000000,  7,  47}, /* 2609 '/' */
    { 0xa2000000,  8,  93}, /* 2610 ']' */
    { 0xa3000000,  9,  58}, /* 2611 ':' */
    { 0xaa000000,  9,  39}, /* 2612 '\'' */
    { 0xa3c00000, 10, 114}, /* 2613 'r' */
    { 0xaac00000, 10, 102}, /* 2614 'f' */
    { 0xab000000, 10, 108}, /* 2615 'l' */
    { 0xab800000, 10, 110}, /* 2616 'n' */
    { 0xa3800000, 11,  63}, /* 2617 '?' */
    { 0xa3a00000, 11,  33}, /* 2618 '!' */
    { 0xaa800000, 11,   0}, /* 2619 '0x00' */
    { 0xaaa00000, 11, 119}, /* 2620 'w' */
    { 0xab600000, 11, 104}, /* 2621 'h' */
    { 0xabc00000, 11,  45}, /* 2622 '-' */
    { 0xabe00000, 12,  52}, /* 2623 '4' */
    { 0xab500000, 13, 116}, /* 2624 't' */
    { 0xab580000, 13,  64}, /* 2625 '@' */
    { 0xabf00000, 13,  59}, /* 2626 ';' */
    { 0xabf80000, 13,  99}, /* 2627 'c' */
    { 0xab400000, 14,  41}, /* 2628 ')' */
    { 0xab440000, 14,   1}, /* 2629 '0x01' */
    { 0xab4c0000, 14, 100}, /* 2630 'd' */
    { 0xab4a0000, 15, 103}, /* 2631 'g' */
    { 0xab480000, 17,  91}, /* 2632 '[' */
    { 0xab488000, 17, 118}, /* 2633 'v' */
    { 0xab490000, 17, 107}, /* 2634 'k' */
    { 0xab498000, 17, 122}, /* 2635 'z' */
    /*   44                             */
    { 0x40000000,  2,  32}, /* 2636 ' ' */
    { 0x20000000,  3, 116}, /* 2637 't' */
    { 0x80000000,  3, 103}, /* 2638 'g' */
    { 0xe0000000,  3, 100}, /* 2639 'd' */
    { 0x00000000,  4, 115}, /* 2640 's' */
    { 0xa0000000,  4,  97}, /* 2641 'a' */
    { 0xd0000000,  4, 101}, /* 2642 'e' */
    { 0xb0000000,  5,  99}, /* 2643 'c' */
    { 0xc0000000,  5, 105}, /* 2644 'i' */
    { 0x1c000000,  6,  46}, /* 2645 '.' */
    { 0xbc000000,  6, 110}, /* 2646 'n' */
    { 0xcc000000,  6, 111}, /* 2647 'o' */
    { 0x12000000,  7, 117}, /* 2648 'u' */
    { 0x16000000,  7, 118}, /* 2649 'v' */
    { 0x18000000,  7, 102}, /* 2650 'f' */
    { 0xb8000000,  7, 107}, /* 2651 'k' */
    { 0xba000000,  7,  39}, /* 2652 '\'' */
    { 0xc8000000,  7, 121}, /* 2653 'y' */
    { 0xca000000,  7,  44}, /* 2654 ',' */
    { 0x10000000,  8, 109}, /* 2655 'm' */
    { 0x14000000,  8, 108}, /* 2656 'l' */
    { 0x15000000,  8,  45}, /* 2657 '-' */
    { 0x11800000,  9, 119}, /* 2658 'w' */
    { 0x1a800000,  9,  58}, /* 2659 ':' */
    { 0x11400000, 10, 122}, /* 2660 'z' */
    { 0x1a000000, 10, 104}, /* 2661 'h' */
    { 0x1b000000, 10,  98}, /* 2662 'b' */
    { 0x1b400000, 10, 106}, /* 2663 'j' */
    { 0x1b800000, 10, 114}, /* 2664 'r' */
    { 0x11000000, 11, 112}, /* 2665 'p' */
    { 0x1a600000, 11, 120}, /* 2666 'x' */
    { 0x1be00000, 11,  63}, /* 2667 '?' */
    { 0x11300000, 12,  59}, /* 2668 ';' */
    { 0x1a500000, 12,  41}, /* 2669 ')' */
    { 0x1bc00000, 12,  33}, /* 2670 '!' */
    { 0x1bd00000, 12, 113}, /* 2671 'q' */
    { 0x11200000, 13,  47}, /* 2672 '/' */
    { 0x11280000, 13,   0}, /* 2673 '0x00' */
    { 0x1a400000, 13,   1}, /* 2674 '0x01' */
    { 0x1a480000, 14,  66}, /* 2675 'B' */
    { 0x1a4c0000, 16,  93}, /* 2676 ']' */
    { 0x1a4d0000, 16,  34}, /* 2677 '\"' */
    { 0x1a4e0000, 16,  64}, /* 2678 '@' */
    { 0x1a4f0000, 16,  42}, /* 2679 '*' */
    /*   48                             */
    { 0x20000000,  3, 117}, /* 2680 'u' */
    { 0x60000000,  3,  32}, /* 2681 ' ' */
    { 0x80000000,  3, 114}, /* 2682 'r' */
    { 0xe0000000,  3, 110}, /* 2683 'n' */
    { 0x00000000,  4, 108}, /* 2684 'l' */
    { 0xc0000000,  4, 109}, /* 2685 'm' */
    { 0xd0000000,  4, 102}, /* 2686 'f' */
    { 0x10000000,  5, 118}, /* 2687 'v' */
    { 0x40000000,  5, 115}, /* 2688 's' */
    { 0x48000000,  5, 112}, /* 2689 'p' */
    { 0xa0000000,  5, 116}, /* 2690 't' */
    { 0xa8000000,  5, 111}, /* 2691 'o' */
    { 0xb8000000,  5, 119}, /* 2692 'w' */
    { 0x18000000,  6, 107}, /* 2693 'k' */
    { 0x1c000000,  6, 105}, /* 2694 'i' */
    { 0x50000000,  6, 103}, /* 2695 'g' */
    { 0x5c000000,  6,  99}, /* 2696 'c' */
    { 0xb4000000,  6, 100}, /* 2697 'd' */
    { 0x56000000,  7, 101}, /* 2698 'e' */
    { 0x58000000,  7, 121}, /* 2699 'y' */
    { 0x5a000000,  7,  97}, /* 2700 'a' */
    { 0xb2000000,  7,  98}, /* 2701 'b' */
    { 0x55000000,  8, 104}, /* 2702 'h' */
    { 0xb0000000,  8,  46}, /* 2703 '.' */
    { 0x54000000,  9,  45}, /* 2704 '-' */
    { 0x54800000,  9,  44}, /* 2705 ',' */
    { 0xb1400000, 10,  39}, /* 2706 '\'' */
    { 0xb1c00000, 10, 120}, /* 2707 'x' */
    { 0xb1200000, 11,  58}, /* 2708 ':' */
    { 0xb1a00000, 11, 122}, /* 2709 'z' */
    { 0xb1100000, 12,  63}, /* 2710 '?' */
    { 0xb1900000, 12, 106}, /* 2711 'j' */
    { 0xb1000000, 13,  33}, /* 2712 '!' */
    { 0xb1880000, 13, 113}, /* 2713 'q' */
    { 0xb1080000, 14,  74}, /* 2714 'J' */
    { 0xb1800000, 14,  47}, /* 2715 '/' */
    { 0xb1840000, 14,  41}, /* 2716 ')' */
    { 0xb10d0000, 16,  59}, /* 2717 ';' */
    { 0xb10e0000, 16,  71}, /* 2718 'G' */
    { 0xb10c0000, 17,  34}, /* 2719 '\"' */
    { 0xb10f0000, 17,   1}, /* 2720 '0x01' */
    { 0xb10c8000, 18,  93}, /* 2721 ']' */
    { 0xb10cc000, 18,  64}, /* 2722 '@' */
    { 0xb10f8000, 19,  52}, /* 2723 '4' */
    { 0xb10fa000, 19,   0}, /* 2724 '0x00' */
    { 0xb10fc000, 19,  66}, /* 2725 'B' */
    { 0xb10fe000, 20,  79}, /* 2726 'O' */
    { 0xb10ff000, 20,  67}, /* 2727 'C' */
    /*   39                             */
    { 0x20000000,  3, 108}, /* 2728 'l' */
    { 0x40000000,  3,  32}, /* 2729 ' ' */
    { 0x60000000,  3, 111}, /* 2730 'o' */
    { 0xa0000000,  3, 114}, /* 2731 'r' */
    { 0xe0000000,  3, 101}, /* 2732 'e' */
    { 0x00000000,  4, 112}, /* 2733 'p' */
    { 0xc0000000,  4,  97}, /* 2734 'a' */
    { 0xd0000000,  4, 105}, /* 2735 'i' */
    { 0x18000000,  5, 116}, /* 2736 't' */
    { 0x80000000,  5, 117}, /* 2737 'u' */
    { 0x88000000,  5, 104}, /* 2738 'h' */
    { 0x90000000,  5, 115}, /* 2739 's' */
    { 0x14000000,  6, 109}, /* 2740 'm' */
    { 0x12000000,  7, 100}, /* 2741 'd' */
    { 0x9a000000,  7, 121}, /* 2742 'y' */
    { 0x9c000000,  7,  46}, /* 2743 '.' */
    { 0x9e000000,  7,  44}, /* 2744 ',' */
    { 0x10000000,  8,  45}, /* 2745 '-' */
    { 0x11800000,  9,  63}, /* 2746 '?' */
    { 0x98800000,  9,  58}, /* 2747 ':' */
    { 0x99000000, 10,  39}, /* 2748 '\'' */
    { 0x99400000, 10,  93}, /* 2749 ']' */
    { 0x99800000, 10,  43}, /* 2750 '+' */
    { 0x99c00000, 10,  98}, /* 2751 'b' */
    { 0x11200000, 11, 102}, /* 2752 'f' */
    { 0x11400000, 11, 107}, /* 2753 'k' */
    { 0x11600000, 11,  33}, /* 2754 '!' */
    { 0x98200000, 11,  99}, /* 2755 'c' */
    { 0x98400000, 11, 110}, /* 2756 'n' */
    { 0x98600000, 11, 119}, /* 2757 'w' */
    { 0x11000000, 12,   0}, /* 2758 '0x00' */
    { 0x11100000, 12,  59}, /* 2759 ';' */
    { 0x98100000, 12,  47}, /* 2760 '/' */
    { 0x98080000, 13, 103}, /* 2761 'g' */
    { 0x98040000, 14,  41}, /* 2762 ')' */
    { 0x98020000, 15,  34}, /* 2763 '\"' */
    { 0x98010000, 16,  83}, /* 2764 'S' */
    { 0x98000000, 17,   1}, /* 2765 '0x01' */
    { 0x98008000, 17,  66}, /* 2766 'B' */
    /*   13                             */
    { 0x80000000,  1, 117}, /* 2767 'u' */
    { 0x00000000,  3,  44}, /* 2768 ',' */
    { 0x20000000,  3,  46}, /* 2769 '.' */
    { 0x60000000,  3,  32}, /* 2770 ' ' */
    { 0x48000000,  5,  98}, /* 2771 'b' */
    { 0x50000000,  5,  39}, /* 2772 '\'' */
    { 0x58000000,  5, 105}, /* 2773 'i' */
    { 0x44000000,  6,  97}, /* 2774 'a' */
    { 0x40000000,  8,  63}, /* 2775 '?' */
    { 0x41000000,  8,  58}, /* 2776 ':' */
    { 0x43000000,  8,  41}, /* 2777 ')' */
    { 0x42000000,  9,   1}, /* 2778 '0x01' */
    { 0x42800000,  9, 119}, /* 2779 'w' */
    /*   47                             */
    { 0x00000000,  3,  97}, /* 2780 'a' */
    { 0x20000000,  3, 111}, /* 2781 'o' */
    { 0x80000000,  3, 105}, /* 2782 'i' */
    { 0xc0000000,  3,  32}, /* 2783 ' ' */
    { 0xe0000000,  3, 101}, /* 2784 'e' */
    { 0x40000000,  4, 115}, /* 2785 's' */
    { 0x50000000,  4, 116}, /* 2786 't' */
    { 0x70000000,  5, 100}, /* 2787 'd' */
    { 0xa0000000,  5, 110}, /* 2788 'n' */
    { 0xa8000000,  5, 121}, /* 2789 'y' */
    { 0x68000000,  6, 117}, /* 2790 'u' */
    { 0x6c000000,  6, 109}, /* 2791 'm' */
    { 0x7c000000,  6, 107}, /* 2792 'k' */
    { 0xb4000000,  6, 108}, /* 2793 'l' */
    { 0xb8000000,  6,  46}, /* 2794 '.' */
    { 0xbc000000,  6, 114}, /* 2795 'r' */
    { 0x60000000,  7, 102}, /* 2796 'f' */
    { 0x64000000,  7,  44}, /* 2797 ',' */
    { 0x66000000,  7, 118}, /* 2798 'v' */
    { 0xb0000000,  7,  99}, /* 2799 'c' */
    { 0xb2000000,  7, 103}, /* 2800 'g' */
    { 0x63000000,  8,  39}, /* 2801 '\'' */
    { 0x78000000,  8,  45}, /* 2802 '-' */
    { 0x79000000,  8,  98}, /* 2803 'b' */
    { 0x7b000000,  8, 112}, /* 2804 'p' */
    { 0x62800000,  9,  58}, /* 2805 ':' */
    { 0x7a000000,  9, 119}, /* 2806 'w' */
    { 0x7a800000, 10,  63}, /* 2807 '?' */
    { 0x7ac00000, 10, 104}, /* 2808 'h' */
    { 0x62400000, 11,  33}, /* 2809 '!' */
    { 0x62100000, 12, 113}, /* 2810 'q' */
    { 0x62200000, 12, 106}, /* 2811 'j' */
    { 0x62300000, 12,   0}, /* 2812 '0x00' */
    { 0x62600000, 12,  47}, /* 2813 '/' */
    { 0x62080000, 13,  59}, /* 2814 ';' */
    { 0x62780000, 13,  41}, /* 2815 ')' */
    { 0x62700000, 14,  56}, /* 2816 '8' */
    { 0x62740000, 14, 122}, /* 2817 'z' */
    { 0x62020000, 15,  34}, /* 2818 '\"' */
    { 0x62060000, 15,  93}, /* 2819 ']' */
    { 0x62000000, 16,  84}, /* 2820 'T' */
    { 0x62040000, 16, 120}, /* 2821 'x' */
    { 0x62050000, 16,   1}, /* 2822 '0x01' */
    { 0x62018000, 17,  90}, /* 2823 'Z' */
    { 0x62010000, 18,  42}, /* 2824 '*' */
    { 0x62014000, 19,  68}, /* 2825 'D' */
    { 0x62016000, 19,  66}, /* 2826 'B' */
    /*   45                             */
    { 0x00000000,  1,  32}, /* 2827 ' ' */
    { 0xa0000000,  3, 116}, /* 2828 't' */
    { 0x80000000,  4,  46}, /* 2829 '.' */
    { 0xe0000000,  4, 101}, /* 2830 'e' */
    { 0x98000000,  5,  44}, /* 2831 ',' */
    { 0xc0000000,  5, 111}, /* 2832 'o' */
    { 0xc8000000,  5, 115}, /* 2833 's' */
    { 0xf0000000,  5, 104}, /* 2834 'h' */
    { 0xf8000000,  5, 105}, /* 2835 'i' */
    { 0x94000000,  6,  99}, /* 2836 'c' */
    { 0xd0000000,  6, 117}, /* 2837 'u' */
    { 0xd8000000,  6, 112}, /* 2838 'p' */
    { 0xde000000,  7,  97}, /* 2839 'a' */
    { 0x91000000,  8, 110}, /* 2840 'n' */
    { 0x93000000,  8, 109}, /* 2841 'm' */
    { 0xd4000000,  8, 121}, /* 2842 'y' */
    { 0xd6000000,  8,  58}, /* 2843 ':' */
    { 0xdc000000,  8, 108}, /* 2844 'l' */
    { 0xdd000000,  8, 107}, /* 2845 'k' */
    { 0x90800000,  9,  98}, /* 2846 'b' */
    { 0x92000000,  9, 102}, /* 2847 'f' */
    { 0xd5000000,  9, 119}, /* 2848 'w' */
    { 0xd7800000,  9,  39}, /* 2849 '\'' */
    { 0x90000000, 10,  33}, /* 2850 '!' */
    { 0x90400000, 10, 103}, /* 2851 'g' */
    { 0x92800000, 10, 114}, /* 2852 'r' */
    { 0xd5800000, 10,  63}, /* 2853 '?' */
    { 0xd5c00000, 10,  45}, /* 2854 '-' */
    { 0xd7400000, 10, 113}, /* 2855 'q' */
    { 0xd7200000, 11, 100}, /* 2856 'd' */
    { 0x92c00000, 12,  47}, /* 2857 '/' */
    { 0x92d00000, 12,  41}, /* 2858 ')' */
    { 0x92f00000, 12,   0}, /* 2859 '0x00' */
    { 0xd7000000, 12,  93}, /* 2860 ']' */
    { 0xd7100000, 12,  59}, /* 2861 ';' */
    { 0x92e80000, 13, 118}, /* 2862 'v' */
    { 0x92e20000, 15,  34}, /* 2863 '\"' */
    { 0x92e60000, 15, 122}, /* 2864 'z' */
    { 0x92e00000, 16, 106}, /* 2865 'j' */
    { 0x92e10000, 16,   1}, /* 2866 '0x01' */
    { 0x92e40000, 16,  91}, /* 2867 '[' */
    { 0x92e58000, 17,  64}, /* 2868 '@' */
    { 0x92e54000, 18,  84}, /* 2869 'T' */
    { 0x92e50000, 19, 120}, /* 2870 'x' */
    { 0x92e52000, 19,  96}, /* 2871 '`' */
    /*   49                             */
    { 0x80000000,  2, 104}, /* 2872 'h' */
    { 0x00000000,  3, 105}, /* 2873 'i' */
    { 0x40000000,  3, 111}, /* 2874 'o' */
    { 0x60000000,  3, 101}, /* 2875 'e' */
    { 0xe0000000,  3,  32}, /* 2876 ' ' */
    { 0x20000000,  4,  97}, /* 2877 'a' */
    { 0x30000000,  5, 117}, /* 2878 'u' */
    { 0xc8000000,  5, 114}, /* 2879 'r' */
    { 0xd8000000,  5, 115}, /* 2880 's' */
    { 0x3c000000,  6,  46}, /* 2881 '.' */
    { 0xc4000000,  6, 116}, /* 2882 't' */
    { 0xd0000000,  6, 121}, /* 2883 'y' */
    { 0x3a000000,  7,  99}, /* 2884 'c' */
    { 0xc2000000,  7, 108}, /* 2885 'l' */
    { 0x39000000,  8,  45}, /* 2886 '-' */
    { 0xc0000000,  8, 118}, /* 2887 'v' */
    { 0xc1000000,  8, 109}, /* 2888 'm' */
    { 0xd5000000,  8, 119}, /* 2889 'w' */
    { 0xd6000000,  8,  44}, /* 2890 ',' */
    { 0xd7000000,  8,  39}, /* 2891 '\'' */
    { 0x38000000,  9, 110}, /* 2892 'n' */
    { 0x38c00000, 10,  63}, /* 2893 '?' */
    { 0xd4400000, 10,  98}, /* 2894 'b' */
    { 0xd4800000, 10,  58}, /* 2895 ':' */
    { 0x38800000, 11,  33}, /* 2896 '!' */
    { 0xd4000000, 11, 122}, /* 2897 'z' */
    { 0xd4c00000, 11, 100}, /* 2898 'd' */
    { 0xd4e00000, 11, 102}, /* 2899 'f' */
    { 0xd4200000, 12, 120}, /* 2900 'x' */
    { 0x38a00000, 13, 103}, /* 2901 'g' */
    { 0x38a80000, 13,  59}, /* 2902 ';' */
    { 0xd4300000, 13, 112}, /* 2903 'p' */
    { 0x38b00000, 14,  80}, /* 2904 'P' */
    { 0x38b40000, 14,   0}, /* 2905 '0x00' */
    { 0x38bc0000, 14,  41}, /* 2906 ')' */
    { 0xd4380000, 14,  47}, /* 2907 '/' */
    { 0xd43c0000, 14, 107}, /* 2908 'k' */
    { 0x38b80000, 16,  64}, /* 2909 '@' */
    { 0x38ba0000, 16,  69}, /* 2910 'E' */
    { 0x38b98000, 17,  93}, /* 2911 ']' */
    { 0x38bb8000, 17,  34}, /* 2912 '\"' */
    { 0x38b90000, 18,  70}, /* 2913 'F' */
    { 0x38bb0000, 18,   1}, /* 2914 '0x01' */
    { 0x38b94000, 19, 106}, /* 2915 'j' */
    { 0x38bb4000, 19,  49}, /* 2916 '1' */
    { 0x38bb6000, 19,  91}, /* 2917 '[' */
    { 0x38b96000, 20,  92}, /* 2918 '\\' */
    { 0x38b97000, 21,  75}, /* 2919 'K' */
    { 0x38b97800, 21,  67}, /* 2920 'C' */
    /*   43                             */
    { 0x60000000,  3, 116}, /* 2921 't' */
    { 0xa0000000,  3, 110}, /* 2922 'n' */
    { 0xc0000000,  3, 115}, /* 2923 's' */
    { 0xe0000000,  3, 114}, /* 2924 'r' */
    { 0x10000000,  4, 100}, /* 2925 'd' */
    { 0x20000000,  4, 101}, /* 2926 'e' */
    { 0x50000000,  4, 108}, /* 2927 'l' */
    { 0x80000000,  4, 112}, /* 2928 'p' */
    { 0x00000000,  5,  98}, /* 2929 'b' */
    { 0x08000000,  5,  32}, /* 2930 ' ' */
    { 0x30000000,  5, 105}, /* 2931 'i' */
    { 0x40000000,  5,  97}, /* 2932 'a' */
    { 0x48000000,  5, 103}, /* 2933 'g' */
    { 0x90000000,  5,  99}, /* 2934 'c' */
    { 0x98000000,  5, 109}, /* 2935 'm' */
    { 0x38000000,  7, 121}, /* 2936 'y' */
    { 0x3a000000,  8, 122}, /* 2937 'z' */
    { 0x3c000000,  8,  39}, /* 2938 '\'' */
    { 0x3e000000,  8, 102}, /* 2939 'f' */
    { 0x3f000000,  8, 107}, /* 2940 'k' */
    { 0x3d000000,  9,  44}, /* 2941 ',' */
    { 0x3b400000, 10,  45}, /* 2942 '-' */
    { 0x3b800000, 10, 111}, /* 2943 'o' */
    { 0x3d800000, 10,  46}, /* 2944 '.' */
    { 0x3dc00000, 10, 120}, /* 2945 'x' */
    { 0x3b000000, 11, 119}, /* 2946 'w' */
    { 0x3bc00000, 11,  58}, /* 2947 ':' */
    { 0x3b200000, 12, 113}, /* 2948 'q' */
    { 0x3be00000, 12, 104}, /* 2949 'h' */
    { 0x3bf00000, 12, 118}, /* 2950 'v' */
    { 0x3b300000, 13, 106}, /* 2951 'j' */
    { 0x3b380000, 15, 117}, /* 2952 'u' */
    { 0x3b3a0000, 15,  63}, /* 2953 '?' */
    { 0x3b3e0000, 16,  47}, /* 2954 '/' */
    { 0x3b3f0000, 16,  33}, /* 2955 '!' */
    { 0x3b3c8000, 17,   1}, /* 2956 '0x01' */
    { 0x3b3c0000, 18,  92}, /* 2957 '\\' */
    { 0x3b3c4000, 18,   0}, /* 2958 '0x00' */
    { 0x3b3d0000, 18,  59}, /* 2959 ';' */
    { 0x3b3d8000, 18,  74}, /* 2960 'J' */
    { 0x3b3dc000, 18,  41}, /* 2961 ')' */
    { 0x3b3d4000, 19,  84}, /* 2962 'T' */
    { 0x3b3d6000, 19,  93}, /* 2963 ']' */
    /*   26                             */
    { 0x80000000,  1, 101}, /* 2964 'e' */
    { 0x40000000,  2, 105}, /* 2965 'i' */
    { 0x00000000,  3,  97}, /* 2966 'a' */
    { 0x30000000,  4, 111}, /* 2967 'o' */
    { 0x28000000,  5,  46}, /* 2968 '.' */
    { 0x24000000,  6,  32}, /* 2969 ' ' */
    { 0x21000000,  8,  39}, /* 2970 '\'' */
    { 0x23000000,  8, 121}, /* 2971 'y' */
    { 0x20000000,  9, 117}, /* 2972 'u' */
    { 0x22000000,  9, 115}, /* 2973 's' */
    { 0x22800000, 10, 114}, /* 2974 'r' */
    { 0x20800000, 11,  45}, /* 2975 '-' */
    { 0x20c00000, 11,  44}, /* 2976 ',' */
    { 0x22c00000, 11, 110}, /* 2977 'n' */
    { 0x20a00000, 12, 103}, /* 2978 'g' */
    { 0x20b00000, 12, 118}, /* 2979 'v' */
    { 0x20f00000, 12, 108}, /* 2980 'l' */
    { 0x22e00000, 12,  64}, /* 2981 '@' */
    { 0x22f00000, 12,  58}, /* 2982 ':' */
    { 0x20e80000, 13, 107}, /* 2983 'k' */
    { 0x20e00000, 14,  98}, /* 2984 'b' */
    { 0x20e40000, 15, 116}, /* 2985 't' */
    { 0x20e60000, 16, 100}, /* 2986 'd' */
    { 0x20e70000, 17,  47}, /* 2987 '/' */
    { 0x20e78000, 18,   1}, /* 2988 '0x01' */
    { 0x20e7c000, 18,  49}, /* 2989 '1' */
    /*   39                             */
    { 0x00000000,  2, 105}, /* 2990 'i' */
    { 0x60000000,  3, 101}, /* 2991 'e' */
    { 0x80000000,  3,  32}, /* 2992 ' ' */
    { 0xa0000000,  3, 115}, /* 2993 's' */
    { 0xc0000000,  3, 104}, /* 2994 'h' */
    { 0x50000000,  4,  97}, /* 2995 'a' */
    { 0xf0000000,  4, 111}, /* 2996 'o' */
    { 0x40000000,  5,  46}, /* 2997 '.' */
    { 0x48000000,  5, 119}, /* 2998 'w' */
    { 0xe0000000,  5, 110}, /* 2999 'n' */
    { 0xec000000,  7, 114}, /* 3000 'r' */
    { 0xea000000,  8,  44}, /* 3001 ',' */
    { 0xeb000000,  8, 108}, /* 3002 'l' */
    { 0xef000000,  8, 121}, /* 3003 'y' */
    { 0xe9000000,  9,  99}, /* 3004 'c' */
    { 0xee800000,  9,  98}, /* 3005 'b' */
    { 0xe8800000, 10,  58}, /* 3006 ':' */
    { 0xe9800000, 10, 109}, /* 3007 'm' */
    { 0xe9c00000, 10,  39}, /* 3008 '\'' */
    { 0xee000000, 10, 100}, /* 3009 'd' */
    { 0xe8000000, 11, 102}, /* 3010 'f' */
    { 0xe8200000, 11,  93}, /* 3011 ']' */
    { 0xe8400000, 11,  33}, /* 3012 '!' */
    { 0xe8e00000, 11, 107}, /* 3013 'k' */
    { 0xee400000, 11,  45}, /* 3014 '-' */
    { 0xe8700000, 12, 103}, /* 3015 'g' */
    { 0xe8c00000, 12,  63}, /* 3016 '?' */
    { 0xe8d00000, 12, 116}, /* 3017 't' */
    { 0xee700000, 12, 112}, /* 3018 'p' */
    { 0xe8600000, 13,   0}, /* 3019 '0x00' */
    { 0xe8680000, 13, 117}, /* 3020 'u' */
    { 0xee680000, 13,  41}, /* 3021 ')' */
    { 0xee640000, 14, 106}, /* 3022 'j' */
    { 0xee620000, 15, 113}, /* 3023 'q' */
    { 0xee600000, 16,  47}, /* 3024 '/' */
    { 0xee610000, 17,  59}, /* 3025 ';' */
    { 0xee61c000, 18,  91}, /* 3026 '[' */
    { 0xee618000, 19,   1}, /* 3027 '0x01' */
    { 0xee61a000, 19,  66}, /* 3028 'B' */
    /*   39                             */
    { 0x40000000,  2, 112}, /* 3029 'p' */
    { 0xc0000000,  2, 116}, /* 3030 't' */
    { 0xa0000000,  3,  32}, /* 3031 ' ' */
    { 0x00000000,  4, 105}, /* 3032 'i' */
    { 0x20000000,  4,  97}, /* 3033 'a' */
    { 0x80000000,  4,  99}, /* 3034 'c' */
    { 0x10000000,  5, 117}, /* 3035 'u' */
    { 0x38000000,  5, 101}, /* 3036 'e' */
    { 0x98000000,  5,  45}, /* 3037 '-' */
    { 0x18000000,  6, 102}, /* 3038 'f' */
    { 0x34000000,  6, 111}, /* 3039 'o' */
    { 0x90000000,  6,  46}, /* 3040 '.' */
    { 0x1c000000,  7,  44}, /* 3041 ',' */
    { 0x1e000000,  7, 109}, /* 3042 'm' */
    { 0x32000000,  7, 121}, /* 3043 'y' */
    { 0x31000000,  8,  57}, /* 3044 '9' */
    { 0x95000000,  8,  39}, /* 3045 '\'' */
    { 0x96000000,  8, 113}, /* 3046 'q' */
    { 0x30000000,  9, 115}, /* 3047 's' */
    { 0x94000000,  9,  58}, /* 3048 ':' */
    { 0x97000000,  9, 104}, /* 3049 'h' */
    { 0x30c00000, 10,  63}, /* 3050 '?' */
    { 0x94800000, 10, 108}, /* 3051 'l' */
    { 0x97800000, 10, 119}, /* 3052 'w' */
    { 0x30800000, 11,  65}, /* 3053 'A' */
    { 0x94c00000, 11, 120}, /* 3054 'x' */
    { 0x94e00000, 11,  98}, /* 3055 'b' */
    { 0x97e00000, 11,  41}, /* 3056 ')' */
    { 0x30b00000, 12,  47}, /* 3057 '/' */
    { 0x30a00000, 13,  52}, /* 3058 '4' */
    { 0x30a80000, 13,  33}, /* 3059 '!' */
    { 0x97c00000, 13, 103}, /* 3060 'g' */
    { 0x97c80000, 13,   0}, /* 3061 '0x00' */
    { 0x97d80000, 13,  59}, /* 3062 ';' */
    { 0x97d20000, 15, 118}, /* 3063 'v' */
    { 0x97d40000, 15,  70}, /* 3064 'F' */
    { 0x97d60000, 15,  69}, /* 3065 'E' */
    { 0x97d00000, 16,   1}, /* 3066 '0x01' */
    { 0x97d10000, 16,  67}, /* 3067 'C' */
    /*   45                             */
    { 0x80000000,  1,  32}, /* 3068 ' ' */
    { 0x00000000,  3, 111}, /* 3069 'o' */
    { 0x30000000,  4, 115}, /* 3070 's' */
    { 0x50000000,  4,  46}, /* 3071 '.' */
    { 0x48000000,  5,  44}, /* 3072 ',' */
    { 0x60000000,  5, 101}, /* 3073 'e' */
    { 0x70000000,  5,  39}, /* 3074 '\'' */
    { 0x24000000,  6,  97}, /* 3075 'a' */
    { 0x28000000,  6, 105}, /* 3076 'i' */
    { 0x2c000000,  6, 100}, /* 3077 'd' */
    { 0x44000000,  6, 110}, /* 3078 'n' */
    { 0x6c000000,  6,  58}, /* 3079 ':' */
    { 0x7c000000,  6, 108}, /* 3080 'l' */
    { 0x20000000,  7, 119}, /* 3081 'w' */
    { 0x68000000,  7, 116}, /* 3082 't' */
    { 0x6a000000,  7, 109}, /* 3083 'm' */
    { 0x7a000000,  7,  45}, /* 3084 '-' */
    { 0x23000000,  8,  98}, /* 3085 'b' */
    { 0x40000000,  8,  63}, /* 3086 '?' */
    { 0x41000000,  8, 114}, /* 3087 'r' */
    { 0x42000000,  8, 112}, /* 3088 'p' */
    { 0x43000000,  8, 102}, /* 3089 'f' */
    { 0x79000000,  8,  99}, /* 3090 'c' */
    { 0x22400000, 10,  59}, /* 3091 ';' */
    { 0x22800000, 10,  74}, /* 3092 'J' */
    { 0x78000000, 10, 104}, /* 3093 'h' */
    { 0x78400000, 10,  33}, /* 3094 '!' */
    { 0x78c00000, 10, 103}, /* 3095 'g' */
    { 0x22000000, 11,  41}, /* 3096 ')' */
    { 0x22e00000, 11,  47}, /* 3097 '/' */
    { 0x78a00000, 11,  93}, /* 3098 ']' */
    { 0x22300000, 12, 107}, /* 3099 'k' */
    { 0x22c00000, 12,   1}, /* 3100 '0x01' */
    { 0x22d00000, 12, 117}, /* 3101 'u' */
    { 0x78900000, 12,   0}, /* 3102 '0x00' */
    { 0x22200000, 13, 122}, /* 3103 'z' */
    { 0x78880000, 13,  34}, /* 3104 '\"' */
    { 0x222c0000, 14, 106}, /* 3105 'j' */
    { 0x78840000, 14,  50}, /* 3106 '2' */
    { 0x22280000, 15, 121}, /* 3107 'y' */
    { 0x222a0000, 15, 120}, /* 3108 'x' */
    { 0x78800000, 16, 118}, /* 3109 'v' */
    { 0x78810000, 16,  84}, /* 3110 'T' */
    { 0x78820000, 16,  69}, /* 3111 'E' */
    { 0x78830000, 16,  80}, /* 3112 'P' */
    /*   37                             */
    { 0x80000000,  2, 101}, /* 3113 'e' */
    { 0x20000000,  3,  97}, /* 3114 'a' */
    { 0x60000000,  3, 122}, /* 3115 'z' */
    { 0xc0000000,  3,  32}, /* 3116 ' ' */
    { 0xe0000000,  3, 105}, /* 3117 'i' */
    { 0x10000000,  4, 108}, /* 3118 'l' */
    { 0x40000000,  4, 121}, /* 3119 'y' */
    { 0x50000000,  5, 111}, /* 3120 'o' */
    { 0x00000000,  6,  99}, /* 3121 'c' */
    { 0x08000000,  6,  44}, /* 3122 ',' */
    { 0x0c000000,  6,  46}, /* 3123 '.' */
    { 0x58000000,  6, 119}, /* 3124 'w' */
    { 0x04000000,  7,  39}, /* 3125 '\'' */
    { 0x06000000,  8,  58}, /* 3126 ':' */
    { 0x07000000,  8, 116}, /* 3127 't' */
    { 0x5d000000,  8, 109}, /* 3128 'm' */
    { 0x5c000000,  9, 107}, /* 3129 'k' */
    { 0x5e000000,  9,  45}, /* 3130 '-' */
    { 0x5e800000,  9, 117}, /* 3131 'u' */
    { 0x5f800000,  9,  98}, /* 3132 'b' */
    { 0x5c800000, 11, 115}, /* 3133 's' */
    { 0x5ca00000, 11,  47}, /* 3134 '/' */
    { 0x5ce00000, 11, 100}, /* 3135 'd' */
    { 0x5f200000, 11, 112}, /* 3136 'p' */
    { 0x5f600000, 11,  63}, /* 3137 '?' */
    { 0x5cc00000, 12, 104}, /* 3138 'h' */
    { 0x5f000000, 12,  64}, /* 3139 '@' */
    { 0x5f400000, 12,  41}, /* 3140 ')' */
    { 0x5cd00000, 13,  33}, /* 3141 '!' */
    { 0x5f180000, 13, 118}, /* 3142 'v' */
    { 0x5f500000, 13, 103}, /* 3143 'g' */
    { 0x5cd80000, 14, 102}, /* 3144 'f' */
    { 0x5cdc0000, 14, 114}, /* 3145 'r' */
    { 0x5f100000, 14, 113}, /* 3146 'q' */
    { 0x5f140000, 14, 110}, /* 3147 'n' */
    { 0x5f580000, 14,   1}, /* 3148 '0x01' */
    { 0x5f5c0000, 14,  93}, /* 3149 ']' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 3150 '0x01' */
    { 0x80000000,  1,   1}, /* 3151 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 3152 '0x01' */
    { 0x80000000,  1,   1}, /* 3153 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 3154 '0x01' */
    { 0x80000000,  1,   0}, /* 3155 '0x00' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 3156 '0x01' */
    { 0x80000000,  1,   1}, /* 3157 '0x01' */
    /*    2                             */
    { 0x00000000,  1,   1}, /* 3158 '0x01' */
    { 0x80000000,  1,   1}  /* 3159 '0x01' */
};

unsigned huffIndex2[] = {
           0, /*   0 */
          51, /*   1 */
          53, /*   2 */
          53, /*   3 */
          55, /*   4 */
          57, /*   5 */
          59, /*   6 */
          61, /*   7 */
          63, /*   8 */
          65, /*   9 */
          67, /*  10 */
          69, /*  11 */
          71, /*  12 */
          73, /*  13 */
          75, /*  14 */
          77, /*  15 */
          79, /*  16 */
          81, /*  17 */
          83, /*  18 */
          85, /*  19 */
          87, /*  20 */
          89, /*  21 */
          91, /*  22 */
          93, /*  23 */
          95, /*  24 */
          97, /*  25 */
          99, /*  26 */
         101, /*  27 */
         103, /*  28 */
         105, /*  29 */
         107, /*  30 */
         109, /*  31 */
         111, /*  32 */
         191, /*  33 */
         204, /*  34 */
         240, /*  35 */
         242, /*  36 */
         250, /*  37 */
         253, /*  38 */
         265, /*  39 */
         336, /*  40 */
         391, /*  41 */
         403, /*  42 */
         416, /*  43 */
         419, /*  44 */
         439, /*  45 */
         503, /*  46 */
         568, /*  47 */
         619, /*  48 */
         654, /*  49 */
         692, /*  50 */
         724, /*  51 */
         760, /*  52 */
         795, /*  53 */
         827, /*  54 */
         860, /*  55 */
         891, /*  56 */
         922, /*  57 */
         950, /*  58 */
         959, /*  59 */
         961, /*  60 */
         963, /*  61 */
         965, /*  62 */
         967, /*  63 */
         979, /*  64 */
         985, /*  65 */
        1043, /*  66 */
        1083, /*  67 */
        1130, /*  68 */
        1177, /*  69 */
        1231, /*  70 */
        1267, /*  71 */
        1304, /*  72 */
        1336, /*  73 */
        1388, /*  74 */
        1417, /*  75 */
        1457, /*  76 */
        1493, /*  77 */
        1540, /*  78 */
        1575, /*  79 */
        1634, /*  80 */
        1673, /*  81 */
        1686, /*  82 */
        1723, /*  83 */
        1782, /*  84 */
        1826, /*  85 */
        1871, /*  86 */
        1909, /*  87 */
        1939, /*  88 */
        1963, /*  89 */
        1987, /*  90 */
        2005, /*  91 */
        2039, /*  92 */
        2041, /*  93 */
        2050, /*  94 */
        2052, /*  95 */
        2054, /*  96 */
        2085, /*  97 */
        2136, /*  98 */
        2175, /*  99 */
        2232, /* 100 */
        2278, /* 101 */
        2339, /* 102 */
        2374, /* 103 */
        2414, /* 104 */
        2455, /* 105 */
        2499, /* 106 */
        2513, /* 107 */
        2553, /* 108 */
        2596, /* 109 */
        2636, /* 110 */
        2680, /* 111 */
        2728, /* 112 */
        2767, /* 113 */
        2780, /* 114 */
        2827, /* 115 */
        2872, /* 116 */
        2921, /* 117 */
        2964, /* 118 */
        2990, /* 119 */
        3029, /* 120 */
        3068, /* 121 */
        3113, /* 122 */
        3150, /* 123 */
        3152, /* 124 */
        3154, /* 125 */
        3156, /* 126 */
        3158, /* 127 */
        3160  /* 128 */
};
//}}}

//{{{
const char* huffDecode (const unsigned char *src, size_t size) {

  const char* kNoneStr = "none";

  if ((src[0] == 0x1F) && (src[1] == 1 || src[1] == 2)) {
    struct tHuffTable* hufftable = (src[1] == 1) ? huffTable1 : huffTable2;
    unsigned int* huffindex = (src[1] == 1) ? huffIndex1 : huffIndex2;

    int uncompressed_len = 30;
    char* uncompressed = (char *)calloc(1, uncompressed_len + 1);
    int p = 0;

    unsigned value = 0;
    unsigned byte = 2;
    unsigned bit = 0;
    while (byte < 6 && byte < size) {
      value |= src[byte] << ((5-byte) * 8);
      byte++;
      }

    char lastch = START;
    do {
      bool found = false;
      unsigned bitShift = 0;
      char nextCh = STOP;
      if (lastch == ESCAPE) {
        found = true;

        // Encoded in the next 8 bits Terminated by the first ASCII character.
        nextCh = (value >> 24) & 0xff;
        bitShift = 8;
        if ((nextCh & 0x80) == 0) {
          if (nextCh < ' ')
            nextCh = STOP;
          lastch = nextCh;
          }
        }
      else {
        unsigned indx = (unsigned)lastch;
        for (unsigned j = huffindex[indx]; j < huffindex[indx+1]; j++) {
          unsigned mask = 0, maskbit = 0x80000000;
          for (short kk = 0; kk < hufftable[j].bits; kk++) {
            mask |= maskbit;
            maskbit >>= 1;
            }
          if ((value & mask) == hufftable[j].value) {
            nextCh = hufftable[j].next;
            bitShift = hufftable[j].bits;
            found = true;
            lastch = nextCh;
            break;
            }
          }
        }

      if (found) {
        if (nextCh != STOP && nextCh != ESCAPE) {
          if (p >= uncompressed_len) {
            uncompressed_len += 10;
            uncompressed = (char *)realloc (uncompressed, uncompressed_len + 1);
            }
          uncompressed[p++] = nextCh;
          uncompressed[p] = 0;
          }

        // Shift up by the number of bits.
        for (unsigned b = 0; b < bitShift; b++) {
          value = (value << 1) & 0xfffffffe;
          if (byte < size)
            value |= (src[byte] >> (7-bit)) & 1;
          if (bit == 7) {
            bit = 0;
            byte++;
            }
          else
            bit++;
          }
        }
      else
        return kNoneStr;
      } while (lastch != STOP && byte < size+4);

    return uncompressed;
    }
  else
    return NULL;
  }
//}}}
//}}}

//{{{
class cBitstream {
// used to parse H264 stream to find I frames
public:
  //{{{
  cBitstream (const uint8_t* buffer, uint32_t bit_len) {
    mDecBuffer = buffer;
    mDecBufferSize = bit_len;
    mNumOfBitsInBuffer = 0;
    mBookmarkOn = false;
    };
  //}}}
  ~cBitstream() {};

  //{{{
  uint32_t peekBits (uint32_t bits) {

    bookmark (true);
    uint32_t ret = getBits (bits);
    bookmark (false);
    return ret;
    }
  //}}}
  //{{{
  uint32_t getBits (uint32_t numBits) {

    //{{{
    static const uint32_t msk[33] = {
      0x00000000, 0x00000001, 0x00000003, 0x00000007,
      0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
      0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
      0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
      0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
      0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
      0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
      0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
      0xffffffff
      };
    //}}}

    if (numBits == 0)
      return 0;

    uint32_t retData;
    if (mNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
      mNumOfBitsInBuffer -= numBits;
      retData = mDecData >> mNumOfBitsInBuffer;
      // wmay - this gets done below...retData &= msk[numBits];
      }
    else {
      uint32_t nbits;
      nbits = numBits - mNumOfBitsInBuffer;
      if (nbits == 32)
        retData = 0;
      else
        retData = mDecData << nbits;

      switch ((nbits - 1) / 8) {
        case 3:
          nbits -= 8;
          if (mDecBufferSize < 8)
            return 0;
          retData |= *mDecBuffer++ << nbits;
          mDecBufferSize -= 8;
          // fall through
        case 2:
          nbits -= 8;
          if (mDecBufferSize < 8)
            return 0;
          retData |= *mDecBuffer++ << nbits;
          mDecBufferSize -= 8;
        case 1:
          nbits -= 8;
          if (mDecBufferSize < 8)
            return 0;
          retData |= *mDecBuffer++ << nbits;
          mDecBufferSize -= 8;
        case 0:
          break;
        }
      if (mDecBufferSize < nbits)
        return 0;

      mDecData = *mDecBuffer++;
      mNumOfBitsInBuffer = min(8u, mDecBufferSize) - nbits;
      mDecBufferSize -= min(8u, mDecBufferSize);
      retData |= (mDecData >> mNumOfBitsInBuffer) & msk[nbits];
      }

    return (retData & msk[numBits]);
    };
  //}}}

  //{{{
  uint32_t getUe() {

    uint32_t bits;
    uint32_t read;
    int bits_left;
    bool done = false;
    bits = 0;

    // we want to read 8 bits at a time - if we don't have 8 bits,
    // read what's left, and shift.  The exp_golomb_bits calc remains the same.
    while (!done) {
      bits_left = bits_remain();
      if (bits_left < 8) {
        read = peekBits (bits_left) << (8 - bits_left);
        done = true;
        }
      else {
        read = peekBits (8);
        if (read == 0) {
          getBits (8);
          bits += 8;
          }
        else
         done = true;
        }
      }

    uint8_t coded = exp_golomb_bits[read];
    getBits (coded);
    bits += coded;

    return getBits (bits + 1) - 1;
    }
  //}}}
  //{{{
  int32_t getSe() {

    uint32_t ret;
    ret = getUe();
    if ((ret & 0x1) == 0) {
      ret >>= 1;
      int32_t temp = 0 - ret;
      return temp;
      }

    return (ret + 1) >> 1;
    }
  //}}}

  //{{{
  void check_0s (int count) {

    uint32_t val = getBits (count);
    if (val != 0)
      cLog::log (LOGINFO, "field error - %d bits should be 0 is %x", count, val);
    }
  //}}}
  //{{{
  int bits_remain() {
    return mDecBufferSize + mNumOfBitsInBuffer;
    };
  //}}}
  //{{{
  int byte_align() {

    int temp = 0;
    if (mNumOfBitsInBuffer != 0)
      temp = getBits (mNumOfBitsInBuffer);
    else {
      // if we are byte aligned, check for 0x7f value - this will indicate
      // we need to skip those bits
      uint8_t readval = peekBits (8);
      if (readval == 0x7f)
        readval = getBits (8);
      }

    return temp;
    };
  //}}}

private:
  //{{{
  const uint8_t exp_golomb_bits[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
    };
  //}}}

  //{{{
  void bookmark (bool on) {

    if (on) {
      mNumOfBitsInBuffer_bookmark = mNumOfBitsInBuffer;
      mDecBuffer_bookmark = mDecBuffer;
      mDecBufferSize_bookmark = mDecBufferSize;
      mBookmarkOn = 1;
      mDecData_bookmark = mDecData;
      }

    else {
      mNumOfBitsInBuffer = mNumOfBitsInBuffer_bookmark;
      mDecBuffer = mDecBuffer_bookmark;
      mDecBufferSize = mDecBufferSize_bookmark;
      mDecData = mDecData_bookmark;
      mBookmarkOn = 0;
      }

    };
  //}}}

  uint32_t mNumOfBitsInBuffer;
  const uint8_t* mDecBuffer;
  uint8_t mDecData;
  uint8_t mDecData_bookmark;
  uint32_t mDecBufferSize;

  bool mBookmarkOn;
  uint32_t mNumOfBitsInBuffer_bookmark;
  const uint8_t* mDecBuffer_bookmark;
  uint32_t mDecBufferSize_bookmark;
  };
//}}}
//{{{
class cPidInfo {
public:
  //{{{
  cPidInfo (int pid, bool isSection) : mPid(pid), mIsSection(isSection) {

    switch (pid) {
      case PID_PAT: mInfo = "Pat"; break;
      case PID_CAT: mInfo = "Cat"; break;
      case PID_SDT: mInfo = "Sdt"; break;
      case PID_NIT: mInfo = "Nit"; break;
      case PID_EIT: mInfo = "Eit"; break;
      case PID_RST: mInfo = "Rst"; break;
      case PID_TDT: mInfo = "Tdt"; break;
      case PID_SYN: mInfo = "Syn"; break;
      default:;
      }
    }
  //}}}
  ~cPidInfo() {}

  void print() {
    cLog::log (LOGINFO, "pid:%d sid:%d streamType:%d - packets:%d disContinuity:%d repContinuity:%d",
                         mPid, mSid, mStreamType, mTotal, mDisContinuity, mRepeatContinuity);
    }

  int mPid;
  bool mIsSection;

  int mSid = -1;
  int mStreamType = 0;
  int mTotal = 0;

  int mContinuity = -1;
  int mDisContinuity = 0;
  int mRepeatContinuity = 0;

  int64_t mPts = -1;
  int64_t mFirstPts = -1;
  int64_t mLastPts = -1;

  int mSectionLength = 0;

  // content buffer
  int mBufSize = 0;
  uint8_t* mBuffer = nullptr;
  uint8_t* mBufPtr = nullptr;

  int64_t mStreamPos = -1;

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

    cLog::log (LOGINFO, prefix + timeStr + " " + dec(mDuration/60) + " " + mTitle);
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
  cService (int sid, int tsid, int onid, int type, int vid, int aud, const std::string& name) :
      mSid(sid), mTsid(tsid), mOnid(onid), mType(type), mVidPid(vid), mAudPid(aud), mName(name) {}
  //}}}
  ~cService() {}

  //{{{  gets
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

  int getProgramPid() const { return mProgramPid; }
  int getVidPid() const { return mVidPid; }
  int getAudPid() const { return mAudPid; }
  int getSubPid() const { return mSubPid; }

  cEpgItem* getNow() { return &mNow; }
  std::string getName() { return mName; }
  //}}}
  //{{{  sets
  //{{{
  void setVidPid (int pid, int streamType) {
    if (pid != mVidPid) {
      //cLog::log (LOGINFO, "setVidPid - esPid:%d streamType:%d", pid, streamType);
      mVidPid = pid;
      }
    }
  //}}}
  //{{{
  void setAudPid (int pid, int streamType) {
    if ((pid != mAudPid) && (pid != mAudPid1)) {
      // use first aud pid, may be many
      if (mAudPid == -1)
        mAudPid = pid;
      else if (mAudPid1 == -1)
        mAudPid1 = pid;
      //cLog::log (LOGINFO, "setAudPid - esPid:%d streamType:%d, mAudPid %d mAudPid1 %d",
      //                    pid, streamType, mAudPid, mAudPid1);
      }
    }
  //}}}

  void setSubPid (int pid, int streamType) { mSubPid = pid; }
  void setProgramPid (int pid) { mProgramPid = pid; }

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
  //}}}

  //{{{
  void print() {
    cLog::log (LOGINFO, "sid:%d tsid:%d onid:%d - prog:%d - v:%d - a:%d - sub:%d %s <%s>",
                        mSid, mTsid, mOnid,
                        mProgramPid, mVidPid, mAudPid,
                        mSubPid, getTypeStr().c_str(), mName.c_str());

    mNow.print ("");
    for (auto &epgItem : mEpgItemMap)
      epgItem.second.print ("- ");
    }
  //}}}

private:
  int mSid;
  int mTsid;
  int mOnid;
  int mType;

  int mProgramPid = -1;
  int mVidPid = -1;
  int mAudPid = -1;
  int mAudPid1 = -1;
  int mSubPid = -1;

  std::string mName;

  cEpgItem mNow;

  typedef std::map<time_t,cEpgItem> tEpgItemMap;  // startTime, cEpgItem
  tEpgItemMap mEpgItemMap;
  };
//}}}

class cTransportStream {
public:
  virtual ~cTransportStream() {}

  //{{{
  static char getFrameType (uint8_t* pesBuf, int64_t pesBufSize, int streamType) {
  // return frameType of video pes

    auto pesEnd = pesBuf + pesBufSize;
    if (streamType == 2) {
      //{{{  mpeg2 minimal parser
      while (pesBuf + 6 < pesEnd) {
        // look for pictureHeader 00000100
        if (!pesBuf[0] && !pesBuf[1] && (pesBuf[2] == 0x01) && !pesBuf[3])
          // extract frameType I,B,P
          switch ((pesBuf[5] >> 3) & 0x03) {
            case 1: return 'I';
            case 2: return 'P';
            case 3: return 'B';
            default: return '?';
            }
        pesBuf++;
        }
      }
      //}}}

    else if (streamType == 27) {
      // h264 minimal parser
      while (pesBuf < pesEnd) {
        //{{{  skip past startcode, find next startcode
        auto buf = pesBuf;
        auto bufSize = pesBufSize;

        uint32_t startOffset = 0;
        if (!buf[0] && !buf[1]) {
          if (!buf[2] && buf[3] == 1) {
            buf += 4;
            startOffset = 4;
            }
          else if (buf[2] == 1) {
            buf += 3;
            startOffset = 3;
            }
          }

        // find next start code
        auto offset = startOffset;
        uint32_t nalSize;
        uint32_t val = 0xffffffff;
        while (offset++ < bufSize - 3) {
          val = (val << 8) | *buf++;
          if (val == 0x0000001) {
            nalSize = offset - 4;
            break;
            }
          if ((val & 0x00ffffff) == 0x0000001) {
            nalSize = offset - 3;
            break;
            }

          nalSize = (uint32_t)bufSize;
          }
        //}}}

        if (nalSize > 3) {
          // parse NAL bitStream
          cBitstream bitstream (buf, (nalSize - startOffset) * 8);
          bitstream.check_0s (1);
          bitstream.getBits (2);
          switch (bitstream.getBits (5)) {
            case 1:
            case 5:
              //cLog::log(LOGINFO, "SLICE");
              bitstream.getUe();
              switch (bitstream.getUe()) {
                case 5: return 'P';
                case 6: return 'B';
                case 7: return 'I';
                default:return '?';
                }
              break;
            //case 6: cLog::log(LOGINFO, ("SEI"); break;
            //case 7: cLog::log(LOGINFO, ("SPS"); break;
            //case 8: cLog::log(LOGINFO, ("PPS"); break;
            //case 9: cLog::log(LOGINFO,  ("AUD"); break;
            //case 0x0d: cLog::log(LOGINFO, ("SEQEXT"); break;
            }
          }

        pesBuf += nalSize;
        }
      }

    return '?';
    }
  //}}}
  //{{{  gets
  uint64_t getPackets() { return mPackets; }
  int getDiscontinuity() { return mDiscontinuity; }

  std::string getTimeString() { return mTimeStr; }
  std::string getNetworkString() { return mNetworkNameStr; }

  //{{{
  bool getService (int index, int& audPid, int& vidPid, int64_t& basePts, int64_t& lastPts) {

    audPid = 0;
    vidPid = 0;
    basePts = -1;
    lastPts = -1;

    auto service = mServiceMap.begin();
    if (service != mServiceMap.end()) {
      audPid = service->second.getAudPid();
      vidPid = service->second.getVidPid();
      auto pidInfoIt = mPidInfoMap.find (audPid);
      if (pidInfoIt != mPidInfoMap.end()) {
        basePts = pidInfoIt->second.mFirstPts;
        lastPts = pidInfoIt->second.mLastPts;
        cLog::log (LOGINFO, "getService - vidPid:%d audPid:%d %s %s",
                            vidPid, audPid,
                            getFullPtsString(basePts).c_str(), getFullPtsString(lastPts).c_str());
        return true;
        }
      }

    return false;
    }
  //}}}
  //}}}

  //{{{
  int64_t demux (uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip) {
  // demux from tsBuffer to tsBuffer + tsBufferSize, streamPos is offset into full stream of first packet
  // - return bytes decoded

    auto tsPtr = tsBuf;
    auto tsEnd = tsBuf + tsBufSize;

    if (skip)
      //{{{  reset pid continuity, buffers
      for (auto &pidInfo : mPidInfoMap) {
        pidInfo.second.mBufPtr = nullptr;
        pidInfo.second.mStreamPos = -1;
        pidInfo.second.mContinuity = -1;
        }
      //}}}

    int lostSync = 0;
    bool decoded = false;
    while (!decoded && (tsPtr+188 <= tsEnd)) {
      if (*tsPtr++ != 0x47)
        lostSync++;
      else {
        if (lostSync) {
          //{{{  lostSync warning
          cLog::log (LOGINFO, "%d demux ****** resynced bytes:%d ******", mPackets, lostSync);
          streamPos += lostSync;
          lostSync = 0;
          }
          //}}}

        // check for full packet, followed by start syncCode if not end
        if ((tsPtr+187 >= tsEnd) || (*(tsPtr+187) == 0x47)) {
          //{{{  parse ts packet
          uint16_t pid = ((tsPtr[0] & 0x1F) << 8) | tsPtr[1];
          uint8_t continuityCount = tsPtr[2] & 0x0F;
          bool payloadStart = tsPtr[0] & 0x40;

          // skip past adaption field
          uint8_t headerBytes = (tsPtr[2] & 0x20) ? 4 + tsPtr[3] : 3;

          auto isSection = (pid == PID_PAT) || (pid == PID_SDT) ||
                           (pid == PID_EIT) || (pid == PID_TDT) ||
                           (mProgramMap.find (pid) != mProgramMap.end());

          // find or create pidInfo
          auto pidInfoIt = mPidInfoMap.find (pid);
          if (pidInfoIt == mPidInfoMap.end()) {
            // new pid, insert new cPidInfo, get pidInfoIt iterator
            auto insertPair = mPidInfoMap.insert (std::map<int,cPidInfo>::value_type (pid, cPidInfo(pid, isSection)));
            pidInfoIt = insertPair.first;
            pidInfoIt->second.mBufSize = kBufSize;
            pidInfoIt->second.mBuffer = (uint8_t*)malloc (kBufSize);
            }

          auto pidInfo = &pidInfoIt->second;

          // test continuity, reset buffers if fail
          if ((pid != 0x1FFF) &&
              (pidInfo->mContinuity >= 0) && (pidInfo->mContinuity != ((continuityCount-1) & 0x0F))) {
            // test continuity
            if (pidInfo->mContinuity == continuityCount) // strange case of bbc subtitles
              pidInfo->mRepeatContinuity++;
            else {
              mDiscontinuity++;
              pidInfo->mDisContinuity++;
              }
            pidInfo->mBufPtr = nullptr;
            }

          pidInfo->mContinuity = continuityCount;
          pidInfo->mTotal++;

          tsPtr += headerBytes;
          auto tsFrameBytesLeft = 187 - headerBytes;
          //}}}

          if (isSection) {
            //{{{  parse section pids
            if (payloadStart) {
              // parse sectionStart
              int pointerField = *tsPtr;
              if (pointerField && pidInfo->mBufPtr) {
                //{{{  payStart packet starts with end of lastSection
                // copy to end of buffer
                memcpy (pidInfo->mBufPtr, tsPtr+1, pointerField);

                // parse buffer if enough bytes for sectionLength
                pidInfo->mBufPtr += pointerField;
                if (pidInfo->mBufPtr - pidInfo->mBuffer >= pidInfo->mSectionLength)
                  parseSection (pid, pidInfo->mBuffer, 1);

                /// reset buffer
                pidInfo->mBufPtr = nullptr;
                }
                //}}}

              do {
                pidInfo->mSectionLength = ((tsPtr[pointerField+2] & 0x0F) << 8) | tsPtr[pointerField+3] + 3;
                if (pointerField < 183 - pidInfo->mSectionLength) {
                  parseSection (pid, tsPtr + pointerField + 1, 3);
                  pointerField += pidInfo->mSectionLength;
                  }
                else {
                  // section straddles packets, start buffering
                  if (183 - pointerField > 0) {
                    memcpy (pidInfo->mBuffer, tsPtr + pointerField + 1, 183 - pointerField);
                    pidInfo->mBufPtr = pidInfo->mBuffer + 183 - pointerField;
                    }
                  else
                    cLog::log (LOGERROR, "demux - section packet pid:%d pointerField:%d", pid, pointerField);
                  break;
                  }
                } while (tsPtr[pointerField+1] != 0xFF);
              }

            else if (pidInfo->mBufPtr) {
              // add to buffered section
              if (pidInfo->mBufPtr + tsFrameBytesLeft > pidInfo->mBuffer + pidInfo->mBufSize)
                cLog::log (LOGINFO, "%d sectionBuffer overflow > 4096 %d",
                        mPackets, (int)(pidInfo->mBufPtr - pidInfo->mBuffer));
              else {
                memcpy (pidInfo->mBufPtr, tsPtr, tsFrameBytesLeft);
                pidInfo->mBufPtr += tsFrameBytesLeft;
                }

              if ((pidInfo->mBufPtr - pidInfo->mBuffer) >= pidInfo->mSectionLength) {
                // enough bytes to parse buffered section
                parseSection (pid, pidInfo->mBuffer, 4);
                pidInfo->mBufPtr = 0;
                }
              }
            }
            //}}}
          else {
            if (payloadStart && !tsPtr[0] && !tsPtr[1] && (tsPtr[2] == 0x01)) {
              //{{{  start new payload
              // look for recognised streamId
              bool isVid = (tsPtr[3] == 0xE0);
              bool isAud = (tsPtr[3] == 0xBD) || (tsPtr[3] == 0xC0);

              if (isVid || isAud) {
                if (pidInfo->mBufPtr && pidInfo->mStreamType) {
                  if (isVid) {
                    decoded = vidDecodePes (pidInfo, skip);
                    skip = false;
                    }
                  else
                    decoded = audDecodePes (pidInfo, skip);
                  }

                // start next pes
                pidInfo->mBufPtr = pidInfo->mBuffer;
                pidInfo->mStreamPos = streamPos;

                pidInfo->mPts = (tsPtr[7] & 0x80) ? parseTimeStamp (tsPtr+9) : 0;
                if (pidInfo->mFirstPts == -1)
                  pidInfo->mFirstPts = pidInfo->mPts;
                if (pidInfo->mPts > pidInfo->mLastPts)
                  pidInfo->mLastPts = pidInfo->mPts;

                int pesHeaderBytes = 9 + tsPtr[8];
                tsPtr += pesHeaderBytes;
                tsFrameBytesLeft -= pesHeaderBytes;
                }
              }
              //}}}
            if (pidInfo->mBufPtr) {
              //{{{  copy rest of packet to mBuffer
              if (tsFrameBytesLeft <= 0)
                cLog::log (LOGERROR, "demux - copy error - pid:%d tsFrameBytesLeft:%d", pid, tsFrameBytesLeft);

              else if ((pidInfo->mBufPtr + tsFrameBytesLeft) <= (pidInfo->mBuffer + pidInfo->mBufSize)) {
                memcpy (pidInfo->mBufPtr, tsPtr, tsFrameBytesLeft);
                pidInfo->mBufPtr += tsFrameBytesLeft;
                }

              else {
                pidInfo->mBufSize *= 2;
                auto ptrOffset = pidInfo->mBufPtr - pidInfo->mBuffer;
                pidInfo->mBuffer = (uint8_t*)realloc (pidInfo->mBuffer, pidInfo->mBufSize);

                memcpy (pidInfo->mBuffer + ptrOffset, tsPtr, tsFrameBytesLeft);
                pidInfo->mBufPtr = pidInfo->mBuffer + ptrOffset + tsFrameBytesLeft;

                //cLog::log (LOGERROR, "demux - %d - realloc %d", pid, pidInfo->mBufSize);
                }
              }
              //}}}
            }

          tsPtr += tsFrameBytesLeft;
          streamPos += 188;
          mPackets++;
          }
        }
      }

    return tsPtr - tsBuf;
    }
  //}}}

  //{{{
  void printPidInfos() {

    cLog::log (LOGINFO, "--- pidInfos");
    for (auto &pidInfo : mPidInfoMap)
      pidInfo.second.print();
    cLog::log (LOGINFO, "---");
    }
  //}}}
  //{{{
  void printPrograms() {

    cLog::log (LOGINFO, "--- programs");
    for (auto &map : mProgramMap)
      cLog::log (LOGINFO, "programPid:%d sid:%d", map.first, map.second);
    cLog::log (LOGINFO, "---");
    }
  //}}}
  //{{{
  void printServices() {

    cLog::log (LOGINFO, "--- services");
    for (auto &service : mServiceMap)
      service.second.print();
    cLog::log (LOGINFO, "---");
    }
  //}}}

  // vars
  std::map<int,int>      mProgramMap; // PAT insert <pid,sid>'s    into mProgramMap
  std::map<int,cService> mServiceMap; // SDT insert <sid,cService> into mServiceMap
  std::map<int,cPidInfo> mPidInfoMap; // pid insert <pid,cPidInfo> into mPidInfoMap
                                      // PMT set cService pids
                                      // EIT add cService Now,Epg events
protected:
  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) { return false; }
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) { return false; }
  virtual void startProgram (int vpid, int apid, const std::string& name, const std::string& startTime) {}

private:
  //{{{
  void parsePat (uint8_t* buf) {
  // PAT declares programPid,sid to mProgramMap, recogneses programPid PMT to declare service streams

    auto Pat = (pat_t*)buf;
    auto sectionLength = HILO(Pat->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  bad crc
      cLog::log (LOGINFO, "parsePAT - bad crc %d", sectionLength);
      return;
      }
      //}}}
    if (Pat->table_id != TID_PAT) {
      //{{{  wrong tid
      cLog::log (LOGINFO, "parsePAT - unexpected TID %x", Pat->table_id);
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
        mProgramMap.insert (std::map<int,int>::value_type (pid, sid));

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
      cLog::log (LOGINFO, "parseSDT - bad crc %d", sectionLength);
      return;
      }
      //}}}
    if (Sdt->table_id == TID_SDT_OTH) // ignore other multiplex services for now
      return;
    if (Sdt->table_id != TID_SDT_ACT) {
      //{{{  wrong tid
      cLog::log (LOGINFO, "parseSDT - unexpected TID %x", Sdt->table_id);
      return;
      }
      //}}}

    //cLog::log (LOGINFO, "SDT - tsid:%d onid:%d", tsid, onid);

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
                auto pair = mServiceMap.insert (
                  std::map<int,cService>::value_type (sid, cService (sid, tsid, onid, serviceType, -1,-1, nameStr)));
                auto serviceIt = pair.first;
                cLog::log (LOGINFO, "SDT new cService tsid:%d sid:%d %s name<%s>",
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
            cLog::log (LOGINFO, "SDT - unexpected tag:%x", GetDescrTag(ptr));
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
      cLog::log (LOGINFO, "parsePMT - pid:%d bad crc %d", pid, sectionLength);
      return;
      }
      //}}}
    if (pmt->table_id != TID_PMT) {
      //{{{  wrong tid
      cLog::log (LOGINFO, "parsePMT - wrong TID %x", pmt->table_id);
      return;
      }
      //}}}

    auto sid = HILO (pmt->program_number);
    //cLog::log (LOGINFO, "PMT - pid:%d sid:%d", pid, sid);

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end()) {
      //{{{  add to service
      serviceIt->second.setProgramPid (pid);

      // point programPid to service by sid
      auto sectionIt = mPidInfoMap.find (pid);
      if (sectionIt != mPidInfoMap.end())
        sectionIt->second.mSid = sid;
      updatePidInfo (pid);

      //HILO (pmt->PCR_PID));

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
          case 2:  serviceIt->second.setVidPid (esPid, streamType); break; // ISO 13818-2 video
          case 27: serviceIt->second.setVidPid (esPid, streamType); break; // HD vid

          case 3:  serviceIt->second.setAudPid (esPid, streamType); break; // ISO 11172-3 audio
          case 4:  serviceIt->second.setAudPid (esPid, streamType); break; // ISO 13818-3 audio
          case 15: serviceIt->second.setAudPid (esPid, streamType); break; // HD aud ADTS
          case 17: serviceIt->second.setAudPid (esPid, streamType); break; // HD aud LATM
          case 129: serviceIt->second.setAudPid (esPid, streamType); break; // aud AC3

          case 6:  serviceIt->second.setSubPid (esPid, streamType); break; // subtitle

          case 5:  recognised = false; break;
          case 11: recognised = false; break;

          case 13:
          default:
            cLog::log (LOGINFO, "parsePmt - unknown streamType:%d sid:%d esPid:%d", streamType, sid, esPid);
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
      // simple tsFile with no SDT, pid 32 used to allocate service with sid
      cLog::log (LOGINFO, "parsePmt - serviceMap.insert pid 32");
      mServiceMap.insert (std::map<int,cService>::value_type (sid, cService (sid,0,0, kServiceTypeTV, -1,-1, "file32")));
      }
    else if (pid == 256) {
      // simple tsFile with no SDT, pid 256 used to allocate service with sid
      cLog::log (LOGINFO, "parsePmt - serviceMap.insert pid 0x100");
      mServiceMap.insert (std::map<int,cService>::value_type (sid, cService (sid,0,0, kServiceTypeTV, 258,257, "file256")));
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
      mTimeStr = dec(time.tm_hour,2,'0') + ":" +
                 dec(time.tm_min,2,'0') + ":" +
                 dec(time.tm_sec,2,'0') + ":00 " +
                 wday_name[time.tm_wday] + " " +
                 dec(time.tm_mday) + " " +
                 mon_name[time.tm_mon] + " " +
                 dec(1900 + time.tm_year);
      }
    }
  //}}}
  //{{{
  void parseNit (uint8_t* buf) {

    auto Nit = (nit_t*)buf;
    auto sectionLength = HILO(Nit->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  bad crc
      cLog::log (LOGINFO, "parseNIT - bad crc %d", sectionLength);
      return;
      }
      //}}}
    if ((Nit->table_id != TID_NIT_ACT) &&
        (Nit->table_id != TID_NIT_OTH) &&
        (Nit->table_id != TID_BAT)) {
      //{{{  wrong tid
      cLog::log (LOGINFO, "parseNIT - wrong TID %x", Nit->table_id);
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
  void parseEit (uint8_t* buf, int tag) {

    auto Eit = (eit_t*)buf;
    auto sectionLength = HILO(Eit->section_length) + 3;
    if (crc32 (buf, sectionLength)) {
      //{{{  error, return
      cLog::log (LOGINFO, "%d parseEit len:%d tag:%d Bad CRC  ", mPackets, sectionLength, tag);
      return;
      }
      //}}}

    auto tid = Eit->table_id;
    auto sid = HILO (Eit->service_id);
    auto now = (tid == TID_EIT_ACT);
    auto next = (tid == TID_EIT_OTH);
    auto epg = (tid == TID_EIT_ACT_SCH) || (tid == TID_EIT_OTH_SCH) ||
               (tid == TID_EIT_ACT_SCH+1) || (tid == TID_EIT_OTH_SCH+1);
    if (now || next || epg) {
      auto ptr = buf + EIT_LEN;
      sectionLength -= EIT_LEN + 4;
      while (sectionLength > 0) {
        auto EitEvent = (eit_event_t*)ptr;
        auto loopLength = HILO (EitEvent->descrs_loop_length);
        if (loopLength > sectionLength - EIT_EVENT_LEN)
          return;
        ptr += EIT_EVENT_LEN;

        // parse Descrs
        auto DescrLength = 0;
        while ((DescrLength < loopLength) &&
               (GetDescrLength (ptr) > 0) && (GetDescrLength (ptr) <= loopLength - DescrLength)) {
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
                cLog::log (LOGINFO, "EIT extendedEvent sid:%d descLen:%d lastDescNum:%d DescNum:%d item:%d",
                        sid, GetDescrLength (ptr),
                        CastExtendedEventDescr(ptr)->last_descr_number,
                        CastExtendedEventDescr(ptr)->descr_number,
                        CastExtendedEventDescr(ptr)->length_of_items);

                for (auto i = 0; i < GetDescrLength (ptr) -2; i++) {
                  char c = *(ptr + 2 + i);
                  if ((c >= 0x20) && (c <= 0x7F))
                    cLog::log (LOGINFO, "%c", c);
                  else
                    cLog::log (LOGINFO, "<%02x> ", c & 0xFF);
                  }
                //}}}
              #endif

              break;
              }
              //}}}
            }
          DescrLength += GetDescrLength (ptr);
          ptr += GetDescrLength (ptr);
          }
        sectionLength -= loopLength + EIT_EVENT_LEN;
        ptr += loopLength;
        }
      }
    else {
      //{{{  unexpected tid, error, return
      cLog::log (LOGINFO, "parseEIT - unexpected tid:%x", tid);
      return;
      }
      //}}}
    }
  //}}}
  //{{{
  void parseSection (int pid, uint8_t* buf, int tag) {

    switch (pid) {
      case PID_PAT: parsePat (buf); break;
      case PID_SDT: parseSdt (buf); break;
      case PID_TDT: parseTdt (buf); break;
      case PID_NIT: parseNit (buf); break;
      case PID_EIT: parseEit (buf, tag); break;
      default:      parsePmt (pid, buf); break;
      }
    }
  //}}}

  //{{{
  int64_t parseTimeStamp (uint8_t* tsPtr) {
  // return 33 bits of pts,dts

    if ((tsPtr[0] & 0x01) && (tsPtr[2] & 0x01) && (tsPtr[4] & 0x01)) {
      // valid marker bits
      int64_t pts = tsPtr[0] & 0x0E;
      pts = (pts << 7) | tsPtr[1];
      pts = (pts << 8) | (tsPtr[2] & 0xFE);
      pts = (pts << 7) | tsPtr[3];
      pts = (pts << 7) | (tsPtr[4] >> 1);
      return pts;
      }
    else {
      cLog::log (LOGNOTICE, "parseTs  %02x %02x %02x %02x 0x02 - invalid",
                             tsPtr[0], tsPtr[1],tsPtr[2],tsPtr[3],tsPtr[4]);
      return 0;
      }
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

        cLog::log (LOGINFO, "extended event - %d %d %c%c%c %s",
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
          cLog::log (LOGINFO, "- %s %s", text.c_str(), text2.c_str());

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
        cLog::log (LOGINFO, "component %2d %2d %d %s",
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
          //cLog::log (LOGINFO,  ("serviceList sid:%5d type:%d", sid, serviceType);
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
        cLog::log (LOGINFO, "parseDescr - %s unexpected descr:%02X", sectionName.c_str(), GetDescrTag(buf));
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
        std::string str = serviceIt->second.getName() + " - " + serviceIt->second.getNow()->mTitle;
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
  std::string mTimeStr;
  std::string mNetworkNameStr;

  uint64_t mPackets = 0;
  int mDiscontinuity = 0;

  time_t mCurTime;
  //}}}
  };
