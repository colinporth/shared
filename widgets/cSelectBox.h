// cSelectBox.h
#pragma once
#include "cWidget.h"

class cSelectBox : public cWidget {
public:
  cSelectBox (std::string text, int myValue, int& value, bool& changed, float width) :
    cWidget (width), mText(text), mMyValue(myValue), mValue(value), mChanged(changed) {}
  cSelectBox (std::string text, int myValue, int& value, bool& changed, float width, float height) :
    cWidget (width, height), mText(text), mMyValue(myValue), mValue(value), mChanged(changed) {}
  virtual ~cSelectBox() {}

  virtual void onDown (int16_t x, int16_t y, bool controlled) {
    cWidget::pressed (x, y);
    if (mMyValue != mValue) {
      mValue = mMyValue;
      mChanged = true;
      }
    }

  virtual void onDraw (iDraw* draw) {
    draw->rectClipped (mOn ? COL_YELLOW : mMyValue == mValue ? COL_LIGHTRED : mColour, mX+1, mY+1, mWidth-1, mHeight-1);
    draw->text (mTextColour, getFontHeight(), mText, mX+2, mY+1, mWidth-1, mHeight-1);
    }

private :
  std::string mText;
  int mMyValue;
  int& mValue;
  bool& mChanged;
  uint32_t mTextColour = COL_DARKGREY;
  };
