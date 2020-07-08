// cSemaphore.h
//{{{  includes
#pragma once

#include <mutex>
#include <condition_variable>

#include "cLog.h"
//}}}

class cSemaphore {
public:
  cSemaphore() : mCount(0) {}
  cSemaphore (int count) : mCount(count) {}
  cSemaphore (const std::string& name) : mName(name), mCount(0) {}

  //{{{
  void wait() {

    if (!mName.empty())
      cLog::log (LOGINFO1, mName + " - wait");

    std::unique_lock<std::mutex> lock (mMutex);
    while (mCount == 0)
      mConditionVariable.wait (lock);
    mCount--;

    if (!mName.empty())
      cLog::log (LOGINFO1, mName + " - signalled");
    }
  //}}}
  //{{{
  void notify() {

    if (!mName.empty())
      cLog::log (LOGINFO1,  mName + " - notify");

    std::unique_lock<std::mutex> lock (mMutex);
    mCount++;
    mConditionVariable.notify_one();
    }
  //}}}
  //{{{
  void notifyAll() {

    if (!mName.empty())
      cLog::log (LOGINFO1,  mName + " - notifyAll");

    std::unique_lock<std::mutex> lock (mMutex);
    mCount++;
    mConditionVariable.notify_all();
    }
  //}}}

private:
  std::mutex mMutex;
  std::condition_variable mConditionVariable;
  std::string mName;
  int mCount;
  };
