#include "stdio.h"
#include "crash.h"
#include "../utils/cLog.h"

void testProc() {

  cLog::log (LOGINFO, "Hello colin in proc");
  int* p = NULL;
  *p = 0;
  }

int main() {
  cLog::init (LOGINFO, false, "");

  cCrash crash;

  cLog::log (LOGINFO, "Hello colin");
  testProc();
  return 0;
  }
