// utils.h
#pragma once

static double performanceFrequency = 0.0;
static __int64 performanceCounterStart = 0;

inline double startTimer() {
  LARGE_INTEGER largeInteger;
  QueryPerformanceFrequency (&largeInteger);
  performanceFrequency = double (largeInteger.QuadPart);
  QueryPerformanceCounter (&largeInteger);
  performanceCounterStart = largeInteger.QuadPart;
  return double (largeInteger.QuadPart - performanceCounterStart) / performanceFrequency;
  }

inline double getTimer() {
  LARGE_INTEGER largeInteger;
  QueryPerformanceCounter (&largeInteger);
  return double (largeInteger.QuadPart - performanceCounterStart) / performanceFrequency;
  }
