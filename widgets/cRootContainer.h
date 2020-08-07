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
  virtual void prox (float x, float y) {

    mProxWidget = picked (x, y, 0);
    if (mProxWidget)
      mProxWidget->prox (x - mProxWidget->getPixX(), y - mProxWidget->getPixY());
    }
  //}}}
  //{{{
  virtual void onDown (int pressCount, float x, float y, float z, float xinc, float yinc) {

    if (!pressCount) {
      mPressedWidget = picked (x, y, z);
      if (mPressedWidget)
        mPressedWidget->onDown (x - mPressedWidget->getPixX(), y - mPressedWidget->getPixY());
      }
    else if (mPressedWidget)
      mPressedWidget->onMove (x - mPressedWidget->getPixX(), y - mPressedWidget->getPixY(), z, xinc, yinc);
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
      draw->drawRect (COL_DARKGREEN, mX, mY, mWidth, mHeight);
    cContainer::onDraw  (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
