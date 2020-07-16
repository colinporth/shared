#include "stdio.h"
#include "crash.h"

void testProc() {

  printf ("Hello colin in proc\n");
  int* p = NULL;
  *p = 0;
  }

int main() {
  Debug::cCrash crash;

  printf ("Hello colin\n");
  testProc();
  return 0;
  }
