// cWidget.h - base widget, draws as box with width-1, height-1
//{{{  includes
#pragma once

class cContainer;
#include "iDraw.h"

#include <functional>
//}}}

class cWidget {
public:
  static constexpr uint16_t kSmallFontHeight = 16;
  static constexpr uint16_t kFontHeight = 18;
  static constexpr uint16_t kBigFontHeight = 40;

  static constexpr uint16_t kBoxHeight = 20;
  static constexpr float kPixel =  1.f / kBoxHeight;

  //{{{
  cWidget (float widthInBoxes, const std::string& debugName)
    : mDebugName(debugName),
      mLayoutSize(widthInBoxes * kBoxHeight, kBoxHeight),
      mPixSize(widthInBoxes * kBoxHeight, kBoxHeight) {}
  //}}}
  //{{{
  cWidget (float widthInBoxes, float heightInBoxes, const std::string& debugName)
    : mDebugName(debugName),
      mLayoutSize(widthInBoxes * kBoxHeight, heightInBoxes * kBoxHeight),
      mPixSize(widthInBoxes * kBoxHeight, heightInBoxes * kBoxHeight) {}
  //}}}
  //{{{
  cWidget (const sColourF& colour, float widthInBoxes, float heightInBoxes, const std::string& debugName)
    : mDebugName(debugName),
      mColour(colour),
      mLayoutSize(widthInBoxes * kBoxHeight, heightInBoxes * kBoxHeight),
      mPixSize(widthInBoxes * kBoxHeight, heightInBoxes * kBoxHeight) {}
  //}}}
  //{{{
  cWidget (uint16_t widthInPix, uint16_t heightInPix, const std::string& debugName)
    : mDebugName(debugName), mLayoutSize(widthInPix, heightInPix), mPixSize(widthInPix, heightInPix) {}
  //}}}
  //{{{
  cWidget (const sColourF& colour, uint16_t widthInPix, uint16_t heightInPix, const std::string& debugName)
    : mDebugName(debugName), mColour(colour),
      mLayoutSize(widthInPix, heightInPix), mPixSize(widthInPix, heightInPix) {}
  //}}}
  virtual ~cWidget() {}

  //{{{  gets
  float getPixX() { return mPixOrg.x; }
  float getPixY() { return mPixOrg.y; }
  cPointF getPixOrg() { return mPixOrg; }

  float getPixWidth() { return mPixSize.x; }
  float getPixHeight() { return mPixSize.y; }
  cPointF getPixSize() { return mPixSize; }
  cPointF getPixCentre() { return mPixSize / 2.f; }

  virtual bool isOn() { return mOn; }
  virtual bool isVisible() { return mVisible; }

  virtual bool isPressed() { return mPressedCount > 0; }
  virtual int getPressedCount() { return mPressedCount; }

  //{{{
  virtual bool isWithin (const cPointF& point) {
  // return true if p inside widget, even if not visible

    return (point.x >= mPixOrg.x) && (point.x < mPixOrg.x + mPixSize.x) &&
           (point.y >= mPixOrg.y) && (point.y < mPixOrg.y + mPixSize.y);
    }
  //}}}
  //{{{
  virtual cWidget* isPicked (const cPointF& point) {
  // return true if point inside visible widget

    return (mVisible && isWithin (point)) ? this : nullptr;
    }
  //}}}

  virtual std::string getDebugName() { return mDebugName; }
  //}}}
  //{{{  sets
  virtual void setPixOrg (const cPointF& point) { mPixOrg = point; }
  virtual void setPixOrg (float x, float y) { mPixOrg = { x,y }; }

  virtual void setPixSize (const cPointF& size) { mPixSize = size; }
  virtual void setPixSize (float widthInPix, float heightInPix) { mPixSize = {widthInPix, heightInPix}; }
  virtual void setPixWidth (float widthInPix) { mPixSize.x = widthInPix; }
  virtual void setPixHeight (float heightInPix) { mPixSize.y = heightInPix; }

  virtual void setColour (const sColourF& colour) { mColour = colour; }
  virtual void setParent (cContainer* parent) { mParent = parent; }

  virtual void setOn (bool on) { mOn = on; }
  virtual void toggleOn() { mOn = !mOn; }

  virtual void setVisible (bool visible) { mVisible = visible; }
  virtual void toggleVisible() { mVisible = !mVisible; }
  //}}}
  //{{{
  virtual void layout (const cPointF& size) {
    setPixSize ((mLayoutSize.x <= 0.f) ? size.x + mLayoutSize.x : mPixSize.x,
                (mLayoutSize.y <= 0.f) ? size.y + mLayoutSize.y : mPixSize.y);
    }
  //}}}

  virtual void onProx (const cPointF& p) {}
  virtual void onDown (const cPointF& p) { mPressedCount++; }
  virtual void onMove (const cPointF& p, const cPointF& inc) {}
  virtual void onUp() { mPressedCount = 0; }
  virtual void onWheel (float delta) {}

  //{{{
  virtual void onDraw (iDraw* draw) {
    draw->drawRect (mOn ? kLightRedF : mColour, mPixOrg + cPointF(1.f,1.f), mPixSize - cPointF(1.f,1.f));
    }
  //}}}

protected:
  std::string mDebugName;

  sColourF mColour = kLightGreyF;
  const cPointF mLayoutSize = { 0.f,0.f };

  cContainer* mParent = nullptr;
  cPointF mPixOrg = { 0.f,0.f };
  cPointF mPixSize = { 0.f,0.f };

  bool mVisible = true;
  bool mOn = false;

  int mPressedCount = 0;
  };
