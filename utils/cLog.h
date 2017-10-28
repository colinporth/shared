#pragma once
#include <stdio.h>
#include <string>
#include <time.h>

enum eLogLevel { LOGTITLE, LOGNOTICE, LOGERROR, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3, LOGMAX };
const int kBst = 1;

class cLog {
public:
  //{{{
  class cLine {
  public:
    cLine() {}
    cLine (eLogLevel logLevel, uint32_t timeMs, std::string str) :
      mLogLevel(logLevel), mTimeMs(timeMs), mStr(str) {}

    eLogLevel mLogLevel;
    uint32_t mTimeMs;
    std::string mStr;
    };
  //}}}

  ~cLog() { close(); }

  static bool init (std::string title, enum eLogLevel logLevel, bool buffer = false, std::string path = "");
  static void close();

  static enum eLogLevel getLogLevel();
  static void setLogLevel (enum eLogLevel logLevel);

  static void log (enum eLogLevel logLevel, std::string logStr);
  static void log (enum eLogLevel logLevel, const char* format, ... );

  static bool getLine (cLine& line, int lineNum);
  };
