#pragma once
//{{{  includes
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//}}}

class iPic {
public:
  virtual ~iPic() {}

  virtual uint16_t getWidth() = 0;
  virtual uint16_t getHeight() = 0;
  virtual uint16_t getComponents() = 0;
  virtual uint8_t* getPic() = 0;

  virtual void setPic (const uint8_t* buffer, int size, uint16_t components) = 0;
  };
