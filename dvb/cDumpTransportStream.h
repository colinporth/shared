// cDumpTransportStream.h
#pragma once
#include "cTransportStream.h"

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const std::string& rootName, bool recordAll) :
    mRootName(rootName), mRecordAll(recordAll) {}
  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void start (cService* service, const std::string& name, time_t startTime, bool selected) {

    service->closeFile();

    if (selected || mRecordAll) {
      std::lock_guard<std::mutex> lockGuard (mRecordMutex);

      if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
        auto validFileName = validFileString (name, "<>:/|?*\"\'\\");
        auto str = mRootName + "/" + validFileName + " - " + getTimetDateString (startTime) + ".ts";
        cLog::log (LOGNOTICE, "start file " + str);

        if (!service->openFile (str, 0x1234, 32))
          cLog::log (LOGERROR, "start failed");
        }
      }
    }
  //}}}
  //{{{
  void pesPacket (cPidInfo* pidInfo, uint8_t* ts) {
  // save pes packet

    std::lock_guard<std::mutex> lockGuard (mRecordMutex);
    auto serviceIt = mServiceMap.find (pidInfo->mSid);
    if (serviceIt != mServiceMap.end())
      serviceIt->second.writePesPacket (ts, pidInfo->mPid);
    }
  //}}}
  //{{{
  void stop (cService* service) {

    service->closeFile();
    }
  //}}}

private:
  std::mutex mRecordMutex;
  std::string mRootName;
  bool mRecordAll;
  };
