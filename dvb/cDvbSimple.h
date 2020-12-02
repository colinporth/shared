// cDvbSimple.h
//{{{  includes
#pragma once

#include <string>
#include <vector>
//}}}

class cDvbSimple {
public:
  cDvbSimple (int frequency);
  virtual ~cDvbSimple();

  void tune (int frequency);
  void run();
  bool getTsBlock (uint8_t*& block, int& blockSize);
  void releaseTsBlock (int blockSize);

  void captureThread();

  std::string mErrorStr = "waiting";
  std::string mTuneStr = "untuned";
  std::string mSignalStr = "no signal";
  };
