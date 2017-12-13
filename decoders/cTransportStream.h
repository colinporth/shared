//{{{  cTransportStream.h - transport stream parser
// PAT inserts <pid,sid> into mProgramMap
// PMT declares pgm and elementary stream pids, adds cService into mServiceMap
// SDT names a service in mServiceMap
//}}}
//{{{  includes
#pragma once

#include <string>
#include <map>
#include <time.h>

#include "dvbSubtitle.h"
#include "huffman.h"

using namespace std;
//}}}
//{{{  macros
#define HILO(x) (x##_hi << 8 | x##_lo)

#define MjdToEpochTime(x) (unsigned int)((((x##_hi << 8) | x##_lo) - 40587) * 86400)

#define BcdTimeToSeconds(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                               (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                                     ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))
//}}}
//{{{  const, struct
const int kBufSize = 512;
//{{{  pid const
#define PID_PAT   0x00   /* Program Association Table */
#define PID_CAT   0x01   /* Conditional Access Table */
#define PID_NIT   0x10   /* Network Information Table */
#define PID_SDT   0x11   /* Service Description Table */
#define PID_EIT   0x12   /* Event Information Table */
#define PID_RST   0x13   /* Running Status Table */
#define PID_TDT   0x14   /* Time Date Table */
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

//{{{  pat_t
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
//}}}
//{{{  pat_prog_t
typedef struct {
  uint8_t program_number_hi         :8;
  uint8_t program_number_lo         :8;

  uint8_t network_pid_hi            :5;
  uint8_t                           :3;
  uint8_t network_pid_lo            :8;
  /* or program_map_pid (if prog_num=0)*/
  } pat_prog_t;
//}}}

//{{{  pmt_t
typedef struct {
   unsigned char table_id            :8;

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
//}}}
//{{{  pmt_info_t
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

//{{{  nit_t
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
//}}}
//{{{  nit_mid_t
typedef struct {                                 // after descrs
  uint8_t transport_stream_loop_length_hi  :4;
  uint8_t                                  :4;
  uint8_t transport_stream_loop_length_lo  :8;
  } nit_mid_t;
//}}}
//{{{  nit_end
struct nit_end_struct {
  long CRC;
  };
//}}}
//{{{  nit_ts_t
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

//{{{  eit_t
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
//}}}
//{{{  eit_event_t
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

//{{{  std_t
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
//}}}
//{{{  sdt_descr_t
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

//{{{  tdt_t
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

//{{{  descr_gen_t
typedef struct descr_gen_struct {
  uint8_t descr_tag        :8;
  uint8_t descr_length     :8;
  } descr_gen_t;
//}}}
#define getDescrTag(x) (((descr_gen_t *) x)->descr_tag)
#define getDescrLength(x) (((descr_gen_t *) x)->descr_length+2)

//{{{  0x48 service_descr
typedef struct descr_service_struct {
  uint8_t descr_tag             :8;
  uint8_t descr_length          :8;
  uint8_t service_type          :8;
  uint8_t provider_name_length  :8;
  } descr_service_t;
//}}}
//{{{  0x4D short_event_descr
typedef struct descr_short_event_struct {
  uint8_t descr_tag         :8;
  uint8_t descr_length      :8;
  uint8_t lang_code1        :8;
  uint8_t lang_code2        :8;
  uint8_t lang_code3        :8;
  uint8_t event_name_length :8;
  } descr_short_event_t;
//}}}
//{{{  0x4E extended_event_descr
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
typedef struct item_extended_event_struct {
  uint8_t item_description_length               :8;
  } item_extended_event_t;

#define CastExtendedEventItem(x) ((item_extended_event_t *)(x))
//}}}
//}}}

//{{{
uint32_t crc32 (uint32_t crc, uint8_t* data, int len) {

  //{{{
  // CRC32 lookup table for polynomial 0x04c11db7
  const unsigned long crcTable[256] = {
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

  for (auto i = 0; i < len; i++)
    crc = (crc << 8) ^ crcTable[((crc >> 24) ^ *data++) & 0xff];

  return crc;
  }
//}}}
//{{{
class cPidInfo {
public:
  cPidInfo (int pid, bool isPsi) : mPid(pid), mPsi(isPsi) {}
  ~cPidInfo() {}

  int getBufUsed() { return int(mBufPtr - mBuffer); }

  //{{{
  string getTypeString() {

    // known pids
    switch (mPid) {
      case PID_PAT: return "PAT";
      case PID_CAT: return "CAT";
      case PID_NIT: return "NIT";
      case PID_SDT: return "SDT";
      case PID_EIT: return "EIT";
      case PID_RST: return "RST";
      case PID_TDT: return "TDT";
      case PID_SYN: return "SYN";
      }

    if (mSid != -1) {
      // service - pgm or es
      switch (mStreamType) {
        case   0: return "pgm";
        case   2: return "m2v"; // ISO 13818-2 video
        case   3: return "m2a"; // ISO 11172-3 audio
        case   4: return "m3a"; // ISO 13818-3 audio
        case   5: return "mtd"; // private mpeg2 tabled data - private
        case   6: return "sub"; // subtitle
        case  11: return "dsm"; // dsm cc u_n
        case  13: return "dsm"; // dsm cc tabled data
        case  15: return "aac"; // HD aud ADTS
        case  17: return "aac"; // HD aud LATM
        case  27: return "264"; // HD vid
        case 129: return "ac3"; // aud AC3
        default : return dec(mStreamType,3);
        }
      }

    // unknown pid
    return "---";
    }
  //}}}
  //{{{
  string getInfoString() {

    return (mSid > 0 ? dec(mSid) : "") + (mInfoStr.empty() ? "" : " " + mInfoStr);
    }
  //}}}
  //{{{
  int addToBuffer (uint8_t* buf, int bufSize) {

    if (getBufUsed() + bufSize > mBufSize) {
      // realloc buffer to twice size
      mBufSize *= 2;
      cLog::log (LOGINFO1, "demux pid:" + dec(mPid) + " realloc to " + dec(mBufSize));

      auto ptrOffset = getBufUsed();
      mBuffer = (uint8_t*)realloc (mBuffer, mBufSize);
      mBufPtr = mBuffer + ptrOffset;
      }

    memcpy (mBufPtr, buf, bufSize);
    mBufPtr += bufSize;

    return getBufUsed();
    }
  //}}}

  //{{{
  void clearContinuity() {

    mBufPtr = nullptr;
    mStreamPos = -1;
    mContinuity = -1;
    }
  //}}}
  //{{{
  void clearCounts() {

    mPackets = 0;
    mDisContinuity = 0;
    mRepeatContinuity = 0;
    }
  //}}}

  //{{{
  void print() {
    cLog::log (LOGINFO, "pid:%d sid:%d streamType:%d - packets:%d disContinuity:%d repContinuity:%d",
                        mPid, mSid, mStreamType, mPackets, mDisContinuity, mRepeatContinuity);
    }
  //}}}

  int mPid;
  bool mPsi;

  int mSid = -1;
  int mStreamType = 0;

  int mPackets = 0;
  int mContinuity = -1;
  int mDisContinuity = 0;
  int mRepeatContinuity = 0;

  int64_t mPts = -1;
  int64_t mFirstPts = -1;
  int64_t mLastPts = -1;

  int mSectionLength = 0;

  int mBufSize = 0;
  uint8_t* mBuffer = nullptr;
  uint8_t* mBufPtr = nullptr;

  int64_t mStreamPos = -1;

  string mInfoStr;
  };
//}}}
//{{{
class cEpgItem {
public:
  //{{{
  cEpgItem (time_t startTime, int duration, const string& title, string shortDescription)
      : mStartTime(startTime), mDuration(duration), mTitle(title), mShortDescription(shortDescription) {}
  //}}}

  time_t getStartTime() { return mStartTime; }

  string getTitleString() { return mTitle; }
  string getStartTimeString() { return getTimetShortString (mStartTime); }
  string getDurationString() { return dec(mDuration/60) + "m"; }

  //{{{
  void set (time_t startTime, int duration, const string& title, string shortDescription) {

    mDuration = duration;
    mStartTime = startTime;

    mTitle = title;
    mShortDescription = shortDescription;
    }
  //}}}

  //{{{
  void print (string prefix) {

    struct tm time = *localtime (&mStartTime);
    char* timeStr = asctime (&time);
    timeStr[24] = 0;

    cLog::log (LOGINFO, prefix + timeStr + " " + dec(mDuration/60) + " " + mTitle);
    }
  //}}}

private:
  time_t mStartTime = 0;
  int mDuration = 0;

  string mTitle;
  string mShortDescription;
  };
//}}}
//{{{
class cService {
public:
  cService (int sid) : mSid(sid) {}
  ~cService() {}

  //{{{  gets
  int getSid() const { return mSid; }

  int getProgramPid() const { return mProgramPid; }

  int getVidPid() const { return mVidPid; }
  int getVidStreamType() const { return mVidStreamType; }

  int getAudPid() const { return mAudPid; }
  int getAudStreamType() const { return mAudStreamType; }
  int getAudOtherPid() const { return mAudOtherPid; }

  int getSubPid() const { return mSubPid; }

  string getNameString() { return mName; }

  vector<cEpgItem*> getNowVec() { return mNowVec; }
  string getNowTitleString() { return mNowVec.empty() ? "" : mNowVec[0]->getTitleString(); }
  //}}}
  //{{{  sets
  void setName (const string& name) { mName = name;}

  //{{{
  void setVidPid (int pid, int streamType) {
    mVidPid = pid;
    mVidStreamType = streamType;
    }
  //}}}
  void setSubPid (int pid, int streamType) { mSubPid = pid; }
  void setProgramPid (int pid) { mProgramPid = pid; }
  //{{{
  void setAudPid (int pid, int streamType) {

    if ((pid != mAudPid) && (pid != mAudOtherPid)) {
      // use first aud pid, may be many
      if (mAudPid == -1) {
        mAudPid = pid;
        mAudStreamType = streamType;
        }
      else if (mAudOtherPid == -1)
        mAudOtherPid = pid;
      }
    }
  //}}}

  //{{{
  bool setNow (time_t startTime, int duration, string str1, string str2) {

    for (auto epgItem : mNowVec)
      if (startTime == epgItem->getStartTime())
        return false;

    mNowVec.push_back (new cEpgItem (startTime, duration, str1, str2));
    return true;
    }
  //}}}
  //{{{
  void setEpg (time_t startTime, int duration, string str1, string str2) {

    mEpgItemMap.insert (map<time_t, cEpgItem>::value_type (startTime, cEpgItem(startTime, duration, str1, str2)));
    }
  //}}}
  //}}}

  //{{{
  void print() {
    cLog::log (LOGINFO, "sid:" + dec(mSid) +
                        " pgmPid:" + dec (mProgramPid) +
                        " vidPid:" + dec(mVidPid) +
                        " audPid:" + dec (mAudPid) +
                        " subPid:" + dec (mSubPid) +
                        " " + mName);

    for (auto nowEpgItem : mNowVec)
      nowEpgItem->print ("");
    for (auto &epgItem : mEpgItemMap)
      epgItem.second.print ("- ");
    }
  //}}}

private:
  int mSid;

  int mProgramPid = -1;
  int mVidPid = -1;
  int mVidStreamType = 0;
  int mAudPid = -1;
  int mAudOtherPid = -1;
  int mAudStreamType = 0;
  int mSubPid = -1;

  string mName;

  vector<cEpgItem*> mNowVec;
  map<time_t, cEpgItem> mEpgItemMap;
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

  uint64_t getPackets() { return mPackets; }
  uint64_t getDiscontinuity() { return mDiscontinuity; }
  //{{{
  cService* getService (int index, int64_t& firstPts, int64_t& lastPts) {

    firstPts = -1;
    lastPts = -1;

    int i = 0;
    for (auto &service : mServiceMap) {
      if (i == index) {
        auto pidInfoIt = mPidInfoMap.find (service.second.getAudPid());
        if (pidInfoIt != mPidInfoMap.end()) {
          firstPts = pidInfoIt->second.mFirstPts;
          lastPts = pidInfoIt->second.mLastPts;
          cLog::log (LOGNOTICE, "getService " + dec(index) +
                                " firstPts:" + getFullPtsString (firstPts) +
                                " lastPts:" + getFullPtsString (lastPts));
          return &service.second;
          }
        }
      i++;
      }

    return nullptr;
    }
  //}}}

  //{{{
  int64_t demux (uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip, int decodePid) {
  // demux from tsBuffer to tsBuffer + tsBufferSize, streamPos is offset into full stream of first packet
  // if decodePid != -1, only process pesPid == decodePid, saves lots of buffering
  // - return bytes decoded

    if (skip)
      clearContinuity();

    bool decoded = false;
    auto tsPtr = tsBuf;
    auto tsEnd = tsBuf + tsBufSize;
    auto nextTsPtr = tsPtr + 188;
    while (!decoded && (nextTsPtr <= tsEnd)) {
      if (*tsPtr == 0x47) {
        // check for full packet, followed by start syncCode if not end
        if ((tsPtr+188 >= tsEnd) || (*(tsPtr+188) == 0x47)) {
          tsPtr++;
          int tsBytesLeft = 188-1;

          // parse ts packetHeader
          int pid = ((tsPtr[0] & 0x1F) << 8) | tsPtr[1];
          if (pid != 0x1FFF) {
            int continuityCount = tsPtr[2] & 0x0F;
            bool payloadStart = tsPtr[0] & 0x40;
            int headerBytes = (tsPtr[2] & 0x20) ? 4 + tsPtr[3] : 3; // adaption field

            auto pidInfo = findPidCreatePsiPidInfo (pid);
            if (pidInfo) {
              if ((pidInfo->mContinuity >= 0) && (continuityCount != (pidInfo->mContinuity+1 & 0x0F))) {
                //{{{  continuity error
                if (pidInfo->mContinuity == continuityCount) // strange case of bbc subtitles
                  pidInfo->mRepeatContinuity++;

                else {
                  mDiscontinuity++;
                  pidInfo->mDisContinuity++;
                  }

                // abandon any buffered pes or section
                pidInfo->mBufPtr = nullptr;
                }
                //}}}
              pidInfo->mContinuity = continuityCount;
              pidInfo->mPackets++;

              if (pidInfo->mPsi) {
                //{{{  psi packet body
                tsPtr += headerBytes;
                tsBytesLeft -= headerBytes;

                if (payloadStart) {
                  auto payloadPtr = tsPtr;
                  auto payloadBytesLeft = tsBytesLeft;

                  auto pointerField = *payloadPtr++;
                  payloadBytesLeft--;
                  if ((pointerField > 0) && pidInfo->mBufPtr)
                    // nonZero pointerField, finish lastSection
                    addToSectionBuffer (pidInfo, payloadPtr, pointerField);

                  // goto real payload start
                  payloadPtr += pointerField;
                  payloadBytesLeft -= pointerField;

                  if (pidInfo->mBufPtr)
                    cLog::log (LOGINFO1, "demux - unused section buffer " + dec(pid) +
                                         " sectionLength:" + dec(pidInfo->mSectionLength) +
                                         " got:" +  dec(pidInfo->getBufUsed()));

                  while ((payloadBytesLeft >= 3) && (payloadPtr[0] != 0xFF)) {
                    // valid section tableId, get section length
                    pidInfo->mSectionLength = ((payloadPtr[1] & 0x0F) << 8) + payloadPtr[2] + 3;;
                    if (payloadBytesLeft >= pidInfo->mSectionLength) {
                      // parse section from payload without buffer
                      parsePsi (pidInfo, payloadPtr);
                      payloadPtr += pidInfo->mSectionLength;
                      payloadBytesLeft -= pidInfo->mSectionLength;
                      pidInfo->mBufPtr = nullptr;
                      }
                    else {
                      // start payload buffer, straddles packets
                      memcpy (pidInfo->mBuffer, payloadPtr, payloadBytesLeft);
                      pidInfo->mBufPtr = pidInfo->mBuffer + payloadBytesLeft;
                      payloadBytesLeft = 0;
                      }
                    }
                  }

                else if (pidInfo->mBufPtr)
                  addToSectionBuffer (pidInfo, tsPtr, tsBytesLeft);

                else
                  cLog::log (LOGINFO1, "demux - trying to add section to section not started " + dec(pid) +
                                       " " +  dec(pidInfo->mSectionLength));
                }
                //}}}
              else if ((decodePid == -1) || (decodePid == pid)) {
                //{{{  pes packet body
                pesPacket (pidInfo, tsPtr-1);

                tsPtr += headerBytes;
                tsBytesLeft -= headerBytes;
                if (payloadStart && !tsPtr[0] && !tsPtr[1] && (tsPtr[2] == 0x01)) {
                  // start new payload, recognise streamIds
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

                    pidInfo->mStreamPos = streamPos;

                    // form pts, firstPts, lastPts
                    pidInfo->mPts = (tsPtr[7] & 0x80) ? getPtsDts (tsPtr+9) : -1;
                    if (pidInfo->mFirstPts == -1)
                      pidInfo->mFirstPts = pidInfo->mPts;
                    if (pidInfo->mPts > pidInfo->mLastPts)
                      pidInfo->mLastPts = pidInfo->mPts;

                    // skip past pesHeader
                    int pesHeaderBytes = 9 + tsPtr[8];
                    tsPtr += pesHeaderBytes;
                    tsBytesLeft -= pesHeaderBytes;

                    // start new buffer
                    pidInfo->mBufPtr = pidInfo->mBuffer;
                    }
                  }

                if (pidInfo->mBufPtr && (tsBytesLeft > 0))
                  pidInfo->addToBuffer (tsPtr, tsBytesLeft);
                }
                //}}}
              }
            }

          tsPtr = nextTsPtr;
          nextTsPtr = tsPtr + 188;
          streamPos += 188;
          mPackets++;
          }
        }
      else {
        cLog::log (LOGERROR, "demux - lostSync");
        return tsEnd - tsBuf;
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
  map<int,cPidInfo> mPidInfoMap;
  map<int,cService> mServiceMap;

protected:
  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) { return false; }
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) { return false; }
  virtual void startProgram (cService* service, const string& name, time_t startTime) {}
  virtual void pesPacket (cPidInfo* pidInfo, uint8_t* tsPtr) {}

  //{{{
  void clearContinuity() {

    for (auto &pidInfo : mPidInfoMap)
      pidInfo.second.clearContinuity();
    }
  //}}}
  //{{{
  void clearCounts() {

    for (auto &pidInfo : mPidInfoMap)
      pidInfo.second.clearCounts();
    }
  //}}}

private:
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
        cLog::log (LOGERROR, "field error - %d bits should be 0 is %x", count, val);
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
  cPidInfo* findPidCreatePsiPidInfo (int pid) {
  // find pidInfo by pid
  // - create pidInfo for psi pids

    auto pidInfoIt = mPidInfoMap.find (pid);
    if (pidInfoIt == mPidInfoMap.end()) {
      if ((pid == PID_PAT) || (pid == PID_CAT) || (pid == PID_NIT) || (pid == PID_SDT) ||
          (pid == PID_EIT) || (pid == PID_RST) || (pid == PID_TDT) || (pid == PID_SYN) ||
          (mProgramMap.find (pid) != mProgramMap.end())) {
        // create new psi cPidInfo, insert
        auto insertPair = mPidInfoMap.insert (map<int,cPidInfo>::value_type (pid, cPidInfo(pid, true)));

        // allocate buffer
        pidInfoIt = insertPair.first;
        auto pidInfo = &pidInfoIt->second;

        pidInfo->mBufSize = kBufSize;
        pidInfo->mBuffer = (uint8_t*)malloc (kBufSize);
        //cLog::log (LOGINFO, "findPidCreatePsiPidInfo - create " + dec(pid));
        return pidInfo;
        }
      else
        return nullptr;
      }

    // return psi and pes pidInfos
    return &pidInfoIt->second;
    }
  //}}}
  //{{{
  cPidInfo* createEsPidInfo (int pid, int sid, int streamType) {
  // look for pid, if none create esPidInfo

    auto pidInfoIt = mPidInfoMap.find (pid);
    if (pidInfoIt == mPidInfoMap.end()) {
      // create and  insert new cPidInfo, allocate buffer
      auto insertPair = mPidInfoMap.insert (map<int,cPidInfo>::value_type (pid, cPidInfo(pid, false)));
      pidInfoIt = insertPair.first;

      pidInfoIt->second.mSid = sid;
      pidInfoIt->second.mStreamType = streamType;
      pidInfoIt->second.mBufSize = kBufSize;
      pidInfoIt->second.mBuffer = (uint8_t*)malloc (kBufSize);

      //cLog::log (LOGINFO, "createEsPidInfo - created " + dec(pid) + " " + dec(sid) + " " + dec(streamType));
      return &pidInfoIt->second;
      }

    return nullptr;
    }
  //}}}
  //{{{
  void addToSectionBuffer (cPidInfo* pidInfo, uint8_t* buf, int bufSize) {

    if (pidInfo->addToBuffer (buf, bufSize) >= pidInfo->mSectionLength) {
      parsePsi (pidInfo, pidInfo->mBuffer);
      pidInfo->mBufPtr = nullptr;
      }
    }
  //}}}

  //{{{
  void parsePat (cPidInfo* pidInfo, uint8_t* buf) {
  // PAT declares programPid,sid to mProgramMap to recognise programPid PMT to declare service streams

    //cLog::log (LOGINFO1, "parsePat");
    auto pat = (pat_t*)buf;
    auto sectionLength = HILO(pat->section_length) + 3;
    if (crc32 (0xffffffff, buf, sectionLength) != 0) {
      //{{{  bad crc
      cLog::log (LOGERROR, "parsePAT - bad crc - sectionLength:" + dec(sectionLength));
      return;
      }
      //}}}

    if (pat->table_id == TID_PAT) {
      buf += sizeof(pat_t);
      sectionLength -= sizeof(pat_t) + 4;
      while (sectionLength > 0) {
        auto patProgram = (pat_prog_t*)buf;
        auto sid = HILO (patProgram->program_number);
        auto pid = HILO (patProgram->network_pid);
        if (mProgramMap.find (pid) == mProgramMap.end())
          mProgramMap.insert (map<int,int>::value_type (pid, sid));

        sectionLength -= sizeof(pat_prog_t);
        buf += sizeof(pat_prog_t);
        }
      }
    }
  //}}}
  //{{{
  void parseSdt (cPidInfo* pidInfo, uint8_t* buf) {
  // SDT name services in mServiceMap

    //cLog::log (LOGINFO1, "parseSdt");
    auto sdt = (sdt_t*)buf;
    auto sectionLength = HILO(sdt->section_length) + 3;
    if (crc32 (0xffffffff, buf, sectionLength) != 0) {
      //{{{  wrong crc
      cLog::log (LOGERROR, "parseSDT - bad crc " + dec(sectionLength));
      return;
      }
      //}}}

    if (sdt->table_id == TID_SDT_ACT) {
      // only want this multiplex services
      buf += sizeof(sdt_t);
      sectionLength -= sizeof(sdt_t) + 4;
      while (sectionLength > 0) {
        auto sdtDescr = (sdt_descr_t*)buf;
        buf += sizeof(sdt_descr_t);

        auto sid = HILO (sdtDescr->service_id);
        auto freeChannel = sdtDescr->free_ca_mode == 0;
        auto loopLength = HILO (sdtDescr->descrs_loop_length);

        auto descrLength = 0;
        while ((descrLength < loopLength) &&
               (getDescrLength (buf) > 0) && (getDescrLength (buf) <= loopLength - descrLength)) {
          switch (getDescrTag (buf)) {
            case DESCR_SERVICE: {
              //{{{  service
              auto name = getDescrStr (
                buf + sizeof(descr_service_t) + ((descr_service_t*)buf)->provider_name_length + 1,
                *((uint8_t*)(buf + sizeof (descr_service_t) + ((descr_service_t*)buf)->provider_name_length)));

              auto it = mServiceMap.find (sid);
              if (it != mServiceMap.end()) {
                if (it->second.getNameString().empty()) {
                  it->second.setName (name);
                  cLog::log (LOGINFO, "SDT - service named " + dec(sid) +  " " + name);
                  }
                }
              else
                cLog::log (LOGERROR, "SDT - service not found " + dec(sid) +  " " + name);

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
              cLog::log (LOGINFO, "SDT - unexpected tag " + dec(getDescrTag(buf)));
              break;
              //}}}
            }

          descrLength += getDescrLength (buf);
          buf += getDescrLength (buf);
          }

        sectionLength -= loopLength + sizeof(sdt_descr_t);
        }
      }
    }
  //}}}
  //{{{
  void parseTdt (cPidInfo* pidInfo, uint8_t* buf) {

    //cLog::log (LOGINFO1, "parseTdt");
    auto tdt = (tdt_t*)buf;
    if (tdt->table_id == TID_TDT) {
      mCurTime = MjdToEpochTime (tdt->utc_mjd) + BcdTimeToSeconds (tdt->utc_time);

      if (mFirstTime == 0)
        mFirstTime = mCurTime;

      pidInfo->mInfoStr = getTimetString (mFirstTime) + " to " + getTimetDateString (mCurTime);
      }
    }
  //}}}
  //{{{
  void parseNit (cPidInfo* pidInfo, uint8_t* buf) {

    //cLog::log (LOGINFO1, "parseNit");
    auto nit = (nit_t*)buf;
    auto sectionLength = HILO(nit->section_length) + 3;
    if (crc32 (0xffffffff, buf, sectionLength) != 0) {
      //{{{  bad crc
      cLog::log (LOGERROR, "parseNIT - bad crc " + dec(sectionLength));
      return;
      }
      //}}}
    if ((nit->table_id != TID_NIT_ACT) &&
        (nit->table_id != TID_NIT_OTH) &&
        (nit->table_id != TID_BAT)) {
      //{{{  wrong tid
      cLog::log (LOGERROR, "parseNIT - wrong TID " +dec (nit->table_id));
      return;
      }
      //}}}

    auto networkId = HILO (nit->network_id);

    buf += sizeof(nit_t);
    auto loopLength = HILO (nit->network_descr_length);

    sectionLength -= sizeof(nit_t) + 4;
    if (loopLength <= sectionLength) {
      if (sectionLength >= 0)
        parseDescrs (networkId, buf, loopLength, nit->table_id);
      sectionLength -= loopLength;

      buf += loopLength;
      auto nitMid = (nit_mid_t*)buf;
      loopLength = HILO (nitMid->transport_stream_loop_length);
      if ((sectionLength > 0) && (loopLength <= sectionLength)) {
        // iterate nitMids
        sectionLength -= sizeof(nit_mid_t);
        buf += sizeof(nit_mid_t);

        while (loopLength > 0) {
          auto TSDesc = (nit_ts_t*)buf;
          auto tsid = HILO (TSDesc->transport_stream_id);
          auto loopLength2 = HILO (TSDesc->transport_descrs_length);
          buf += sizeof(nit_ts_t);
          if (loopLength2 <= loopLength)
            if (loopLength >= 0)
              parseDescrs (tsid, buf, loopLength2, nit->table_id);

          loopLength -= loopLength2 + sizeof(nit_ts_t);
          sectionLength -= loopLength2 + sizeof(nit_ts_t);
          buf += loopLength2;
          }
        }
      }
    }
  //}}}
  //{{{
  void parseEit (cPidInfo* pidInfo, uint8_t* buf) {

    //cLog::log (LOGINFO1, "parseEit");
    auto eit = (eit_t*)buf;
    auto sectionLength = HILO(eit->section_length) + 3;
    if (crc32 (0xffffffff, buf, sectionLength) != 0) {
      //{{{  bad crc
      cLog::log (LOGERROR, "parseEit - bad CRC " + dec(sectionLength));
      return;
      }
      //}}}

    auto tid = eit->table_id;
    auto sid = HILO (eit->service_id);
    auto now = (tid == TID_EIT_ACT);
    auto next = (tid == TID_EIT_OTH);
    auto epg = (tid == TID_EIT_ACT_SCH) || (tid == TID_EIT_OTH_SCH) ||
               (tid == TID_EIT_ACT_SCH+1) || (tid == TID_EIT_OTH_SCH+1);

    if (now || next || epg) {
      buf += sizeof(eit_t);
      sectionLength -= sizeof(eit_t) + 4;
      while (sectionLength > 0) {
        auto eitEvent = (eit_event_t*)buf;
        auto loopLength = HILO (eitEvent->descrs_loop_length);
        if (loopLength > sectionLength - sizeof(eit_event_t))
          return;
        buf += sizeof(eit_event_t);

        // parse Descrs
        auto descrLength = 0;
        while ((descrLength < loopLength) &&
               (getDescrLength (buf) > 0) && (getDescrLength (buf) <= loopLength - descrLength)) {
          switch (getDescrTag(buf)) {
            case DESCR_SHORT_EVENT: {
              //{{{  shortEvent
              auto serviceIt = mServiceMap.find (sid);
              if (serviceIt != mServiceMap.end()) {
                // recognise service
                auto startTime = MjdToEpochTime (eitEvent->mjd) + BcdTimeToSeconds (eitEvent->start_time);
                auto duration = BcdTimeToSeconds (eitEvent->duration);
                auto running = eitEvent->running_status == 0x04;

                auto title = huffDecode (buf + sizeof(descr_short_event_struct),
                                         ((descr_short_event_t*)(buf))->event_name_length);
                string titleStr = title ? title : getDescrStr (buf + sizeof(descr_short_event_struct),
                                                               ((descr_short_event_t*)(buf))->event_name_length);

                auto shortDescription = huffDecode (
                  buf + sizeof(descr_short_event_struct) + ((descr_short_event_t*)(buf))->event_name_length+1,
                  size_t(buf + sizeof(descr_short_event_struct) + ((descr_short_event_t*)(buf))->event_name_length));

                string shortDescriptionStr = shortDescription ?
                  shortDescription : getDescrStr (
                    buf + sizeof(descr_short_event_struct) + ((descr_short_event_t*)(buf))->event_name_length+1,
                    *((uint8_t*)(buf + sizeof(descr_short_event_struct) + ((descr_short_event_t*)(buf))->event_name_length)));

                if (now) {
                  if (running &&
                      !serviceIt->second.getNameString().empty() &&
                      (serviceIt->second.getProgramPid() != -1)) {
                    // wait for named service with pgmPid
                    if (serviceIt->second.setNow (startTime, duration, titleStr, shortDescriptionStr)) {
                      // new now
                      auto pidInfoIt = mPidInfoMap.find (serviceIt->second.getProgramPid());
                      if (pidInfoIt != mPidInfoMap.end()) // update service pgmPid infoStr with new now
                        pidInfoIt->second.mInfoStr =
                          serviceIt->second.getNameString() + " " + serviceIt->second.getNowTitleString();

                      startProgram (&serviceIt->second, titleStr, startTime);
                      }
                    }
                  }
                else if (epg)
                  serviceIt->second.setEpg (startTime, duration, titleStr, shortDescriptionStr);
                }

              break;
              }
              //}}}
            case DESCR_EXTENDED_EVENT: {
              //{{{  extendedEvent
              if (false) {
                size_t len = ((descr_gen_t*)(buf))->descr_length;
                cLog::log(LOGINFO, "EIT extendedEvent sid:" + dec(sid) + " len:" + dec(len));

                string str;
                for (auto i = 0; i < len; i++) {
                  int n = buf[i];
                  str += hex(n,2) + " ";
                  if ((i % 16) == 15) {
                    cLog::log (LOGINFO, str);
                    str = "";
                    }
                  }
                cLog::log (LOGINFO, str);
                }
              break;
              }
              //}}}
            }
          descrLength += getDescrLength (buf);
          buf += getDescrLength (buf);
          }
        sectionLength -= loopLength + sizeof(eit_event_t);
        buf += loopLength;
        }
      }
    else {
      cLog::log (LOGERROR, "parseEIT - unexpected tid " + dec(tid));
      return;
      }
    }
  //}}}
  //{{{
  void parsePmt (cPidInfo* pidInfo, uint8_t* buf) {
  // PMT declares pgmPid and streams for a service

    //cLog::log (LOGINFO1, "parsePmt " + dec(pidInfo->mPid));
    auto pmt = (pmt_t*)buf;
    auto sectionLength = HILO(pmt->section_length) + 3;
    if (crc32 (0xffffffff, buf, sectionLength) != 0) {
      //{{{  bad crc
      cLog::log (LOGERROR, "parsePMT - pid:%d bad crc %d", pidInfo->mPid, sectionLength);
      return;
      }
      //}}}

    if (pmt->table_id == TID_PMT) {
      auto sid = HILO (pmt->program_number);

      auto serviceIt = mServiceMap.find (sid);
      if (serviceIt == mServiceMap.end()) {
        // service not found, create one
        auto insertPair = mServiceMap.insert (map<int,cService>::value_type (sid, cService(sid)));
        serviceIt = insertPair.first;
        cLog::log (LOGINFO, "parsePmt - create service " + dec(sid));
        }
      auto service = &serviceIt->second;
      service->setProgramPid (pidInfo->mPid);

      pidInfo->mSid = sid;
      pidInfo->mInfoStr = service->getNameString() + " " + service->getNowTitleString();

      buf += sizeof(pmt_t);
      sectionLength -= 4;
      auto programInfoLength = HILO (pmt->program_info_length);
      auto streamLength = sectionLength - programInfoLength - sizeof(pmt_t);
      if (streamLength >= 0)
        parseDescrs (sid, buf, programInfoLength, pmt->table_id);

      buf += programInfoLength;
      while (streamLength > 0) {
        auto pmtInfo = (pmt_info_t*)buf;

        auto esPid = HILO (pmtInfo->elementary_PID);
        auto esPidInfo = createEsPidInfo (esPid, sid, pmtInfo->stream_type);
        if (esPidInfo) {
          //{{{  set service esPids
          //cLog::log (LOGINFO, "parsePmt - add esPid:"+ dec(esPid) + " serv:" + dec(sid));

          switch (esPidInfo->mStreamType) {
            case   2: // ISO 13818-2 video
            case  27: // HD vid
              service->setVidPid (esPid, esPidInfo->mStreamType); break;
            case   3: // ISO 11172-3 audio
            case   4: // ISO 13818-3 audio
            case  15: // HD aud ADTS
            case  17: // HD aud LATM
            case 129: // aud AC3
              service->setAudPid (esPid, esPidInfo->mStreamType);  break;
            case   6: // subtitle
              service->setSubPid (esPid, esPidInfo->mStreamType);  break;
            case   5: // private mpeg2 tabled data - private
            case  11: // dsm cc u_n
            case  13: // dsm cc tabled data
              break;
            default:
              cLog::log (LOGERROR, "parsePmt - unknown streamType:%d sid:%d pid:%d",
                                   esPidInfo->mStreamType, sid, esPid);
              break;
            }
          }
          //}}}

        auto loopLength = HILO (pmtInfo->ES_info_length);
        if (loopLength >= 0)
          parseDescrs (sid, buf, loopLength, pmt->table_id);

        buf += sizeof(pmt_info_t);
        streamLength -= loopLength + sizeof(pmt_info_t);
        buf += loopLength;
        }
      }
    }
  //}}}
  //{{{
  void parsePsi (cPidInfo* pidInfo, uint8_t* buf) {

    switch (pidInfo->mPid) {
      case PID_PAT: parsePat (pidInfo, buf); break;
      case PID_CAT: break;
      case PID_NIT: parseNit (pidInfo, buf); break;
      case PID_SDT: parseSdt (pidInfo, buf); break;
      case PID_EIT: parseEit (pidInfo, buf); break;
      case PID_RST: break;
      case PID_TDT: parseTdt (pidInfo, buf); break;
      case PID_SYN: break;
      default:      parsePmt (pidInfo, buf); break;
      }
    }
  //}}}

  //{{{
  int64_t getPtsDts (uint8_t* tsPtr) {
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
      cLog::log (LOGERROR, "getParseTs - failed - %02x %02x %02x %02x 0x02",
                           tsPtr[0], tsPtr[1],tsPtr[2],tsPtr[3],tsPtr[4]);
      return -1;
      }
    }
  //}}}
  //{{{
  string getDescrStr (uint8_t* buf, int len) {

    string str;
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
  void parseDescr (int key, uint8_t* buf, int tid) {

    switch (getDescrTag(buf)) {
      case DESCR_EXTENDED_EVENT: {
        auto text = getDescrStr (
          buf + sizeof(descr_extended_event_struct) + CastExtendedEventDescr(buf)->length_of_items + 1,
          *((uint8_t*)(buf + sizeof(descr_extended_event_struct) + CastExtendedEventDescr(buf)->length_of_items)));

        cLog::log (LOGINFO, "extended event - %d %d %c%c%c %s",
                             CastExtendedEventDescr(buf)->descr_number, CastExtendedEventDescr(buf)->last_descr_number,
                             CastExtendedEventDescr(buf)->lang_code1,
                             CastExtendedEventDescr(buf)->lang_code2,
                             CastExtendedEventDescr(buf)->lang_code3, text.c_str());

        auto ptr = buf + sizeof(descr_extended_event_struct);
        auto length = CastExtendedEventDescr(buf)->length_of_items;
        while ((length > 0) && (length < getDescrLength (buf))) {
          text = getDescrStr (ptr + sizeof(item_extended_event_struct), CastExtendedEventItem(ptr)->item_description_length);
          auto text2 = getDescrStr (
            ptr + sizeof(item_extended_event_struct) +
            CastExtendedEventItem(ptr)->item_description_length + 1,
            *(uint8_t*)(ptr + sizeof(item_extended_event_struct) +
                        CastExtendedEventItem(ptr)->item_description_length));
          cLog::log (LOGINFO, "- %s %s", text.c_str(), text2.c_str());

          length -= sizeof(item_extended_event_struct) + CastExtendedEventItem(ptr)->item_description_length +
                    *((uint8_t*)(ptr + sizeof(item_extended_event_struct) +
                                  CastExtendedEventItem(ptr)->item_description_length)) + 1;

          ptr += sizeof(item_extended_event_struct) + CastExtendedEventItem(ptr)->item_description_length +
                 *((uint8_t*)(ptr + sizeof(item_extended_event_struct) +
                               CastExtendedEventItem(ptr)->item_description_length)) + 1;
          }

        break;
        }

      // known tags
      case DESCR_COMPONENT:
      case DESCR_NW_NAME:
      case DESCR_TERR_DEL_SYS:
      case DESCR_SERVICE_LIST:
      case 0x25: // NIT2
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
      case 0xC3: break;

      default:
        cLog::log (LOGERROR, "parseDescr - unknown tag:" + dec(getDescrTag(buf)));
        break;
      }
    }
  //}}}
  //{{{
  void parseDescrs (int key, uint8_t* buf, int len, uint8_t tid) {

   auto descrLength = 0;
   while (descrLength < len) {
     if ((getDescrLength (buf) <= 0) || (getDescrLength (buf) > len - descrLength))
       return;

      parseDescr (key, buf, tid);

      descrLength += getDescrLength (buf);
      buf += getDescrLength (buf);
      }
    }
  //}}}

  //{{{  private vars
  map<int,int> mProgramMap;

  time_t mFirstTime;
  time_t mCurTime;

  uint64_t mPackets = 0;
  uint64_t mDiscontinuity = 0;
  //}}}
  };
