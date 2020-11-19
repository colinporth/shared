// cRootContainer.h - singleton root widget
//{{{  includes
#pragma once

#include "cContainer.h"
//}}}

class cRootContainer : public cContainer {
public:
  //{{{
  cRootContainer (float width, float height, const std::string& id = "cRootContainer")
    : cContainer(width, height, id) {}
  //}}}
  virtual ~cRootContainer() {}

  //{{{
  virtual void onProx (cPointF point) {

    mProxWidget = isPicked (point);

    if (mProxWidget)
      mProxWidget->onProx (point - mProxWidget->getOrg());
    else
      mProxWidget = this;
    }
  //}}}
  //{{{
  virtual void onDown (cPointF point) {

    mPressedWidget = isPicked (point);
    if (mPressedWidget)
      mPressedWidget->onDown (point - mPressedWidget->getOrg());
    }
  //}}}
  //{{{
  virtual void onMove (cPointF point, cPointF inc) {

    if (mPressedWidget)
      mPressedWidget->onMove (point - mPressedWidget->getOrg(), inc);
    }
  //}}}
  //{{{
  virtual void onUp() {

    if (mPressedWidget) {
      mPressedWidget->onUp();
      mPressedWidget = nullptr;
      }
    }
  //}}}
  //{{{
  virtual void onWheel (float delta) {

    if (mProxWidget)
      mProxWidget->onWheel (delta);
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {

    // turn green if we ever turn on
    if (mOn)
      draw->drawRect (kDarkGreenF, mOrg, mSize);

    cContainer::onDraw  (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
