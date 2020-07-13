//{{{  cTransportStream.h - transport stream parser
// PAT inserts <pid,sid> into mProgramMap
// PMT declares pgm and elementary stream pids, adds cService into mServiceMap
// SDT names a service in mServiceMap
//}}}
//{{{  includes
#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>

#include <time.h>
#include <chrono>
#include "../date/date.h"
//}}}

//{{{
class cPidInfo {
public:
  cPidInfo (int pid, bool isPsi);
  ~cPidInfo();

  int getBufUsed();
  std::string getTypeString();
  std::string getInfoString();
  std::wstring getTypeWstring();
  std::wstring getInfoWstring();
  int addToBuffer (uint8_t* buf, int bufSize);

  void clearCounts();
  void clearContinuity();
  void print();

  const int mPid;
  const bool mPsi;

  int mSid = -1;
  int mStreamType = 0;

  int mPackets = 0;
  int mContinuity = -1;
  int mDisContinuity = 0;
  int mRepeatContinuity = 0;

  int64_t mPts = -1;
  int64_t mFirstPts = -1;
  int64_t mLastPts = -1;

  int mBufSize = 0;
  uint8_t* mBuffer = nullptr;
  uint8_t* mBufPtr = nullptr;

  int64_t mStreamPos = -1;

  std::string mInfoStr;
  };
//}}}
//{{{
class cEpgItem {
public:
  cEpgItem (bool now, bool record, std::chrono::system_clock::time_point time, std::chrono::seconds duration,
            const std::string& title, const std::string& description);
  ~cEpgItem();

  std::string getTitleString() { return mTitle; }
  std::string getDesriptionString() { return mDescription; }

  std::chrono::seconds getDuration() { return mDuration; }
  std::chrono::system_clock::time_point getTime() { return mTime; }

  bool getRecord() { return mRecord; }

  bool toggleRecord();
  void set (std::chrono::system_clock::time_point time, std::chrono::seconds duration, const std::string& title, const std::string& description);

  void print (const std::string& prefix);

private:
  const bool mNow = false;

  bool mRecord = false;

  std::chrono::system_clock::time_point mTime;
  std::chrono::seconds mDuration;
  std::string mTitle;
  std::string mDescription;
  };
//}}}
//{{{
class cService {
public:
  cService (int sid);
  ~cService();

  // gets
  int getSid() const { return mSid; }
  int getProgramPid() const { return mProgramPid; }
  int getVidPid() const { return mVidPid; }
  int getVidStreamType() const { return mVidStreamType; }
  int getAudPid() const { return mAudPid; }
  int getAudStreamType() const { return mAudStreamType; }
  int getAudOtherPid() const { return mAudOtherPid; }

  std::string getChannelName() { return mChannelName; }

  bool getShowEpg() { return mShowEpg; }
  cEpgItem* getNowEpgItem() { return mNowEpgItem; }
  std::string getNowTitleString() { return mNowEpgItem ? mNowEpgItem->getTitleString() : ""; }
  std::map<std::chrono::system_clock::time_point,cEpgItem*>& getEpgItemMap() { return mEpgItemMap; }

  bool isEpgRecord (const std::string& title, std::chrono::system_clock::time_point startTime);

  //  sets
  void setVidPid (int pid, int streamType);
  void setAudPid (int pid, int streamType);
  void toggleShowEpg();

  void setChannelName (const std::string& channelName);
  void setProgramPid (int pid);
  bool setNow (bool record, std::chrono::system_clock::time_point time, std::chrono::seconds duration,
               const std::string& str1, const std::string& str2);
  bool setEpg (bool record, std::chrono::system_clock::time_point startTime, std::chrono::seconds duration,
               const std::string& title, const std::string& description);
  // file
  bool openFile (const std::string& fileName, int tsid);
  void writePacket (uint8_t* ts, int pid);
  void closeFile();

  void print();

private:
  uint8_t* tsHeader (uint8_t* ts, int pid, int continuityCount);
  void writePat (int tsid);
  void writePmt();
  void writeSection (uint8_t* ts, uint8_t* tsSectionStart, uint8_t* tsPtr);

  const int mSid;
  std::string mChannelName;

  int mProgramPid = -1;
  int mVidPid = -1;
  int mVidStreamType = 0;
  int mAudPid = -1;
  int mAudOtherPid = -1;
  int mAudStreamType = 0;

  cEpgItem* mNowEpgItem = nullptr;
  std::map<std::chrono::system_clock::time_point,cEpgItem*> mEpgItemMap;

  bool mShowEpg = true;
  FILE* mFile = nullptr;
  };
//}}}

class cTransportStream {
friend class cTsEpgBox;
friend class cTsPidBox;
public:
  virtual ~cTransportStream() { clear(); }

  //  gets
  static char getFrameType (uint8_t* pesBuf, int64_t pesBufSize, int streamType);
  uint64_t getDiscontinuity() { return mDiscontinuity; }
  std::chrono::system_clock::time_point getTime() { return mTime; }
  cService* getService (int index, int64_t& firstPts, int64_t& lastPts);
  int64_t demux (uint8_t* tsBuf, int64_t tsBufSize, int64_t streamPos, bool skip, int decodePid);
  virtual void clear();

  std::mutex mMutex;
  std::map<int,cPidInfo> mPidInfoMap;
  std::map<int,cService> mServiceMap;

protected:
  virtual bool audDecodePes (cPidInfo* pidInfo, bool skip) { return false; }
  virtual bool vidDecodePes (cPidInfo* pidInfo, bool skip) { return false; }

  virtual void start (cService* service, const std::string& name, 
                      std::chrono::system_clock::time_point time, 
                      std::chrono::system_clock::time_point startTime,
                      bool selected) {}
  virtual void pesPacket (int sid, int pid, uint8_t* ts) {}
  virtual void stop (cService* service) {}

  void clearPidCounts();
  void clearPidContinuity();

private:
  cPidInfo* getPidInfo (int pid, bool createPsiOnly);
  int64_t getPtsDts (uint8_t* tsPtr);
  std::string getDescrStr (uint8_t* buf, int len);

  void parseDescr (int key, uint8_t* buf, int tid);
  void parseDescrs (int key, uint8_t* buf, int len, uint8_t tid);

  void parsePat (cPidInfo* pidInfo, uint8_t* buf);
  void parseNit (cPidInfo* pidInfo, uint8_t* buf);
  void parseSdt (cPidInfo* pidInfo, uint8_t* buf);
  void parseEit (cPidInfo* pidInfo, uint8_t* buf);
  void parseTdt (cPidInfo* pidInfo, uint8_t* buf);
  void parsePmt (cPidInfo* pidInfo, uint8_t* buf);
  int parsePsi (cPidInfo* pidInfo, uint8_t* buf);

  // vars
  uint64_t mDiscontinuity = 0;

  std::map<int,int> mProgramMap;

  bool mTimeDefined = false;
  std::chrono::system_clock::time_point mTime;
  std::chrono::system_clock::time_point mFirstTime;
  };
