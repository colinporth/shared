// cDvb.cpp
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
#include "../decoders/cDumpTransportStream.h"

#include "cDvb.h"

using namespace std;
//}}}

// public:
//{{{
cDvb::cDvb (const string& root) : mTs(root) {
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
void cDvb::uSleep (uint64_t uSec) {

  struct timespec timeRequest = { 0 };

  timeRequest.tv_sec = uSec / 1000000;
  timeRequest.tv_nsec = (uSec % 1000000) * 1000;

  while ((nanosleep (&timeRequest, &timeRequest) == -1) &&
         (errno == EINTR) &&
         (timeRequest.tv_nsec > 0 || timeRequest.tv_sec > 0));
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
      streamPos += mTs.demux (ptr, blockSize, 0, false, -1);
      decommitBlock (blockSize);

      bool show = (mTs.getDiscontinuity() != mLastDiscontinuity) || (blockSize > mLastBlockSize);
      mLastDiscontinuity = mTs.getDiscontinuity();
      if (blockSize > mLastBlockSize)
        mLastBlockSize = blockSize;
      if (blockSize > mMaxBlockSize)
        mMaxBlockSize = blockSize;

      mPacketStr = dec(mTs.getDiscontinuity()) +
                   ":" + dec(mTs.getPackets()) +
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
      streamPos += mTs.demux (buffer, bytesRead, streamPos, false, -1);
    else
      break;
    mPacketStr = dec(mTs.getDiscontinuity()) + ":" + dec(mTs.getPackets());
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
  tune (frequency * 1000000, BANDWIDTH_8_MHZ);

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
void cDvb::tune (unsigned int frequency, fe_bandwidth_t bandwidth) {

  struct dvb_frontend_info feInfo;
  auto error = ioctl (mFrontEnd, FE_GET_INFO, &feInfo);
  if (error < 0) {
    cLog::log (LOGERROR, "dvbTune::ioctl FE_GET_INFO " + dec(frequency) + " " + dec(error));
    return;
    }
  mTuneStr = "tune " + string(feInfo.name) + " to " + dec(frequency/1000000);
  cLog::log (LOGNOTICE, mTuneStr);
  uSleep (100000);

  struct dvb_frontend_parameters feParams;
  feParams.frequency = frequency;
  feParams.u.ofdm.bandwidth = bandwidth;
  feParams.inversion = INVERSION_OFF;
  feParams.u.ofdm.constellation = QAM_AUTO;
  feParams.u.ofdm.code_rate_HP = FEC_AUTO;
  feParams.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
  feParams.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
  feParams.u.ofdm.code_rate_LP = FEC_AUTO;
  feParams.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
  error = ioctl (mFrontEnd, FE_SET_FRONTEND, &feParams);
  if (error < 0) {
    cLog::log (LOGERROR, "tune - FE_SET_FRONTEND failed " + dec(error));
    return;
    }

  //{{{  wait for lock
  struct pollfd pfd[1];
  pfd[0].fd = mFrontEnd;
  pfd[0].events = POLLPRI;

  time_t tm1 = time ((time_t*)NULL);

  int lockCountDown = 2;
  fe_status_t feStatus;
  do {
    if (poll (pfd, 1, 3000) > 0)
      if (pfd[0].revents & POLLPRI)
        if (ioctl (mFrontEnd, FE_READ_STATUS, &feStatus) >= 0)
          if (feStatus & FE_HAS_LOCK)
            lockCountDown--;
    uSleep (10000);
    } while (lockCountDown && !(feStatus & FE_TIMEDOUT) && (time ((time_t*)NULL) - tm1 < 4));
  //}}}

  if (lockCountDown)
    cLog::log (LOGERROR, "tune - unable to lock");
  else {
    error = ioctl (mFrontEnd, FE_GET_FRONTEND, feParams);
    if (error >= 0)
      cLog::log (LOGINFO, "Frequency " + dec(feParams.frequency) + " " + dec(error));
    mTuneStr = "tuned " + string(feInfo.name) + " to " + dec(feParams.frequency/1000000);
    }

  updateSignalString();
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
