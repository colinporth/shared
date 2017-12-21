// cNumBox.h
//{{{  includes
#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "../utils/utils.h"
#include "cWidget.h"
//}}}

class cNumBox : public cWidget {
public:
  //{{{
  cNumBox (std::string text, float& value, bool& changed, float width) :
    cWidget (COL_GREY, width, 1.0f), mValue(value), mChanged(changed), mText(text) { mChanged = false; }
  //}}}
  virtual ~cNumBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    cWidget::onDraw (draw);
    draw->drawText (mTextColour, getFontHeight(), mText + dec(int(mValue)), mX+2, mY+1, mWidth-1, mHeight-1);
    }
  //}}}

private :
  float& mValue;
  bool& mChanged;

  std::string mText;
  uint32_t mTextColour = COL_DARKGREY;
  };
