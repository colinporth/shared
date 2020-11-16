// cWidget.h - base widget, draws as box with width-1, height-1
//{{{  includes
#pragma once

class cContainer;
#include "iDraw.h"

#include <functional>
//}}}

class cWidget {
public:
  static constexpr float kBoxHeight = 20.f;
  static constexpr float kSmallFontHeight = 16.f;
  static constexpr float kFontHeight = 18.f;
  static constexpr float kBigFontHeight = 40.f;

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
  cPointF getPixOrg() { return mPixOrg; }
  float getPixX() { return mPixOrg.x; }
  float getPixY() { return mPixOrg.y; }

  cPointF getPixSize() { return mPixSize; }
  float getPixWidth() { return mPixSize.x; }
  float getPixHeight() { return mPixSize.y; }

  cPointF getPixCentre() { return mPixOrg + (mPixSize / 2.f); }
  float getPixCentreX() { return mPixOrg.x + (mPixSize.x / 2.f); }
  float getPixCentreY() { return mPixOrg.y + (mPixSize.y / 2.f); }

  cPointF getPixEnd() { return mPixOrg + mPixSize; }
  float getPixEndX() { return mPixOrg.x + mPixSize.x; }
  float getPixEndY() { return mPixOrg.y + mPixSize.y; }
  //}}}
  //{{{  gets - virtual
  virtual bool isOn() { return mOn; }
  virtual bool isVisible() { return mVisible; }

  virtual bool isPressed() { return mPressedCount > 0; }
  virtual int getPressedCount() { return mPressedCount; }

  //{{{
  virtual cWidget* isPicked (const cPointF& point) {
  // return true if point inside visible widget

    return (mVisible && isInside (point)) ? this : nullptr;
    }
  //}}}

  virtual std::string getDebugName() { return mDebugName; }
  //}}}
  //{{{  sets - virtual
  virtual void setPixOrg (const cPointF& point) { mPixOrg = point; }
  virtual void setPixSize (const cPointF& size) { mPixSize = size; }

  virtual void setColour (const sColourF& colour) { mColour = colour; }
  virtual void setParent (cWidget* parent) { mParent = parent; }

  virtual void setOn (bool on) { mOn = on; }
  virtual void toggleOn() { mOn = !mOn; }

  virtual void setVisible (bool visible) { mVisible = visible; }
  virtual void toggleVisible() { mVisible = !mVisible; }
  //}}}
  //{{{
  virtual void layout() {

    if (mParent)
      setPixSize (cPointF(mLayoutSize.x <= 0.f ? mParent->getPixSize().x + mLayoutSize.x : mPixSize.x,
                          mLayoutSize.y <= 0.f ? mParent->getPixSize().y + mLayoutSize.y : mPixSize.y));
    }
  //}}}

  virtual void onProx (const cPointF& point) {}
  virtual void onDown (const cPointF& point) { mPressedCount++; }
  virtual void onMove (const cPointF& point, const cPointF& inc) {}
  virtual void onUp() { mPressedCount = 0; }
  virtual void onWheel (float delta) {}

  //{{{
  virtual void onDraw (iDraw* draw) {
    draw->drawRect (mOn ? kLightRedF : mColour, mPixOrg + cPointF(1.f,1.f), mPixSize - cPointF(1.f,1.f));
    }
  //}}}

protected:
  const std::string mDebugName;
  sColourF mColour = kLightGreyF;
  const cPointF mLayoutSize = { 0.f,0.f };
  cPointF mPixOrg = { 0.f,0.f };
  cPointF mPixSize = { 0.f,0.f };

  cWidget* mParent = nullptr;
  bool mVisible = true;
  bool mOn = false;

  int mPressedCount = 0;

private:
  //{{{
  bool isInside (const cPointF& point) {
  // return true if point is inside widget, even if not visible

    return (point.x >= mPixOrg.x) && (point.x < mPixOrg.x + mPixSize.x) &&
           (point.y >= mPixOrg.y) && (point.y < mPixOrg.y + mPixSize.y);
    }
  //}}}
  };
