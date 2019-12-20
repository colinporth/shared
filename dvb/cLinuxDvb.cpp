// cLiunuxDvb.cpp
//{{{  includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>

#include <chrono>
#include "../utils/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"
#include "../utils/cBipBuffer.h"

#include "cDumpTransportStream.h"

#include "cLinuxDvb.h"

using namespace std;
//}}}

// public:
//{{{
cDvb::cDvb (const string& root) : cDumpTransportStream (root, true) {
  allocateBuffer (2048 * 128 * 188); // 50m - T2 5m a second
  }
//}}}
//{{{
cDvb::~cDvb() {
  close (mDvr);
  close (mDemux);
  close (mFrontEnd);
  }
//}}}

//{{{
void cDvb::tune (int frequency) {

  if (frequency < 1000)
    frequency *= 1000000UL;
  else if (frequency < 1000000)
    frequency *= 1000UL;
  bool t2 = frequency == 626000000;

  struct dvb_frontend_info fe_info;
  if (ioctl (mFrontEnd, FE_GET_INFO, &fe_info) < 0) {
    //{{{  error
    cLog::log (LOGERROR, "FE_GET_INFO failed");
    return;
    }
    //}}}
  cLog::log (LOGINFO, "tuning %s %s to %u ", fe_info.name, t2 ? "T2" : "T", frequency);

  // discard stale events
  struct dvb_frontend_event ev;
  while (true)
    if (ioctl (mFrontEnd, FE_GET_EVENT, &ev) == -1)
      break;
    else
      cLog::log (LOGINFO, "discarding stale event");

  //{{{
  struct dtv_property tuneProps[10];

  tuneProps[0].cmd = DTV_CLEAR;

  tuneProps[1].cmd = DTV_DELIVERY_SYSTEM;
  tuneProps[1].u.data = t2 ? SYS_DVBT2 : SYS_DVBT;

  tuneProps[2].cmd = DTV_FREQUENCY;
  tuneProps[2].u.data = frequency;

  tuneProps[3].cmd = DTV_MODULATION;
  tuneProps[3].u.data = QAM_AUTO;

  tuneProps[4].cmd = DTV_CODE_RATE_HP;
  tuneProps[4].u.data = FEC_AUTO;

  tuneProps[5].cmd = DTV_CODE_RATE_LP;
  tuneProps[5].u.data = 0;

  tuneProps[6].cmd = DTV_INVERSION;
  tuneProps[6].u.data = INVERSION_AUTO;

  tuneProps[7].cmd = DTV_TRANSMISSION_MODE;
  tuneProps[7].u.data = TRANSMISSION_MODE_AUTO;

  tuneProps[8].cmd = DTV_BANDWIDTH_HZ;
  tuneProps[8].u.data = 8000000;

  tuneProps[9].cmd = DTV_TUNE;
  //}}}
  struct dtv_properties cmdTune = { .num = 10, .props = tuneProps };
  if ((ioctl (mFrontEnd, FE_SET_PROPERTY, &cmdTune)) == -1) {
    //{{{  error
    cLog::log (LOGERROR, "FE_SET_PROPERTY TUNE failed");
    return;
    }
    //}}}

  // wait for zero status indicating start of tuning
  do {
    ioctl (mFrontEnd, FE_GET_EVENT, &ev);
    cLog::log (LOGINFO, "waiting to tune");
    } while (ev.status != 0);

  // wait for tuning
  for (int i = 0; i < 10; i++) {
    usleep (100000);

    if (ioctl (mFrontEnd, FE_GET_EVENT, &ev) == -1) // no answer, consider it as not locked situation
      ev.status = FE_NONE;

    if (ev.status & FE_HAS_LOCK) {
      // tuning locked
      struct dtv_property getProps[] = {
          { .cmd = DTV_DELIVERY_SYSTEM },
          { .cmd = DTV_MODULATION },
          { .cmd = DTV_INNER_FEC },
          { .cmd = DTV_CODE_RATE_HP },
          { .cmd = DTV_CODE_RATE_LP },
          { .cmd = DTV_INVERSION },
          { .cmd = DTV_GUARD_INTERVAL },
          { .cmd = DTV_TRANSMISSION_MODE },
        };
      struct dtv_properties cmdGet = {
        .num = sizeof(getProps) / sizeof (getProps[0]), .props = getProps };
      if ((ioctl (mFrontEnd, FE_GET_PROPERTY, &cmdGet)) == -1) {
        //{{{  error
        cLog::log (LOGERROR, "FE_GET_PROPERTY failed");
        return;
        }
        //}}}

      //{{{
      const char* deliveryTab[] = {
        "UNDEFINED",
        "DVBC_ANNEX_A",
        "DVBC_ANNEX_B",
        "DVBT",
        "DSS",
        "DVBS",
        "DVBS2",
        "DVBH",
        "ISDBT",
        "ISDBS",
        "ISDBC",
        "ATSC",
        "ATSCMH",
        "DTMB",
        "CMMB",
        "DAB",
        "DVBT2",
        "TURBO",
        "DVBC_ANNEX_C"
        };
      //}}}
      //{{{
      const char* modulationTab[] =  {
        "QPSK",
        "QAM_16",
        "QAM_32",
        "QAM_64",
        "QAM_128",
        "QAM_256",
        "QAM_AUTO",
        "VSB_8",
        "VSB_16",
        "PSK_8",
        "APSK_16",
        "APSK_32",
        "DQPSK"
      };
      //}}}
      //{{{
      const char* inversionTab[] = {
        "INV_OFF",
        "INV_ON",
        "INV_AUTO"
        };
      //}}}
      //{{{
      const char* codeRateTab[] = {
        "FEC_NONE",
        "FEC_1_2",
        "FEC_2_3",
        "FEC_3_4",
        "FEC_4_5",
        "FEC_5_6",
        "FEC_6_7",
        "FEC_7_8",
        "FEC_8_9",
        "FEC_AUTO",
        "FEC_3_5",
        "FEC_9_10",
        };
      //}}}
      //{{{
      const char* transmissionModeTab[] = {
        "TM_2K",
        "TM_8K",
        "TM_AUTO",
        "TM_4K",
        "TM_1K",
        "TM_16K",
        "TM_32K"
        };
      //}}}
      //{{{
      const char* guardTab[] = {
        "GUARD_1_32",
        "GUARD_1_16",
        "GUARD_1_8",
        "GUARD_1_4",
        "GUARD_AUTO",
        "GUARD_1_128",
        "GUARD_19_128",
        "GUARD_19_256"
        };
      //}}}
      cLog::log (LOGINFO, "locked using %s %s %s %s %s %s %s ",
        deliveryTab [getProps[0].u.buffer.data[0]],
        modulationTab [getProps[1].u.buffer.data[0]],
        codeRateTab [getProps[2].u.buffer.data[0]],
        codeRateTab [getProps[3].u.buffer.data[0]],
        codeRateTab [getProps[4].u.buffer.data[0]],
        inversionTab [getProps[5].u.buffer.data[0]],
        guardTab [getProps[6].u.buffer.data[0]],
        transmissionModeTab [getProps[7].u.buffer.data[0]]);

      monitorFe();
      mTuneStr = string(fe_info.name) + " " + dec(frequency/1000000) + "Mhz";
      updateSignalString();
      return;
      }
    else
      cLog::log (LOGINFO, "waiting for lock");
    }

  cLog::log (LOGERROR, "tuning failed\n");
  }
//}}}

//{{{
void cDvb::captureThread (int frequency) {
// thread - read packet and write to bipBuffer

  cLog::setThreadName ("capt");

  init (frequency);

  while (true) {
    int bytesAllocated = 0;
    auto ptr = reserve (128 * 188, bytesAllocated);
    if (ptr) {
      int bytesRead = read (mDvr, ptr, bytesAllocated);
      commit (bytesRead);
      if (bytesRead < bytesAllocated)
        cLog::log (LOGERROR, "write bytesRead " + dec(bytesRead) + " < " + dec(bytesAllocated));
      }
    else
      cLog::log (LOGERROR, "bipBuffer.reserve failed " + dec(bytesAllocated));
    }

  cLog::log (LOGERROR, "exit");
  }
//}}}
//{{{
void cDvb::grabThread() {

  cLog::setThreadName ("grab");

  uint64_t streamPos = 0;
  bool run = true;
  while (run) {
    int blockSize = 0;
    auto ptr = getContiguousBlock (blockSize);
    if (blockSize > 0) {
      streamPos += demux (ptr, blockSize, 0, false, -1);
      decommitBlock (blockSize);

      bool show = (getDiscontinuity() != mLastDiscontinuity) || (blockSize > mLastBlockSize);
      mLastDiscontinuity = getDiscontinuity();
      if (blockSize > mLastBlockSize)
        mLastBlockSize = blockSize;
      if (blockSize > mMaxBlockSize)
        mMaxBlockSize = blockSize;

      mPacketStr = dec(getDiscontinuity()) +
                   " " + dec(blockSize,6) +
                   ":" + dec(mMaxBlockSize);
      updateSignalString();

      if (show)
        cLog::log (LOGINFO, mPacketStr + " " + mSignalStr);
      }
    else
      uSleep (1000);
    }

  cLog::log (LOGERROR, "exit");
  }
//}}}
//{{{
void cDvb::readThread (const string& inTs) {

  cLog::setThreadName ("read");

  auto file = fopen (inTs.c_str(), "rb");
  if (!file) {
    //{{{  error, return
    cLog::log (LOGERROR, "no file " + inTs);
    return;
    }
    //}}}

  uint64_t streamPos = 0;
  auto blockSize = 188 * 8;
  auto buffer = (uint8_t*)malloc (blockSize);

  bool run = true;
  while (run) {
    int bytesRead = fread (buffer, 1, blockSize, file);
    if (bytesRead > 0)
      streamPos += demux (buffer, bytesRead, streamPos, false, -1);
    else
      break;
    mPacketStr = dec(getDiscontinuity());
    }

  fclose (file);
  free (buffer);

  cLog::log (LOGERROR, "exit");
  }
//}}}

// private
//{{{
void cDvb::init (int frequency) {

  // frontend nonBlocking rw
  mFrontEnd = open ("/dev/dvb/adapter0/frontend0", O_RDWR | O_NONBLOCK);
  if (mFrontEnd < 0){
    cLog::log (LOGERROR, "open frontend failed");
    return;
    }
  tune (frequency);

  // demux nonBlocking rw
  mDemux = open ("/dev/dvb/adapter0/demux0", O_RDWR | O_NONBLOCK);
  if (mDemux < 0) {
    cLog::log (LOGERROR, "open demux failed");
    return;
    }
  setTsFilter (8192, DMX_PES_OTHER);

  // dvr blocking reads
  mDvr = open ("/dev/dvb/adapter0/dvr0", O_RDONLY);
  if (mDvr < 0) {
    cLog::log (LOGERROR, "open dvr failed");
    return;
    }
  }
//}}}

//{{{
void cDvb::setTsFilter (uint16_t pid, dmx_pes_type_t pestype) {

  struct dmx_pes_filter_params pesFilterParams;
  pesFilterParams.pid = pid;
  pesFilterParams.input = DMX_IN_FRONTEND;
  pesFilterParams.output = DMX_OUT_TS_TAP;
  pesFilterParams.pes_type = pestype;
  pesFilterParams.flags = DMX_IMMEDIATE_START;

  auto error = ioctl (mDemux, DMX_SET_PES_FILTER, &pesFilterParams);
  if (error < 0)
    cLog::log (LOGERROR, "Demux set filter pid " + dec(pid) + " " + dec(error));
  }
//}}}

//{{{
void cDvb::monitorFe() {

  fe_status_t festatus;
  if (ioctl (mFrontEnd, FE_READ_STATUS, &festatus) == -1)
    cLog::log (LOGERROR, "FE_READ_STATUS failed");
  else
    cLog::log (LOGINFO, "status %s%s%s%s%s%s",
               festatus & FE_HAS_LOCK ? "lock " : "",
               festatus & FE_HAS_SYNC ? "sync " : "",
               festatus & FE_TIMEDOUT ? "timedout " : "",
               festatus & FE_HAS_SIGNAL ? "sig " : "",
               festatus & FE_HAS_CARRIER ? "carrier " : "",
               festatus & FE_HAS_VITERBI ? "viterbi " : "");


  struct dtv_property getProps[] = {
      { .cmd = DTV_STAT_SIGNAL_STRENGTH },
      { .cmd = DTV_STAT_CNR },
      { .cmd = DTV_STAT_PRE_ERROR_BIT_COUNT },
      { .cmd = DTV_STAT_PRE_TOTAL_BIT_COUNT },
      { .cmd = DTV_STAT_POST_ERROR_BIT_COUNT },
      { .cmd = DTV_STAT_POST_TOTAL_BIT_COUNT },
      { .cmd = DTV_STAT_ERROR_BLOCK_COUNT },
      { .cmd = DTV_STAT_TOTAL_BLOCK_COUNT },
    };
  struct dtv_properties cmdGet = {
    .num = sizeof(getProps) / sizeof (getProps[0]),
    .props = getProps
    };
  if ((ioctl (mFrontEnd, FE_GET_PROPERTY, &cmdGet)) == -1)
    cLog::log (LOGERROR, "FE_GET_PROPERTY failed");
  else
    for (int i = 0; i < 8; i++) {
      auto uvalue = getProps[i].u.st.stat[0].uvalue;
      cLog::log (LOGINFO, "stats %d len:%d scale:%d uvalue:%d",
                 i,
                 getProps[i].u.st.len,
                 getProps[i].u.st.stat[0].scale,
                 int(uvalue));
      }
    //__s64 svalue;
  }
//}}}
//{{{
void cDvb::updateSignalString() {

  fe_status_t feStatus;
  auto error = ioctl (mFrontEnd, FE_READ_STATUS, &feStatus);
  if (error < 0)
    cLog::log (LOGERROR, "FE_READ_STATUS " + dec(error));

  string str = "";
  if (feStatus & FE_TIMEDOUT)
    str += "timeout ";
  if (feStatus & FE_HAS_LOCK)
    str += "lock ";
  if (feStatus & FE_HAS_SIGNAL)
    str += "s";
  if (feStatus & FE_HAS_CARRIER)
    str += "c";
  if (feStatus & FE_HAS_VITERBI)
    str += "v";
  if (feStatus & FE_HAS_SYNC)
    str += "s";

  uint32_t value;
  error = ioctl (mFrontEnd, FE_READ_SIGNAL_STRENGTH, &value);
  if (error < 0)
    cLog::log (LOGERROR, "FE_READ_SIGNAL_STRENGTH " + dec(error));
  str += dec((value*100)/0x10000, 3);

  error = ioctl (mFrontEnd, FE_READ_SNR, &value);
  if (error < 0)
    cLog::log (LOGERROR, "FE_READ_SNR " + dec(error));
  str += ":" + dec(value,3);

  error = ioctl (mFrontEnd, FE_READ_BER, &value);
  if (error < 0)
    cLog::log (LOGERROR, "FE_READ_BER " + dec(error));
  str += ":" + dec(value);

  mSignalStr = str;
  }
//}}}
//{{{
void cDvb::uSleep (uint64_t uSec) {

  struct timespec timeRequest = { 0 };

  timeRequest.tv_sec = uSec / 1000000;
  timeRequest.tv_nsec = (uSec % 1000000) * 1000;

  while ((nanosleep (&timeRequest, &timeRequest) == -1) &&
         (errno == EINTR) &&
         (timeRequest.tv_nsec > 0 || timeRequest.tv_sec > 0));
  }
//}}}
