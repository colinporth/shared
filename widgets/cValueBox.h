// cValueBox.h
//{{{  includes
#pragma once
#include "cWidget.h"
//}}}

class cValueBox : public cWidget {
public:
  cValueBox (float& value, bool& changed, const sColourF& colour, float width, float height) :
    cWidget (colour, width, height), mValue(value), mChanged(changed) { mChanged = false; }
  virtual ~cValueBox() {}

  //{{{
  void onDown (float x, float y) {

    cWidget::onDown (x, y);
    if (mWidth > mHeight)
      setValue (x / mWidth);
    else
      setValue (y / mHeight);
    }
  //}}}
  //{{{
  void onMove (float x, float y, float xinc, float yinc) {
    cWidget::onMove (x, y, xinc, yinc);
    if (mWidth > mHeight)
      setValue (x / mWidth);
    else
      setValue (y / mHeight);
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {

    if (mWidth > mHeight)
      draw->drawRect (mOn ? kLightRedF : mColour, cPoint(mX, mY), cPoint(mWidth * limitValue (mValue), mHeight));
    else
      draw->drawRect (mOn ? kLightRedF : mColour, cPoint(mX, mY), cPoint(mWidth, mHeight * limitValue (mValue)));
    }
  //}}}

private :
  //{{{
  void setValue (float value) {

    value = limitValue (value);
    if (value != mValue) {
      mValue = value;
      mChanged = true;
      }
    }
  //}}}
  float limitValue (float value) { return value > mMaxValue ? mMaxValue : (value < mMinValue ? mMinValue : value); }

  float& mValue;
  bool& mChanged;

  float mMinValue = 0.0f;
  float mMaxValue = 1.0f;
  };
