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
  #define debug(str)            std::cout << str << std::endl

#elif _WIN32
  void* debugMalloc (size_t size, const char* tag, uint8_t heap);
  void debugFree (void* ptr);
  #define bigMalloc(size,tag)   debugMalloc (size, tag, 0)
  #define bigFree               debugFree
  #define smallMalloc(size,tag) debugMalloc (size, tag, 1)
  #define smallFree             debugFree
  #define debug(str)            std::cout << str << std::endl

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
template <typename T> std::string hex (T value, uint16_t width = 0) {

  std::ostringstream os;
  os << std::hex << std::setfill ('0') << std::setw (width) << value;
  return os.str();
  }
//}}}
//{{{
template <typename T> std::string dec (T value, uint16_t width = 0, char fill = ' ') {

  std::ostringstream os;
  os << std::setfill (fill) << std::setw (width) << value;
  return os.str();
  }
//}}}
//{{{
template <typename T> std::string decFrac (T value, int width, int precision, char fill) {

  std::ostringstream os;
  os << std::setfill (fill) << std::setw (width) << setprecision (precision) << value;
  return os.str();
  }
//}}}

//{{{
inline std::string getPtsStr (uint64_t pts) {

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
//}}}
//{{{
inline std::string getPtsStrDeb (uint64_t pts) {

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
//}}}
//{{{
inline std::string getMsTimeStr (int bst, uint32_t msTime) {

  // convert msTime to str
  int subSec = msTime % 1000;
  msTime /= 1000;
  int second = msTime % 60;
  msTime /= 60;
  int minute = msTime  % 60;
  msTime /= 60;
  int hour = bst + msTime;

  return dec(hour,2,'0') + "."  + dec(minute,2, '0') + "." + dec(second,2, '0') + "." + dec(subSec,3, '0');
  }
//}}}
