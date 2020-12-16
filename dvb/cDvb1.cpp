// cDvb.c
//{{{  includes
#include "cDvb1.h"
#include "cDvbUtils.h"

#include <thread>

// c
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

// dvb
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

// utils
#include "../../shared/fmt/core.h"
#include "../../shared/utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}
//{{{  defines
#define MAX_DELIVERY_SYSTEMS 2

#define DELSYS 0
#define FREQUENCY 1
#define MODULATION 2
#define INVERSION 3
#define SYMBOL_RATE 4
#define BANDWIDTH 4
#define FEC_INNER 5
#define FEC_LP 6
#define GUARD 7
#define TRANSMISSION 8
#define HIERARCHY 9
#define PLP_ID 10
//}}}

namespace {
  constexpr int kDvrFdIndex = 0;
  constexpr int kDvrBufferSize = 40*188*1024;
  //{{{  dtv_properties
  //{{{
  struct dtv_property info_cmdargs[] = { DTV_API_VERSION, 0,0,0, 0,0 };
  //}}}
  //{{{
  struct dtv_properties info_cmdseq = {
    .num = sizeof(info_cmdargs)/sizeof(struct dtv_property),
    .props = info_cmdargs
    };
  //}}}

  //{{{
  struct dtv_property enum_cmdargs[] = { DTV_ENUM_DELSYS, 0,0,0, 0,0 };
  //}}}
  //{{{
  struct dtv_properties enum_cmdseq = {
    .num = sizeof(enum_cmdargs)/sizeof(struct dtv_property),
    .props = enum_cmdargs
    };
  //}}}

  //{{{
  struct dtv_property dvbt_cmdargs[] = {
    { DTV_DELIVERY_SYSTEM,   0,0,0, SYS_DVBT,0 },
    { DTV_FREQUENCY,         0,0,0, 0,0 },
    { DTV_MODULATION,        0,0,0, QAM_AUTO,0 },
    { DTV_INVERSION,         0,0,0, INVERSION_AUTO,0 },
    { DTV_BANDWIDTH_HZ,      0,0,0, 8000000,0 },
    { DTV_CODE_RATE_HP,      0,0,0, FEC_AUTO,0 },
    { DTV_CODE_RATE_LP,      0,0,0, FEC_AUTO,0 },
    { DTV_GUARD_INTERVAL,    0,0,0, 0,0 },
    { DTV_TRANSMISSION_MODE, 0,0,0, TRANSMISSION_MODE_AUTO,0 },
    { DTV_HIERARCHY,         0,0,0, HIERARCHY_AUTO,0 },
    { DTV_TUNE,              0,0,0, 0,0 }
    };
  //}}}
  //{{{
  struct dtv_properties dvbt_cmdseq = {
    .num = sizeof(dvbt_cmdargs)/sizeof(struct dtv_property),
    .props = dvbt_cmdargs
    };
  //}}}

  //{{{
  struct dtv_property dvbt2_cmdargs[] = {
    { DTV_DELIVERY_SYSTEM,   0,0,0, SYS_DVBT2,0 },
    { DTV_FREQUENCY,         0,0,0, 0,0 },
    { DTV_MODULATION,        0,0,0, QAM_AUTO,0 },
    { DTV_INVERSION,         0,0,0, INVERSION_AUTO,0 },
    { DTV_BANDWIDTH_HZ,      0,0,0, 8000000,0 },
    { DTV_CODE_RATE_HP,      0,0,0, FEC_AUTO,0 },
    { DTV_CODE_RATE_LP,      0,0,0, FEC_AUTO,0 },
    { DTV_GUARD_INTERVAL,    0,0,0, 0,0 },
    { DTV_TRANSMISSION_MODE, 0,0,0, TRANSMISSION_MODE_AUTO,0 },
    { DTV_HIERARCHY,         0,0,0, HIERARCHY_AUTO,0 },
    { DTV_TUNE,              0,0,0, 0,0 }
    };
  //}}}
  //{{{
  struct dtv_properties dvbt2_cmdseq = {
    .num = sizeof(dvbt2_cmdargs)/sizeof(struct dtv_property),
    .props = dvbt2_cmdargs
    };
  //}}}

  //{{{
  struct dtv_property pclear[] = { DTV_CLEAR, 0,0,0, 0,0 };
  //}}}
  //{{{
  struct dtv_properties cmdclear = {
    .num = 1,
    .props = pclear
    };
  //}}}
  //}}}
  //{{{  macros
  #define GET_FEC_INNER(fec, val)                         \
    if ((fe_caps & FE_CAN_##fec) && (fecValue == val)) \
      return fec;
  //}}}
  }

//{{{
cDvb::cDvb (int frequency, int adapter) {

  cLog::log (LOGINFO, format ("DVB api v{}.{} frequency:{}",
             DVB_API_VERSION, DVB_API_VERSION_MINOR, frequency));

  mFrequency = frequency;
  mAdapter = adapter;

  string frontend = format ("/dev/dvb/adapter{}/frontend{}", mAdapter, mFeNum);
  mFrontEnd = open (frontend.c_str(), O_RDWR | O_NONBLOCK);
  if (mFrontEnd < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "dvbOpen openDevice failed" + frontend);
    exit (1);
    }
    //}}}

  frontendSetup();
  while (!frontendStatus())
    this_thread::sleep_for (100ms);

  string dvr = format ("/dev/dvb/adapter{}/dvr{}", mAdapter, mFeNum);
  fds[kDvrFdIndex].fd = open (dvr.c_str(), O_RDONLY | O_NONBLOCK);
  fds[kDvrFdIndex].events = POLLIN;
  if (fds[kDvrFdIndex].fd < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "opening device faile" + dvr);
    exit (1);
    }
    //}}}

  if (ioctl (fds[kDvrFdIndex].fd, DMX_SET_BUFFER_SIZE, kDvrBufferSize) < 0)
    cLog::log (LOGERROR, "dvbOpen setBufferSize failed" + dvr);

  cLog::log (LOGINFO, "dvb tuned with lock, bufferSize %d", kDvrBufferSize);
  }
//}}}
//{{{
cDvb::~cDvb() {
  close (fds[kDvrFdIndex].fd);
  }
//}}}

//{{{
cTsBlock* cDvb::read (cTsBlockPool* blockPool) {

  constexpr int kMaxRead = 50;
  struct iovec iovecs[kMaxRead];

  cTsBlock* block = mBlockFreeList;
  cTsBlock** current = &block;

  for (int i = 0; i < kMaxRead; i++) {
    if (!(*current))
      *current = blockPool->newBlock();
    iovecs[i].iov_base = (*current)->mTs;
    iovecs[i].iov_len = 188;
    current = &(*current)->mNextBlock;
    }

  while (poll (fds, 1, -1) <= 0)
    cLog::log (LOGINFO, "poll waiting");

  int size = readv (fds[kDvrFdIndex].fd, iovecs, kMaxRead);
  if (size < 0) {
    cLog::log (LOGERROR, format ("readv DVR failed {}", strerror(errno)));
    size = 0;
    }
  size /= 188;

  current = &block;
  while (size && *current) {
    current = &(*current)->mNextBlock;
    size--;
    }

  mBlockFreeList = *current;
  *current = NULL;

  return block;
  }
//}}}

//{{{
string cDvb::getStatusString() {

  // old api style signal strength
  //uint32_t strength = 0;
  //ioctl (mFrontEnd, FE_READ_SIGNAL_STRENGTH, &strength);

  // old api style signal snr
  //uint32_t snr = 0;
  //ioctl (mFrontEnd, FE_READ_SNR, &snr);

  struct dtv_property props[] = {
    { .cmd = DTV_STAT_SIGNAL_STRENGTH },   // max 0xFFFF percentage
    { .cmd = DTV_STAT_CNR },               // 0.001db
    { .cmd = DTV_STAT_ERROR_BLOCK_COUNT },
    { .cmd = DTV_STAT_TOTAL_BLOCK_COUNT }, // count
    { .cmd = DTV_STAT_PRE_ERROR_BIT_COUNT },
    { .cmd = DTV_STAT_PRE_TOTAL_BIT_COUNT },
    { .cmd = DTV_STAT_POST_ERROR_BIT_COUNT },
    { .cmd = DTV_STAT_POST_TOTAL_BIT_COUNT },
    };

  struct dtv_properties cmdProperty = {
    .num = 8,
    .props = props
    };

  if ((ioctl (mFrontEnd, FE_GET_PROPERTY, &cmdProperty)) < 0)
    return "status failed";

  return format ("strength:{:5.2f}% snr:{:5.2f}db block:{:x},{:x}, pre:{:x},{:x}, post:{:x},{:x}",
                 100.f * ((props[0].u.st.stat[0].uvalue & 0xFFFF) / float(0xFFFF)),
                 props[1].u.st.stat[0].svalue / 1000.f,
                 (__u64)props[2].u.st.stat[0].uvalue,
                 (__u64)props[3].u.st.stat[0].uvalue,
                 (__u64)props[4].u.st.stat[0].uvalue,
                 (__u64)props[5].u.st.stat[0].uvalue,
                 (__u64)props[6].u.st.stat[0].uvalue,
                 (__u64)props[7].u.st.stat[0].uvalue);
  }
//}}}

//{{{
void cDvb::reset() {
  frontendSetup();
  }
//}}}
//{{{
int cDvb::setFilter (uint16_t pid) {

  string filter = format ("/dev/dvb/adapter{}/demux{}", mAdapter, mFeNum);

  int fd = open (filter.c_str(), O_RDWR);
  if (fd < 0) {
    // error return
    cLog::log (LOGERROR, format ("dvbSetFilter - open failed pid:{}", pid));
    return -1;
    }

  struct dmx_pes_filter_params s_filter_params;
  s_filter_params.pid = pid;
  s_filter_params.input = DMX_IN_FRONTEND;
  s_filter_params.output = DMX_OUT_TS_TAP;
  s_filter_params.flags = DMX_IMMEDIATE_START;
  s_filter_params.pes_type = DMX_PES_OTHER;
  if (ioctl (fd, DMX_SET_PES_FILTER, &s_filter_params) < 0) {
    // error return
    cLog::log (LOGERROR, format ("dvbSetFilter - set_pesFilter failed pid:{} {}", pid, strerror (errno)));
    close (fd);
    return -1;
    }

  cLog::log (LOGINFO1, format ("dvbSetFilter pid:{}", pid));
  return fd;
  }
//}}}
//{{{
void cDvb::unsetFilter (int fd, uint16_t pid) {

  if (ioctl (fd, DMX_STOP) < 0)
    cLog::log (LOGERROR, format("dvbUnsetFilter - stop failed {}", strerror (errno)));
  else
    cLog::log (LOGINFO1, format ("dvbUnsetFilter - unsetting pid:{}", pid));

  close (fd);
  }
//}}}

// private
//{{{
fe_hierarchy_t cDvb::getHierarchy() {

  switch (mHierarchy) {
    case 0: return HIERARCHY_NONE;
    case 1: return HIERARCHY_1;
    case 2: return HIERARCHY_2;
    case 4: return HIERARCHY_4;
    default:
      cLog::log (LOGERROR, "invalid intramission mode %d", mTransmission);
    case -1: return HIERARCHY_AUTO;
    }
  }
//}}}
//{{{
fe_guard_interval_t cDvb::getGuard() {

  switch (mGuard) {
    case 32: return GUARD_INTERVAL_1_32;
    case 16: return GUARD_INTERVAL_1_16;
    case  8: return GUARD_INTERVAL_1_8;
    case  4: return GUARD_INTERVAL_1_4;

    default: cLog::log (LOGERROR, "invalid guard interval %d", mGuard);
    case -1:
    case  0: return GUARD_INTERVAL_AUTO;
    }
  }
//}}}
//{{{
fe_transmit_mode_t cDvb::getTransmission() {

  switch (mTransmission) {
    case 2: return TRANSMISSION_MODE_2K;
    case 8: return TRANSMISSION_MODE_8K;
  #ifdef TRANSMISSION_MODE_4K
    case 4: return TRANSMISSION_MODE_4K;
  #endif
    default: cLog::log (LOGERROR, "invalid tranmission mode %d", mTransmission);
    case -1:
    case 0: return TRANSMISSION_MODE_AUTO;
    }
  }
//}}}
//{{{
fe_spectral_inversion_t cDvb::getInversion() {

  switch (mInversion) {
    case 0:  return INVERSION_OFF;
    case 1:  return INVERSION_ON;
    case -1: return INVERSION_AUTO;
    default: cLog::log (LOGERROR, "invalid inversion %d", mInversion); return INVERSION_AUTO;
    }
  }
//}}}
//{{{
fe_code_rate_t cDvb::getFEC (fe_caps_t fe_caps, int fecValue) {

  GET_FEC_INNER(FEC_AUTO, 999);
  GET_FEC_INNER(FEC_AUTO, -1);
  if (fecValue == 0)
    return FEC_NONE;

  GET_FEC_INNER(FEC_1_2, 12);
  GET_FEC_INNER(FEC_2_3, 23);
  GET_FEC_INNER(FEC_3_4, 34);
  if (fecValue == 35)
    return FEC_3_5;

  GET_FEC_INNER(FEC_4_5, 45);
  GET_FEC_INNER(FEC_5_6, 56);
  GET_FEC_INNER(FEC_6_7, 67);
  GET_FEC_INNER(FEC_7_8, 78);
  GET_FEC_INNER(FEC_8_9, 89);
  if (fecValue == 910)
    return FEC_9_10;

  cLog::log (LOGERROR, "invalid FEC %d", fecValue);
  return FEC_AUTO;
  }
//}}}

//{{{
void cDvb::frontendInfo (struct dvb_frontend_info& info, uint32_t version,
                         fe_delivery_system_t* systems, int numSystems) {

  cLog::log (LOGINFO, format ("frontend - version {} min {} max {} stepSize {} tolerance {}",
                              version, info.frequency_min, info.frequency_max,
                              info.frequency_stepsize, info.frequency_tolerance));

  // info
  string infoString = "has - ";
  //{{{  report fec
  if (info.caps & FE_IS_STUPID)
    infoString += "stupid ";

  if (info.caps & FE_CAN_INVERSION_AUTO)
    infoString += "inversionAuto ";

  if (info.caps & FE_CAN_FEC_AUTO)
    infoString += "fecAuto ";

  if (info.caps & FE_CAN_FEC_1_2)
    infoString += "fec12 ";

  if (info.caps & FE_CAN_FEC_2_3)
    infoString += "fec23 ";

  if (info.caps & FE_CAN_FEC_3_4)
    infoString += "fec34 ";

  if (info.caps & FE_CAN_FEC_4_5)
    infoString += "fec45 ";

  if (info.caps & FE_CAN_FEC_5_6)
    infoString += "fec56 ";

  if (info.caps & FE_CAN_FEC_6_7)
    infoString += "fec67 ";

  if (info.caps & FE_CAN_FEC_7_8)
    infoString += "fec78 ";

  if (info.caps & FE_CAN_FEC_8_9)
    infoString += "fec89 ";

  cLog::log (LOGINFO, infoString);
  //}}}
  //{{{  report qam
  infoString = "has - ";

  if (info.caps & FE_CAN_QPSK)
    infoString += "qpsk ";

  if (info.caps & FE_CAN_QAM_AUTO)
    infoString += "qamAuto ";

  if (info.caps & FE_CAN_QAM_16)
    infoString += "qam16 ";

  if (info.caps & FE_CAN_QAM_32)
    infoString += "qam32 ";

  if (info.caps & FE_IS_STUPID)
    infoString += "qam64 ";

  if (info.caps & FE_CAN_QAM_128)
    infoString += "qam128 ";

  if (info.caps & FE_CAN_QAM_256)
    infoString += "qam256 ";

  cLog::log (LOGINFO, infoString);
  //}}}
  //{{{  report other
  infoString = "has - ";

  if (info.caps & FE_CAN_TRANSMISSION_MODE_AUTO)
    infoString += "transmissionAuto ";

  if (info.caps & FE_CAN_BANDWIDTH_AUTO)
    infoString += "bandwidthAuto ";

  if (info.caps & FE_CAN_GUARD_INTERVAL_AUTO)
    infoString += "guardAuto ";

  if (info.caps & FE_CAN_HIERARCHY_AUTO)
    infoString += "hierachyAuto ";

  if (info.caps & FE_CAN_8VSB)
    infoString += "8vsb ";

  if (info.caps & FE_CAN_16VSB)
    infoString += "16vsb ";

  if (info.caps & FE_HAS_EXTENDED_CAPS)
    infoString += "extendedinfo.caps ";

  if (info.caps & FE_CAN_2G_MODULATION)
    infoString += "2G ";

  if (info.caps & FE_CAN_MULTISTREAM)
    infoString += "multistream ";

  if (info.caps & FE_CAN_TURBO_FEC)
    infoString += "turboFec ";

  if (info.caps & FE_IS_STUPID)
    infoString += "needsBending ";

  if (info.caps & FE_CAN_RECOVER)
    infoString += "canRecover ";

  if (info.caps & FE_CAN_MUTE_TS)
    infoString += "canMute ";

  cLog::log (LOGINFO, infoString);
  //}}}
  //{{{  report delivery systems
  infoString = "has - ";

  for (int i = 0; i < numSystems; i++)
    switch (systems[i]) {
      case SYS_DVBT:  infoString += "DVBT "; break;
      case SYS_DVBT2: infoString += "DVBT2 "; break;
      default: break;
      }

  cLog::log (LOGINFO, infoString);
  //}}}
  }
//}}}
//{{{
void cDvb::frontendSetup() {

  struct dvb_frontend_info info;
  if (ioctl (mFrontEnd, FE_GET_INFO, &info) < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "frontend FE_GET_INFO failed %s", strerror(errno));
    exit (1);
    }
    //}}}

  if (ioctl (mFrontEnd, FE_GET_PROPERTY, &info_cmdseq) < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "frontend FE_GET_PROPERTY api version failed");
    exit (1);
    }
    //}}}
  int version = info_cmdargs[0].u.data;

  if (ioctl (mFrontEnd, FE_GET_PROPERTY, &enum_cmdseq) < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "frontend FE_GET_PROPERTY failed");
    exit (1);
    }
    //}}}

  fe_delivery_system_t systems[MAX_DELIVERY_SYSTEMS] = {SYS_UNDEFINED, SYS_UNDEFINED};
  int numSystems = enum_cmdargs[0].u.buffer.len;
  if (numSystems < 1) {
    //{{{  error exit
    cLog::log (LOGERROR, "no available delivery system");
    exit(1);
    }
    //}}}
  for (int i = 0; i < numSystems; i++)
    systems[i] = (fe_delivery_system_t)enum_cmdargs[0].u.buffer.data[i];

  frontendInfo (info, version, systems, numSystems);

  // clear frontend commands
  if (ioctl (mFrontEnd, FE_SET_PROPERTY, &cmdclear) < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "Unable to clear frontend");
    exit (1);
    }
    //}}}

  struct dtv_properties* p;
  fe_delivery_system_t system = mFrequency == 626000000 ? SYS_DVBT2 : SYS_DVBT;
  switch (system) {
    //{{{
    case SYS_DVBT:
      p = &dvbt_cmdseq;
      p->props[DELSYS].u.data = system;
      p->props[FREQUENCY].u.data = mFrequency;
      p->props[INVERSION].u.data = getInversion();
      p->props[BANDWIDTH].u.data = mBandwidth * 1000000;
      p->props[FEC_INNER].u.data = getFEC (info.caps, mFec);
      p->props[FEC_LP].u.data = getFEC (info.caps, mFecLp);
      p->props[GUARD].u.data = getGuard();
      p->props[TRANSMISSION].u.data = getTransmission();
      p->props[HIERARCHY].u.data = getHierarchy();

      cLog::log (LOGINFO, "DVB-T %d band:%d inv:%d fec:%d fecLp:%d hier:%d guard:%d trans:%d",
                 mFrequency, mBandwidth, mInversion, mFec, mFecLp, mHierarchy, mGuard, mTransmission);
      break;
    //}}}
    //{{{
    case SYS_DVBT2:
      p = &dvbt2_cmdseq;
      p->props[DELSYS].u.data = system;
      p->props[FREQUENCY].u.data = mFrequency;
      p->props[INVERSION].u.data = getInversion();
      p->props[BANDWIDTH].u.data = mBandwidth * 1000000;
      p->props[FEC_INNER].u.data = getFEC (info.caps, mFec);
      p->props[FEC_LP].u.data = getFEC (info.caps, mFecLp);
      p->props[GUARD].u.data = getGuard();
      p->props[TRANSMISSION].u.data = getTransmission();
      p->props[HIERARCHY].u.data = getHierarchy();
      p->props[PLP_ID].u.data = 0; //dvb_plp_id;

      cLog::log (LOGINFO, "DVB-T2 %d band:%d inv:%d fec:%d fecLp:%d hier:%d mod:%s guard:%d trans:%d plpId:%d",
                 mFrequency, mBandwidth, mInversion, mFec, mFecLp,
                 mHierarchy, "qam_auto", mGuard, mTransmission, p->props[PLP_ID].u.data);
      break;
    //}}}
    //{{{
    default:
      cLog::log (LOGERROR, "unknown frontend type %d", info.type);
      exit(1);
    //}}}
    }

  // empty the event queue
  while (true) {
    struct dvb_frontend_event event;
    if ((ioctl (mFrontEnd, FE_GET_EVENT, &event) < 0) && (errno == EWOULDBLOCK))
      break;
    }

  // send properties to frontend device
  if (ioctl (mFrontEnd, FE_SET_PROPERTY, p) < 0) {
    //{{{  error exit
    cLog::log (LOGERROR, "setting frontend failed %s", strerror(errno));
    exit (1);
    }
    //}}}

  mLastStatus = (fe_status)0;
  }
//}}}
//{{{
bool cDvb::frontendStatus() {

  bool result = false;

  struct dvb_frontend_event event;
  if (ioctl (mFrontEnd, FE_GET_EVENT, &event) < 0) {
    if (errno == EWOULDBLOCK)
      return false;
    cLog::log (LOGERROR, "cDvb read FE_GET_EVENT failed");
    return false;
    }

  // report status change
  fe_status_t status = event.status;
  fe_status_t diff = (fe_status_t)(status ^ mLastStatus);
  if (diff) {
    string statusString = "status - ";

    if (diff & FE_HAS_SIGNAL) {
      //{{{  signal
      if (status & FE_HAS_SIGNAL)
        statusString += "signal ";
      else
        statusString += "lostSignal ";
      }
      //}}}
    if (diff & FE_HAS_CARRIER) {
      //{{{  carrier
      if (status & FE_HAS_CARRIER)
        statusString += "carrier ";
      else
        statusString += "lostCarrier ";
      }
      //}}}
    if (diff & FE_HAS_VITERBI) {
      //{{{  fec
      if (status & FE_HAS_VITERBI)
        statusString += "stableFec ";
      else
        statusString += "lostFec ";
      }
      //}}}
    if (diff & FE_HAS_SYNC) {
      //{{{  sync
      if (status & FE_HAS_SYNC)
        statusString += "sync ";
      else
        statusString += "lostSync ";
      }
      //}}}
    if (diff & FE_HAS_LOCK) {
      //{{{  lock
      if (status & FE_HAS_LOCK) {

        statusString += "lock ";

        int32_t value = 0;
        if (ioctl (mFrontEnd, FE_READ_BER, &value) >= 0)
          statusString += format ("ber:{} ", value);
        if (ioctl (mFrontEnd, FE_READ_SIGNAL_STRENGTH, &value) >= 0)
          statusString += format ("strength:{} ", value);
        if (ioctl (mFrontEnd, FE_READ_SNR, &value) >= 0)
          statusString += format ("snr:{} ", value);
        result = true;
        }
      else
        statusString += "lostLock ";
      }
      //}}}

    cLog::log (LOGINFO, statusString);
    }

  mLastStatus = status;

  return result;
  }
//}}}
