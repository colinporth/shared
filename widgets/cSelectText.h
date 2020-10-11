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
  void onDown (float x, float y) {
    cWidget::onDown (x, y);
    if (mMyValue != mValue) {
      mValue = mMyValue;
      mChanged = true;
      }
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {
    draw->drawText (mOn ? COL_YELLOW : (mMyValue == mValue) ? COL_WHITE : COL_GREY, getFontHeight(), mText, 
                    cPoint(mX+2.f, mY+1.f), cPoint(mWidth-1.f, mHeight-1.f));
    }
  //}}}

private :
  std::string mText;
  int mMyValue;
  int& mValue;
  bool& mChanged;
  };
