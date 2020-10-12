// cWidget.h - base widget, draws as box with width-1, height-1
//{{{  includes
#pragma once

class cContainer;
#include "iDraw.h"
//}}}

class cWidget {
public:
  static constexpr uint16_t getBoxHeight()       { return 19; }
  static constexpr uint16_t getSmallFontHeight() { return 16; }
  static constexpr uint16_t getFontHeight()      { return 18; }
  static constexpr uint16_t getBigFontHeight()   { return 40; }
  static constexpr float getPixel() { return 1.0f / getBoxHeight(); }

  cWidget() {}
  //{{{
  cWidget (uint16_t width, uint16_t height)
    : mPixSize (width, height), mLayoutSize(width, height) {}
  //}}}
  //{{{
  cWidget (const sColourF& colour, uint16_t width, uint16_t height)
    : mColour(colour), mPixSize(width, height), mLayoutSize(mPixSize) {}
  //}}}
  //{{{
  cWidget (float widthInBoxes)
    : mPixSize (widthInBoxes * getBoxHeight(), getBoxHeight()), mLayoutSize(mPixSize) {}
  //}}}
  //{{{
  cWidget (float widthInBoxes, float heightInBoxes)
    : mPixSize (widthInBoxes * getBoxHeight(), heightInBoxes * getBoxHeight()), mLayoutSize(mPixSize) {}
  //}}}
  //{{{
  cWidget (const sColourF& colour, float widthInBoxes, float heightInBoxes)
    : mColour(colour),
      mPixSize(widthInBoxes * getBoxHeight(), heightInBoxes * getBoxHeight()), mLayoutSize(mPixSize) {}
  //}}}
  virtual ~cWidget() {}

  // gets
  float getPixX() { return mPixOrg.x; }
  float getPixY() { return mPixOrg.y; }
  cPointF getPixOrg() { return mPixOrg; }

  float getPixWidth() { return mPixSize.x; }
  float getPixHeight() { return mPixSize.y; }
  cPointF getPixSize() { return mPixSize; }

  int getPressedCount() { return mPressedCount; }
  bool isPressed() { return mPressedCount > 0; }
  bool isOn() { return mOn; }
  bool isVisible() { return mVisible; }

  //{{{  sets
  //{{{
  void setBoxOrg (float x, float y) {
    setPixOrg (x*getBoxHeight(), y*getBoxHeight());
    }
  //}}}

  //{{{
  void setPixOrg (float x, float y) {
    mPixOrg.x = x;
    mPixOrg.y = y;
    }
  //}}}
  //{{{
  void setPixOrg (const cPointF& p) {
    mPixOrg = p;
    }
  //}}}

  //{{{
  void setPixWidth (float width) {
    mPixSize.x = width;
    }
  //}}}
  //{{{
  void setPixHeight (float height) {
    mPixSize.y = height;
    }
  //}}}
  //{{{
  void setPixSize (float width, float height) {
    mPixSize.x = width;
    mPixSize.y = height;
    }
  //}}}
  //{{{
  void setPixSize (const cPointF& size) {
    mPixSize = size;
    }
  //}}}

  void setColour (const sColourF& colour) { mColour = colour; }
  void setParent (cContainer* parent) { mParent = parent; }

  void setOn (bool on) { mOn = on; }
  void setVisible (bool visible) { mVisible = visible; }
  //}}}

  //{{{
  virtual cWidget* isPicked (float x, float y) {
    return (x >= mPixOrg.x) && (x < mPixOrg.x + mPixSize.x) &&
           (y >= mPixOrg.y) && (y < mPixOrg.y + mPixSize.y) ? this : nullptr;
    }
  //}}}
  //{{{
  virtual void layout (const cPointF& size) {
    setPixSize ((mLayoutSize.x <= 0.f) ? size.x + mLayoutSize.x : mPixSize.x,
                (mLayoutSize.y <= 0.f) ? size.y + mLayoutSize.y : mPixSize.y);
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
    draw->drawRect (mOn ? kLightRedF : mColour, cPointF(mPixOrg.x+1.f, mPixOrg.y+1.f), cPointF(mPixSize.x -1.f, mPixSize.y -1.f));
    }
  //}}}

protected:
  sColourF mColour = kLightGreyF;

  cPointF mPixOrg = { 0.f,0.f };
  cPointF mPixSize = { 0.f,0.f };
  cPointF mLayoutSize = { 0.f,0.f };

  int mPressedCount = 0;
  bool mOn = false;

  cContainer* mParent = nullptr;
  bool mVisible = true;
  };
