// cLinuxDvb.h
//{{{  includes
#pragma once
#include <string>

#include <linux/dvb/version.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

#include "cDumpTransportStream.h"

class cBipBuffer;
//}}}

class cDvb : public cDumpTransportStream {
public:
  cDvb (int frequency, const std::string& root, bool recordAll);
  virtual ~cDvb();

  void tune (int frequency);

  void captureThread();
  void grabThread();

  // public for widgets
  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "signal";

private:
  void setTsFilter (uint16_t pid, dmx_pes_type_t pestype);
  void updateSignalString();
  void monitorFe();
  void uSleep (uint64_t uSec);

  int mFrontEnd = 0;
  int mDemux = 0;
  int mDvr = 0 ;

  uint64_t mLastDiscontinuity = 0;
  int mLastBlockSize = 0;
  int mMaxBlockSize = 0;
  cBipBuffer* mBipBuffer;
  };
