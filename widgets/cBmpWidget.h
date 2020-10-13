// cBitmapWidget.h
//{{{  includes
#pragma once
#include "cPicWidget.h"
//}}}

class cBmpWidget : public cPicWidget {
public:
  cBmpWidget (const uint8_t* bmp, float width, float height, int index,
              std::function<void (cPicWidget* widget, int index)> hitCallback = [](cPicWidget*, int) {}))
      : cPicWidget (width, height, index, hitCallback) {

    setPic ((uint8_t*)bmp+54, *(bmp + 0x12), *(bmp + 0x16), 3);
    }

  cBmpWidget (const uint8_t* bmp, int index,
              std::function<void (cPicWidget* widget, int index)> hitCallback = [](cPicWidget*, int) {}))
      : cPicWidget ((*(bmp + 0x12)) / (float)getBoxHeight(), (*(bmp + 0x16)) / (float)getBoxHeight(),
                    index, hitCallback)  {

    setPic ((uint8_t*)bmp+54, *(bmp + 0x12), *(bmp + 0x16), 3);
    }

  virtual ~cBmpWidget() {}
  };
