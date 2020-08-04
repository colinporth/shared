// cDvb.h
//{{{  includes
#pragma once
#include <string>

#include "cDumpTransportStream.h"
#include "cSubtitleDecoder.h"
//}}}

class cDvb : public cDumpTransportStream {
public:
  cDvb (int frequency, const std::string& root,
        const std::vector<std::string>& channelNames, const std::vector<std::string>& recordNames);

  virtual ~cDvb();

  void tune (int frequency);

  void captureThread();
  void grabThread();
  void readThread (const std::string& fileName);

  // public for widget observe
  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";

  cSubtitle mSubtitle;

protected:
  virtual bool subDecodePes (cPidInfo* pidInfo);
  };
