// iClockTime.h -  clockTime interface
#pragma once
#include <chrono>
#include <string>

class iClockTime  {
public:
  virtual ~iClockTime() {}

  virtual int getDayLightSeconds() = 0;
  virtual std::chrono::system_clock::time_point getNowRaw() = 0;
  virtual std::chrono::system_clock::time_point getNowDayLight() = 0;
  };
