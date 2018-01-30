// cDumpTransportStream.h
#pragma once
#include "cTransportStream.h"

class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const std::string& rootName, bool recordAll) :
    mRootName(rootName), mRecordAll(recordAll){}
  virtual ~cDumpTransportStream() { clear(); }

  //{{{
  virtual void clear() {

    cLog::log (LOGNOTICE, "cDumpTransportStream clear");
    cTransportStream::clear();
    mRecordServiceMap.clear();
    }
  //}}}
  //{{{
  virtual void start (cService* service, const std::string& name, time_t startTime, bool selected) {

    auto recordServiceIt = mRecordServiceMap.find (service->getSid());
    if (recordServiceIt != mRecordServiceMap.end())
      recordServiceIt->second.close();

    if (selected || mRecordAll) {
      std::lock_guard<std::mutex> lockGuard (mRecordMutex);

      if ((service->getVidPid() > 0) && (service->getAudPid() > 0)) {
        if (recordServiceIt == mRecordServiceMap.end())
          recordServiceIt = mRecordServiceMap.insert (
          std::map<int,cRecordService>::value_type (
            service->getSid(), cRecordService (service->getVidPid(), service->getAudPid()))).first;

        auto validFileName = validFileString (name, "<>:/|?*\"\'\\");
        auto str = mRootName + "/" + validFileName + " - " + getTimetDateString (startTime) + ".ts";
        cLog::log (LOGNOTICE, "start file " + str);

        recordServiceIt->second.create (str, service);
        }
      }
    }
  //}}}
  //{{{
  virtual void stop (cService* service) {

    std::lock_guard<std::mutex> lockGuard (mRecordMutex);

    auto recordServiceIt = mRecordServiceMap.find (service->getSid());
    if (recordServiceIt != mRecordServiceMap.end()) // close any record
      mRecordServiceMap.erase (recordServiceIt);
    }
  //}}}

protected:
  //{{{
  void pesPacket (cPidInfo* pidInfo, uint8_t* ts) {
  // save pes packet

    std::lock_guard<std::mutex> lockGuard (mRecordMutex);

    auto recordServiceIt = mRecordServiceMap.find (pidInfo->mSid);
    if (recordServiceIt != mRecordServiceMap.end())
      recordServiceIt->second.writePes (pidInfo->mPid, ts);
    }
  //}}}

private:
  //{{{
  class cRecordService {
  public:
    cRecordService (int vidPid, int audPid) : mVidPid(vidPid), mAudPid(audPid) {}
    ~cRecordService() { close(); }

    //{{{
    void create (const std::string& name, cService* service) {

      mFile = fopen (name.c_str(), "wb");
      if (mFile) {
        writePat (0x1234, service->getSid(), kPgmPid); // tsid, sid, pgmPid

        writePmt (service->getSid(), kPgmPid, service->getVidPid(),
                  service->getVidPid(), service->getVidStreamType(),
                  service->getAudPid(), service->getAudStreamType());
        }
      else
        cLog::log (LOGERROR, "cRecordService::create failed");
      }
    //}}}
    //{{{
    void writePes (int pid, uint8_t* ts) {

      //cLog::log (LOGINFO, "writePes");
      if (mFile && ((pid == mVidPid) || (pid == mAudPid)))
        fwrite (ts, 1, 188, mFile);
      }
    //}}}
    //{{{
    void close() {

      if (mFile)
        fclose (mFile);
      mFile = nullptr;
      }
    //}}}

  private:
    //{{{
    uint8_t* tsHeader (uint8_t* ts, int pid, int continuityCount) {

      memset (ts, 0xFF, 188);

      *ts++ = 0x47;                         // sync byte
      *ts++ = 0x40 | ((pid & 0x1f00) >> 8); // payload_unit_start_indicator + pid upper
      *ts++ = pid & 0xff;                   // pid lower
      *ts++ = 0x10 | continuityCount;       // no adaptControls + cont count
      *ts++ = 0;                            // pointerField

      return ts;
      }
    //}}}
    //{{{
    void writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr) {

      // tsSection crc, calc from tsSection start to here
      auto crc = getCrc32 (0xffffffff, tsSectionStart, int(tsPtr - tsSectionStart));
      *tsPtr++ = (crc & 0xff000000) >> 24;
      *tsPtr++ = (crc & 0x00ff0000) >> 16;
      *tsPtr++ = (crc & 0x0000ff00) >>  8;
      *tsPtr++ =  crc & 0x000000ff;

      fwrite (ts, 1, 188, mFile);
      }
    //}}}
    //{{{
    void writePat (int tsid, int sid, int pgmPid) {

      //cLog::log (LOGINFO, "writePat");
      uint8_t ts[188];
      uint8_t* tsSectionStart = tsHeader (ts, PID_PAT, 0);
      uint8_t* tsPtr = tsSectionStart;

      auto sectionLength = 5+4 + 4;
      *tsPtr++ = TID_PAT;                                // PAT tid
      *tsPtr++ = 0xb0 | ((sectionLength & 0x0F00) >> 8); // Pat sectionLength upper
      *tsPtr++ = sectionLength & 0x0FF;                  // Pat sectionLength lower

      // sectionLength from here to end of crc
      *tsPtr++ = (tsid & 0xFF00) >> 8;                   // transportStreamId
      *tsPtr++ =  tsid & 0x00FF;
      *tsPtr++ = 0xc1;                                   // For simplicity, we'll have a version_id of 0
      *tsPtr++ = 0x00;                                   // First section number = 0
      *tsPtr++ = 0x00;                                   // last section number = 0

      *tsPtr++ = (sid & 0xFF00) >> 8;                    // first section serviceId
      *tsPtr++ =  sid & 0x00FF;
      *tsPtr++ = 0xE0 | ((pgmPid & 0x1F00) >> 8);        // first section pgmPid
      *tsPtr++ = pgmPid & 0x00FF;

      writeSection (ts, tsSectionStart, tsPtr);
      }
    //}}}
    //{{{
    void writePmt (int serviceId, int pgmPid, int pcrPid,
                   int vidPid, int vidStreamType, int audPid, int audStreamType) {

      //cLog::log (LOGINFO, "writePmt");
      uint8_t ts[188];
      uint8_t* tsSectionStart = tsHeader (ts, pgmPid, 0);
      uint8_t* tsPtr = tsSectionStart;

      int sectionLength = 23; // 5+4 + program_info_length + esStreams * (5 + ES_info_length) + 4
      *tsPtr++ = TID_PMT;
      *tsPtr++ = 0xb0 | ((sectionLength & 0x0F00) >> 8);
      *tsPtr++ = sectionLength & 0x0FF;

      // sectionLength from here to end of crc
      *tsPtr++ = (serviceId & 0xFF00) >> 8;
      *tsPtr++ = serviceId & 0x00FF;
      *tsPtr++ = 0xc1; // version_id of 0
      *tsPtr++ = 0x00; // first section number = 0
      *tsPtr++ = 0x00; // last section number = 0

      *tsPtr++ = 0xE0 | ((pcrPid & 0x1F00) >> 8);
      *tsPtr++ = pcrPid & 0x00FF;

      *tsPtr++ = 0xF0;
      *tsPtr++ = 0; // program_info_length;

      // vid es
      *tsPtr++ = vidStreamType; // elementary stream_type
      *tsPtr++ = 0xE0 | ((vidPid & 0x1F00) >> 8); // elementary_PID
      *tsPtr++ = vidPid & 0x00FF;
      *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;  // ES_info_length
      *tsPtr++ = 0 & 0x00FF;

      // aud es
      *tsPtr++ = audStreamType; // elementary stream_type
      *tsPtr++ = 0xE0 | ((audPid & 0x1F00) >> 8); // elementary_PID
      *tsPtr++ = audPid & 0x00FF;
      *tsPtr++ = ((0 & 0xFF00) >> 8) | 0xF0;  // ES_info_length
      *tsPtr++ = 0 & 0x00FF;

      writeSection (ts, tsSectionStart, tsPtr);
      }
    //}}}

    const int kPgmPid = 32;

    FILE* mFile = nullptr;
    int mVidPid = -1;
    int mAudPid = -1;
    };
  //}}}

  std::mutex mRecordMutex;
  std::string mRootName;
  bool mRecordAll;
  std::map<int,cRecordService> mRecordServiceMap;
  };
