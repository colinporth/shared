#pragma once
#include <stdio.h>
#include <string>

enum eLogCode { LOGNONE, LOGNOTICE, LOGINFO, LOGINFO1, LOGINFO2, LOGINFO3, LOGDEBUG, LOGWARNING, LOGERROR } ;

class cLog {
public:
  ~cLog() { close(); }

  static bool init (std::string path, enum eLogCode logLevel);
  static void close();

  static enum eLogCode getLogLevel();
  static void setLogLevel (enum eLogCode logLevel);

  static void log (enum eLogCode logCode, std::string logStr);
  static void log (enum eLogCode logCode, const char *format, ... );

  static std::string getLine (int n);
  };
