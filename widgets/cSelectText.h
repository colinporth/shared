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
  void onDown (const cPointF& p) {
    cWidget::onDown (p);
    if (mMyValue != mValue) {
      mValue = mMyValue;
      mChanged = true;
      }
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {
    draw->drawText (mOn ?kYellowF : (mMyValue == mValue) ? kWhiteF : kGreyF, getFontHeight(), mText,
                    mPixOrg +cPointF(2.f, 1.f), mPixSize - cPointF(1.f, 1.f));
    }
  //}}}

private :
  std::string mText;
  int mMyValue;
  int& mValue;
  bool& mChanged;
  };
