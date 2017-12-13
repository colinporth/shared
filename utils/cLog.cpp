// cLog.cpp - simple logging
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "windows.h"

#include "date.h"
#include "cLog.h"

#include <algorithm>
#include <string>
#include <mutex>
#include <deque>
#include <map>

#include "utils.h"

#define remove_utf8   remove
#define rename_utf8   rename
#define fopen64_utf8  fopen
#define stat64_utf8   stat64

using namespace std;
//}}}
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

const char* prefixFormat =        "%02.2d:%02.2d:%02.2d.%06d %s";
const char* postfix =             "\033[m\n";
//}}}
const int kMaxBuffer = 100000;

enum eLogLevel mLogLevel = LOGERROR;

mutex mLinesMutex;
deque<cLog::cLine> mLines;

bool mBuffer = false;
int mDaylightSecs = 0;
FILE* mFile = NULL;

std::map<uint64_t,std::string> mThreadNames;

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
std::string cLog::getThreadNameString (uint64_t threadId) {
  auto it = mThreadNames.find (threadId);
  if (it == mThreadNames.end())
    return hex(threadId/8,4);
  else
    return it->second;
  }
//}}}
//{{{
std::wstring cLog::getThreadNameWstring (uint64_t threadId) {

  auto it = mThreadNames.find (threadId);
  if (it == mThreadNames.end())
    return whex(threadId/8,4);
  else
    return strToWstr(it->second);
  }
//}}}

//{{{
void cLog::setLogLevel (enum eLogLevel logLevel) {

  logLevel = std::min (logLevel, eLogLevel(LOGMAX-1));
  logLevel = std::max (logLevel, LOGNOTICE);
  mLogLevel = logLevel;
  }
//}}}
//{{{
void cLog::setThreadName (const string& name) {

  auto it = mThreadNames.find (getThreadId());
  if (it == mThreadNames.end())
    mThreadNames.insert (map<uint64_t,string>::value_type (getThreadId(), name));

  log (LOGNOTICE, "start");
  }
//}}}

//{{{
void cLog::log (enum eLogLevel logLevel, const wstring& wstr) {

  //  get time
  auto timePoint = chrono::system_clock::now() + chrono::seconds (mDaylightSecs);
  auto logStr = wstrToStr (wstr);

  lock_guard<mutex> lockGuard (mLinesMutex);

  if (mBuffer) {
    mLines.push_front (cLine (logLevel, getThreadId(), timePoint, logStr));
    if (mLines.size() > kMaxBuffer)
      mLines.pop_back();
    }

  else if (logLevel <= mLogLevel) {
    auto datePoint = floor<date::days>(timePoint);
    auto timeOfDay = date::make_time (chrono::duration_cast<chrono::microseconds>(timePoint - datePoint));
    auto h = timeOfDay.hours().count();
    auto m = timeOfDay.minutes().count();
    auto s = timeOfDay.seconds().count();
    auto subSec = timeOfDay.subseconds().count();

    // write log line
    char buffer[40];
    sprintf (buffer, prefixFormat, h, m, s, subSec, levelColours[logLevel]);
    fputs (buffer, stdout);
    fputs (logStr.c_str(), stdout);
    fputs (postfix, stdout);
    }

  if (mFile) {
    auto datePoint = floor<date::days>(timePoint);
    auto timeOfDay = date::make_time (chrono::duration_cast<chrono::microseconds>(timePoint - datePoint));
    auto h = timeOfDay.hours().count();
    auto m = timeOfDay.minutes().count();
    auto s = timeOfDay.seconds().count();
    auto subSec = timeOfDay.subseconds().count();

    char buffer[40];
    sprintf (buffer, prefixFormat, h, m, s, subSec, levelNames[logLevel]);
    fputs (buffer, mFile);
    fputc (' ', mFile);
    fputs (logStr.c_str(), mFile);
    fputc ('\n', mFile);
    fflush (mFile);
    }
  }
//}}}
//{{{
void cLog::log (enum eLogLevel logLevel, const string& logStr) {

  //  get time
  auto timePoint = chrono::system_clock::now() + chrono::seconds (mDaylightSecs);

  lock_guard<mutex> lockGuard (mLinesMutex);

  if (mBuffer) {
    mLines.push_front (cLine (logLevel, getThreadId(), timePoint, logStr));
    if (mLines.size() > kMaxBuffer)
      mLines.pop_back();
    }

  else if (logLevel <= mLogLevel) {
    auto datePoint = floor<date::days>(timePoint);
    auto timeOfDay = date::make_time (chrono::duration_cast<chrono::microseconds>(timePoint - datePoint));
    auto h = timeOfDay.hours().count();
    auto m = timeOfDay.minutes().count();
    auto s = timeOfDay.seconds().count();
    auto subSec = timeOfDay.subseconds().count();

    // write log line
    char buffer[40];
    sprintf (buffer, prefixFormat, h, m, s, subSec, levelColours[logLevel]);
    fputs (buffer, stdout);
    fputs (getThreadNameString (getThreadId()).c_str(), stdout);
    fputc (' ', stdout);
    fputs (logStr.c_str(), stdout);
    fputs (postfix, stdout);
    }

  if (mFile) {
    auto datePoint = floor<date::days>(timePoint);
    auto timeOfDay = date::make_time (chrono::duration_cast<chrono::microseconds>(timePoint - datePoint));
    auto h = timeOfDay.hours().count();
    auto m = timeOfDay.minutes().count();
    auto s = timeOfDay.seconds().count();
    auto subSec = timeOfDay.subseconds().count();

    char buffer[40];
    sprintf (buffer, prefixFormat, h, m, s, subSec, levelNames[logLevel]);
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

// private
//{{{
uint64_t cLog::getThreadId() {
  return GetCurrentThreadId();
  }
//}}}
