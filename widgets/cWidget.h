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
  cWidget (float width, float height, const std::string& debugName)
    : mDebugName(debugName), mLayoutSize(width, height), mSize(width, height) {}
  //}}}
  //{{{
  cWidget (const sColourF& colour, float width, float height, const std::string& debugName)
    : mDebugName(debugName), mColour(colour),
      mLayoutSize(width, height), mSize(width, height) {}
  //}}}
  virtual ~cWidget() {}

  //{{{  gets
  cPointF getOrg() { return mOrg; }
  float getX() { return mOrg.x; }
  float getY() { return mOrg.y; }

  cPointF getSize() { return mSize; }
  float getWidth() { return mSize.x; }
  float getHeight() { return mSize.y; }

  cPointF getCentre() { return mOrg + (mSize / 2.f); }
  float getCentreX() { return mOrg.x + (mSize.x / 2.f); }
  float getCentreY() { return mOrg.y + (mSize.y / 2.f); }

  cPointF getEnd() { return mOrg + mSize; }
  float getEndX() { return mOrg.x + mSize.x; }
  float getEndY() { return mOrg.y + mSize.y; }
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
  virtual void setOrg (const cPointF& point) { mOrg = point; }
  virtual void setSize (const cPointF& size) { mSize = size; }

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
      setSize (cPointF(mLayoutSize.x <= 0.f ? mParent->getSize().x + mLayoutSize.x : mSize.x,
                          mLayoutSize.y <= 0.f ? mParent->getSize().y + mLayoutSize.y : mSize.y));
    }
  //}}}

  virtual void onProx (const cPointF& point) {}
  virtual void onDown (const cPointF& point) { mPressedCount++; }
  virtual void onMove (const cPointF& point, const cPointF& inc) {}
  virtual void onUp() { mPressedCount = 0; }
  virtual void onWheel (float delta) {}

  //{{{
  virtual void onDraw (iDraw* draw) {
    draw->drawRect (mOn ? kLightRedF : mColour, mOrg + cPointF(1.f,1.f), mSize - cPointF(1.f,1.f));
    }
  //}}}

protected:
  const std::string mDebugName;
  sColourF mColour = kLightGreyF;

  const cPointF mLayoutSize = { 0.f,0.f };
  cPointF mOrg = { 0.f,0.f };
  cPointF mSize = { 0.f,0.f };

  cWidget* mParent = nullptr;
  bool mVisible = true;
  bool mOn = false;

  int mPressedCount = 0;

private:
  //{{{
  bool isInside (const cPointF& point) {
  // return true if point is inside widget, even if not visible

    return (point.x >= mOrg.x) && (point.x < getEndX()) &&
           (point.y >= mOrg.y) && (point.y < getEndY());
    }
  //}}}
  };
