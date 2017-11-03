// cSemaphore.h
#pragma once
#include <mutex>
#include <condition_variable>

class cSemaphore {
public:
  cSemaphore (int count = 0) : mCount(count) {}
  //{{{
  void wait() {
    std::unique_lock<std::mutex> lock (mMutex);
    while (mCount == 0)
      mConditionVariable.wait (lock);
    mCount--;
    }
  //}}}
  //{{{
  void notify() {
    std::unique_lock<std::mutex> lock (mMutex);
    mCount++;
    mConditionVariable.notify_one();
    }
  //}}}
  //{{{
  void notifyAll() {
    std::unique_lock<std::mutex> lock (mMutex);
    mCount++;
    mConditionVariable.notify_all();
    }
  //}}}

private:
  std::mutex mMutex;
  std::condition_variable mConditionVariable;
  int mCount;
  };
