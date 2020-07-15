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
//{{{  includes
#pragma once

#include <stddef.h>
#include <unistd.h>
#include <assert.h>
//}}}

extern "C" {
  void* __malloc_impl (size_t size);
  void* malloc (size_t size) throw();
  void free (void* ptr) throw();
  }

namespace Debug {
  class cCrash {
  //{{{  description
  //  This class installs a SEGFAULT signal handler to print
  // a nice stack trace and (if requested) generate a core dump.
  // @details In cCrash's constructor, a SEGFAULT signal handler
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
    //  Installs the SIGSEGV/etc. signal handler.
    // @param altstack If true, allocate and use a dedicated signal handler stack.
    // backtrace() will report nothing then, but the handler will survive a stack
    // overflow.
    cCrash (bool altstack = false);
    //}}}
    //{{{
    //  This is called on normal program termination. Previously installed
    // SIGSEGV and SIGABRT signal handlers are removed.
    ~cCrash();
    //}}}

    //{{{  gets
    bool generate_core_dump() const { return generate_core_dump_; }
    bool leanup() const { return cleanup_; }
    bool quick_exit() const { return quick_exit_; }
    int frames_count() const { return frames_count_; }
    bool cut_common_path_root() const { return cut_common_path_root_; }
    bool cut_relative_paths() const { return cut_relative_paths_; }
    bool append_pid() const { return append_pid_; }
    bool thread_safe() const { return thread_safe_; }
    OutputCallback output_callback() const { return output_callback_; }
    //}}}
    //{{{  sets
    void set_generate_core_dump (bool value) { generate_core_dump_ = value; }
    void set_cleanup (bool value) { cleanup_ = value; }
    void set_quick_exit (bool value) { quick_exit_ = value; }
    void set_frames_count (int value) { assert(value > 0 && value <= 100); frames_count_ = value; }
    void set_cut_common_path_root (bool value) { cut_common_path_root_ = value; }
    void set_cut_relative_paths (bool value) { cut_relative_paths_ = value; }
    void set_append_pid (bool value) { append_pid_ = value; }
    void set_thread_safe (bool value) { thread_safe_ = value; }
    void set_output_callback (cCrash::OutputCallback value) { output_callback_ = value; }
    //}}}

   private:
    friend void* ::__malloc_impl (size_t);
    friend void* ::malloc (size_t) throw();
    friend void ::free (void*) throw();

    static inline void print (const char* msg, size_t len = 0);

    static void handleSignal (int sig, void* info, void* secret);

    // static vars
    static OutputCallback output_callback_;

    static inline const size_t kNeededMemory = 16384;

    static inline int frames_count_ = 16;
    static inline bool generate_core_dump_ = true;

    static inline bool cleanup_ = true;
    static inline bool thread_safe_ = true;

    static inline bool append_pid_ = true;
    static inline bool cut_common_path_root_ = true;
    static inline bool cut_relative_paths_ = true;

    static inline bool quick_exit_ = false;

    static inline char* memory_ = NULL;
    static inline void* malloc_ = NULL;
    static inline void* free_ = NULL;
    static inline bool heap_trap_active_ = false;
    };
  }
