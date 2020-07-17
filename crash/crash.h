#pragma once
#include <stddef.h>
#include <unistd.h>
#include <assert.h>

class cCrash {
public:
  #ifdef _WIN32
    cCrash() {}
    ~cCrash() {}
  #else
    cCrash();
    ~cCrash();

   private:
    static void handleSignal (int sig, void* info, void* secret);

    // static vars
    static inline const size_t kNeededMemory = 16384;

    static inline int frames_count_ = 16;
    static inline char* memoryAlloc = NULL;
  #endif
  };
