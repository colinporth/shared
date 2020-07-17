// cLog.h
//{{{  includes
#pragma once

#include <string>
#include <chrono>
//}}}

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

    eLogLevel mLogLevel = LOGNOTICE;
    uint64_t mThreadId = 0;
    std::chrono::time_point<std::chrono::system_clock> mTimePoint;
    std::string mStr;
    };
  //}}}

  ~cLog();

  static bool init (enum eLogLevel logLevel, bool buffer = false, std::string path = "", std::string title = "");
  static void setDaylightOffset (int offset);
  static void close();

  static enum eLogLevel getLogLevel();
  static std::string getThreadNameString (uint64_t threadId);
  static std::wstring getThreadNameWstring (uint64_t threadId);

  static void setLogLevel (enum eLogLevel logLevel);
  static void setThreadName (const std::string& name);

  static void log (enum eLogLevel logLevel, const std::string& logStr);
  static void log (enum eLogLevel logLevel, const char* format, ... );

  static bool getLine (cLine& line, unsigned lineNum, unsigned& lastLineIndex);

  static void avLogCallback (void* ptr, int level, const char* fmt, va_list vargs);

private:
  static uint64_t getThreadId();
  };
