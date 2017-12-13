// cDumpTransportStream.h
//{{{  includes
#include "stdafx.h"

#include "../../shared/decoders/cTransportStream.h"
//}}}


class cDumpTransportStream : public cTransportStream {
public:
  cDumpTransportStream (const std::string& rootName) : mRootName(rootName){}
  virtual ~cDumpTransportStream() {}

protected:
  //{{{
  void startProgram (cService* service, const std::string& name, time_t startTime) {

    cLog::log (LOGNOTICE, "startProgram " + name);

    auto recordFileIt = mRecordFileMap.find (service->getSid());
    if (recordFileIt == mRecordFileMap.end()) // create new cRecordFile for this cService
      recordFileIt = mRecordFileMap.insert (
        std::map<int,cRecordFile>::value_type (
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
  //{{{
  class cRecordFile {
  public:
    cRecordFile (int vidPid, int audPid) : mVidPid(vidPid), mAudPid(audPid) {}
    ~cRecordFile() { closeFile(); }

    //{{{
    void createFile (const std::string& name, cService* service) {

      // close any previous file
      closeFile();

      mOk = (service->getVidPid() > 0) && (service->getAudPid() > 0);
      if (mOk) {
        mFile = CreateFile (name.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

        writePat (0x1234, service->getSid(), kPgmPid); // tsid, sid, pgmPid
        writePmt (service->getSid(), kPgmPid, service->getVidPid(),
                  service->getVidPid(), service->getVidStreamType(),
                  service->getAudPid(), service->getAudStreamType());
        }
      }
    //}}}
    //{{{
    void writePes (int pid, uint8_t* ts) {

      //cLog::log (LOGINFO, "writePes");
      if (mOk && ((pid == mVidPid) || (pid == mAudPid)))
        writePacket (ts);
      }
    //}}}
    //{{{
    void closeFile() {

      if (mFile != 0) {
        CloseHandle (mFile);
        mFile = 0;
        }
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

      writePacket (ts);
      }
    //}}}
    //{{{
    void writePacket (uint8_t* ts) {

      DWORD numBytesUsed;
      WriteFile (mFile, ts, 188, &numBytesUsed, NULL);
      if (numBytesUsed != 188)
        cLog::log (LOGERROR, "writePacket " + dec(numBytesUsed));
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

    bool mOk = false;
    HANDLE mFile = 0;
    int mVidPid = -1;
    int mAudPid = -1;
    };
  //}}}

  std::string mRootName;
  std::map<int,cRecordFile> mRecordFileMap;
  };
