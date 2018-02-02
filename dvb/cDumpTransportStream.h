// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"
#include <chrono>
#include "../utils/date.h"
//}}}

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const std::string& rootName, bool recordAll) :
    mRootName(rootName), mRecordAll(recordAll) {}
  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void start (cService* service, const std::string& name,
              std::chrono::system_clock::time_point startTime, bool selected) {

    service->closeFile();

    if (selected || mRecordAll) {
      if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
        auto validFileName = validFileString (name, "<>:/|?*\"\'\\");
        service->openFile (
          mRootName + "/" + validFileName + " @ " +
          date::format ("%D %T", floor<std::chrono::seconds>(startTime)) + ".ts",
          0x1234, 32);
        }
      }
    }
  //}}}
  //{{{
  void pesPacket (int sid, int pid, uint8_t* ts) {
  // look up service and write it

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end())
      serviceIt->second.writePacket (ts, pid);
    }
  //}}}
  //{{{
  void stop (cService* service) {

    service->closeFile();
    }
  //}}}

private:
  std::string mRootName;
  bool mRecordAll;
  };
