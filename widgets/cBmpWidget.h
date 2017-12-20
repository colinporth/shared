// cBitmapWidget.h
#pragma once
#include "cPicWidget.h"

class cBmpWidget : public cPicWidget {
public:
  //{{{
  cBmpWidget (const uint8_t* bmp, float width, float height, int myValue, int& value, bool& changed)
      : cPicWidget (width, height, myValue, value, changed) {
    setPic ((uint8_t*)bmp+54, *(bmp + 0x12), *(bmp + 0x16), 3);
    }
  //}}}
  //{{{
  cBmpWidget (const uint8_t* bmp, int myValue, int& value, bool& changed)
      : cPicWidget ((*(bmp + 0x12)) / (float)getBoxHeight(),
                    (*(bmp + 0x16)) / (float)getBoxHeight(),
                    myValue, value, changed) {
    setPic ((uint8_t*)bmp+54, *(bmp + 0x12), *(bmp + 0x16), 3);
    }
  //}}}
  virtual ~cBmpWidget() {}
  };
