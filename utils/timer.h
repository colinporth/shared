// timer.h
#pragma once

inline double getTimer() {
  LARGE_INTEGER largeInteger;
  QueryPerformanceFrequency (&largeInteger);
  auto performanceFrequency = double (largeInteger.QuadPart);
  QueryPerformanceCounter (&largeInteger);
  return double (largeInteger.QuadPart) / performanceFrequency;
  }
