// cDvb.h
//{{{  includes
#pragma once
#include <string>

#include "cDumpTransportStream.h"
//}}}

class cDvb : public cDumpTransportStream {
public:
  cDvb (int frequency, const std::string& root, 
        const std::vector<std::string>& channelNames, const std::vector<std::string>& recordNames);

  virtual ~cDvb();

  void tune (int frequency);

  void captureThread();
  void grabThread();

  // public for widget observe
  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";
  };
