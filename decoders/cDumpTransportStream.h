// cDumpTransportStream.h
//{{{  includes
#include "stdafx.h"

#include "../../shared/decoders/cTransportStream.h"

using namespace std;
//}}}

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const string& rootName) : mRootName(rootName){}
  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void startProgram (cService* service, const string& name, time_t startTime) {

    cLog::log (LOGNOTICE, "startProgram " + name);

    auto recordFileIt = mRecordFileMap.find (service->getSid());
    if (recordFileIt == mRecordFileMap.end()) // create new cRecordFile for this cService
      recordFileIt = mRecordFileMap.insert (
        map<int,cRecordFile>::value_type (
          service->getSid(), cRecordFile (service->getVidPid(), service->getAudPid()))).first;

    auto validFileName = validString (service->getNameString() + " - " + name, "<>:\\|?*""/");
    recordFileIt->second.createFile (mRootName + "/" + validFileName + ".ts", service);
    }
  //}}}
  //{{{
  void pesPacket (cPidInfo* pidInfo, uint8_t* ts) {
  // save pes packet

    auto recordFileIt = mRecordFileMap.find (pidInfo->mSid);
    if (recordFileIt != mRecordFileMap.end())
      recordFileIt->second.writePes (pidInfo->mPid, ts);
    }
  //}}}

private:
  string mRootName;
  map<int,cRecordFile> mRecordFileMap;
  };
