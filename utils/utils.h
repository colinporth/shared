// utils.h
#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <locale>
#include  <codecvt>

#ifdef USE_NANOVG
  #define bigMalloc(size,tag)   malloc (size)
  #define bigFree               free
  #define smallMalloc(size,tag) malloc (size)
  #define smallFree             free

#elif _WIN32
  void* debugMalloc (size_t size, const char* tag, uint8_t heap);
  void debugFree (void* ptr);
  #define bigMalloc(size,tag)   malloc (size)
  #define bigFree               free
  #define smallMalloc(size,tag) malloc (size)
  #define smallFree             free

#elif STM32
  #include "cLcd.h"
  #define bigMalloc(size,tag)    pvPortMalloc(size)
  #define bigFree                vPortFree
  #define smallMalloc(size,tag)  malloc(size)
  #define smallFree              free
  #define debug(str)             cLcd::get()->info (str)
#endif

// string utils
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
inline std::string wstrToStr (std::wstring wstr) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  return converter.to_bytes (wstr);
  }
//}}}
