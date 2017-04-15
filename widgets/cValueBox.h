// cValueBox.h
#pragma once
#include "cWidget.h"

class cValueBox : public cWidget {
public:
  cValueBox (float& value, bool& changed, uint32_t colour, float width, float height) :
    cWidget (colour, width, height), mValue(value), mChanged(changed) { mChanged = false; }
  virtual ~cValueBox() {}

  //{{{
  virtual void pressed (int16_t x, int16_t y, bool controlled) {

    cWidget::pressed (x, y, controlled);
    if (mWidth > mHeight)
      setValue ((float)x / (float)mWidth);
    else
      setValue ((float)y / (float)mHeight);
    }
  //}}}
  //{{{
  virtual void moved (int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc, bool controlled) {
    cWidget::moved (x, y, z, xinc, yinc, controlled);
    if (mWidth > mHeight)
      setValue ((float)x / (float)mWidth);
    else
      setValue ((float)y / (float)mHeight);
    }
  //}}}

  //{{{
  virtual void render (iDraw* draw) {

    if (mWidth > mHeight)
      draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX, mY, int(mWidth * limitValue (mValue)), mHeight);
    else
      draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX, mY, mWidth, int(mHeight * limitValue (mValue)));
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
