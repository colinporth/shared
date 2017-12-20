// cNumBox.h
#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "cWidget.h"

//{{{
static std::string intStr (int value, int width = 0, char fill = ' ') {

  std::ostringstream os;
  if (width)
    os << std::setfill (fill) << std::setw (width) << value;
  else
    os << value;
  return os.str();
  }
//}}}

class cNumBox : public cWidget {
public:
  cNumBox (std::string text, float& value, bool& changed, float width) :
    cWidget (COL_GREY, width, 1.0f), mValue(value), mChanged(changed), mText(text) { mChanged = false; }
  virtual ~cNumBox() {}

  virtual void onDraw (iDraw* draw) {

    cWidget::onDraw (draw);
    draw->drawText (mTextColour, getFontHeight(), mText + intStr (int(mValue)), mX+2, mY+1, mWidth-1, mHeight-1);
    }

private :
  float& mValue;
  bool& mChanged;

  std::string mText;
  uint32_t mTextColour = COL_DARKGREY;
  };
