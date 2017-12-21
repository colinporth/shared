// cRootContainer.h - singleton root widget
#pragma once
#include "cContainer.h"

class cRootContainer : public cContainer {
public:
  //{{{
  cRootContainer (uint16_t width, uint16_t height) :
    cContainer(width / getBoxHeight(), height / getBoxHeight()) {}
  //}}}
  virtual ~cRootContainer() {}

  //{{{
  void prox (int16_t x, int16_t y) {

    mProxWidget = picked (x, y, 0);
    if (mProxWidget)
      mProxWidget->prox (x - mProxWidget->getPixX(), y - mProxWidget->getPixY());
    }
  //}}}
  //{{{
  void onDown (int pressCount, int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc) {

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
  void onUp() {

    if (mPressedWidget) {
      mPressedWidget->onUp();
      mPressedWidget = nullptr;
      }
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {

    if (mOn)
      draw->drawRect (COL_DARKGREEN, mX, mY, mWidth, mHeight);
    cContainer::onDraw  (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
