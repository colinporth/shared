// cDvbRtp.h
#pragma once
//{{{  includes
#include <cstdlib>
#include <cstdint>
#include <string>

class cTsBlock;
class cTsBlockPool;
class cDvb;
//}}}

class cDvbRtp {
public:
  cDvbRtp (cDvb* dvb, cTsBlockPool* blockPool);
  ~cDvbRtp();

  uint64_t getNumPackets();
  uint64_t getNumErrors();
  uint64_t getNumInvalids();
  uint64_t getNumDiscontinuities();
  std::string getTimeString();

  int getNumOutputs();
  std::string getOutputInfoString (int outputNum);

  static bool selectOutput (const std::string& addressString, int sid);
  static void processBlockList (cTsBlock* blockList);
  };
