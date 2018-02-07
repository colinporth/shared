// cLog.cpp - simple logging
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #include "windows.h"
#else
  #include <cstdarg>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/syscall.h>
  #define gettid() syscall(SYS_gettid)
#endif

#include <algorithm>
#include <string>
#include <mutex>
#include <deque>
#include <map>

#include "date.h"
#include "utils.h"
#include "cLog.h"

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

using namespace std;
//}}}
const int kMaxBuffer = 10000;

enum eLogLevel mLogLevel = LOGERROR;

mutex mLinesMutex;
deque<cLog::cLine> mLineDeque;

bool mBuffer = false;
int mDaylightSecs = 0;
FILE* mFile = NULL;

map<uint64_t,string> mThreadNameMap;

#ifdef _WIN32
  HANDLE hStdOut = 0;
#endif

//{{{
bool cLog::init (enum eLogLevel logLevel, bool buffer, string path) {

  #ifdef _WIN32
    hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    DWORD consoleMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode (hStdOut, consoleMode);

    TIME_ZONE_INFORMATION timeZoneInfo;
    if (GetTimeZoneInformation (&timeZoneInfo) == TIME_ZONE_ID_DAYLIGHT)
      mDaylightSecs = -timeZoneInfo.DaylightBias * 60;
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

  setThreadName ("main");
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
string cLog::getThreadNameString (uint64_t threadId) {
  auto it = mThreadNameMap.find (threadId);
  if (it == mThreadNameMap.end())
    return hex(threadId/8,4);
  else
    return it->second;
  }
//}}}
//{{{
wstring cLog::getThreadNameWstring (uint64_t threadId) {

  auto it = mThreadNameMap.find (threadId);
  if (it == mThreadNameMap.end())
    return whex(threadId/8,4);
  else
    return strToWstr(it->second);
  }
//}}}

//{{{
void cLog::setLogLevel (enum eLogLevel logLevel) {

  logLevel = max (LOGNOTICE, min (eLogLevel(LOGMAX-1), logLevel));

  if (mLogLevel != logLevel) {
    switch (logLevel) {
      case LOGTITLE:  cLog::log (LOGNOTICE, "setLogLevel to LOGTITLE"); break;
      case LOGNOTICE: cLog::log (LOGNOTICE, "setLogLevel to LOGNOTICE"); break;
      case LOGERROR:  cLog::log (LOGNOTICE, "setLogLevel to LOGERROR"); break;
      case LOGINFO:   cLog::log (LOGNOTICE, "setLogLevel to LOGINFO");  break;
      case LOGINFO1:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO1"); break;
      case LOGINFO2:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO2"); break;
      case LOGINFO3:  cLog::log (LOGNOTICE, "setLogLevel to LOGINFO3"); break;
      default: break;
      }
    mLogLevel = logLevel;
    }
  }
//}}}
//{{{
void cLog::setThreadName (const string& name) {

  auto it = mThreadNameMap.find (getThreadId());
  if (it == mThreadNameMap.end())
    mThreadNameMap.insert (map<uint64_t,string>::value_type (getThreadId(), name));

  log (LOGINFO, "start");
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, const string& logStr) {

  if (!mBuffer && (logLevel > mLogLevel)) // bomb out early without lock
    return;

  lock_guard<mutex> lockGuard (mLinesMutex);

  auto timePoint = chrono::system_clock::now() + chrono::seconds (mDaylightSecs);

  if (mBuffer) {
    // to buffer for widget access
    mLineDeque.push_front (cLine (logLevel, getThreadId(), timePoint, logStr));
    if (mLineDeque.size() > kMaxBuffer)
      mLineDeque.pop_back();
    }

  else if (logLevel <= mLogLevel) {
    auto datePoint = date::floor<date::days>(timePoint);
    auto timeOfDay = date::make_time (chrono::duration_cast<chrono::microseconds>(timePoint - datePoint));
    int h = timeOfDay.hours().count();
    int m = timeOfDay.minutes().count();
    int s = (int)timeOfDay.seconds().count();
    int subSec = (int)timeOfDay.subseconds().count();

    // to stdout
    char buffer[40];
    //{{{
    const char levelColours[][13] = {
      "\033[38;5;208m\000",   // note   orange
      "\033[38;5;208m\000",   // title  orange
      "\033[38;5;196m\000",   // error  light red
      "\033[38;5;220m\000",   // info   yellow
      "\033[38;5;112m\000",   // info1  green
      "\033[38;5;144m\000",   // info2  nnn
      "\033[38;5;147m\000",   // info3  bluish
      };
    //}}}
    const char* prefixFormat = "%02.2d:%02.2d:%02.2d.%06d %s";
    sprintf (buffer, prefixFormat, h, m, s, subSec, levelColours[logLevel]);
    fputs (buffer, stdout);
    fputs (getThreadNameString (getThreadId()).c_str(), stdout);
    fputc (' ', stdout);
    fputs (logStr.c_str(), stdout);
    const char* postfix = "\033[m\n";
    fputs (postfix, stdout);

    if (mFile) {
      // to file
      const char levelNames[][6] = { "Title", "Note-", "Err--", "Info-", "Info1", "Info2", "Info3", };
      sprintf (buffer, prefixFormat, h, m, s, subSec, levelNames[logLevel]);
      fputs (buffer, mFile);
      fputc (' ', mFile);
      fputs (logStr.c_str(), mFile);
      fputc ('\n', mFile);
      fflush (mFile);
      }
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const char* format, ... ) {

  if (!mBuffer && (logLevel > mLogLevel)) // bomb out early without lock
    return;

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
bool cLog::getLine (cLine& line, unsigned lineNum, unsigned& lastLineIndex) {
// still a bit too dumb, holding onto lastLineIndex between searches helps

  lock_guard<mutex> lockGuard (mLinesMutex);

  unsigned matchingLineNum = 0;
  for (auto i = lastLineIndex; i < mLineDeque.size(); i++)
    if (mLineDeque[i].mLogLevel <= mLogLevel)
      if (lineNum == matchingLineNum++) {
        line = mLineDeque[i];
        return true;
        }

  return false;
  }
//}}}

// private
//{{{
uint64_t cLog::getThreadId() {

  #ifdef _WIN32
    return GetCurrentThreadId();
  #else
    return gettid();
  #endif
  }
//}}}
