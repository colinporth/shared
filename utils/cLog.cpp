// cLog.cpp - simple logging
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #include "windows.h"
  //{{{
  void gettimeofday (struct timeval* tp, struct timezone* tzp) {
  // timezone information is stored outside the kernel so tzp isn't used anymore.
  // Note: this function is not for Win32 high precision timing purpose. See * elapsed_time().

    SYSTEMTIME system_time;
    GetSystemTime (&system_time);

    FILETIME file_time;
    SystemTimeToFileTime (&system_time, &file_time);

    ULARGE_INTEGER ularge;
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long) ((ularge.QuadPart - 116444736000000000ULL) / 10000000L);

    // actually millisecs
    tp->tv_usec = (long) system_time.wMilliseconds;
    }
  //}}}
#else
  #include <stdint.h>
  #include <string.h>
  #include <time.h>
  #include <cstdarg>
  #include <memory>
  #include <sys/stat.h>
  #include <sys/time.h>
  //{{{
  typedef struct _SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
    } SYSTEMTIME;
  //}}}
#endif

#include <string>
#include <mutex>
#include <deque>

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

#include "cLog.h"
//}}}
const int kMaxBuffer = 10000;
//{{{  const
const char levelNames[][6] =    { "Note ",
                                  "Error",
                                  "Warn ",
                                  "Info ",
                                  "Info1",
                                  "Info2",
                                  "Info3",
                                  };

const char levelColours[][12] = { "\033[38;5;208m",   // note   orange
                                  "\033[38;5;196m",   // error  light red
                                  "\033[38;5;207m",   // warn   mauve
                                  "\033[38;5;220m",   // info   yellow
                                  "\033[38;5;112m",   // info1  green
                                  "\033[38;5;144m",   // info2  nnn
                                  "\033[38;5;147m",   // info3  bluish
                                   };

#ifdef _WIN32
  const char* prefixFormat =        "%02.2d:%02.2d:%02.2d.%03d %s";
#else
  const char* prefixFormat =        "%02.2d:%02.2d:%02.2d.%06d %s";
#endif

const char* postfix =             "\033[m\n";
//}}}

enum eLogLevel mLogLevel = LOGERROR;

bool gBuffer = false;
FILE* mFile = NULL;
std::mutex mLogMutex;

std::deque<cLog::cLine> mLines;

#ifdef _WIN32
  HANDLE hStdOut = 0;
#endif

//{{{
bool cLog::init (std::string path, enum eLogLevel logLevel, bool buffer) {

  #ifdef _WIN32
    hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    DWORD consoleMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode (hStdOut, consoleMode);
  #endif

  gBuffer = buffer;

  mLogLevel = logLevel;
  if (mLogLevel > LOGNOTICE) {
    if (!path.empty() && !mFile) {
      std::string strLogFile = path + "/omxPlayer.log";
      std::string strLogFileOld = path + "/omxPlayer.old.log";

      struct stat info;
      if (stat (strLogFileOld.c_str(), &info) == 0 && remove (strLogFileOld.c_str()) != 0)
        return false;
      if (stat (strLogFile.c_str(), &info) == 0 && rename (strLogFile.c_str(), strLogFileOld.c_str()) != 0)
        return false;

      mFile = fopen (strLogFile.c_str(), "wb");
      }
    }

  return mFile != NULL;
  }
//}}}
//{{{
void cLog::close() {

  if (mFile) {
    fclose (mFile);
    mFile = NULL;
    }
  }
//}}}

//{{{
enum eLogLevel cLog::getLogLevel() {
  return mLogLevel;
  }
//}}}
//{{{
void cLog::setLogLevel (enum eLogLevel logLevel) {

  logLevel = min (logLevel, LOGINFO3);
  logLevel = max (logLevel, LOGNOTICE);
  mLogLevel = logLevel;
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, std::string logStr) {

  //  get time
  struct timeval now;
  gettimeofday (&now, NULL);

  std::lock_guard<std::mutex> lockGuard (mLogMutex);

  if (gBuffer) {
    uint32_t timeMs = ((now.tv_sec % (24 * 60 * 60)) * 1000) + now.tv_usec;
    mLines.push_front (cLine (logLevel, timeMs, logStr));
    if (mLines.size() > kMaxBuffer)
      mLines.pop_back();
    }

  else if (logLevel <= mLogLevel) {
    auto hour = kBst + (now.tv_sec/3600) % 24;
    auto minute = (now.tv_sec/60) % 60;
    auto second = now.tv_sec % 60;
    auto subSec = now.tv_usec;

    // write log line
    char buffer[40];
    sprintf (buffer, prefixFormat, hour, minute, second, subSec, levelColours[logLevel]);
    fputs (buffer, stdout);
    fputs (logStr.c_str(), stdout);
    fputs (postfix, stdout);

    if (mFile) {
      sprintf (buffer, prefixFormat, hour, minute, second, subSec, levelNames[logLevel]);
      fputs (buffer, mFile);
      fputs (logStr.c_str(), mFile);
      fputc ('\n', mFile);
      fflush (mFile);
      }
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const char* format, ... ) {

  // form logStr
  va_list args;
  va_start (args, format);

  // get size of str
  size_t size = std::vsnprintf (nullptr, 0, format, args) + 1; // Extra space for '\0'

  // allocate buffer
  std::unique_ptr<char[]> buf (new char[size]);

  // form buffer
  std::vsnprintf (buf.get(), size, format, args);

  va_end (args);

  log (logLevel, std::string (buf.get(), buf.get() + size-1));
  }
//}}}

//{{{
cLog::cLine* cLog::getLine (int n) {

  std::lock_guard<std::mutex> lockGuard (mLogMutex);
  return n < mLines.size() ? &mLines[n] : nullptr;
  }
//}}}
