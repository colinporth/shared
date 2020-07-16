//{{{
/*

  Copyright (c) 2012, Samsung R&D Institute Russia
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

/*! @file death_handler.h
 *   Declaration of the SIGSEGV/SIGABRT handler which prints the debug stack
 *  trace.
 *  @author Markovtsev Vadim <gmarkhor@gmail.com>
 *  @version 1.0
 *  @license Simplified BSD License
 *  @copyright 2012 Samsung R&D Institute Russia, 2016 Moscow Institute of Physics and Technology
 */

/*! @mainpage SIGSEGV/SIGABRT handler which prints the debug stack trace.
 *  Example
 *  =======
 *  ~~~~{.cc}
 *  #include "death_handler.h"
 *
 *  int main() {
 *    Debug::DeathHandler dh;
 *    int* p = NULL;
 *    *p = 0;
 *    return 0;
 *  }
 *  ~~~~
 *
 *  Underlying code style is very similar to [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml). It is checked with cpplint.py.
 */
//}}}
#pragma once

#ifdef _WIN32
  //{{{  minimal class
  namespace Debug {
    class cCrash {
    public:
      cCrash() {}
      ~cCrash() {}
      };

    class cCrashSafe {
    public:
      cCrashSafe() {}
      ~cCrashSafe() {}
      };
    }
  //}}}
#else

//{{{  includes
#include <stddef.h>
#include <unistd.h>
#include <assert.h>
//}}}

namespace Debug {
  class cCrash{
  //{{{  description
  //  This class installs a SEGFAULT signal handler to print
  // a nice stack trace and (if requested) generate a core dump.
  // @details In cCrashSafe's constructor, a SEGFAULT signal handler
  // is installed via sigaction(). If your program encounters a segmentation
  // fault, the call stack is unwinded with backtrace(), converted into
  // function names with line numbers via addr2line (fork() + execlp()).
  // Addresses from shared libraries are also converted thanks to dladdr().
  // All C++ symbols are demangled. Printed stack trace includes the faulty
  // thread id obtained with pthread_self() and each line contains the process
  // id to distinguish several stack traces printed by different processes at
  // the same time.
  //}}}

  public:
    typedef ssize_t (*OutputCallback)(const char*, size_t);
    //{{{
    // Installs the SIGSEGV/etc. signal handler.
    // - altstack If true, allocate and use a dedicated signal handler stack.
    //   backtrace() will report nothing then, but the handler will survive a stack overflow.
    cCrash();
    //}}}
    //{{{
    //  This is called on normal program termination. Previously installed
    // SIGSEGV and SIGABRT signal handlers are removed.
    ~cCrash();
    //}}}

   private:
    static void handleSignal (int sig, void* info, void* secret);

    // static vars
    static inline const size_t kNeededMemory = 16384;

    static inline int frames_count_ = 16;
    static inline char* memoryAlloc = NULL;
    };
  }

#endif
