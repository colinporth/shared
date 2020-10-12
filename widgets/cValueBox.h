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
  void onDown (const cPointF& p) {

    cWidget::onDown (p);
    if (mPixSize.x > mPixSize.y)
      setValue (p.x / mPixSize.x);
    else
      setValue (p.y / mPixSize.y);
    }
  //}}}
  //{{{
  void onMove (const cPointF& p, const cPointF& inc) {
    cWidget::onMove (p, inc);
    if (mPixSize.x > mPixSize.y)
      setValue (p.x / mPixSize.x);
    else
      setValue (p.y / mPixSize.y);
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {

    if (mPixSize.x > mPixSize.y)
      draw->drawRect (mOn ? kLightRedF : mColour, mPixOrg, cPointF(mPixSize.x * limitValue (mValue), mPixSize.y));
    else
      draw->drawRect (mOn ? kLightRedF : mColour, mPixOrg, cPointF(mPixSize.x, mPixSize.y * limitValue (mValue)));
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
