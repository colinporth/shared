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

#include "cLog.h"

#include <string>
#include <mutex>
#include <deque>

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

using namespace std;
//}}}
const int kMaxBuffer = 10000;
//{{{  const
const char levelNames[][6] =    { "Title",
                                  "Note-",
                                  "Err--",
                                  "Info-",
                                  "Info1",
                                  "Info2",
                                  "Info3",
                                  };

const char levelColours[][12] = { "\033[38;5;208m",   // note   orange
                                  "\033[38;5;208m",   // title  orange
                                  "\033[38;5;196m",   // error  light red
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

mutex mLinesMutex;
deque<cLog::cLine> mLines;

bool mBuffer = false;
FILE* mFile = NULL;

#ifdef _WIN32
  HANDLE hStdOut = 0;
#endif

//{{{
bool cLog::init (enum eLogLevel logLevel, bool buffer, string path) {

  #ifdef _WIN32
    hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    DWORD consoleMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode (hStdOut, consoleMode);
  #endif

  mBuffer = buffer;

  mLogLevel = logLevel;
  if (mLogLevel > LOGNOTICE) {
    if (!path.empty() && !mFile) {
      string strLogFile = path + "/log.txt";
      string strLogFileOld = path + "/log.old.txt";

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

  logLevel = min (logLevel, eLogLevel(LOGMAX-1));
  logLevel = max (logLevel, LOGNOTICE);
  mLogLevel = logLevel;
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, string logStr) {

  //  get time
  struct timeval now;
  gettimeofday (&now, NULL);

  lock_guard<mutex> lockGuard (mLinesMutex);

  if (mBuffer) {
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
    }

  if (mFile) {
    auto hour = kBst + (now.tv_sec / 3600) % 24;
    auto minute = (now.tv_sec / 60) % 60;
    auto second = now.tv_sec % 60;
    auto subSec = now.tv_usec;

    char buffer[40];
    sprintf (buffer, prefixFormat, hour, minute, second, subSec, levelNames[logLevel]);
    fputs (buffer, mFile);
    fputc (' ', mFile);
    fputs (logStr.c_str(), mFile);
    fputc ('\n', mFile);
    fflush (mFile);
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const char* format, ... ) {

  // form logStr
  va_list args;
  va_start (args, format);

  // get size of str
  size_t size = vsnprintf (nullptr, 0, format, args) + 1; // Extra space for '\0'

  // allocate buffer
  unique_ptr<char[]> buf (new char[size]);

  // form buffer
  vsnprintf (buf.get(), size, format, args);

  va_end (args);

  log (logLevel, string (buf.get(), buf.get() + size-1));
  }
//}}}

//{{{
bool cLog::getLine (cLine& line, int lineNum) {

  lock_guard<mutex> lockGuard (mLinesMutex);

  int matchingLineNum = 0;
  for (int lineIndex = 0; lineIndex < mLines.size(); lineIndex++)
    if (mLines[lineIndex].mLogLevel <= mLogLevel)
      if (matchingLineNum++ == lineNum) {
        line = mLines[lineIndex];
        return true;
        }

  return false;
  }
//}}}
