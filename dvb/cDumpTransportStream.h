// cDumpTransportStream.h
//{{{  includes
#pragma once
#include "cTransportStream.h"

#include "cSubtitleDecoder.h"
//}}}
const bool kSubtitleDebug = true;
const bool kSubtitleDecodeDebug = true;

class cDumpTransportStream : public cTransportStream {
  public:
    cDumpTransportStream (const std::string& rootName,
                          const std::vector <std::string>& channelStrings,
                          const std::vector <std::string>& saveStrings)
      : mRootName(rootName),
        mChannelStrings(channelStrings), mSaveStrings(saveStrings),
        mRecordAll ((channelStrings.size() == 1) && (channelStrings[0] == "all")) {}

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

      if (!mRecordAll) {
        // filter and rename channel prefix
        size_t i = 0;
        for (auto& channelString : mChannelStrings) {
          if (channelString == service->getChannelString()) {
            record = true;
            if (i < mSaveStrings.size())
              saveName = mSaveStrings[i] +  " ";
            break;
            }
          i++;
          }
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

    //{{{
    bool audDecodePes (cPidInfo* pidInfo, bool skip) {

      //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " a - " + dec(pidInfo->getBufUsed());
      return false;
      }
    //}}}
    //{{{
    bool vidDecodePes (cPidInfo* pidInfo, bool skip) {

      //cLog::log (LOGINFO, getPtsString (pidInfo->mPts) + " v - " + dec(pidInfo->getBufUsed());

      return false;
      }
    //}}}
    //{{{
    bool subDecodePes (cPidInfo* pidInfo) {

      if (kSubtitleDebug)
        cLog::log (LOGINFO, "subDecodePes - pts:" + getPtsString (pidInfo->mPts) +
                            " size:" + dec(pidInfo->getBufUsed(),4) +
                            " sid:" + dec(pidInfo->mPid));

      if (kSubtitleDecodeDebug) {
        cSubtitleDecoder decoder;
        auto subtitle = decoder.decode (pidInfo->mBuffer, pidInfo->getBufUsed());
        if (subtitle)
          subtitle->debug();
        delete subtitle;
        }

      return false;
      }
    //}}}

  private:
    std::string mRootName;

    std::vector<std::string> mChannelStrings;
    std::vector<std::string> mSaveStrings;
    bool mRecordAll;

    std::mutex mFileMutex;
    };
