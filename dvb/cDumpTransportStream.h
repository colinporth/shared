// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"
//}}}

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const string& rootName, bool recordAll) :
    mRootName(rootName), mRecordAll(recordAll) {}
  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void start (cService* service, const string& name, system_clock::time_point time, bool selected) {

    lock_guard<mutex> lockGuard (mFileMutex);

    service->closeFile();

    if (selected || mRecordAll) {
      if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
        auto validName = validFileString (name, "<>:/|?*\"\'\\");
        auto timeStr = date::format ("@%H.%M %a %d %b %Y", date::floor<seconds>(time));
        service->openFile (mRootName + "/" + validName + timeStr  + ".ts", 0x1234);
        }
      }
    }
  //}}}
  //{{{
  void pesPacket (int sid, int pid, uint8_t* ts) {
  // look up service and write it

    lock_guard<mutex> lockGuard (mFileMutex);

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end())
      serviceIt->second.writePacket (ts, pid);
    }
  //}}}
  //{{{
  void stop (cService* service) {

    lock_guard<mutex> lockGuard (mFileMutex);

    service->closeFile();
    }
  //}}}

private:
  mutex mFileMutex;
  string mRootName;
  bool mRecordAll;
  };
