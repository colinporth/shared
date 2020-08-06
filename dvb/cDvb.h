// cDvb.h
//{{{  includes
#pragma once
#include <string>
#include <vector>
//}}}

class cSubtitle;
class cTransportStream;

class cDvb {
public:
  cDvb (int frequency, const std::string& root,
        const std::vector<std::string>& channelNames, const std::vector<std::string>& recordNames);

  virtual ~cDvb();

  cTransportStream* getTransportStream();
  cSubtitle* getSubtitleBySid (int sid);

  void tune (int frequency);

  void captureThread();
  void grabThread (const std::string& root, const std::string& multiplexName);
  void readThread (const std::string& fileName);

  // public for widget observe
  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";
  };
