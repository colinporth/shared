// utils.h
//{{{  includes
#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <locale>
#include <codecvt>
#include <algorithm>
//}}}

#ifdef USE_NANOVG
  //{{{  malloc, free defines
  #define bigMalloc(size,tag)   malloc (size)
  #define bigFree               free
  #define smallMalloc(size,tag) malloc (size)
  #define smallFree             free
  //}}}
#elif _WIN32
  //{{{  malloc, free defines
  void* debugMalloc (size_t size, const char* tag, uint8_t heap);
  void debugFree (void* ptr);
  #define bigMalloc(size,tag)   malloc (size)
  #define bigFree               free
  #define smallMalloc(size,tag) malloc (size)
  #define smallFree             free
  //}}}
#elif STM32
  //{{{  malloc, free defines
  #include "cLcd.h"
  #define bigMalloc(size,tag)    pvPortMalloc(size)
  #define bigFree                vPortFree
  #define smallMalloc(size,tag)  malloc(size)
  #define smallFree              free
  #define debug(str)             cLcd::get()->info (str)
  //}}}
#endif

// string utils
//{{{
inline std::string wstrToStr (const std::wstring& wstr) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  return converter.to_bytes (wstr);
  }
//}}}

//{{{
template <typename T> std::string hex (T value, int width = 0) {

  std::ostringstream os;
  os << std::hex << std::setfill ('0') << std::setw (width) << value;
  return os.str();
  }
//}}}
//{{{
template <typename T> std::string dec (T value, int width = 0, char fill = ' ') {

  std::ostringstream os;
  os << std::setfill (fill) << std::setw (width) << value;
  return os.str();
  }
//}}}
//{{{
template <typename T> std::string decFrac (T value, int width, int precision, char fill) {

  std::ostringstream os;
  os << std::setfill (fill) << std::setw (width) << std::setprecision (precision) << value;
  return os.str();
  }
//}}}

//{{{
inline std::string getPtsString (int64_t pts) {
// miss out zeros

  if (pts < 0)
    return "--:--:--";
  else {
    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;

    std::string str (hours ? (dec (hours) + ':' + dec (mins, 2, '0')) : dec (mins));

    return str + ':' + dec(secs, 2, '0') + ':' + dec(hs, 2, '0');
    }
  }
//}}}
//{{{
inline std::string getFullPtsString (int64_t pts) {
// all digits

  if (pts < 0)
    return "--:--:--:--";
  else {
    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;

    return dec (hours,2,'0') + ':' + dec (mins,2,'0') + ':' + dec(secs,2,'0') + ':' + dec(hs,2,'0');
    }
  }
//}}}
//{{{
inline std::string getTimeString (uint32_t secs) {

  return dec (secs / (60*60)) + ':' +
         dec ((secs / 60) % 60, 2, '0') + ':' +
         dec( secs % 60, 2, '0');
  }
//}}}
//{{{
inline std::string getTimetString (time_t& time) {

  tm localTm;
  localtime_s (&localTm, &time);

  return dec(localTm.tm_hour,2,'0') + ":" +
         dec(localTm.tm_min,2,'0') + ":" +
         dec(localTm.tm_sec,2,'0');
  }
//}}}
//{{{
inline std::string getTimetDateString (time_t& time) {

  tm localTm;
  localtime_s(&localTm, &time);

  const char day_name[][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  const char mon_name[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

  return dec(localTm.tm_hour,2,'0') + ":" +
         dec(localTm.tm_min,2,'0') + ":" +
         dec(localTm.tm_sec,2,'0') + " " +
         day_name[localTm.tm_wday] + " " +
         dec(localTm.tm_mday) + " " +
         mon_name[localTm.tm_mon] + " " +
         dec(1900 + localTm.tm_year);
  }
//}}}
//{{{
inline std::string getTimetShortString (time_t& time) {

  tm localTm;
  localtime_s (&localTm, &time);

  return dec(localTm.tm_hour,2,'0') + ":" +
         dec(localTm.tm_min,2,'0');
  }
//}}}

//{{{
inline std::string validString (const std::string& str, char* inValidChars) {

  auto validStr = str;
  for (auto i = 0u; i < strlen(inValidChars); ++i)
    validStr.erase (std::remove (validStr.begin(), validStr.end(), inValidChars[i]), validStr.end());

  return validStr;
  }
//}}}

// wstring utils
//{{{
inline std::wstring strToWstr (const std::string& str) {
  return std::wstring (str.begin(), str.end());
  }
//}}}

//{{{
template <typename T> std::wstring whex (T value, int width = 0) {

  std::wstringstream os;
  os << std::hex << std::setfill (L'0') << std::setw (width) << value;
  return os.str();
  }
//}}}
//{{{
template <typename T> std::wstring wdec (T value, int width = 0, wchar_t fill = L' ') {

  std::wstringstream os;
  os << std::setfill (fill) << std::setw (width) << value;
  return os.str();
  }
//}}}
//{{{
template <typename T> std::wstring decFrac (T value, int width, int precision, wchar_t fill) {

  std::wstringstream os;
  os << std::setfill (fill) << std::setw (width) << std::setprecision (precision) << value;
  return os.str();
  }
//}}}

//{{{
inline std::wstring getPtsWstring (int64_t pts) {
// miss out zeros

  if (pts < 0)
    return L"--:--:--";
  else {
    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;

    std::wstring wstr (hours ? (wdec (hours) + L':' + wdec (mins, 2, '0')) : wdec (mins));

    return wstr + L':' + wdec(secs, 2, '0') + L':' + wdec(hs, 2, '0');
    }
  }
//}}}
//{{{
inline std::wstring getFullPtsWstring (int64_t pts) {
// all digits

  if (pts < 0)
    return L"--:--:--:--";
  else {
    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;

    return wdec (hours,2,'0') + L':' + wdec (mins,2,'0') + L':' + wdec(secs,2,'0') + L':' + wdec(hs,2,'0');
    }
  }
//}}}
//{{{
inline std::wstring getTimeWstring (uint32_t secs) {

  return wdec (secs / (60*60)) + L':' +
         wdec ((secs / 60) % 60, 2, '0') + L':' +
         wdec( secs % 60, 2, '0');
  }
//}}}
//{{{
inline std::wstring getTimetWstring (time_t& time) {

  tm localTm;
  localtime_s (&localTm, &time);

  return wdec(localTm.tm_hour,2,'0') + L":" +
         wdec(localTm.tm_min,2,'0') + L":" +
         wdec(localTm.tm_sec,2,'0');
  }
//}}}
//{{{
inline std::wstring getTimetDateWstring (time_t& time) {

  tm localTm;
  localtime_s(&localTm, &time);

  const wchar_t day_name[][4] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
  const wchar_t mon_name[][4] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                                  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };

  return wdec(localTm.tm_hour,2,'0') + L":" +
         wdec(localTm.tm_min,2,'0') + L":" +
         wdec(localTm.tm_sec,2,'0') + L" " +
         day_name[localTm.tm_wday] + L" " +
         wdec(localTm.tm_mday) + L" " +
         mon_name[localTm.tm_mon] + L" " +
         wdec(1900 + localTm.tm_year);
  }
//}}}
//{{{
inline std::wstring getTimetShortWstring (time_t& time) {

  tm localTm;
  localtime_s (&localTm, &time);

  return wdec(localTm.tm_hour,2,'0') + L":" +
         wdec(localTm.tm_min,2,'0');
  }
//}}}
