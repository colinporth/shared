#pragma once
#include <string>
#include <chrono>

enum eLogLevel { LOGTITLE, LOGNOTICE, LOGERROR, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3, LOGMAX };
const int kBst = 1;

class cLog {
public:
  //{{{
  class cLine {
  public:
    cLine() {}
    cLine (eLogLevel logLevel, uint64_t threadId,
           std::chrono::time_point<std::chrono::system_clock> timePoint, const std::string& str) :
      mLogLevel(logLevel), mThreadId(threadId), mTimePoint(timePoint), mStr(str) {}

    eLogLevel mLogLevel;
    uint64_t mThreadId;
    std::chrono::time_point<std::chrono::system_clock> mTimePoint;
    std::string mStr;
    };
  //}}}

  ~cLog() { close(); }

  static bool init (const std::string& title, enum eLogLevel logLevel,
                    bool buffer = false, std::string path = "");
  static void setDaylightOffset (int offset);
  static void close();

  static enum eLogLevel getLogLevel();
  static void setLogLevel (enum eLogLevel logLevel);

  static void log (enum eLogLevel logLevel, const std::wstring& logStr);
  static void log (enum eLogLevel logLevel, const std::string& logStr);
  static void log (enum eLogLevel logLevel, const char* format, ... );

  static bool getLine (cLine& line, int lineNum);

private:
  static uint64_t getThreadId();
  };
