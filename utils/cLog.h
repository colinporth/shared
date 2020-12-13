// cLog.h
//{{{  includes
#pragma once

#include <string>
#include <chrono>
//}}}

enum eLogLevel { LOGTITLE, LOGNOTICE, LOGERROR, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3 };

class cLog {
public:
  //{{{
  class cLine {
  public:
    cLine (eLogLevel logLevel, uint64_t threadId,
           std::chrono::time_point<std::chrono::system_clock> timePoint, const std::string& lineString)
      : mLogLevel(logLevel), mThreadId(threadId), mTimePoint(timePoint), mString(lineString) {}

    eLogLevel mLogLevel;
    uint64_t mThreadId;
    std::chrono::time_point<std::chrono::system_clock> mTimePoint;
    std::string mString;
    };
  //}}}
  ~cLog();

  static bool init (eLogLevel logLevel = LOGINFO,
                    bool buffer = false,
                    const std::string& logFilePath = "",
                    const std::string& title = "");

  static void setDaylightOffset (int offset);
  static void close();

  static enum eLogLevel getLogLevel();
  static std::string getThreadNameString (uint64_t threadId);
  static std::wstring getThreadNameWstring (uint64_t threadId);

  static void cycleLogLevel();
  static void setLogLevel (eLogLevel logLevel);
  static void setThreadName (const std::string& name);

  static void log (eLogLevel logLevel, const std::string& logStr);
  static void log (eLogLevel logLevel, const char* format, ... );
  static void logDvb (void* unused, const char* format, ... );

  static void clearScreen();
  static void status (int row, int colourIndex, const std::string& statusString);

  static bool getLine (cLine& line, unsigned lineNum, unsigned& lastLineIndex);

  static void avLogCallback (void* ptr, int level, const char* fmt, va_list vargs);
  };
