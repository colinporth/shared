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

#include <signal.h>
#include <pthread.h>

#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <bfd.h>
#include <link.h>
//}}}

namespace Debug {
  typedef void (*sa_sigaction_handler) (int, siginfo_t*, void*);

  //{{{
  class FileMatch {
  public:
     FileMatch (void* addr) : mAddress(addr), mFile(NULL), mBase(NULL) {}

     void*       mAddress;
     const char* mFile;
     void*       mBase;
    };
  //}}}
  //{{{
  static int findMatchingFile (struct dl_phdr_info* info, size_t size, void* data) {

    FileMatch* match = (FileMatch*)data;

    for (uint32_t i = 0; i < info->dlpi_phnum; i++) {
      const ElfW(Phdr)& phdr = info->dlpi_phdr[i];

      if (phdr.p_type == PT_LOAD) {
        ElfW(Addr) vaddr = phdr.p_vaddr + info->dlpi_addr;
        ElfW(Addr) maddr = ElfW(Addr)(match->mAddress);
        if ((maddr >= vaddr) && ( maddr < vaddr + phdr.p_memsz)) {
          match->mFile = info->dlpi_name;
          match->mBase = (void*)info->dlpi_addr;
          return 1;
          }
        }
      }

    return 0;
    }
  //}}}
  //{{{
  static asymbol** slurpSymbolTable (bfd* abfd, const char* fileName) {

    if (!(bfd_get_file_flags (abfd) & HAS_SYMS)) {
      printf ("Error - bfd file %s has no symbols\n", fileName);
      return NULL;
      }

    asymbol** syms;
    unsigned int size;

    long symcount = bfd_read_minisymbols (abfd, false, (void**)&syms, &size);
    if (symcount == 0)
      symcount = bfd_read_minisymbols (abfd, true, (void**)&syms, &size);

    if (symcount < 0) {
      printf ("Error - bfd file %s found no symbols\n", fileName);
      return NULL;
      }

    return syms;
    }
  //}}}

  //{{{
  class FileLineDesc {
  public:
    FileLineDesc (asymbol** syms, bfd_vma pc) : mPc(pc), mFound(false), mSyms(syms) {}

    void findAddressInSection (bfd* abfd, asection* section) {
      if (mFound)
        return;

      if ((bfd_section_flags (section) & SEC_ALLOC) == 0 )
        return;

      bfd_vma vma = bfd_section_vma (section);
      if (mPc < vma )
        return;

      bfd_size_type size = bfd_section_size (section);
      if (mPc >= (vma + size))
        return;

      mFound = bfd_find_nearest_line (abfd, section, mSyms, (mPc - vma),
                                      (const char**)&mFilename, (const char**)&mFunctionname, &mLine );
      }

    bfd_vma      mPc;
    char*        mFilename;
    char*        mFunctionname;
    unsigned int mLine;
    int          mFound;
    asymbol**    mSyms;
    };
  //}}}
  //{{{
  static void findAddressInSection (bfd* abfd, asection* section, void* data) {

    FileLineDesc* desc = (FileLineDesc*)data;
    assert (desc);
    return desc->findAddressInSection (abfd, section);
    }
  //}}}

  //{{{
  static char** translateAddressesBuf (bfd* abfd, bfd_vma* addr, int numAddr, asymbol** syms) {

    char** ret_buf = NULL;
    int32_t total = 0;

    char b;
    char* buf = &b;
    int32_t len = 0;

    for (uint32_t state = 0; state < 2; state++) {
      if (state == 1) {
        ret_buf = (char**)malloc( total + ( sizeof(char*) * numAddr ));
        buf = (char*)(ret_buf + numAddr);
        len = total;
        }

      for (int32_t i = 0; i < numAddr; i++) {
        FileLineDesc desc (syms, addr[i]);

        if (state == 1)
          ret_buf[i] = buf;

        bfd_map_over_sections (abfd, findAddressInSection, (void*)&desc);

        if (!desc.mFound) {
          total += snprintf( buf, len, "[0x%llx] \?\? \?\?:0", (long long unsigned int) addr[i] ) + 1;
          }
        else {
          char* realname = NULL;
          const char* name = desc.mFunctionname;
          if (name == NULL || *name == '\0')
             name = "??";
          else {
            int status;
            realname = abi::__cxa_demangle(name, 0, 0, &status);
            if (status == 0)
              name = realname;
            }
          if (desc.mFilename != NULL) {
            char* h = strrchr (desc.mFilename, '/');
            if (h != NULL)
              desc.mFilename = h + 1;
            }
          total += snprintf (buf, len, "%s:%u %s", desc.mFilename ? desc.mFilename : "??", desc.mLine, name) + 1;
          free (realname);

          // elog << "\"" << buf << "\"\n";
          }
        }

      if (state == 1)
        buf = buf + total + 1;
      }

    return ret_buf;
    }
  //}}}
  //{{{
  static char** processFile (const char* fileName, bfd_vma* addr, int naddr) {

     bfd* abfd = bfd_openr (fileName, NULL);
     if (!abfd) {
       printf ("Error - openng bfd file %s\n", fileName);
       return NULL;
       }

     if (bfd_check_format (abfd, bfd_archive)) {
       printf ( "Cannot get addresses from archive %s\n", fileName);
       bfd_close (abfd);
       return NULL;
       }

     char** matching;
     if (!bfd_check_format_matches (abfd, bfd_object, &matching)) {
       printf ("Format does not match for archive %s\n", fileName);
       bfd_close (abfd);
       return NULL;
       }

     asymbol** syms = slurpSymbolTable (abfd, fileName);
     if (!syms) {
       printf ("Failed to read symbol table for archive %s\n", fileName);
       bfd_close (abfd);
       return NULL;
       }

     char** retBuf = translateAddressesBuf (abfd, addr, naddr, syms);

     free (syms);

     bfd_close (abfd);
     return retBuf;
    }
  //}}}

  //{{{
  char** backtraceSymbols (void* const* addrList, int numAddr) {

    char*** locations = (char***)alloca (sizeof(char**)*numAddr);

    // initialize the bfd library
    bfd_init();

    int total = 0;
    uint32_t idx = numAddr;
    for (int32_t i = 0; i < numAddr; i++) {
      // find which executable, or library the symbol is from
      FileMatch match (addrList[--idx] );
      dl_iterate_phdr (findMatchingFile, &match);

      // adjust the address in the global space of your binary to an
      // offset in the relevant library
      bfd_vma addr  = (bfd_vma)(addrList[idx]);
      addr -= (bfd_vma)(match.mBase);

      // lookup the symbol
      if (match.mFile && strlen (match.mFile))
        locations[idx] = processFile (match.mFile, &addr, 1);
      else
        locations[idx] = processFile ("/proc/self/exe", &addr, 1);

      total += strlen (locations[idx][0]) + 1;
      }

    // return all the file and line information for each address
    char** final = (char**)malloc (total + (numAddr * sizeof(char*)));
    char* f_strings = (char*)(final + numAddr);

    for (int32_t i = 0; i < numAddr; i++ ) {
      strcpy (f_strings, locations[i][0] );
      free (locations[i]);
      final[i] = f_strings;
      f_strings += strlen (f_strings) + 1;
      }

    return final;
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

    //{{{  report signal
    switch (sig) {
      case SIGSEGV:
        printf ("--------- crashed SIGSEGV thread:%lx pid:%d\n", pthread_self(), getppid());
        break;

      case SIGABRT:
        printf ("--------- crashed SIGABRT thread:%lx pid:%d\n", pthread_self(), getppid());
        break;

      case SIGFPE:
        printf ("--------- crashed SIGFPE thread:%lx pid:%d\n", pthread_self(), getppid());
        break;

      default:
        printf ("--------- crashed signal:%d thread:%lx pid:%d\n", sig, pthread_self(), getppid());
        break;
      }
    //}}}

    // get backtrace
    void** trace = reinterpret_cast<void**>(memoryAlloc);

    int traceSize = backtrace (trace, frames_count_ + 2);
    char** symbols = backtraceSymbols (trace, traceSize);
    if (traceSize <= 2)
      abort();

    //  Overwrite sigaction with caller's address
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

    int stackOffset = trace[2] == trace[1] ? 2 : 1;

    for (int i = stackOffset; i < traceSize; i++)
      printf ("%s\n", symbols[i]);

    _Exit (EXIT_SUCCESS);
    }
  //}}}
  }
