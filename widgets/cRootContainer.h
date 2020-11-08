// cRootContainer.h - singleton root widget
//{{{  includes
#pragma once

#include "cContainer.h"
//}}}

class cRootContainer : public cContainer {
public:
  cRootContainer (uint16_t width, uint16_t height) : cContainer(width, height) {}
  virtual ~cRootContainer() {}

  //{{{
  virtual void onProx (const cPointF& point) {

    mProxWidget = isPicked (point);

    if (mProxWidget)
      mProxWidget->onProx (point - mProxWidget->getPixOrg());
    else
      mProxWidget = this;
    }
  //}}}
  //{{{
  virtual void onDown (const cPointF& point) {

    mPressedWidget = isPicked (point);
    if (mPressedWidget)
      mPressedWidget->onDown (point - mPressedWidget->getPixOrg());
    }
  //}}}
  //{{{
  virtual void onMove (const cPointF& point, const cPointF& inc) {

    if (mPressedWidget)
      mPressedWidget->onMove (point - mPressedWidget->getPixOrg(), inc);
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

    if (mOn)
      draw->drawRect (kDarkGreenF, mPixOrg, mPixSize);

    cContainer::onDraw  (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
