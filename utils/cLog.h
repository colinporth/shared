#pragma once
#include <stdio.h>
#include <string>
#include <time.h>

enum eLogCode { LOGNOTICE, LOGERROR, LOGWARNING, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3 } ;
const int kBst = 1;

class cLog {
public:
  ~cLog() { close(); }

  static bool init (std::string path, enum eLogCode logLevel, bool buffer);
  static void close();

  static enum eLogCode getLogLevel();
  static void setLogLevel (enum eLogCode logLevel);

  static void log (enum eLogCode logCode, std::string logStr);
  static void log (enum eLogCode logCode, const char *format, ... );

  static bool getLine (int n, std::string& str, eLogCode& logCode, uint32_t& usTime);
  };
