// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"
//}}}

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const std::string& rootName, bool recordAll) :
    mRootName(rootName), mRecordAll(recordAll) {}
  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void start (cService* service, const std::string& name, std::chrono::system_clock::time_point time, bool selected) {

    std::lock_guard<std::mutex> lockGuard (mMutex);

    service->closeFile();

    if (selected || mRecordAll) {
      if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
        auto validName = validFileString (name, "<>:/|?*\"\'\\");
        auto timeStr = date::format ("@%H.%M %a %d %b %Y", floor<std::chrono::seconds>(time));
        service->openFile (mRootName + "/" + validName + timeStr  + ".ts", 0x1234);
        }
      }
    }
  //}}}
  //{{{
  void pesPacket (int sid, int pid, uint8_t* ts) {
  // look up service and write it

    std::lock_guard<std::mutex> lockGuard (mMutex);

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end())
      serviceIt->second.writePacket (ts, pid);
    }
  //}}}
  //{{{
  void stop (cService* service) {

    std::lock_guard<std::mutex> lockGuard (mMutex);

    service->closeFile();
    }
  //}}}

private:
  std::string mRootName;
  bool mRecordAll;
  };
