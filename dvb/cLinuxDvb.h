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
  cDvb (int frequency, const string& root);
  virtual ~cDvb();

  void tune (int frequency);

  void captureThread();
  void grabThread();
  void readThread (const string& inTs);

  // public for widgets
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "signal";
  std::string mErrorStr = "";

private:
  void init (int frequency);
  void setTsFilter (uint16_t pid, dmx_pes_type_t pestype);
  void monitorFe();
  void updateSignalString();
  void uSleep (uint64_t uSec);

  int mFrontEnd = 0;
  int mDemux = 0;
  int mDvr = 0 ;

  uint64_t mLastDiscontinuity = 0;
  int mLastBlockSize = 0;
  int mMaxBlockSize = 0;
  cBipBuffer* mBipBuffer;
  };
