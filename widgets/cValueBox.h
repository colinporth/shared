// cValueBox.h
//{{{  includes
#pragma once
#include "cWidget.h"
//}}}

class cValueBox : public cWidget {
public:
  cValueBox (float& value, bool& changed, uint32_t colour, float width, float height) :
    cWidget (colour, width, height), mValue(value), mChanged(changed) { mChanged = false; }
  virtual ~cValueBox() {}

  //{{{
  void onDown (int16_t x, int16_t y) {

    cWidget::onDown (x, y);
    if (mWidth > mHeight)
      setValue ((float)x / (float)mWidth);
    else
      setValue ((float)y / (float)mHeight);
    }
  //}}}
  //{{{
  void onMove (int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc) {
    cWidget::onMove (x, y, z, xinc, yinc);
    if (mWidth > mHeight)
      setValue ((float)x / (float)mWidth);
    else
      setValue ((float)y / (float)mHeight);
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {

    if (mWidth > mHeight)
      draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX, mY, mWidth * limitValue (mValue), mHeight);
    else
      draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX, mY, mWidth, mHeight * limitValue (mValue));
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
