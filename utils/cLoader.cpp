// cLoader.cpp - audio,video loader, launches cSongPlayer
//{{{  includes
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

// c++
#include <map>
#include <thread>
#include <functional>

// c
#include "sys/stat.h"

#include "cLoader.h"
#include "cSongPlayer.h"

// utils
#include "../fmt/core.h"
#include "../date/date.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

// container
#include "../utils/cSong.h"

// audio decode
#include "../decoders/cAudioParser.h"
#include "../decoders/iAudioDecoder.h"
#include "../decoders/cFFmpegAudioDecoder.h"

// video pool
#include "../utils/iVideoPool.h"

// net
#ifdef _WIN32
  #include <winsock2.h>
  #include <WS2tcpip.h>
  #include "../net/cWinSockHttp.h"
#endif

#ifdef __linux__
  #include "../net/cLinuxHttp.h"
#endif

#include "readerWriterQueue.h"

#include "../dvb/cDvb.h"
#include "../dvb/cDvbUtils.h"

using namespace std;
using namespace fmt;
using namespace chrono;
//}}}

//{{{
class cDvbEpgItem {
public:
  cDvbEpgItem() {}

  cDvbEpgItem (const string& programName, system_clock::time_point startTime, seconds duration)
    : mProgramName(programName), mStartTime(startTime), mDuration(duration) {}

  cDvbEpgItem (const cDvbEpgItem& epgItem)
    : mProgramName (epgItem.mProgramName), mStartTime (epgItem.mStartTime), mDuration (epgItem.mDuration) {}

  ~cDvbEpgItem() {}

  // gets
  string getProgramName() { return mProgramName; }
  system_clock::time_point getStartTime() { return mStartTime; }
  seconds getDuration() { return mDuration; }

private:
  string mProgramName;
  system_clock::time_point mStartTime;
  seconds mDuration;
  };
//}}}
//{{{
class cDvbService {
public:
  cDvbService (int sid, bool selected) : mSid(sid), mSelected(selected) {}

  bool isSelected() { return mSelected; }
  int getAudioPid() { return mAudioPid; }
  int getVideoPid() { return mVideoPid; }
  int getSubtitlePid() { return mSubtitlePid; }

  void setSelected (bool selected) { mSelected = selected; }
  void setAudioPid (int pid) { mAudioPid = pid; }
  void setVideoPid (int pid) { mVideoPid = pid; }
  void setSubtitlePid (int pid) { mSubtitlePid = pid; }
  //{{{
  bool setName (const string& name) {
  // return true if name changed

    if (mName == name)
      return false;

    mName = name;
    return true;
    }
  //}}}

private:
  const int mSid;

  int mAudioPid = 0;
  int mVideoPid = 0;
  int mSubtitlePid = 0;

  bool mSelected = false;
  string mName;

  cDvbEpgItem mNowEpgItem;
  map <system_clock::time_point, cDvbEpgItem*> mEpgItemMap;
  };
//}}}

// cPidParser
//{{{
class cPidParser {
public:
  cPidParser (int pid, const string& name) : mPid(pid), mPidName(name) {}
  virtual ~cPidParser() {}

  virtual int getQueueSize() { return 0; }
  virtual float getQueueFrac() { return 0.f; }

  //{{{
  void parse (uint8_t* ts, bool reuseFromFront) {
  // ignore any leading payload before a payloadStart

    bool payloadStart = ts[1] & 0x40;
    bool hasPayload = ts[3] & 0x10;

    if (hasPayload && (payloadStart || mGotPayloadStart)) {
      mGotPayloadStart = true;
      int continuityCount = ts[3] & 0x0F;
      int headerSize = 1 + ((ts[3] & 0x20) ? 4 + ts[4] : 3);
      processPayload (ts + headerSize, 188 - headerSize, payloadStart, continuityCount, reuseFromFront);
      }
    }
  //}}}

  virtual void exit() {}
  virtual void processLast (bool reuseFromFront) {}

protected:
  // main override
  //{{{
  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    string info;
    for (int i = 0; i < tsLeft; i++) {
      int value = ts[i];
      info += hex (value, 2) + " ";
      }

    cLog::log (LOGINFO, mPidName + " " + string(payloadStart ? "start ": "") + dec (tsLeft,3) + ":" + info);
    }
  //}}}

  // vars
  const int mPid;
  const string mPidName;
  bool mGotPayloadStart = false;
  };
//}}}
//{{{
class cPatParser : public cPidParser {
// assumes section length fits in this packet, no need to buffer
public:
  //{{{
  cPatParser (function<void (int programPid, int programSid)> callback)
    : cPidParser (0, "pat"), mCallback(callback) {}
  //}}}
  virtual ~cPatParser() {}

  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    if (payloadStart) {
      if (ts[0]) {
        //{{{  pointerfield error, return
        cLog::log (LOGERROR, mPidName + " unexpected pointerField");
        return;
        }
        //}}}
      ts++;
      tsLeft--;

      //int tid = ts[0]; // could check it
      int sectionLength = cDvbUtils::getSectionLength (ts+1);
      if (cDvbUtils::getCrc32 (ts, sectionLength) != 0) {
        //{{{  crc error, return
        cLog::log (LOGERROR, mPidName + " crc error");
        return;
        }
        //}}}
      //{{{  unused fields
      // transport stream id = ((ts[3] & 0xF) << 4) | ts[4]
      // currentNext,versionNumber = ts[5]
      // section number = ts[6]
      // last section number = ts[7]
      //}}}

      // skip past pat header
      constexpr int kPatHeaderLength = 8;
      ts += kPatHeaderLength;
      tsLeft -= kPatHeaderLength;
      sectionLength -= kPatHeaderLength + 4;
      if (sectionLength > tsLeft) {
        //{{{  sectionLength error, return
        cLog::log (LOGERROR, format ("{} sectionLength:{} tsLeft:{}", mPidName, sectionLength, tsLeft));
        return;
        }
        //}}}

      // iterate pat programs
      while (sectionLength > 0) {
        int programSid = (ts[0] << 8) + ts[1];
        int programPid = ((ts[2] & 0x1F) << 8) + ts[3];
        mCallback (programPid, programSid);

        constexpr int kPatProgramLength = 4;
        ts += kPatProgramLength;
        tsLeft -= kPatProgramLength;
        sectionLength -= kPatProgramLength;
        }
      }
    }

private:
  function <void (int programPid, int programSid)> mCallback;
  };
//}}}
//{{{
class cPmtParser : public cPidParser {
// assumes section length fits in this packet, no need to buffer
public:
  //{{{
  cPmtParser (int pid, int sid, function<void (int streamSid, int streamPid, int streamType)> callback)
    : cPidParser (pid, "pmt"), mSid(sid), mCallback(callback) {}
  //}}}
  virtual ~cPmtParser() {}

  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    if (payloadStart) {
      if (ts[0]) {
        //{{{  pointerfield error return
        cLog::log (LOGERROR, mPidName + " unexpected pointerField");
        return;
        }
        //}}}
      ts++;
      tsLeft--;

      //int tid = ts[0];
      // assumes section length fits in this packet, no need to buffer
      int sectionLength = cDvbUtils::getSectionLength (ts+1);
      if (cDvbUtils::getCrc32 (ts, sectionLength) != 0) {
        //{{{  crc error return
        cLog::log (LOGERROR, mPidName + " crc error");
        return;
        }
        //}}}

      int sid = (ts[3] << 8) + ts[4];
      int programInfoLength = ((ts[10] & 0x0f) << 8) + ts[11];
      //{{{  unused fields
      //int versionNumber = ts[5];
      //int sectionNumber = ts[6];
      //int lastSectionNumber = ts[7];
      //int pcrPid = ((ts[8] & 0x1f) << 8) + ts[9];
      //}}}

      // skip past pmt header
      constexpr int kPmtHeaderLength = 12;
      ts += kPmtHeaderLength + programInfoLength;
      tsLeft -= kPmtHeaderLength - programInfoLength;
      sectionLength -= kPmtHeaderLength + 4 - programInfoLength;

      if (sectionLength > tsLeft) {
        //{{{  sectionLength error return
        cLog::log (LOGERROR, format ("{} sectionLength:{} tsLeft:{}", mPidName, sectionLength, tsLeft));
        return;
        }
        //}}}

      // iterate pmt streams
      while (sectionLength > 0) {
        int streamType = ts[0];
        int streamPid = ((ts[1] & 0x1F) << 8) + ts[2];
        int streamInfoLength = ((ts[3] & 0x0F) << 8) + ts[4];
        mCallback (sid, streamPid, streamType);

        constexpr int kPmtStreamLength = 5;
        ts += kPmtStreamLength + streamInfoLength;
        tsLeft -= kPmtStreamLength + streamInfoLength;
        sectionLength -= kPmtStreamLength + streamInfoLength;
        }
      }
    }

private:
  int mSid;
  function <void (int streamSid, int streamPid, int streamType)> mCallback;
  };
//}}}
//{{{
class cTdtParser : public cPidParser {
public:
  //{{{
  cTdtParser (function<void (const string& timeString)> callback)
    : cPidParser (0x14, "tdt"), mCallback(callback) {}
  //}}}
  virtual ~cTdtParser() {}

  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    if (payloadStart) {
      if (ts[0]) {
        //{{{  pointerfield error return
        cLog::log (LOGERROR, mPidName + " unexpected pointerField");
        return;
        }
        //}}}
      ts++;
      tsLeft--;

      int tid = ts[0];
      if (tid == 0x70) {
        int sectionLength = cDvbUtils::getSectionLength (ts+1);
        if (sectionLength >= 8) {
          auto timePoint = system_clock::from_time_t (cDvbUtils::getEpochTime (ts+3) + cDvbUtils::getBcdTime (ts+5));
          string timeString = date::format ("%T", date::floor<seconds>(timePoint));
          mCallback (timeString);
          }
        }
      }
    }

private:
  function <void (const string& timeString)> mCallback;
  };
//}}}

//{{{
class cSectionParser : public cPidParser {
// multiPacket section base class
public:
  //{{{
  cSectionParser (int pid, const string& name) : cPidParser(pid, name), mAllocSectionSize(1024), mSectionSize(0) {
    mSection = (uint8_t*)malloc (mAllocSectionSize);
    }
  //}}}
  //{{{
  virtual ~cSectionParser() {
    free (mSection);
    }
  //}}}

protected:
  //{{{
  void addToSection (uint8_t* buf, int bytesToAdd) {

    if (mSectionSize + bytesToAdd > mAllocSectionSize) {
      // allocate double size of mSection buffer
      mAllocSectionSize *= 2;
      mSection = (uint8_t*)realloc (mSection, mAllocSectionSize);
      cLog::log (LOGINFO1, mPidName + " sdt allocSize doubled to " + dec(mAllocSectionSize));
      }

    memcpy (mSection + mSectionSize, buf, bytesToAdd);
    mSectionSize += bytesToAdd;
    }
  //}}}
  //{{{
  bool haveCompleteSection() {

    if (mSectionSize < mSectionLength) // not enough section
      return false;

    if (cDvbUtils::getCrc32 (mSection, mSectionLength) != 0) {
      // section crc error
      cLog::log (LOGERROR, format ("{} crc sectionSize:{} sectionLength:{}", mPidName, mSectionSize, mSectionLength));
      mSectionSize = 0;
      return false;
      }

    return true;
    }
  //}}}

  int mAllocSectionSize = 0;
  int mSectionSize = 0;
  uint8_t* mSection = nullptr;

  int mSectionLength = 0;
  };
//}}}
//{{{
class cSdtParser : public cSectionParser {
public:
  cSdtParser (function<void (int sid, const string& name)> callback)
    : cSectionParser (0x11, "sdt"), mCallback(callback) {}
  virtual ~cSdtParser() {}

  //{{{
  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    if (payloadStart) {
      //{{{  start section
      mSectionSize = 0;

      int pointerField = ts[0];
      if (pointerField)
        cLog::log (LOGINFO, format ("{} pointerField:{}", mPidName, pointerField));

      ts++;
      tsLeft--;

      if (ts[0] == 0x42) // our sdt
        mSectionLength = cDvbUtils::getSectionLength (ts+1);
      else
        mSectionLength = 0;
      }
      //}}}

    if (mSectionLength > 0) {
      // add ts packet to mSection
      addToSection (ts, tsLeft);

      if (haveCompleteSection()) {
        ts = mSection;
        //{{{  unused sdt header fields
        //int tsid = (ts[3] << 8) + ts[4];
        //int versionNumber = ts[5];
        //int sectionNumber = ts[6];
        //int lastSectionNumber = ts[7];
        //int onid = ((ts[8] & 0x1f) << 8) + ts[9];
        //}}}

        // skip past sdt header
        constexpr int kSdtHeaderLength = 11;
        ts += kSdtHeaderLength;
        mSectionLength -= kSdtHeaderLength + 4;

        // iterate sdt sections
        while (mSectionLength > 0) {
          // sdt descriptor
          int sid = (ts[0] << 8) + ts[1];
          int loopLength = ((ts[3] & 0x0F) << 8) + ts[4];
          //{{{  unused fields
          //bool presentFollowing = ts[2] & 0x01;
          //bool scheduleFlag = ts[2] & 0x02;
          //int runningStatus = ts[3] >> 5;
          //}}}

          // skip past sdt descriptor
          constexpr int kSdtDescriptorLength = 5;
          ts += kSdtDescriptorLength;
          mSectionLength -= kSdtDescriptorLength;

          // iterate descriptors
          int i = 0;
          int descrLength = ts[1] + 2;
          while ((i < loopLength) && (descrLength > 0) && (descrLength <= loopLength - i)) {
            int tag = ts[0];
            //{{{  unused fields
            //int serviceType = ts[2];
            //}}}
            switch (tag) {
              //{{{
              case 0x48: // service
                mCallback (sid, cDvbUtils::getString (ts+4));
                break;
              //}}}
              //{{{
              case 0x5F: // privateData
                cLog::log (LOGINFO1, format ("privateData descriptor len:{}", descrLength));
                break;
              //}}}
              //{{{
              case 0x73: // defaultAuthority
                cLog::log (LOGINFO1, format ("defaultAuthority descriptor len:{}", descrLength));
                break;
              //}}}
              //{{{
              case 0x7e: // futureUse
                cLog::log (LOGINFO1, format ("futureDescriptor len:{}", descrLength));
                break;
              //}}}
              //{{{
              default:
                cLog::log (LOGERROR, format ("unknown descriptor tag:{} len:{}", tag, descrLength));
                break;
              //}}}
              }
            i += descrLength;
            ts += descrLength;
            descrLength = ts[1] + 2;
            }
          mSectionLength -= loopLength;
          }
        }
      mSectionSize = 0;
      }
    }
  //}}}

private:
  function <void (int sid, const string& name)> mCallback;
  };
//}}}
//{{{
class cEitParser : public cSectionParser {
public:
  cEitParser (function<void (int sid, bool now, cDvbEpgItem& epgItem)> callback)
    : cSectionParser (0x12, "eit"), mCallback(callback) {}
  virtual ~cEitParser() {}

  //{{{
  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {

    int pointerField = 0;
    int tsOffsetBytes = 0;
    uint8_t* tsOffset = nullptr;

    if (payloadStart) {
      //{{{  section start, pointerField nonsense
      pointerField = ts[0];
      ts++;
      tsLeft--;

      if (pointerField == 0) {
        // start new section
        mSectionSize = 0;
        mSectionLength = cDvbUtils::getSectionLength (ts+1);
        }
      else {
        // pointerField - save packet offsets for newSection starting after finish of lastSection
        tsOffset = ts + pointerField;
        tsOffsetBytes = tsLeft - pointerField;
        tsLeft = pointerField;
        }
      }
      //}}}

    if (mSectionLength > 0) {
      addToSection (ts, tsLeft);

      if (haveCompleteSection()) {
        ts = mSection;
        string tidInfo;
        int tid = mSection[0];
        //{{{  report EIT tid
        switch (tid) {
          case 0x4E: tidInfo = "Now"; break;
          case 0x4F: tidInfo = "NowOther"; break;
          case 0x50: tidInfo = "Schedule"; break;
          case 0x51: tidInfo = "Schedule1"; break;
          case 0x60: tidInfo = "ScheduleOther"; break;
          case 0x61: tidInfo = "Schedule1Other"; break;
          default: tidInfo = "unknown"; break;
          }
        //}}}
        int sid = (ts[3] << 8) + ts[4];
        //{{{  unused eit header fields
        //int versionNumber = ts[5];
        //int sectionNumber = ts[6];
        //int lastSectionNumber = ts[7];
        //int tsId = (ts[8] << 8) + ts[9];
        //int onId = (ts[10]<< 8) + ts[11];
        //}}}

        // skip past eit header to first eitEvent
        constexpr int kEitHeaderLength = 14;
        ts += kEitHeaderLength;
        mSectionLength -= kEitHeaderLength + 4;

        // iterate eit events
        while (mSectionLength > 0) {
          system_clock::time_point startTime = system_clock::from_time_t (cDvbUtils::getEpochTime (ts+2) + cDvbUtils::getBcdTime (ts+4));
          seconds duration (cDvbUtils::getBcdTime (ts+7));
          int running = (ts[10] & 0xE0) >> 5;
          int loopLength = ((ts[10] & 0x0F) << 8) + ts[11];
          //{{{  unused fields
          //int eventId = (ts[0] << 8) + ts[1];
          //bool caMode = ts[10] & 0x10;
          //}}}

          // skip past eitEvent
          constexpr int kEitEventLength = 12;
          ts += kEitEventLength;
          mSectionLength -= kEitEventLength;

          // iterate eit event descriptors
          int i = 0;
          int descrLength = ts[1] + 2;
          while ((i < loopLength) && (descrLength > 0) && (descrLength <= loopLength - i)) {
            int tag = ts[0];
            switch (tag) {
              //{{{
              case 0x4D: // shortEvent
                {
                bool now = (tid == 0x4E) && (running == 0x04);
                bool epg = (tid == 0x50) || (tid == 0x51);
                if (now || epg) {
                  cDvbEpgItem dvbEpgItem (cDvbUtils::getString (ts+5), startTime, duration);
                  mCallback (sid, now, dvbEpgItem);
                  }
                }
                break;
              //}}}
              //{{{
              case 0x4E: // extendedEvent
                cLog::log (LOGINFO,  format ("{} extendedEvent descriptor tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              case 0x50: // component
                cLog::log (LOGINFO1, format ("{} componentDescriptor tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              case 0x54: // current
                cLog::log (LOGINFO1,  format ("{} currentDescriptor tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              case 0x5F: // defaultAuthority
                cLog::log (LOGINFO1,  format ("{} defaultAuthority tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              case 0x76: // contentId
                cLog::log (LOGINFO1,  format ("{} contentId tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              case 0x7E: // ftaContentMangement
                cLog::log (LOGINFO1,  format ("{} ftaContentMangement tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              case 0x89: // guidance
                cLog::log (LOGINFO1,  format ("{} guidance tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              //{{{
              default:
                cLog::log (LOGERROR, format ("{} unknown eitEvent descriptor tag:{} len:{}", tidInfo, tag, descrLength));
                break;
              //}}}
              }
            i += descrLength;
            ts += descrLength;
            descrLength = ts[1] + 2;
            }
          mSectionLength -= loopLength;
          }
        mSectionSize = 0;
        }
      }

    if (payloadStart && pointerField) {
      //{{{  start new section, copy rest of packet from pointerField offset
      memcpy (mSection, tsOffset, tsOffsetBytes);
      mSectionSize = tsOffsetBytes;
      mSectionLength = ((mSection[1] & 0x0F) << 8) + mSection[2] + 3;
      }
      //}}}
    }
  //}}}

private:
  function <void (int sid, bool now, cDvbEpgItem& epgItem)> mCallback;
  };
//}}}

//{{{
class cPesParser : public cPidParser {
//{{{
class cPesItem {
public:
  //{{{
  cPesItem (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts)
      : mReuseFromFront(reuseFromFront), mPesSize(size), mPts(pts), mDts(dts) {
    mPes = (uint8_t*)malloc (size);
    memcpy (mPes, pes, size);
    }
  //}}}
  //{{{
  ~cPesItem() {
    free (mPes);
    }
  //}}}

  bool mReuseFromFront;
  uint8_t* mPes;
  const int mPesSize;
  const int64_t mPts;
  const int64_t mDts;
  };
//}}}
public:
  //{{{
  cPesParser (int pid, const string& name, bool useQueue) : cPidParser(pid, name), mUseQueue(useQueue) {

    mPes = (uint8_t*)malloc (kInitPesSize);
    if (useQueue)
      thread ([=](){ dequeThread(); }).detach();
    }
  //}}}
  //{{{
  virtual ~cPesParser() {
    free (mPes);
    }
  //}}}

  virtual int getQueueSize() { return (int)mQueue.size_approx(); }
  virtual float getQueueFrac() { return (float)mQueue.size_approx() / mQueue.max_capacity(); }

  //{{{
  virtual void processLast (bool reuseFromFront) {

    if (mPesSize) {
      dispatchDecode (reuseFromFront, mPes, mPesSize, mPts, mDts);
      mPesSize = 0;
      }
    }
  //}}}
  //{{{
  virtual void exit() {

    if (mUseQueue) {
      mQueueExit = true;
      //while (mQueueRunning)
      this_thread::sleep_for (100ms);
      }
    }
  //}}}

protected:
  //{{{
  virtual void processPayload (uint8_t* ts, int tsLeft, bool payloadStart, int continuityCount, bool reuseFromFront) {
  // ts[0],ts[1],ts[2],ts[3] = stream id 0x000001xx
  // ts[4],ts[5] = packetLength, 0 for video
  // ts[6] = 0x80 marker, 0x04 = dataAlignmentIndicator
  // ts[7] = 0x80 = pts, 0x40 = dts
  // ts[8] = optional header length

    if ((mContinuityCount >= 0) &&
        (continuityCount != ((mContinuityCount + 1) & 0xF))) {
      // !!! should abandon pes !!!!
      cLog::log (LOGERROR, "continuity count error pid:%d %d %d", mPid, continuityCount, mContinuityCount);
      }
    mContinuityCount = continuityCount;

    if (payloadStart) {
      bool dataAlignmentIndicator = (ts[6] & 0x84) == 0x84;
      if (dataAlignmentIndicator) {
        processLast (reuseFromFront);

        // get pts dts for next pes
        if (ts[7] & 0x80)
          mPts = cDvbUtils::getPts (ts+9);
        if (ts[7] & 0x40)
          mDts = cDvbUtils::getPts (ts+14);
        }

      int headerSize = 9 + ts[8];
      ts += headerSize;
      tsLeft -= headerSize;
      }

    if (mPesSize + tsLeft > mAllocSize) {
      mAllocSize *= 2;
      mPes = (uint8_t*)realloc (mPes, mAllocSize);
      cLog::log (LOGINFO1, mPidName + " pes allocSize doubled to " + dec(mAllocSize));
      }

    memcpy (mPes + mPesSize, ts, tsLeft);
    mPesSize += tsLeft;
    }
  //}}}

  virtual void decode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) = 0;
  //{{{
  void dispatchDecode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) {

    if (mUseQueue)
      mQueue.enqueue (new cPesParser::cPesItem (reuseFromFront, pes, size, pts, dts));
    else
      decode (reuseFromFront, pes, size, pts, dts);
    }
  //}}}
  //{{{
  void dequeThread() {

    cLog::setThreadName (mPidName + "Q");

    mQueueRunning = true;

    while (!mQueueExit) {
      cPesItem* pesItem;
      if (mQueue.wait_dequeue_timed (pesItem, 40000)) {
        decode (pesItem->mReuseFromFront, pesItem->mPes, pesItem->mPesSize, pesItem->mPts, pesItem->mDts);
        delete pesItem;
        }
      }

    // !!! not sure this is empty the queue on exit !!!!

    mQueueRunning = false;
    }
  //}}}

  int mAllocSize = kInitPesSize;

  uint8_t* mPes;
  int mPesSize = 0;
  int64_t mPts = 0;
  int64_t mDts = 0;
  int mContinuityCount = -1;

private:
  static constexpr int kInitPesSize = 4096;

  bool mUseQueue = true;
  bool mQueueExit = false;
  bool mQueueRunning = false;

  readerWriterQueue::cBlockingReaderWriterQueue <cPesItem*> mQueue;
  };
//}}}
//{{{
class cAudioPesParser : public cPesParser {
public:
  //{{{
  cAudioPesParser (int pid, iAudioDecoder* audioDecoder, bool useQueue,
                   function <void (bool reuseFromFront, float* samples, int64_t pts)> callback)
      : cPesParser(pid, "aud", useQueue), mAudioDecoder(audioDecoder), mCallback(callback) {
    }
  //}}}
  virtual ~cAudioPesParser() {}

protected:
  virtual void decode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) {
  // decode pes to audio frames

    uint8_t* framePes = pes;
    int frameSize;
    while (cAudioParser::parseFrame (framePes, pes + size, frameSize)) {
      // decode a single frame from pes
      float* samples = mAudioDecoder->decodeFrame (framePes, frameSize, pts);
      if (samples) {
        mCallback (reuseFromFront, samples, pts);
        // pts of next frame in pes, assumes 90kz pts, 48khz sample rate
        pts += (mAudioDecoder->getNumSamplesPerFrame() * 90) / 48;
        }
      else
        cLog::log (LOGERROR, "cAudioPesParser decode failed %d %d", size, pts);

      // point to next frame in pes
      framePes += frameSize;
      }
    }

private:
  iAudioDecoder* mAudioDecoder;
  function <void (bool reuseFromFront, float* samples, int64_t pts)> mCallback;
  };
//}}}
//{{{
class cVideoPesParser : public cPesParser {
public:
  //{{{
  cVideoPesParser (int pid, iVideoPool* videoPool, bool useQueue)
    : cPesParser (pid, "vid", useQueue), mVideoPool(videoPool) {}
  //}}}
  virtual ~cVideoPesParser() {}

protected:
  void decode (bool reuseFromFront, uint8_t* pes, int size, int64_t pts, int64_t dts) {
    mVideoPool->decodeFrame (reuseFromFront, pes, size, pts, dts);
    }

private:
  iVideoPool* mVideoPool;
  };
//}}}

// cLoadSource
//{{{
class cLoadSource : public iLoad {
public:
  cLoadSource (const string& name) : mName(name) {}
  virtual ~cLoadSource() {}

  // iLoad gets
  virtual cSong* getSong() = 0;
  virtual iVideoPool* getVideoPool() = 0;
  virtual string getInfoString() { return ""; }
  //{{{
  virtual float getFracs (float& audioFrac, float& videoFrac) {
  // return fracs for spinner graphic, true if ok to display

    audioFrac = 0.f;
    videoFrac = 0.f;
    return 0.f;
    }
  //}}}

  // iLoad actions
  //{{{
  virtual bool togglePlaying() {
    getSong()->togglePlaying();
    return true;
    }
  //}}}
  //{{{
  virtual bool skipBegin() {
    getSong()->setPlayFirstFrame();
    return true;
    }
  //}}}
  //{{{
  virtual bool skipEnd() {
    getSong()->setPlayLastFrame();
    return true;
    }
  //}}}
  //{{{
  virtual bool skipBack (bool shift, bool control) {
    getSong()->incPlaySec (shift ? -300 : control ? -10 : -1, true);
    return true;
    }
  //}}}
  //{{{
  virtual bool skipForward (bool shift, bool control) {
    getSong()->incPlaySec (shift ? 300 : control ? 10 : 1, true);
    return true;
    };
  //}}}
  //{{{
  virtual void exit() {

    if (mSongPlayer)
       mSongPlayer->exit();

    mExit = true;
    while (mRunning) {
      this_thread::sleep_for (100ms);
      cLog::log (LOGINFO, mName + " - waiting to exit");
      }
    }
  //}}}

  // load
  virtual bool recognise (const vector<string>& params) = 0;
  virtual void load() = 0;

protected:
  //{{{
  static iAudioDecoder* createAudioDecoder (eAudioFrameType frameType) {

    switch (frameType) {
      case eAudioFrameType::eMp3:
        cLog::log (LOGINFO, "createAudioDecoder ffmpeg mp3");
        return new cFFmpegAudioDecoder (frameType);

      case eAudioFrameType::eAacAdts:
        cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacAdts");
        return new cFFmpegAudioDecoder (frameType);

      case eAudioFrameType::eAacLatm:
        cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacLatm");
        return new cFFmpegAudioDecoder (frameType);

      default:
        cLog::log (LOGERROR, "createAudioDecoder frameType:%d", frameType);
      }

    return nullptr;
    }
  //}}}

  string mName;
  bool mExit = false;
  bool mRunning = false;

  eAudioFrameType mAudioFrameType = eAudioFrameType::eUnknown;
  int mNumChannels = 0;
  int mSampleRate = 0;

  float mLoadFrac = 0.f;
  cSongPlayer* mSongPlayer = nullptr;
  };
//}}}

//{{{
class cLoadIdle : public cLoadSource {
public:
  cLoadIdle() : cLoadSource("idle") {}
  virtual ~cLoadIdle() {}

  virtual cSong* getSong() { return nullptr; }
  virtual iVideoPool* getVideoPool() { return nullptr; }

  virtual bool recognise (const vector<string>& params) { return true; }
  virtual void load() {}
  virtual void exit() {}

  virtual bool togglePlaying() { return false; }
  virtual bool skipBegin() { return false; }
  virtual bool skipEnd() { return false; }
  virtual bool skipBack (bool shift, bool control) { return false; }
  virtual bool skipForward (bool shift, bool control) { return false; };
  };
//}}}
//{{{
class cLoadDvb : public cLoadSource {
public:
  //{{{
  cLoadDvb() : cLoadSource("dvb") {
    mNumChannels = 2;
    mSampleRate = 48000;
    }
  //}}}
  virtual ~cLoadDvb() {}

  virtual cSong* getSong() { return mPtsSong; }
  virtual iVideoPool* getVideoPool() { return mVideoPool; }
  //{{{
  virtual string getInfoString() {
  // return sizes

    int audioQueueSize = 0;
    int videoQueueSize = 0;

    if (mCurSid > 0) {
      cDvbService* service = mServices[mCurSid];
      if (service) {
        auto audioIt = mPidParsers.find (service->getAudioPid());
        if (audioIt != mPidParsers.end())
          audioQueueSize = (*audioIt).second->getQueueSize();

        auto videoIt = mPidParsers.find (service->getVideoPid());
        if (videoIt != mPidParsers.end())
          videoQueueSize = (*videoIt).second->getQueueSize();
        }
      }

    return format ("sid:{} aq:{} vq:{}", mCurSid, audioQueueSize, videoQueueSize);
    }
  //}}}
  //{{{
  virtual float getFracs (float& audioFrac, float& videoFrac) {
  // return fracs for spinner graphic, true if ok to display

    audioFrac = 0.f;
    videoFrac = 0.f;

    if (mCurSid > 0) {
      cDvbService* service = mServices[mCurSid];
      int audioPid = service->getAudioPid();
      int videoPid = service->getVideoPid();

      auto audioIt = mPidParsers.find (audioPid);
      if (audioIt != mPidParsers.end())
        audioFrac = (*audioIt).second->getQueueFrac();

      auto videoIt = mPidParsers.find (videoPid);
      if (videoIt != mPidParsers.end())
        videoFrac = (*videoIt).second->getQueueFrac();
      }

    return mLoadFrac;
    }
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {

    if (params[0] != "dvb")
      return false;

    mFrequency = 626;
    mMultiplexName =  params[0];

    return true;
    }
  //}}}
  //{{{
  virtual void load() {

    mExit = false;
    mRunning = true;
    mLoadFrac = 0.f;

    cLog::log (LOGINFO, "cDvbSource %d", mFrequency);
    auto dvb = new cDvb (mFrequency);

    mPtsSong = new cPtsSong (eAudioFrameType::eAacAdts, mNumChannels, mSampleRate, 1024, 1920, 0);
    iAudioDecoder* audioDecoder = nullptr;

    bool waitForPts = false;
    int64_t loadPts = -1;

    // init parsers, callbacks
    //{{{
    auto sdtCallback = [&](int sid, const string& name) noexcept {
      auto it = mServices.find (sid);
      if (it != mServices.end()) {
        cDvbService* service = (*it).second;
        if (service->setName (name))
          cLog::log (LOGINFO, format ("SDT name changed sid {} {}", sid, name));
        };
      };
    //}}}
    //{{{
    auto audioFrameCallback = [&](bool reuseFromFront, float* samples, int64_t pts) noexcept {
      mPtsSong->addFrame (reuseFromFront, pts, samples, mPtsSong->getNumFrames()+1);

      if (loadPts < 0)
        // firstTime, setBasePts, sets playPts
        mPtsSong->setBasePts (pts);

      // maybe wait for several frames ???
      if (!mSongPlayer)
        mSongPlayer = new cSongPlayer (mPtsSong, true);

      if (waitForPts) {
        // firstTime since skip, setPlayPts
        mPtsSong->setPlayPts (pts);
        waitForPts = false;
        cLog::log (LOGINFO, "resync pts:" + getPtsFramesString (pts, mPtsSong->getFramePtsDuration()));
        }
      loadPts = pts;
      };
    //}}}
    //{{{
    auto streamCallback = [&](int sid, int pid, int type) noexcept {
      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new stream pid
        auto it = mServices.find (sid);
        if (it != mServices.end()) {
          cDvbService* service = (*it).second;
          switch (type) {
            //{{{
            case 15: // aacAdts
              service->setAudioPid (pid);

              if (service->isSelected()) {
                audioDecoder = createAudioDecoder (eAudioFrameType::eAacAdts);
                mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
                  new cAudioPesParser (pid, audioDecoder, true, audioFrameCallback)));
                }

              break;
            //}}}
            //{{{
            case 17: // aacLatm
              service->setAudioPid (pid);

              if (service->isSelected()) {
                audioDecoder = createAudioDecoder (eAudioFrameType::eAacLatm);
                mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
                  new cAudioPesParser (pid, audioDecoder, true, audioFrameCallback)));
                }

              break;
            //}}}
            //{{{
            case 27: // h264video
              service->setVideoPid (pid);

              if (service->isSelected()) {
                mVideoPool = iVideoPool::create (true, 100, mPtsSong);
                mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cVideoPesParser (pid, mVideoPool, true)));
                }

              break;
            //}}}
            //{{{
            case 6:  // do nothing - subtitle
              //cLog::log (LOGINFO, "subtitle %d %d", pid, type);
              break;
            //}}}
            //{{{
            case 2:  // do nothing - ISO 13818-2 video
              //cLog::log (LOGERROR, "mpeg2 video %d", pid, type);
              break;
            //}}}
            //{{{
            case 3:  // do nothing - ISO 11172-3 audio
              //cLog::log (LOGINFO, "mp2 audio %d %d", pid, type);
              break;
            //}}}
            //{{{
            case 5:  // do nothing - private mpeg2 tabled data
              break;
            //}}}
            //{{{
            case 11: // do nothing - dsm cc u_n
              break;
            //}}}
            default:
              cLog::log (LOGERROR, "loadTs - unrecognised stream type %d %d", pid, type);
            }
          }
        else
          cLog::log (LOGERROR, "loadTs - PMT:%d for unrecognised sid:%d", pid, sid);
        }
      };
    //}}}
    //{{{
    auto programCallback = [&](int pid, int sid) noexcept {
      if ((sid > 0) && (mPidParsers.find (pid) == mPidParsers.end())) {
        cLog::log (LOGINFO, "PAT adding pid:service %d::%d", pid, sid);
        mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cPmtParser (pid, sid, streamCallback)));

        // select first service in PAT
        mServices.insert (map<int,cDvbService*>::value_type (sid, new cDvbService (sid, mCurSid == -1)));
        if (mCurSid == -1)
          mCurSid = sid;
        }
      };
    //}}}
    //{{{
    auto tdtCallback = [&](const string& timeString) noexcept {
      mTimeString = timeString;
      };
    //}}}
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x00, new cPatParser (programCallback)));
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x11, new cSdtParser (sdtCallback)));
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x14, new cTdtParser (tdtCallback)));

    uint8_t* buffer = (uint8_t*)malloc (1024 * 188);
    do {
      int blockSize = 1024 * 188;
      int bytesLeft = dvb->getBlock (buffer, blockSize);

      // process tsBlock
      uint8_t* ts = buffer;
      while (!mExit && (bytesLeft >= 188) && (ts[0] == 0x47)) {
        auto it = mPidParsers.find (((ts[1] & 0x1F) << 8) | ts[2]);
        if (it != mPidParsers.end())
          it->second->parse (ts, true);
        ts += 188;
        bytesLeft -= 188;
        }

      } while (!mExit);

    free (buffer);
    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    for (auto parser : mPidParsers) {
      //{{{  stop and delete pidParsers
      parser.second->exit();
      delete parser.second;
      }
      //}}}
    mPidParsers.clear();

    auto tempSong =  mPtsSong;
    mPtsSong = nullptr;
    delete tempSong;

    auto tempVideoPool = mVideoPool;
    mVideoPool = nullptr;
    delete tempVideoPool;

    delete audioDecoder;
    //}}}
    mRunning = false;
    }
  //}}}

private:
  static constexpr int kFileChunkSize = 16 * 188;

  string mTimeString;

  int mFrequency = 0;
  string mMultiplexName;
  map <int, cPidParser*> mPidParsers;

  cPtsSong* mPtsSong = nullptr;
  iVideoPool* mVideoPool = nullptr;

  int mCurSid = -1;
  map <int, cDvbService*> mServices;
  };
//}}}
//{{{
class cLoadIcyCast : public cLoadSource {
public:
  cLoadIcyCast() : cLoadSource("icyCast") {}
  virtual ~cLoadIcyCast() {}

  virtual cSong* getSong() { return mSong; }
  virtual iVideoPool* getVideoPool() { return mVideoPool; }
  //{{{
  virtual string getInfoString() {
    return format ("{} - {}", mUrl, mLastTitleString);
    }
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {

    mUrl = params[0];
    mParsedUrl.parse (mUrl);
    return mParsedUrl.getScheme() == "http";
    }
  //}}}
  //{{{
  virtual void load() {

    iAudioDecoder* audioDecoder = nullptr;

    mExit = false;
    mRunning = true;
    while (!mExit) {
      int icySkipCount = 0;
      int icySkipLen = 0;
      int icyInfoCount = 0;
      int icyInfoLen = 0;
      char icyInfo[255] = { 0 };

      uint8_t bufferFirst[4096];
      uint8_t* bufferEnd = bufferFirst;
      uint8_t* buffer = bufferFirst;

      int64_t pts = 0;

      cPlatformHttp http;
      http.get (mParsedUrl.getHost(), mParsedUrl.getPath(), "Icy-MetaData: 1",
        //{{{  headerCallback lambda
        [&](const string& key, const string& value) noexcept {
          if (key == "icy-metaint")
            icySkipLen = stoi (value);
          },
        //}}}
        // dataCallback lambda
        [&] (const uint8_t* data, int length) noexcept {
          if ((icyInfoCount >= icyInfoLen) && (icySkipCount + length <= icySkipLen)) {
            //{{{  copy whole body, no metaInfo
            memcpy (bufferEnd, data, length);
            bufferEnd += length;
            icySkipCount += length;

            int sampleRate = 0;
            int numChannels =  0;
            auto frameType = cAudioParser::parseSomeFrames (bufferFirst, bufferEnd, numChannels, sampleRate);
            audioDecoder = createAudioDecoder (frameType);

            int frameSize;
            while (cAudioParser::parseFrame (buffer, bufferEnd, frameSize)) {
              auto samples = audioDecoder->decodeFrame (buffer, frameSize, pts);
              if (samples) {
                if (!mSong)
                  mSong = new cSong (frameType, audioDecoder->getNumChannels(), audioDecoder->getSampleRate(),
                                     audioDecoder->getNumSamplesPerFrame(), 0);

                mSong->addFrame (true, pts, samples,  mSong->getNumFrames()+1);
                pts += mSong->getFramePtsDuration();

                if (!mSongPlayer)
                  mSongPlayer = new cSongPlayer (mSong, true);
                }
              buffer += frameSize;
              }

            if ((buffer > bufferFirst) && (buffer < bufferEnd)) {
              // shuffle down last partial frame
              auto bufferLeft = int(bufferEnd - buffer);
              memcpy (bufferFirst, buffer, bufferLeft);
              bufferEnd = bufferFirst + bufferLeft;
              buffer = bufferFirst;
              }
            }
            //}}}
          else {
            //{{{  copy for metaInfo straddling body
            for (int i = 0; i < length; i++) {
              if (icyInfoCount < icyInfoLen) {
                icyInfo [icyInfoCount] = data[i];
                icyInfoCount++;
                if (icyInfoCount >= icyInfoLen)
                  addIcyInfo (pts, icyInfo);
                }
              else if (icySkipCount >= icySkipLen) {
                icyInfoLen = data[i] * 16;
                icyInfoCount = 0;
                icySkipCount = 0;
                }
              else {
                icySkipCount++;
                *bufferEnd = data[i];
                bufferEnd++;
                }
              }
            }
            //}}}
          return !mExit;
          }
        );
      }

    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    auto tempSong = mSong;
    mSong = nullptr;
    delete tempSong;

    delete audioDecoder;
    //}}}
    mRunning = false;
    }
  //}}}

private:
  //{{{
  void addIcyInfo (int64_t pts, const string& icyInfo) {

    cLog::log (LOGINFO, "addIcyInfo " + icyInfo);

    string icysearchStr = "StreamTitle=\'";
    string searchStr = "StreamTitle=\'";
    auto searchStrPos = icyInfo.find (searchStr);
    if (searchStrPos != string::npos) {
      auto searchEndPos = icyInfo.find ("\';", searchStrPos + searchStr.size());
      if (searchEndPos != string::npos) {
        string titleStr = icyInfo.substr (searchStrPos + searchStr.size(), searchEndPos - searchStrPos - searchStr.size());
        if (titleStr != mLastTitleString) {
          cLog::log (LOGINFO1, "addIcyInfo found title = " + titleStr);
          mSong->getSelect().addMark (pts, titleStr);
          mLastTitleString = titleStr;
          }
        }
      }

    string urlStr = "no url";
    searchStr = "StreamUrl=\'";
    searchStrPos = icyInfo.find (searchStr);
    if (searchStrPos != string::npos) {
      auto searchEndPos = icyInfo.find ('\'', searchStrPos + searchStr.size());
      if (searchEndPos != string::npos) {
        urlStr = icyInfo.substr (searchStrPos + searchStr.size(), searchEndPos - searchStrPos - searchStr.size());
        cLog::log (LOGINFO1, "addIcyInfo found url = " + urlStr);
        }
      }
    }
  //}}}

  string mUrl;
  cUrl mParsedUrl;
  string mLastTitleString;

  cSong* mSong = nullptr;
  iVideoPool* mVideoPool = nullptr;
  };
//}}}

//{{{
class cLoadStream : public cLoadSource {
public:
  cLoadStream (const string& name) : cLoadSource(name) {}
  virtual ~cLoadStream() {}

  virtual cSong* getSong() { return mPtsSong; }
  virtual iVideoPool* getVideoPool() { return mVideoPool; }

  //{{{
  virtual string getInfoString() {

    int audioQueueSize = 0;
    auto audioIt = mPidParsers.find (mAudioPid);
    if (audioIt != mPidParsers.end())
       audioQueueSize = (*audioIt).second->getQueueSize();

    int videoQueueSize = 0;
    auto videoIt = mPidParsers.find (mVideoPid);
    if (videoIt != mPidParsers.end())
      videoQueueSize = (*videoIt).second->getQueueSize();

    return format ("aq:{} vq:{}", audioQueueSize, videoQueueSize);
    }
  //}}}
  //{{{
  virtual float getFracs (float& audioFrac, float& videoFrac) {
  // return fracs for spinner graphic, true if ok to display

    audioFrac = 0.f;
    videoFrac = 0.f;

    auto audioIt = mPidParsers.find (mAudioPid);
    if (audioIt != mPidParsers.end())
      audioFrac = (*audioIt).second->getQueueFrac();

    auto videoIt = mPidParsers.find (mVideoPid);
    if (videoIt != mPidParsers.end())
      videoFrac = (*videoIt).second->getQueueFrac();

    return mLoadFrac;
    }
  //}}}

protected:
  cPtsSong* mPtsSong = nullptr;
  iVideoPool* mVideoPool = nullptr;

  int mLoadSize = 0;
  int mAudioPid = -1;
  int mVideoPid = -1;
  map <int, cPidParser*> mPidParsers;
  };
//}}}
//{{{
class cLoadHls : public cLoadStream {
public:
  //{{{
  cLoadHls() : cLoadStream("hls") {
    mAudioFrameType = eAudioFrameType::eAacAdts;
    mNumChannels = 2;
    mSampleRate = 48000;
    }
  //}}}
  virtual ~cLoadHls() {}

  virtual cSong* getSong() { return mHlsSong; }
  virtual iVideoPool* getVideoPool() { return mVideoPool; }
  //{{{
  virtual string getInfoString() {

    int audioQueueSize = 0;
    auto audioIt = mPidParsers.find (mAudioPid);
    if (audioIt != mPidParsers.end())
       audioQueueSize = (*audioIt).second->getQueueSize();

    int videoQueueSize = 0;
    if (!mRadio && mVideoRate) {
      auto videoIt = mPidParsers.find (mVideoPid);
      if (videoIt != mPidParsers.end())
        videoQueueSize = (*videoIt).second->getQueueSize();
      }

    return format ("{} {}k aq:{} vq:{}", mChannel, mLoadSize/1000, audioQueueSize, videoQueueSize);
    }
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {
  //  parse params to recognise load

    for (auto& param : params) {
      if (param == "bbc1") mChannel = "bbc_one_hd";
      else if (param == "bbc2") mChannel = "bbc_two_hd";
      else if (param == "bbc4") mChannel = "bbc_four_hd";
      else if (param == "news") mChannel = "bbc_news_channel_hd";
      else if (param == "scot") mChannel = "bbc_one_scotland_hd";
      else if (param == "s4c")  mChannel = "s4cpbs";
      else if (param == "sw")   mChannel = "bbc_one_south_west";
      else if (param == "parl") mChannel = "bbc_parliament";

      else if (param == "r1") { mRadio = true; mChannel = "bbc_radio_one";  }
      else if (param == "r2") { mRadio = true; mChannel = "bbc_radio_two";  }
      else if (param == "r3") { mRadio = true; mChannel = "bbc_radio_three"; }
      else if (param == "r4") { mRadio = true; mChannel = "bbc_radio_fourfm"; }
      else if (param == "r5") { mRadio = true; mChannel = "bbc_radio_five_live";  }
      else if (param == "r6") { mRadio = true; mChannel = "bbc_6music"; }

      else if (param == "mfx") mFfmpeg = false;

      else if (param == "v0") mVideoRate = 0;
      else if (param == "v1") mVideoRate = 827008;
      else if (param == "v2") mVideoRate = 1604032;
      else if (param == "v3") mVideoRate = 2812032;
      else if (param == "v4") mVideoRate = 5070016;

      else if (param == "a48")  mAudioRate = 48000;
      else if (param == "a96")  mAudioRate = 96000;
      else if (param == "a128") mAudioRate = 128000;
      else if (param == "a320") mAudioRate = 320000;
      }

    if (mChannel.empty()) // no channel found
      return false;

    mLowAudioRate = mAudioRate < 128000;
    mSamplesPerFrame = mLowAudioRate ? 2048 : 1024;
    mPtsDurationPerFrame = mLowAudioRate ? 3840 : 1920;
    mFramesPerChunk = mLowAudioRate ? (mRadio ? 150 : 180) : (mRadio ? 300 : 360);

    string pathFormat;
    if (mRadio) {
      mHost = "as-hls-uk-live.bbcfmt.s.llnwi.net";
      pathFormat = "pool_904/live/uk/{0}/{0}.isml/{0}-audio={1}";
      }
    else {
      mHost = "vs-hls-uk-live.akamaized.net";
      pathFormat = format ("pool_902/live/uk/{{0}}/{{0}}.isml/{{0}}-pa{0}={{1}}{1}",
                           mLowAudioRate ? 3 : 4, mVideoRate ? "-video={2}" : "");
      }
    mM3u8PathFormat = pathFormat + ".norewind.m3u8";
    mTsPathFormat = pathFormat + "-{3}.ts";

    return true;
    }
  //}}}
  //{{{
  virtual void load() {

    mExit = false;
    mRunning = true;
    mHlsSong = new cHlsSong (eAudioFrameType::eAacAdts, mNumChannels, mSampleRate,
                             mSamplesPerFrame, mPtsDurationPerFrame,
                             mRadio ? 0 : 1000, mFramesPerChunk);

    iAudioDecoder* audioDecoder = nullptr;

    // add parsers, callbacks
    //{{{
    auto audioFrameCallback = [&](bool reuseFromFront, float* samples, int64_t pts) noexcept {
      mHlsSong->addFrame (reuseFromFront, pts, samples, mHlsSong->getNumFrames()+1);
      if (!mSongPlayer)
        mSongPlayer = new cSongPlayer (mHlsSong, true);
      };
    //}}}
    //{{{
    auto streamCallback = [&](int sid, int pid, int type) noexcept {
      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new stream, add stream parser
        switch (type) {
          case 15: // aacAdts
            mAudioPid  = pid;
            audioDecoder = createAudioDecoder (mAudioFrameType);
            mPidParsers.insert (
              map<int,cPidParser*>::value_type (pid,
                new cAudioPesParser (pid, audioDecoder, true, audioFrameCallback)));
            break;

          case 27: // h264video
            if (mVideoRate) {
              mVideoPid  = pid;
              mVideoPool = iVideoPool::create (mFfmpeg, 192, mHlsSong);
              mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cVideoPesParser (pid, mVideoPool, true)));
              }
            break;

          default:
            cLog::log (LOGERROR, "hls - unrecognised stream pid:type %d:%d", pid, type);
          }
        }
      };
    //}}}
    //{{{
    auto programCallback = [&](int pid, int sid) noexcept {
      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new PMT, add parser and new service
        mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cPmtParser (pid, sid, streamCallback)));
        }
      };
    //}}}

    // add PAT parser
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x00, new cPatParser (programCallback)));

    while (!mExit) {
      cPlatformHttp http;

      // get m3u8 file
      string m3u8Path = format (mM3u8PathFormat, mChannel, mAudioRate, mVideoRate);
      mHost = http.getRedirect (mHost, m3u8Path);
      if (http.getContent()) {
        //{{{  parse m3u8 file
        // get value for tag #USP-X-TIMESTAMP-MAP:MPEGTS=
        int64_t mpegTimestamp = stoll (getTagValue (http.getContent(), "#USP-X-TIMESTAMP-MAP:MPEGTS=", ','));

        // get value for tag #EXT-X-PROGRAM-DATE-TIME:
        istringstream inputStream (getTagValue (http.getContent(), "#EXT-X-PROGRAM-DATE-TIME:", '\n'));
        system_clock::time_point extXProgramDateTimePoint;
        inputStream >> date::parse ("%FT%T", extXProgramDateTimePoint);

        // get value for tag #EXT-X-MEDIA-SEQUENCE:
        int extXMediaSequence = stoi (getTagValue (http.getContent(), "#EXT-X-MEDIA-SEQUENCE:", '\n'));

        // 37s is the magic number of seconds that extXProgramDateTimePoint is out from clockTime
        mHlsSong->setBaseHls (mpegTimestamp, extXProgramDateTimePoint, -37s, extXMediaSequence);
        http.freeContent();
        //}}}

        while (!mExit) {
          int64_t loadPts;
          bool reuseFromFront;
          int chunkNum = mHlsSong->getLoadChunkNum (loadPts, reuseFromFront);
          if (chunkNum > 0) {
            // get chunkNum ts file
            int contentParsed = 0;
            string tsPath = format (mTsPathFormat, mChannel, mAudioRate, mVideoRate, chunkNum);
            if (http.get (mHost, tsPath, "",
                          [&](const string& key, const string& value) noexcept {
                            //{{{  header callback lambda
                            if (key == "content-length")
                              cLog::log (LOGINFO1, format ("chunk:{} pts:{} size:{}k",
                                         chunkNum,
                                         getPtsFramesString (loadPts, mHlsSong->getFramePtsDuration()),
                                         http.getHeaderContentSize()/1000));
                            },
                            //}}}
                          [&](const uint8_t* data, int length) noexcept {
                            //{{{  data callback lambda
                            mLoadSize = http.getContentSize();
                            mLoadFrac = float(http.getContentSize()) / http.getHeaderContentSize();

                            // parse ts packets as we receive them
                            while (http.getContentSize() - contentParsed >= 188) {
                              uint8_t* ts = http.getContent() + contentParsed;
                              if (ts[0] == 0x47) {
                                auto it = mPidParsers.find (((ts[1] & 0x1F) << 8) | ts[2]);
                                if (it != mPidParsers.end())
                                  it->second->parse (ts, reuseFromFront);
                                ts += 188;
                                }
                              else
                                cLog::log (LOGERROR, "ts packet sync:%d", contentParsed);
                              contentParsed += 188;
                              }

                            return true;
                            }
                            //}}}
                          ) == 200) {
              for (auto parser : mPidParsers)
                parser.second->processLast (reuseFromFront);
              http.freeContent();
              }
            else {
              //{{{  failed to load chunk, backoff for 250ms
              cLog::log (LOGERROR, "late " + dec(chunkNum));
              this_thread::sleep_for (250ms);
              }
              //}}}
            }
          else // no chunk available yet, backoff for 100ms
            this_thread::sleep_for (100ms);
          }
        }
      }

    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    auto tempVideoPool = mVideoPool;
    mVideoPool = nullptr;
    delete tempVideoPool;

    for (auto parser : mPidParsers) {
      //{{{  stop and delete pidParsers
      parser.second->exit();
      delete parser.second;
      }
      //}}}
    mPidParsers.clear();

    auto tempSong = mHlsSong;
    mHlsSong = nullptr;
    delete tempSong;

    delete audioDecoder;
    //}}}
    mRunning = false;
    }
  //}}}

private:
  //{{{
  static string getTagValue (uint8_t* buffer, const char* tag, char terminator) {
  // crappy get value from tag

    const char* tagPtr = strstr ((const char*)buffer, tag);
    const char* valuePtr = tagPtr + strlen (tag);
    const char* endPtr = strchr (valuePtr, terminator);

    return string (valuePtr, endPtr - valuePtr);
    }
  //}}}

  // params
  bool mRadio = false;
  string mChannel;
  int mVideoRate = 827008;
  int mAudioRate = 128000;
  bool mFfmpeg = true;

  // http
  string mHost;
  string mM3u8PathFormat;
  string mTsPathFormat;

  // song params
  int mLowAudioRate = false;
  int mFramesPerChunk = 0;
  int mSamplesPerFrame = 0;
  int64_t mPtsDurationPerFrame = 0;

  cHlsSong* mHlsSong = nullptr;
  };
//}}}
//{{{
class cLoadRtp : public cLoadStream {
public:
  //{{{
  cLoadRtp() : cLoadStream("rtp") {

    mNumChannels = 2;
    mSampleRate = 48000;
    }
  //}}}
  //{{{
  virtual ~cLoadRtp() {

    for (auto& epgItem : mEpgItemMap)
      delete epgItem.second;

    mEpgItemMap.clear();
    }
  //}}}

  //{{{
  virtual string getInfoString() {
    return format ("sid:{} {} {} {} {} {}",
                   mSid, mServiceName,
                   mTimeString,
                   date::format ("%H:%M", floor<seconds>(mNowEpgItem.getStartTime())),
                   mNowEpgItem.getProgramName(), cLoadStream::getInfoString());
    }
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {

    if (params[0] != "rtp")
      return false;

    mNumChannels = 2;
    mSampleRate = 48000;

    return true;
    }
  //}}}
  //{{{
  virtual void load() {

    mExit = false;
    mRunning = true;
    mLoadFrac = 0.f;

    //{{{  wsa startup
    struct sockaddr_in sendAddr;

    #ifdef _WIN32
      WSADATA wsaData;
      WSAStartup (MAKEWORD(2, 2), &wsaData);
      int sendAddrSize = sizeof (sendAddr);
    #endif

    #ifdef __linux__
      unsigned int sendAddrSize = sizeof (sendAddr);
    #endif
    //}}}
    // create socket to receive datagrams
    auto rtpReceiveSocket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (rtpReceiveSocket == 0) {
      //{{{  error return
      cLog::log (LOGERROR, "socket create failed");
      return;
      }
      //}}}

    // allow multiple sockets to use the same PORT number, no error check?
    char yes = 1;
    setsockopt (rtpReceiveSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

    // bind the socket to anyAddress:specifiedPort
    struct sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    recvAddr.sin_port = htons (mPort);
    if (::bind (rtpReceiveSocket, (struct sockaddr*)&recvAddr, sizeof(recvAddr)) != 0) {
      //{{{  error return
      cLog::log (LOGERROR, "socket bind failed");
      return;
      }
      //}}}

    // request to join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr (mMulticastAddress.c_str());
    mreq.imr_interface.s_addr = htonl (INADDR_ANY);
    if (setsockopt (rtpReceiveSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) != 0) {
      //{{{  error return
      cLog::log (LOGERROR, "socket setsockopt IP_ADD_MEMBERSHIP failed");
      return;
      }
      //}}}

    mPtsSong = new cPtsSong (eAudioFrameType::eAacAdts, mNumChannels, mSampleRate, 1024, 1920, 0);
    iAudioDecoder* audioDecoder = nullptr;

    int64_t loadPts = -1;

    // init parsers, callbacks
    //{{{
    auto audioFrameCallback = [&](bool reuseFromFront, float* samples, int64_t pts) noexcept {

      mPtsSong->addFrame (reuseFromFront, pts, samples, mPtsSong->getNumFrames()+1);

      if (loadPts < 0)
        // firstTime, setBasePts, sets playPts
        mPtsSong->setBasePts (pts);
      loadPts = pts;

      // maybe wait for several frames ???
      if (!mSongPlayer)
        mSongPlayer = new cSongPlayer (mPtsSong, true);
      };
    //}}}
    //{{{
    auto streamCallback = [&](int sid, int pid, int type) noexcept {

      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new stream pid
        switch (type) {
          case  2: // ISO 13818-2 video
          case  3: // ISO 11172-3 audio
          case  5: // private mpeg2 tabled data
          case  6: // subtitle
          case 11: // dsm cc u_n
            break;
          //{{{
          case 17: // aacLatm
            mAudioPid = pid;

            audioDecoder = createAudioDecoder (eAudioFrameType::eAacLatm);
            mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
              new cAudioPesParser (pid, audioDecoder, true, audioFrameCallback)));

            break;
          //}}}
          //{{{
          case 27: // h264video
            mVideoPid = pid;

            mVideoPool = iVideoPool::create (true, 100, mPtsSong);
            mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cVideoPesParser (pid, mVideoPool, true)));

            break;
          //}}}
          default:
            cLog::log (LOGERROR, "loadTs - unrecognised stream type %d %d", pid, type);
          }
        }
      };
    //}}}
    //{{{
    auto programCallback = [&](int pid, int sid) noexcept {

      if ((sid > 0) && (mPidParsers.find (pid) == mPidParsers.end())) {
        cLog::log (LOGINFO, "PAT adding pid:service %d::%d", pid, sid);
        mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cPmtParser (pid, sid, streamCallback)));
        }
      };
    //}}}
    //{{{
    auto sdtCallback = [&](int sid, const string& name) noexcept {
      mSid = sid;
      mServiceName = name;
      };
    //}}}
    //{{{
    auto eitCallback = [&](int sid, bool now, cDvbEpgItem& epgItem) noexcept {

      //cLog::log (LOGINFO, format ("eit callback sid {} {}", sid, name));
      //mSid = sid;
      if (now) // copy to now
        mNowEpgItem = epgItem;

      else {
        // add to epg if new, later than now today
        auto todayTime = system_clock::now();
        auto todayDatePoint = date::floor<date::days>(todayTime);
        auto todayYearMonthDay = date::year_month_day{todayDatePoint};
        auto today = todayYearMonthDay.day();

        auto datePoint = date::floor<date::days>(epgItem.getStartTime());
        auto yearMonthDay = date::year_month_day{datePoint};
        auto day = yearMonthDay.day();

        if ((day == today) && (epgItem.getStartTime() > todayTime)) {
          // later today
          auto epgItemIt = mEpgItemMap.find (epgItem.getStartTime());
          if (epgItemIt == mEpgItemMap.end()) {
            mEpgItemMap.insert (
              map<system_clock::time_point, cDvbEpgItem*>::value_type (epgItem.getStartTime(), new cDvbEpgItem (epgItem)));
            cLog::log (LOGINFO, format ("epg {} {}",
                                date::format ("%H:%M", floor<seconds>(epgItem.getStartTime())),
                                epgItem.getProgramName()));
            }
          }
        }
      };
    //}}}
    //{{{
    auto tdtCallback = [&](const string& timeString) noexcept {
      mTimeString = timeString;
      };
    //}}}
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x00, new cPatParser (programCallback)));
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x11, new cSdtParser (sdtCallback)));
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x12, new cEitParser (eitCallback)));
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x14, new cTdtParser (tdtCallback)));

    constexpr int kUdpBufferSize = 2048;
    char buffer[kUdpBufferSize];
    int bufferLen = kUdpBufferSize;
    do {
      int bytesReceived = recvfrom (rtpReceiveSocket, buffer, bufferLen, 0, (struct sockaddr*)&sendAddr, &sendAddrSize);
      if (bytesReceived != 0) {
        // process block of ts minus rtp header
        int bytesLeft = bytesReceived - 12;
        uint8_t* ts = (uint8_t*)buffer + 12;
        while (!mExit && (bytesLeft >= 188) && (ts[0] == 0x47)) {
          auto it = mPidParsers.find (((ts[1] & 0x1F) << 8) | ts[2]);
          if (it != mPidParsers.end())
            it->second->parse (ts, true);
          ts += 188;
          bytesLeft -= 188;
          }
        }
      else
        cLog::log (LOGERROR, "recvfrom failed");
      } while (!mExit);

    //{{{  close socket and WSAcleanup
    #ifdef _WIN32
      if (closesocket (rtpReceiveSocket) != 0) {
        cLog::log (LOGERROR, "closesocket failed");
        return;
        }
      WSACleanup();
    #endif

    #ifdef __linux__
      close (rtpReceiveSocket);
    #endif
    //}}}
    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    for (auto parser : mPidParsers) {
      //{{{  stop and delete pidParsers
      parser.second->exit();
      delete parser.second;
      }
      //}}}
    mPidParsers.clear();

    auto tempSong =  mPtsSong;
    mPtsSong = nullptr;
    delete tempSong;

    auto tempVideoPool = mVideoPool;
    mVideoPool = nullptr;
    delete tempVideoPool;

    delete audioDecoder;
    //}}}
    mRunning = false;
    }
  //}}}

private:
  string mMulticastAddress = "239.255.1.3";
  int mPort = 5002;

  int mSid = 0;
  string mServiceName;

  string mTimeString;

  cDvbEpgItem mNowEpgItem;
  map <system_clock::time_point, cDvbEpgItem*> mEpgItemMap;
  };
//}}}

//{{{
class cLoadFile : public cLoadSource {
public:
  cLoadFile (const string& name) : cLoadSource(name) {}
  virtual ~cLoadFile() {}

protected:
  //{{{
  int64_t getFileSize (const string& filename) {
  // get fileSize, return 0 if file not found

    mFilename = filename;
    mFileSize = 0;

    #ifdef _WIN32
      struct _stati64 st;
      if (_stat64 (filename.c_str(), &st) == -1)
        return 0;
      else
        mFileSize = st.st_size;
    #endif

    #ifdef __linux__
      struct stat st;
      if (stat (filename.c_str(), &st) == -1)
        return 0;
      else
        mFileSize = st.st_size;
    #endif

    return mFileSize;
    }
  //}}}
  //{{{
  void updateFileSize (const string& filename) {
  // get fileSize, return 0 if file not found

    #ifdef _WIN32
      struct _stati64 st;
      if (_stat64 (filename.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #endif

    #ifdef __linux__
      struct stat st;
      if (stat (filename.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #endif
    }
  //}}}
  //{{{
  eAudioFrameType getAudioFileInfo() {

    uint8_t buffer[1024];
    FILE* file = fopen (mFilename.c_str(), "rb");
    size_t size = fread (buffer, 1, 1024, file);
    fclose (file);

    mAudioFrameType = cAudioParser::parseSomeFrames (buffer, buffer + size, mNumChannels, mSampleRate);
    return mAudioFrameType;
    }
  //}}}

  string mFilename;
  int64_t mFileSize = 0;
  int64_t mStreamPos = 0;
  };
//}}}
//{{{
class cLoadTsFile : public cLoadFile {
public:
  //{{{
  cLoadTsFile() : cLoadFile("ts") {
    mNumChannels = 2;
    mSampleRate = 48000;
    }
  //}}}
  virtual ~cLoadTsFile() {}

  virtual cSong* getSong() { return mPtsSong; }
  virtual iVideoPool* getVideoPool() { return mVideoPool; }
  //{{{
  virtual string getInfoString() {
  // return sizes

    int audioQueueSize = 0;
    int videoQueueSize = 0;

    if (mCurSid > 0) {
      cDvbService* service = mServices[mCurSid];
      if (service) {
        auto audioIt = mPidParsers.find (service->getAudioPid());
        if (audioIt != mPidParsers.end())
          audioQueueSize = (*audioIt).second->getQueueSize();

        auto videoIt = mPidParsers.find (service->getVideoPid());
        if (videoIt != mPidParsers.end())
          videoQueueSize = (*videoIt).second->getQueueSize();
        }
      }

    return format ("{}packets sid:{} aq:{} vq:{}", mStreamPos/188, mCurSid, audioQueueSize, videoQueueSize);
    }
  //}}}
  //{{{
  virtual float getFracs (float& audioFrac, float& videoFrac) {
  // return fracs for spinner graphic, true if ok to display


    audioFrac = 0.f;
    videoFrac = 0.f;

    if (mCurSid > 0) {
      cDvbService* service = mServices[mCurSid];
      int audioPid = service->getAudioPid();
      int videoPid = service->getVideoPid();

      auto audioIt = mPidParsers.find (audioPid);
      if (audioIt != mPidParsers.end())
        audioFrac = (*audioIt).second->getQueueFrac();

      auto videoIt = mPidParsers.find (videoPid);
      if (videoIt != mPidParsers.end())
        videoFrac = (*videoIt).second->getQueueFrac();
      }

    return mLoadFrac;
    }
  //}}}

  //{{{
  virtual bool skipBack (bool shift, bool control) {

    if (mTargetPts == -1)
      mTargetPts = mPtsSong->getPlayPts();

    int64_t secs = shift ? 300 : control ? 10 : 1;
    mTargetPts -= secs * 90000;

    return true;
    };
  //}}}
  //{{{
  virtual bool skipForward (bool shift, bool control) {

    if (mTargetPts == -1)
      mTargetPts = mPtsSong->getPlayPts();

    int64_t secs = shift ? 300 : control ? 10 : 1;
    mTargetPts += secs * 90000;

    return true;
    };
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {

    if (!getFileSize (params[0]))
      return false;

    uint8_t buffer[1024];
    FILE* file = fopen (params[0].c_str(), "rb");
    size_t size = fread (buffer, 1, 1024, file);
    fclose (file);

    return (size > 188) && (buffer[0] == 0x47) && (buffer[188] == 0x47);
    }
  //}}}
  //{{{
  virtual void load() {
  // manage our own file read, block on > 100 frames after playPts, manage kipping
  // - can't mmap because of commmon case of growing ts file size
  // - chunks are ts packet aligned

    mExit = false;
    mRunning = true;

    // get first fileChunk
    FILE* file = fopen (mFilename.c_str(), "rb");
    uint8_t buffer[kFileChunkSize];
    size_t bytesLeft = fread (buffer, 1, kFileChunkSize, file);

    mPtsSong = new cPtsSong (eAudioFrameType::eAacAdts, mNumChannels, mSampleRate, 1024, 1920, 0);
    iAudioDecoder* audioDecoder = nullptr;

    mStreamPos = 0;
    int64_t loadPts = -1;
    bool waitForPts = false;

    // init parsers, callbacks
    //{{{
    auto audioFrameCallback = [&](bool reuseFromFront, float* samples, int64_t pts) noexcept {
      mPtsSong->addFrame (reuseFromFront, pts, samples, mPtsSong->getNumFrames()+1);

      if (loadPts < 0)
        // firstTime, setBasePts, sets playPts
        mPtsSong->setBasePts (pts);

      // maybe wait for several frames ???
      if (!mSongPlayer)
        mSongPlayer = new cSongPlayer (mPtsSong, true);

      if (waitForPts) {
        // firstTime since skip, setPlayPts
        mPtsSong->setPlayPts (pts);
        waitForPts = false;
        cLog::log (LOGINFO, "resync pts:" + getPtsFramesString (pts, mPtsSong->getFramePtsDuration()));
        }
      loadPts = pts;
      };
    //}}}
    //{{{
    auto streamCallback = [&](int sid, int pid, int type) noexcept {
      if (mPidParsers.find (pid) == mPidParsers.end()) {
        // new stream pid
        auto it = mServices.find (sid);
        if (it != mServices.end()) {
          cDvbService* service = (*it).second;
          switch (type) {
            //{{{
            case 15: // aacAdts
              service->setAudioPid (pid);

              if (service->isSelected()) {
                audioDecoder = createAudioDecoder (eAudioFrameType::eAacAdts);
                mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
                  new cAudioPesParser (pid, audioDecoder, true, audioFrameCallback)));
                }

              break;
            //}}}
            //{{{
            case 17: // aacLatm
              service->setAudioPid (pid);

              if (service->isSelected()) {
                audioDecoder = createAudioDecoder (eAudioFrameType::eAacLatm);
                mPidParsers.insert (map<int,cPidParser*>::value_type (pid,
                  new cAudioPesParser (pid, audioDecoder, true, audioFrameCallback)));
                }

              break;
            //}}}
            //{{{
            case 27: // h264video
              service->setVideoPid (pid);

              if (service->isSelected()) {
                mVideoPool = iVideoPool::create (true, 100, mPtsSong);
                mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cVideoPesParser (pid, mVideoPool, true)));
                }

              break;
            //}}}
            //{{{
            case 6:  // do nothing - subtitle
              //cLog::log (LOGINFO, "subtitle %d %d", pid, type);
              break;
            //}}}
            //{{{
            case 2:  // do nothing - ISO 13818-2 video
              //cLog::log (LOGERROR, "mpeg2 video %d", pid, type);
              break;
            //}}}
            //{{{
            case 3:  // do nothing - ISO 11172-3 audio
              //cLog::log (LOGINFO, "mp2 audio %d %d", pid, type);
              break;
            //}}}
            //{{{
            case 5:  // do nothing - private mpeg2 tabled data
              break;
            //}}}
            //{{{
            case 11: // do nothing - dsm cc u_n
              break;
            //}}}
            default:
              cLog::log (LOGERROR, "loadTs - unrecognised stream type %d %d", pid, type);
            }
          }
        else
          cLog::log (LOGERROR, "loadTs - PMT:%d for unrecognised sid:%d", pid, sid);
        }
      };
    //}}}
    //{{{
    auto sdtCallback = [&](int sid, const string& name) noexcept {
      auto it = mServices.find (sid);
      if (it != mServices.end()) {
        cDvbService* service = (*it).second;
        if (service->setName (name))
          cLog::log (LOGINFO, format ("SDT sid {} {}", sid, name));
        };
      };
    //}}}
    //{{{
    auto programCallback = [&](int pid, int sid) noexcept {
      if ((sid > 0) && (mPidParsers.find (pid) == mPidParsers.end())) {
        cLog::log (LOGINFO, "PAT adding pid:service %d::%d", pid, sid);
        mPidParsers.insert (map<int,cPidParser*>::value_type (pid, new cPmtParser (pid, sid, streamCallback)));

        // select first service in PAT
        mServices.insert (map<int,cDvbService*>::value_type (sid, new cDvbService (sid, mCurSid == -1)));
        if (mCurSid == -1)
          mCurSid = sid;
        }
      };
    //}}}
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x00, new cPatParser (programCallback)));
    mPidParsers.insert (map<int,cPidParser*>::value_type (0x11, new cSdtParser (sdtCallback)));

    do {
      // process fileChunk
      uint8_t* ts = buffer;
      while (!mExit && (bytesLeft >= 188) && (ts[0] == 0x47)) {
        auto it = mPidParsers.find (((ts[1] & 0x1F) << 8) | ts[2]);
        if (it != mPidParsers.end())
          it->second->parse (ts, true);
        ts += 188;
        bytesLeft -= 188;

        mStreamPos += 188;
        mLoadFrac = float(mStreamPos) / mFileSize;

        // block load if loadPts > xx audio frames ahead of playPts
        while (!mExit && (mTargetPts == -1) && !waitForPts &&
               (loadPts > mPtsSong->getPlayPts() + (100 * mPtsSong->getFramePtsDuration()))) {
          //cLog::log (LOGINFO, "blocked loadPts:" + getPtsFramesString (loadPts, mPtsSong->getFramePtsDuration()) +
          //                    " playPts:" + getPtsFramesString (mPtsSong->getPlayPts(), mPtsSong->getFramePtsDuration()));
          this_thread::sleep_for (40ms);
          }

        if (mTargetPts > -1) {
          int64_t diffPts = mTargetPts - mPtsSong->getPlayPts();
          cLog::log (LOGINFO, "diffPts:%d", (int)diffPts);
          if (diffPts > 100000) {
            //{{{  skip forward
            mStreamPos += (((diffPts * 50) / 9) / 188) * 188;
            //{{{  fseek
            #ifdef _WIN32
              _fseeki64 (file, mStreamPos, SEEK_SET);
            #endif

            #ifdef __linux__
              fseek (file, mStreamPos, SEEK_SET);
            #endif
            //}}}
            bytesLeft = 0;
            waitForPts = true;
            mVideoPool->flush (mTargetPts);
            }
            //}}}
          else if (diffPts < -100000) {
            //{{{  skip back
            mStreamPos += (((diffPts * 50) / 9) / 188) * 188;
            //{{{  fseek
            #ifdef _WIN32
              _fseeki64 (file, mStreamPos, SEEK_SET);
            #endif

            #ifdef __linux__
              fseek (file, mStreamPos, SEEK_SET);
            #endif
            //}}}
            bytesLeft = 0;
            waitForPts = true;
            mVideoPool->flush (mTargetPts);
            }
            //}}}
          else
            mPtsSong->setPlayPts (mTargetPts);
          mTargetPts = -1;
          }
        }

      // get next fileChunk
      bytesLeft = fread (buffer, 1, kFileChunkSize, file);
      } while (!mExit && (bytesLeft > 188));
    mLoadFrac = 0.f;

    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    for (auto parser : mPidParsers) {
      //{{{  stop and delete pidParsers
      parser.second->exit();
      delete parser.second;
      }
      //}}}
    mPidParsers.clear();

    auto tempSong =  mPtsSong;
    mPtsSong = nullptr;
    delete tempSong;

    auto tempVideoPool = mVideoPool;
    mVideoPool = nullptr;
    delete tempVideoPool;

    delete audioDecoder;
    //}}}
    fclose (file);
    mRunning = false;
    }
  //}}}

private:
  static constexpr int kFileChunkSize = 16 * 188;

  cPtsSong* mPtsSong = nullptr;
  iVideoPool* mVideoPool = nullptr;

  map <int, cPidParser*> mPidParsers;

  int mCurSid = -1;
  map <int, cDvbService*> mServices;

  int64_t mTargetPts = -1;
  };
//}}}
//{{{
class cLoadWavFile : public cLoadFile {
public:
  cLoadWavFile() : cLoadFile("wav") {}
  virtual ~cLoadWavFile() {}

  virtual cSong* getSong() { return mSong; }
  virtual iVideoPool* getVideoPool() { return nullptr; }
  //{{{
  virtual string getInfoString() {
    return format ("{} {}k", mFilename, mStreamPos / 1024);
    }
  //}}}
  //{{{
  virtual float getFracs (float& audioFrac, float& videoFrac) {
  // return fracs for spinner graphic, true if ok to display

    audioFrac = 0.f;
    videoFrac = 0.f;
    return mLoadFrac;
    }
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {

    if (!getFileSize (params[0]))
      return false;

    return getAudioFileInfo() == eAudioFrameType::eWav;
    }
  //}}}
  //{{{
  virtual void load() {
  // wav - ??? could use ffmpeg to decode all the variants ???
  // - preload whole file, could mmap but not really worth it being the exception

    mExit = false;
    mRunning = true;

    // get first fileChunk
    FILE* file = fopen (mFilename.c_str(), "rb");
    uint8_t buffer[kFileChunkSize + 0x100];
    size_t size = fread (buffer, 1, kFileChunkSize, file);
    size_t bytesLeft = size;
    mStreamPos = 0;

    mSong = new cSong (eAudioFrameType::eWav, mNumChannels, mSampleRate, kWavFrameSamples, 0);

    // parse wav header for start of samples
    int frameSize = 0;
    uint8_t* frame = cAudioParser::parseFrame (buffer, buffer + bytesLeft, frameSize);
    bytesLeft = frameSize;

    // pts = frameNum starting at zero
    int64_t pts = 0;
    do {
      // process fileChunk
      while (!mExit &&
             ((kWavFrameSamples * mNumChannels * 4) <= (int)bytesLeft)) {
        // read samples from fileChunk
        float* samples = (float*)malloc (kWavFrameSamples * mNumChannels * 4);
        memcpy (samples, frame, kWavFrameSamples * mNumChannels * 4);
        mSong->addFrame (true, pts, samples, mSong->getNumFrames()+1);
        if (!mSongPlayer)
          mSongPlayer = new cSongPlayer (mSong, false);

        pts += mSong->getFramePtsDuration();
        frame += kWavFrameSamples * mNumChannels * 4;
        bytesLeft -= kWavFrameSamples * mNumChannels * 4;
        }

      // get next fileChunk
      memcpy (buffer, frame, bytesLeft);
      size_t size = fread (buffer + bytesLeft, 1, kFileChunkSize-bytesLeft, file);
      bytesLeft += size;
      mStreamPos += (int)size;
      mLoadFrac = float(mStreamPos) / mFileSize;

      frame = buffer;
      } while (!mExit && (bytesLeft > 0));
    mLoadFrac = 0.f;

    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    auto tempSong = mSong;
    mSong = nullptr;
    delete tempSong;
    //}}}
    fclose (file);
    mRunning = false;
    }
  //}}}

private:
  static constexpr int kWavFrameSamples = 1024;
  static constexpr int kFileChunkSize = kWavFrameSamples * 2 * 4; // 2 channels of 4byte floats

  cSong* mSong = nullptr;
  };
//}}}
//{{{
class cLoadMp3AacFile : public cLoadFile {
public:
  cLoadMp3AacFile() : cLoadFile("mp3Aac") {}
  virtual ~cLoadMp3AacFile() {}

  virtual cSong* getSong() { return mSong; }
  virtual iVideoPool* getVideoPool() { return nullptr; }
  //{{{
  virtual string getInfoString() {
    return format("{} {}x{}hz {}k", mFilename, mNumChannels, mSampleRate, mStreamPos/1024);
    }
  //}}}
  //{{{
  virtual float getFracs (float& audioFrac, float& videoFrac) {
  // return fracs for spinner graphic, true if ok to display

    audioFrac = 0.f;
    videoFrac = 0.f;
    return  mLoadFrac;
    }
  //}}}

  //{{{
  virtual bool recognise (const vector<string>& params) {

    if (!getFileSize (params[0]))
      return false;

    mAudioFrameType = getAudioFileInfo();
    return  (mAudioFrameType == eAudioFrameType::eMp3) || (mAudioFrameType == eAudioFrameType::eAacAdts);
    }
  //}}}
  //{{{
  virtual void load() {
  // aac,mp3 - load file in kFileChunkSize chunks, buffer big enough for last chunk partial frame
  // - preload whole file

    mExit = false;
    mRunning = true;

    // get first fileChunk
    FILE* file = fopen (mFilename.c_str(), "rb");
    uint8_t buffer[kFileChunkSize*2];
    size_t size = fread (buffer, 1, kFileChunkSize, file);
    size_t chunkBytesLeft = size;
    mStreamPos = 0;

    //{{{  jpeg
    //int jpegLen;
    //if (getJpeg (jpegLen)) {
      //{{{  found jpegImage
      //cLog::log (LOGINFO, "found jpeg piccy in file, load it");
      ////delete (cAudioParser::mJpegPtr);
      ////cAudioParser::mJpegPtr = nullptr;
      //}
      //}}}
    //}}}
    iAudioDecoder* decoder = createAudioDecoder (mAudioFrameType);

    // pts = frameNum starting at zero
    int64_t pts = 0;
    do {
      // process fileChunk
      uint8_t* frame = buffer;
      int frameSize = 0;
      while (!mExit &&
             cAudioParser::parseFrame (frame, frame + chunkBytesLeft, frameSize)) {
        // process frame in fileChunk
        float* samples = decoder->decodeFrame (frame, frameSize, pts);
        frame += frameSize;
        chunkBytesLeft -= frameSize;
        mStreamPos += frameSize;
        if (samples) {
          if (!mSong) // first decoded frame gives aacHE sampleRate,samplesPerFrame
            mSong = new cSong (mAudioFrameType, decoder->getNumChannels(), decoder->getSampleRate(),
                               decoder->getNumSamplesPerFrame(), 0);
          mSong->addFrame (true, pts, samples, (mFileSize * mSong->getNumFrames()) / mStreamPos);
          pts += mSong->getFramePtsDuration();
          if (!mSongPlayer)
            mSongPlayer = new cSongPlayer (mSong, false);
          }
        mLoadFrac = float(mStreamPos) / mFileSize;
        }

      // get next fileChunk
      memcpy (buffer, frame, chunkBytesLeft);
      chunkBytesLeft += fread (buffer + chunkBytesLeft, 1, kFileChunkSize, file);
      } while (!mExit && (chunkBytesLeft > 0));
    mLoadFrac = 0.f;

    //{{{  delete resources
    if (mSongPlayer)
      mSongPlayer->wait();
    delete mSongPlayer;

    auto tempSong = mSong;
    mSong = nullptr;
    delete tempSong;

    delete decoder;
    //}}}
    fclose (file);
    mRunning = false;
    }
  //}}}

private:
  static constexpr int kFileChunkSize = 2048;

  eAudioFrameType mAudioFrameType = eAudioFrameType::eUnknown;
  cSong* mSong = nullptr;
  };
//}}}

// cLoader public
//{{{
cLoader::cLoader() {

  mLoadSources.push_back (new cLoadRtp());
  mLoadSources.push_back (new cLoadDvb());
  mLoadSources.push_back (new cLoadHls());
  mLoadSources.push_back (new cLoadIcyCast());
  mLoadSources.push_back (new cLoadTsFile());
  mLoadSources.push_back (new cLoadMp3AacFile());
  mLoadSources.push_back (new cLoadWavFile());

  // create and use idle load
  mLoadIdle = new cLoadIdle();
  mLoadSource = mLoadIdle;
  };
//}}}
//{{{
cLoader::~cLoader() {

  for (auto loadSource : mLoadSources)
    delete loadSource;

  mLoadSources.clear();
  }
//}}}

// cLoader iLoad gets
cSong* cLoader::getSong() { return mLoadSource->getSong(); }
iVideoPool* cLoader::getVideoPool() { return mLoadSource->getVideoPool(); }
string cLoader::getInfoString() { return mLoadSource->getInfoString(); }
float cLoader::getFracs (float& audioFrac, float& videoFrac) { return mLoadSource->getFracs (audioFrac, videoFrac); }

// cLoader iLoad actions
bool cLoader::togglePlaying() { return mLoadSource->togglePlaying(); }
bool cLoader::skipBegin() { return  mLoadSource->skipBegin(); }
bool cLoader::skipEnd() { return mLoadSource->skipEnd(); }
bool cLoader::skipBack (bool shift, bool control) { return mLoadSource->skipBack (shift, control); }
bool cLoader::skipForward (bool shift, bool control) { return mLoadSource->skipForward (shift, control); };
void cLoader::exit() { mLoadSource->exit(); }

// cLoader load
//{{{
void cLoader::launchLoad (const vector<string>& params) {
// launch rcognised load as thread

  mLoadSource->exit();
  mLoadSource = mLoadIdle;

  // check for empty params
  if (params.empty())
    return;

  for (auto loadSource : mLoadSources) {
    if (loadSource->recognise (params)) {
      // loadSource recognises params, launch load thread
      thread ([&]() {
        // lambda
        cLog::setThreadName ("load");
        mLoadSource->load();
        cLog::log (LOGINFO, "exit");
        } ).detach();

      mLoadSource = loadSource;
      return;
      }
    }
  }
//}}}
//{{{
void cLoader::load (const vector<string>& params) {
// run recognised load

  mLoadSource->exit();
  mLoadSource = mLoadIdle;

  // check for empty params
  if (params.empty())
    return;

  for (auto loadSource : mLoadSources) {
    if (loadSource->recognise (params)) {
      // loadSource recognises params, launch load thread
      loadSource->load();
      cLog::log (LOGINFO, "exit");
      return;
      }
    }
  }
//}}}
