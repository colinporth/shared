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

  static double performanceFrequency = 0.0;
  static __int64 performanceCounterStart = 0;
  //{{{
  double startTimer() {
    LARGE_INTEGER largeInteger;
    QueryPerformanceFrequency (&largeInteger);
    performanceFrequency = double (largeInteger.QuadPart);
    QueryPerformanceCounter (&largeInteger);
    performanceCounterStart = largeInteger.QuadPart;
    return double (largeInteger.QuadPart - performanceCounterStart) / performanceFrequency;
    }
  //}}}
  //{{{
  double getTimer() {
    LARGE_INTEGER largeInteger;
    QueryPerformanceCounter (&largeInteger);
    return double (largeInteger.QuadPart - performanceCounterStart) / performanceFrequency;
    }
  //}}}

#elif STM32
  #include "cLcd.h"
  #define bigMalloc(size,tag)    pvPortMalloc(size)
  #define bigFree                vPortFree
  #define smallMalloc(size,tag)  malloc(size)
  #define smallFree              free
  #define debug(str)             cLcd::get()->info (str)
#endif

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
std::string getTimeStrFromPts (uint64_t pts) {

  pts /= 900;
  uint32_t hs = pts % 100;
  pts /= 100;
  uint32_t secs = pts % 60;
  pts /= 60;
  uint32_t mins = pts % 60;
  pts /= 60;
  uint32_t hours = pts % 60;

  if (hours)
    return dec (hours) + ':' + dec (mins, 2, '0') + ':' + dec(secs, 2, '0') + ':' + dec(hs, 2, '0');
  else 
    return dec (mins) + ':' + dec(secs, 2, '0') + ':' + dec(hs, 2, '0');
  }
//}}}
