// cTsDvb.h
//{{{  includes
#pragma once

#include <string>
#include <vector>

#include "cDvb.h"

class cSubtitle;
class cTransportStream;
//}}}

class cTsDvb : public cDvb {
public:
  cTsDvb (int frequency, const std::string& root,
          const std::vector<std::string>& channelNames, const std::vector<std::string>& recordNames,
          bool decodeSubtitle);
  virtual ~cTsDvb();

  cTransportStream* getTransportStream();
  cSubtitle* getSubtitleBySid (int sid);

  void grabThread (const std::string& root, const std::string& multiplexName);
  void readThread (const std::string& fileName);

  // public for widget observe
  bool mDecodeSubtitle = false;
  };
