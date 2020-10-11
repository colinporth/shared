// cRootContainer.h - singleton root widget
//{{{  includes
#pragma once

#include "cContainer.h"
//}}}

class cRootContainer : public cContainer {
public:
  //{{{
  cRootContainer (uint16_t width, uint16_t height) :
    cContainer(width / (float)getBoxHeight(), height / (float)getBoxHeight()) {}
  //}}}
  virtual ~cRootContainer() {}

  //{{{
  virtual void onProx (float x, float y) {

    mProxWidget = isPicked (x, y);

    if (mProxWidget)
      mProxWidget->onProx (x - mProxWidget->getPixX(), y - mProxWidget->getPixY());
    else
      mProxWidget = this;
    }
  //}}}
  //{{{
  virtual void onDown (float x, float y) {

    mPressedWidget = isPicked (x, y);
    if (mPressedWidget)
      mPressedWidget->onDown (x - mPressedWidget->getPixX(), y - mPressedWidget->getPixY());
    }
  //}}}
  //{{{
  virtual void onMove (float x, float y, float xinc, float yinc) {

    if (mPressedWidget)
      mPressedWidget->onMove (x - mPressedWidget->getPixX(), y - mPressedWidget->getPixY(), xinc, yinc);
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
      draw->drawRect (COL_DARKGREEN, cPoint(mX, mY), cPoint(mWidth, mHeight));

    cContainer::onDraw  (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
