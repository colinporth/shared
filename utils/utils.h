// utils.h
#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

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
inline std::string getDebugPtsString (int64_t pts) {

  if (pts < 0)
    return "--:--:--:--";
  else {
    uint32_t frac = pts % 900;

    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;

    std::string str (hours ? (dec (hours) + 'h' + dec (mins, 2, '0') + 'm') : mins ? (dec (mins) + 'm') : "");

    return str + dec (secs, 2, '0') + 's' + dec (hs, 2, '0') + "." + dec (frac,3,'0');
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

