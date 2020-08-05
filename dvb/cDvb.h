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

  int getNumSubtitleServices();
  cSubtitle* getSubtitle (int serviceIndex);
  cTransportStream* getTransportStream();

  void tune (int frequency);

  void captureThread();
  void grabThread();
  void readThread (const std::string& fileName);

  // public for widget observe
  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";
  };
