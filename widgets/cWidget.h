// cWidget.h - base widget, draws as box with width-1, height-1
//{{{  includes
#pragma once

class cContainer;
#include "iDraw.h"
//}}}

class cWidget {
public:
  static uint16_t getBoxHeight()       { return 19; }
  static uint16_t getSmallFontHeight() { return 16; }
  static uint16_t getFontHeight()      { return 18; }
  static uint16_t getBigFontHeight()   { return 40; }

  static float getPixel() { return 1.0f / getBoxHeight(); }

  cWidget() {}
  //{{{
  cWidget (float width)
    : mWidth(width * getBoxHeight()), mHeight(getBoxHeight()) {}
  //}}}
  //{{{
  cWidget (float width, float height)
    : mWidth(width * getBoxHeight()), mHeight(height * getBoxHeight()) {}
  //}}}
  //{{{
  cWidget (uint32_t colour, float width, float height)
    : mColour(colour), mWidth(width * getBoxHeight()), mHeight(height * getBoxHeight()) {}
  //}}}
  virtual ~cWidget() {}

//  gets
  float getX() { return mX / getBoxHeight(); }
  float getY() { return mY / getBoxHeight(); }
  float getWidth() { return mWidth / getBoxHeight(); }
  float getHeight() { return mHeight / getBoxHeight(); }

  float getPixX() { return mX; }
  float getPixY() { return mY; }
  float getPixWidth() { return mWidth; }
  float getPixHeight() { return mHeight; }

  int getPressedCount() { return mPressedCount; }
  bool isPressed() { return mPressedCount > 0; }
  bool isOn() { return mOn; }
  bool isVisible() { return mVisible; }

  //{{{  sets
  //{{{
  void setXY (float x, float y) {
    setPixXY (x*getBoxHeight(), y*getBoxHeight());
    }
  //}}}

  //{{{
  void setPixXY (float x, float y) {
    mX = x;
    mY = y;
    }
  //}}}
  //{{{
  void setPixWidth (float width) {
    mWidth = width;
    }
  //}}}
  //{{{
  void setPixHeight (float height) {
    mHeight = height;
    }
  //}}}
  //{{{
  void setPixSize (float width, float height) {
    mWidth = width;
    mHeight = height;
    }
  //}}}

  void setColour (uint32_t colour) { mColour = colour; }
  void setParent (cContainer* parent) { mParent = parent; }

  void setOn (bool on) { mOn = on; }
  void setVisible (bool visible) { mVisible = visible; }
  //}}}

  //{{{
  virtual cWidget* isPicked (float x, float y) {
    return (x >= mX) && (x < mX + mWidth) &&
           (y >= mY) && (y < mY + mHeight) ? this : nullptr;
    }
  //}}}

  virtual void onProx (float x, float y) {}
  //{{{
  virtual void onDown (float x, float y) {
    if (!mPressedCount)
      mOn = true;
    mPressedCount++;
    }
  //}}}
  virtual void onMove (float x, float y, float xinc, float yinc) {}
  //{{{
  virtual void onUp() {
    mPressedCount = 0;
    mOn = false;
    }
  //}}}
  virtual void onWheel (float delta) {}

  //{{{
  virtual void onDraw (iDraw* draw) {
    draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX+1.f, mY+1.f, mWidth-1.f, mHeight-1.f);
    }
  //}}}

protected:
  uint32_t mColour = COL_LIGHTGREY;

  float mX = 0;
  float mY = 0;
  float mWidth = 0;
  float mHeight = 0;

  int mPressedCount = 0;
  bool mOn = false;

  cContainer* mParent = nullptr;
  bool mVisible = true;
  };
