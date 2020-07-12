// cDvb.h
//{{{  includes
#pragma once
#include <string>

#include "cDumpTransportStream.h"
//}}}

class cDvb : public cDumpTransportStream {
public:
  cDvb (int frequency, const std::string& root, bool recordAll);
  virtual ~cDvb();

  void tune (int frequency);

  void captureThread();
  void grabThread();

  // public for widget observe
  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";
  };
