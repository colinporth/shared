#include "stdio.h"
#include "death.h"

void testProc() {
  printf ("Hello colin in proc\n");

  int* p = NULL;
  *p = 0;
  }

int main() {
  Debug::cDeath death;
  printf ("Hello colin\n");
  testProc();
  return 0;
  }
