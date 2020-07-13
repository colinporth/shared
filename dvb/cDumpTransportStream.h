// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"
//}}}

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const std::string& rootName,
                        const std::vector <std::string>& channelNames,
                        const std::vector <std::string>& saveNames)
    : mRootName(rootName),
      mChannelNames(channelNames), mSaveNames(saveNames),
      mRecordAll ((channelNames.size() == 1) && (channelNames[0] == "all")) {}

  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void start (cService* service, const std::string& name,
              std::chrono::system_clock::time_point time,
              std::chrono::system_clock::time_point starttime,
              bool selected) {

    std::lock_guard<std::mutex> lockGuard (mFileMutex);

    service->closeFile();

    bool record = selected || mRecordAll;
    std::string saveName;

    size_t i = 0;
    for (auto& channelName : mChannelNames) {
      if (channelName == service->getChannelName()) {
        record = true;
        if (i < mSaveNames.size())
          saveName = mSaveNames[i] +  "  ";
        break;
        }
      i++;
      }

    saveName += date::format ("%d %b %y %a %H.%M", date::floor<std::chrono::seconds>(time));

    if (record) {
      if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
        auto validName = validFileString (name, "<>:/|?*\"\'\\");
        auto fileNameStr = mRootName + "/" + saveName + " - " + validName + ".ts";
        service->openFile (fileNameStr, 0x1234);
        cLog::log (LOGINFO, fileNameStr);
        }
      }
    }
  //}}}
  //{{{
  void pesPacket (int sid, int pid, uint8_t* ts) {
  // look up service and write it

    std::lock_guard<std::mutex> lockGuard (mFileMutex);

    auto serviceIt = mServiceMap.find (sid);
    if (serviceIt != mServiceMap.end())
      serviceIt->second.writePacket (ts, pid);
    }
  //}}}
  //{{{
  void stop (cService* service) {

    std::lock_guard<std::mutex> lockGuard (mFileMutex);

    service->closeFile();
    }
  //}}}

private:
  std::string mRootName;

  std::vector<std::string> mChannelNames;
  std::vector<std::string> mSaveNames;
  bool mRecordAll;

  std::mutex mFileMutex;
  };
