//{{{  description
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

/*! @file death_handler.cc
 *   Implementation of the SIGSEGV/SIGABRT handler which prints the debug
 *  stack trace.
 *  @author Markovtsev Vadim <gmarkhor@gmail.com>
 *  @version 1.0
 *  @license Simplified BSD License
 *  @copyright 2012 Samsung R&D Institute Russia, 2016 Moscow Institute of Physics and Technology
 */
//}}}
//{{{  includes
#include "crash.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>

#include <pthread.h>
#include <signal.h>

#define INLINE __attribute__((always_inline)) inline
//}}}

namespace Debug {
  INLINE void print (const char *msg, size_t len = 0) {
    if (len > 0)
      write (STDERR_FILENO, msg, len);
    else
      write (STDERR_FILENO, msg, strlen (msg));
    }
  }

extern "C" {
  //{{{
  void* __malloc_impl (size_t size) {

    char* malloc_buffer = Debug::cCrashSafe::memory_ + Debug::cCrashSafe::kNeededMemory - 512;
    if (size > 512U) {
      const char* msg = "malloc() replacement function should not return "
                        "a memory block larger than 512 bytes\n";
      Debug::print (msg, strlen(msg) + 1);
      _Exit(EXIT_FAILURE);
      }

    return malloc_buffer;
    }
  //}}}
  //{{{
  void* malloc (size_t size) throw() {

    if (!Debug::cCrashSafe::heap_trap_active_) {
      if (!Debug::cCrashSafe::malloc_)
        Debug::cCrashSafe::malloc_ = dlsym (RTLD_NEXT, "malloc");

      return ((void*(*)(size_t))Debug::cCrashSafe::malloc_)(size);
      }

    return __malloc_impl(size);
    }
  //}}}
  //{{{
  void free (void* ptr) throw() {

    if (!Debug::cCrashSafe::heap_trap_active_) {
      if (!Debug::cCrashSafe::free_)
        Debug::cCrashSafe::free_ = dlsym(RTLD_NEXT, "free");
      ((void(*)(void*))Debug::cCrashSafe::free_)(ptr);
      }
    // no-op
    }
  //}}}
  }

#define checked(x) do { if ((x) <= 0) _Exit(EXIT_FAILURE); } while (false)

namespace Debug {
  typedef void (*sa_sigaction_handler) (int, siginfo_t*, void*);
  namespace Safe {
    // non heap libc functions
    //{{{
    INLINE char* itoa (int val, char* memory, int base = 10) {

      char* res = memory;
      if (val == 0) {
        res[0] = '0';
        res[1] = '\0';
        return res;
        }

      const int res_max_length = 32;
      int i;
      bool negative = val < 0;
      res[res_max_length - 1] = 0;
      for (i = res_max_length - 2; val != 0 && i != 0; i--, val /= base)
        res[i] = "0123456789ABCDEF"[val % base];

      if (negative)
        res[i--] = '-';

      return &res[i + 1];
      }
    //}}}
    //{{{
    INLINE char* utoa (uint64_t val, char* memory, int base = 10) {
    //  Converts an unsigned integer to a preallocated string.
    // @pre base must be less than or equal to 16.

      char* res = memory;
      if (val == 0) {
        res[0] = '0';
        res[1] = '\0';
        return res;
        }

      const int res_max_length = 32;
      res[res_max_length - 1] = 0;
      int i;
      for (i = res_max_length - 2; val != 0 && i != 0; i--, val /= base)
        res[i] = "0123456789abcdef"[val % base];

      return &res[i + 1];
      }
    //}}}
    //{{{
    INLINE char* ptoa (const void* val, char* memory) {
    //  Converts a pointer to a preallocated string.

      char* buf = utoa (reinterpret_cast<uint64_t>(val), memory + 32, 16);
      char* result = memory;  // 32

      strcpy (result + 2, buf);
      result[0] = '0';
      result[1] = 'x';
      return result;
      }
    //}}}
    }

  //{{{
  INLINE static void safe_abort() {

    struct sigaction sa;
    sigaction (SIGABRT, NULL, &sa);

    kill (getppid(), SIGCONT);

    sa.sa_handler = SIG_DFL;
    sigaction (SIGABRT, &sa, NULL);

    abort();
    }
  //}}}
  //{{{
  static char* addr2line (const char* image, void* addr, char** memory) {
  // invoke addr2line utility
  // - for function name and line information from an address in the code segment.

    int pipefd[2];
    if (pipe (pipefd) != 0)
      safe_abort();

    pid_t pid = fork();
    if (pid == 0) {
      close (pipefd[0]);
      dup2 (pipefd[1], STDOUT_FILENO);
      dup2 (pipefd[1], STDERR_FILENO);
      if (execlp ("addr2line", "addr2line",
                  Safe::ptoa (addr, *memory), "-f", "-C", "-e", image,
                  reinterpret_cast<void*>(NULL)) == -1) {
        safe_abort();
        }
      }

    close (pipefd[1]);
    const int line_max_length = 4096;
    char* line = *memory;
    *memory += line_max_length;
    ssize_t len = read (pipefd[0], line, line_max_length);
    close (pipefd[0]);

    if (len == 0)
      safe_abort();
    line[len] = 0;

    if (waitpid (pid, NULL, 0) != pid)
      safe_abort();

    if (line[0] == '?') {
      char* straddr = Safe::ptoa (addr, *memory);
      strcpy (line, "\033[32;1m");
      strcat (line, straddr);
      strcat (line, "\033[0m");
      strcat (line, " at ");
      strcat (line, image);
      strcat (line, " ");
      }
    else {
      if (*(strstr (line, "\n") + 1) == '?') {
        char* straddr = Safe::ptoa (addr, *memory);
        strcpy (strstr(line, "\n") + 1, image);
        strcat (line, ":");
        strcat (line, straddr);
        strcat (line, "\n");
        }
      }

    return line;
    }
  //}}}

  // cCrash
  //{{{
  cCrash::cCrash() {

    if (memoryAlloc == NULL)
      memoryAlloc = new char[kNeededMemory];

    struct sigaction sa;
    sa.sa_sigaction = (sa_sigaction_handler)handleSignal;
    sigemptyset (&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction (SIGSEGV, &sa, NULL) < 0)
      perror ("cCrash - sigaction(SIGSEGV)");

    if (sigaction (SIGABRT, &sa, NULL) < 0)
      perror ("cCrash - sigaction(SIGABBRT)");

    if (sigaction (SIGFPE, &sa, NULL) < 0)
      perror ("cCrash - sigaction(SIGFPE)");
    }
  //}}}
  //{{{
  cCrash::~cCrash() {

    // Disable alternative signal handler stack
    stack_t altstack;

    altstack.ss_sp = NULL;
    altstack.ss_size = 0;
    altstack.ss_flags = SS_DISABLE;
    sigaltstack (&altstack, NULL);

    struct sigaction sa;

    sigaction (SIGSEGV, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGSEGV, &sa, NULL);

    sigaction (SIGABRT, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGABRT, &sa, NULL);

    sigaction (SIGFPE, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGFPE, &sa, NULL);

    delete[] memoryAlloc;
    }
  //}}}
  //{{{
  void cCrash::handleSignal (int sig, void* info, void* secret) {

    //{{{  report signal, thread, pid
    switch (sig) {
      case SIGSEGV:
        printf ("--------- crashed SIGSEGV thread:%ld pid:%d\n", pthread_self(), getppid());
        break;

      case SIGABRT:
        printf ("--------- crashed SIGABRT thread:%ld pid:%d\n", pthread_self(), getppid());
        break;

      case SIGFPE:
        printf ("--------- crashed SIGFPE thread:%ld pid:%d\n", pthread_self(), getppid());
        break;

      default:
        printf ("--------- crashed signal:%d thread:%ld pid:%d\n", sig, pthread_self(), getppid());
        break;
      }
    //}}}
    //{{{  get backtrace
    void** trace = reinterpret_cast<void**>(memoryAlloc);

    int traceSize = backtrace (trace, frames_count_ + 2);
    char** symbols = backtrace_symbols (trace, traceSize);
    if (traceSize <= 2)
      abort();
    int stackOffset = trace[2] == trace[1] ? 2 : 1;

    for (int i = stackOffset; i < traceSize; i++) {
      char* mangled_name = NULL;
      char* offset_begin = NULL;
      char* offset_end = NULL;
      for (char* p = symbols[i]; *p; ++p) {
        if (*p == '(')
          mangled_name = p;
        else if (*p == '+')
          offset_begin = p;
        else if (*p == ')') {
          offset_end = p;
          break;
          }
        }

      // if the line could be processed, attempt to demangle the symbol
      printf ("%s\n", symbols[i]);
      if (mangled_name && offset_begin && offset_end && mangled_name < offset_begin) {
        *mangled_name++ = '\0';
        *offset_begin++ = '\0';
        *offset_end++ = '\0';

        int status;
        char* real_name = abi::__cxa_demangle (mangled_name, 0, 0, &status);

        // if demangling is successful, output the demangled function name
        if (status == 0)
          printf ("%s\n", real_name);
        else
          printf ("%s\n", mangled_name);
        }
      }

    // Overwrite sigaction with caller's address
    ucontext_t* uc = reinterpret_cast<ucontext_t*>(secret);
    #if defined(__arm__)
      trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.arm_pc);
    #else
      #if defined(__x86_64__)
        trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.gregs[REG_RIP]);
      #else
        trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.gregs[REG_EIP]);
      #endif
    #endif
    //}}}

    for (int i = stackOffset; i < traceSize; i++) {
      //{{{  addr2line
       //Dl_info dlinfo;
       //bool result = dladdr (trace[i], &dlinfo);
      //char* line;
      //if (result == 0 ||
          //(dlinfo.dli_fname[0] != '/') || !strcmp (name_buf, dlinfo.dli_fname))
        //line = addr2line (name_buf, trace[i], &memory);
      //else
        //line = addr2line (dlinfo.dli_fname, reinterpret_cast<void *>(
                 //reinterpret_cast<char*>(trace[i]) -
                 //reinterpret_cast<char*>(dlinfo.dli_fbase)),
                 //&memory);
      //}}}

      Dl_info dlinfo;
      dladdr (trace[i], &dlinfo);
      int status;
      char* demangled = abi::__cxa_demangle (dlinfo.dli_sname, NULL, 0, &status);
      printf ("%0d %s\n", i, ((status == 0) && demangled) ? demangled : dlinfo.dli_sname);
      free (demangled);
      //printf ("dlinfo    - %p %s %p %s\n", dlinfo.dli_fbase, dlinfo.dli_fname, dlinfo.dli_saddr, symname);
      //printf ("addr2line - %s\n", line);
      }

    _Exit (EXIT_SUCCESS);
    }
  //}}}

  #pragma GCC poison malloc realloc free backtrace_symbols printf fprintf sprintf snprintf scanf sscanf

  //  cCrashSafe
  //{{{
  cCrashSafe::cCrashSafe (bool altstack) {

    if (memory_ == NULL)
      memory_ = new char[kNeededMemory + (altstack ? MINSIGSTKSZ : 0)];

    if (altstack) {
      stack_t altstack;
      altstack.ss_sp = memory_ + kNeededMemory;
      altstack.ss_size = MINSIGSTKSZ;
      altstack.ss_flags = 0;
      if (sigaltstack (&altstack, NULL) < 0) {
        perror ("cCrashSafe - sigaltstack()");
        }
      }

    struct sigaction sa;
    sa.sa_sigaction = (sa_sigaction_handler)handleSignal;
    sigemptyset (&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_SIGINFO | (altstack? SA_ONSTACK : 0);
    if (sigaction (SIGSEGV, &sa, NULL) < 0)
      perror ("cCrashSafe - sigaction(SIGSEGV)");

    if (sigaction (SIGABRT, &sa, NULL) < 0)
      perror ("cCrashSafe - sigaction(SIGABBRT)");

    if (sigaction (SIGFPE, &sa, NULL) < 0)
      perror ("cCrashSafe - sigaction(SIGFPE)");
    }
  //}}}
  //{{{
  cCrashSafe::~cCrashSafe() {

    // Disable alternative signal handler stack
    stack_t altstack;
    altstack.ss_sp = NULL;
    altstack.ss_size = 0;
    altstack.ss_flags = SS_DISABLE;
    sigaltstack (&altstack, NULL);

    struct sigaction sa;

    sigaction (SIGSEGV, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGSEGV, &sa, NULL);

    sigaction (SIGABRT, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGABRT, &sa, NULL);

    sigaction (SIGFPE, NULL, &sa);
    sa.sa_handler = SIG_DFL;
    sigaction (SIGFPE, &sa, NULL);

    delete[] memory_;
    }
  //}}}

  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  //{{{
  void cCrashSafe::handleSignal (int sig, void* /* info */, void* secret) {
  // Stop all other running threads by forking

    pid_t forkedPid = fork();
    if (forkedPid != 0) {
      //{{{  fiddle with threads
      int status;
      if (thread_safe_) {
        // Freeze the original process, until it's child prints the stack trace
        kill (getpid(), SIGSTOP);

        // Wait for the child without blocking and exit as soon as possible,
        // so that no zombies are left.
        waitpid (forkedPid, &status, WNOHANG);
        }

      else
        // Wait for the child, blocking only the current thread.
        // All other threads will continue to run, potentially crashing the parent.
        waitpid (forkedPid, &status, 0);

      if (quick_exit_)
        ::quick_exit (EXIT_FAILURE);

      if (generate_core_dump_) {
        struct sigaction sa;
        sigaction (SIGABRT, NULL, &sa);
        sa.sa_handler = SIG_DFL;
        sigaction (SIGABRT, &sa, NULL);
        abort();
        }
      else if (cleanup_)
        exit (EXIT_FAILURE);
      else
        _Exit (EXIT_FAILURE);
      }
      //}}}

    ucontext_t* uc = reinterpret_cast<ucontext_t*>(secret);

    if (dup2 (STDERR_FILENO, STDOUT_FILENO) == -1)  // redirect stdout to stderr
      print ("Failed to redirect stdout to stderr\n");

    char* memory = memory_;

    //{{{  report signal, thread, pid
    {
    char* msg = memory;
    const int msg_max_length = 128;

    strcpy (msg, "\033[31;1m");

    switch (sig) {
      case SIGSEGV:
        strcat (msg, "Segmentation fault");
        break;

      case SIGABRT:
        strcat (msg, "Aborted");
        break;

      case SIGFPE:
        strcat (msg, "Floating point exception");
        break;

      default:
        strcat (msg, "Caught signal ");
        strcat (msg, Safe::itoa (sig, msg + msg_max_length));
        break;
      }

    strcat (msg, "\033[0m");
    strcat (msg, " (thread ");
    strcat (msg, "\033[33;1m");
    strcat (msg, Safe::utoa (pthread_self(), msg + msg_max_length));

    strcat (msg, "\033[0m");
    strcat (msg, ", pid ");
    strcat (msg, "\033[33;1m");
    strcat (msg, Safe::itoa (getppid(), msg + msg_max_length));

    strcat (msg, "\033[0m");
    strcat (msg, ")");

    print (msg);
    }
    //}}}
    //{{{  get backtrace
    // workaround malloc() inside backtrace()
    print ("\nStack trace:\n");

    void** trace = reinterpret_cast<void**>(memory);
    memory += (frames_count_ + 2) * sizeof(void*);

    heap_trap_active_ = true;
    int trace_size = backtrace (trace, frames_count_ + 2);
    heap_trap_active_ = false;
    if (trace_size <= 2)
      safe_abort();

    // Overwrite sigaction with caller's address
    #if defined(__arm__)
      trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.arm_pc);
    #else
      #if !defined(__i386__) && !defined(__x86_64__)
        #error Only ARM, x86 and x86-64 are supported
      #endif

      #if defined(__x86_64__)
        trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.gregs[REG_RIP]);
      #else
        trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.gregs[REG_EIP]);
      #endif
    #endif
    //}}}
    //{{{  get name_buf
    const int path_max_length = 2048;
    char* name_buf = memory;
    ssize_t name_buf_length = readlink ("/proc/self/exe", name_buf, path_max_length - 1);
    if (name_buf_length < 1)
      safe_abort();

    name_buf[name_buf_length] = 0;
    memory += name_buf_length + 1;
    //}}}
    //{{{  get cwd
    char* cwd = memory;
    if (getcwd (cwd, path_max_length) == NULL)
      safe_abort();
    strcat (cwd, "/");
    memory += strlen(cwd) + 1;
    //}}}

    char* prev_memory = memory;
    int stackOffset = trace[2] == trace[1] ? 2 : 1;
    for (int i = stackOffset; i < trace_size; i++) {
      memory = prev_memory;
      char* line;
      Dl_info dlinf;
      if (dladdr (trace[i], &dlinf) == 0 || dlinf.dli_fname[0] != '/' || !strcmp (name_buf, dlinf.dli_fname))
        line = addr2line (name_buf, trace[i], &memory);
      else
        line = addr2line (dlinf.dli_fname, reinterpret_cast<void *>(
                 reinterpret_cast<char*>(trace[i]) -
                 reinterpret_cast<char*>(dlinf.dli_fbase)),
                 &memory);

      char* function_name_end = strstr (line, "\n");
      if (function_name_end != NULL) {
        *function_name_end = 0;
        {
        char* msg = memory;
        const int msg_max_length = 512;
        strcpy (msg, "\033[34;1m");
        //strcat (msg, Safe::itoa (i, msg + msg_max_length));
        strcat (msg, "[");
        strcat (msg, line);
        strcat (msg, "]");
        if (append_pid_) {
          strcat (msg, "\033[0m\033[33;1m");
          strcat (msg, " (");
          strcat (msg, Safe::itoa (getppid(), msg + msg_max_length));
          strcat (msg, ")");
          strcat (msg, "\033[0m");
          strcat (msg, "\n");
          }
        else {
          strcat (msg, "\033[0m");
          strcat (msg, "\n");
          }
        print (msg);
        }
        line = function_name_end + 1;

        if (cut_common_path_root_) {
          //{{{  remove the common path root
          int cpi;
          for (cpi = 0; cwd[cpi] == line[cpi]; cpi++) {};

          if (line[cpi - 1] != '/')
            for (; line[cpi - 1] != '/'; cpi--) {};

          if (cpi > 1)
            line = line + cpi;
          }
          //}}}
        if (cut_relative_paths_) {
          //{{{  remove relative path root
          char* path_cut_pos = strstr (line, "../");
          if (path_cut_pos != NULL) {
            path_cut_pos += 3;
            while (!strncmp (path_cut_pos, "../", 3))
              path_cut_pos += 3;
            line = path_cut_pos;
            }
          }
          //}}}

        char* number_pos = strstr (line, ":");
        if (number_pos != NULL) {
          //{{{  mark line number
          char* line_number = memory;  // 128
          strcpy (line_number, number_pos);

          // Overwrite the new line char
          line_number[strlen (line_number) - 1] = 0;

          // \033[32;1m%s\033[0m\n
          strcpy (number_pos, "\033[32;1m");
          strcat (line, line_number);
          strcat (line, "\033[0m\n");
          }
          //}}}
        }

      // Overwrite the new line char
      line[strlen (line) - 1] = 0;

      // Append pid
      if (append_pid_) {
        // %s\033[33;1m(%i)\033[0m\n
        strcat (line, " ");
        strcat (line, "\033[33;1m");
        strcat (line, "(");
        strcat (line, Safe::itoa(getppid(), memory));
        strcat (line, ")");
        strcat (line, "\033[0m");
        }

      strcat (line, "\n");
      print (line);
      }

    // Write '\0' to indicate the end of the output
    char end = '\0';
    write (STDERR_FILENO, &end, 1);

    if (thread_safe_) // Resume the parent process
      kill (getppid(), SIGCONT);

    // This is called in the child process
    _Exit (EXIT_SUCCESS);
    }
  //}}}
  #pragma GCC diagnostic pop
  }
