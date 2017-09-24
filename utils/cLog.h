#pragma once
#include <stdio.h>
#include <string>
#include <time.h>

enum eLogCode { LOGNOTICE, LOGERROR, LOGWARNING, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3 } ;
const int kBst = 1;

class cLog {
public:
  //{{{
  class cLine {
  public:
    cLine (eLogCode logCode, uint32_t timeMs, std::string str) : 
      mLogCode(logCode), mTimeMs(timeMs), mStr(str) {}

    eLogCode mLogCode;
    uint32_t mTimeMs;
    std::string mStr;
    };
  //}}}

  ~cLog() { close(); }

  static bool init (std::string path, enum eLogCode logLevel, bool buffer);
  static void close();

  static enum eLogCode getLogLevel();
  static void setLogLevel (enum eLogCode logLevel);

  static void log (enum eLogCode logCode, std::string logStr);
  static void log (enum eLogCode logCode, const char *format, ... );

  static cLine* getLine (int n);
  };
