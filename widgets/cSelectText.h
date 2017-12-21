// cSelectText.h
//{{{  includes
#pragma once
#include "cWidget.h"
//}}}

class cSelectText : public cWidget {
public:
  //{{{
  cSelectText (std::string text, int myValue, int& value, bool& changed, float width) :
    cWidget (width), mText(text), mMyValue(myValue), mValue(value), mChanged(changed) {}
  //}}}
  //{{{
  cSelectText (std::string text, int myValue, int& value, bool& changed, float width, float height) :
    cWidget (width, height), mText(text), mMyValue(myValue), mValue(value), mChanged(changed) {}
  //}}}
  virtual ~cSelectText() {}

  //{{{
  void onDown (int16_t x, int16_t y) {
    cWidget::onDown (x, y);
    if (mMyValue != mValue) {
      mValue = mMyValue;
      mChanged = true;
      }
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {
    draw->drawText (mOn ? COL_YELLOW : (mMyValue == mValue) ? COL_WHITE : COL_GREY, getFontHeight(), mText, mX+2, mY+1, mWidth-1, mHeight-1);
    }
  //}}}

private :
  std::string mText;
  int mMyValue;
  int& mValue;
  bool& mChanged;
  };
