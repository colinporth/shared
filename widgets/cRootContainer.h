// cRootContainer.h - singleton root widget
#pragma once
#include "cContainer.h"

class cRootContainer : public cContainer {
public:
  cRootContainer (uint16_t width, uint16_t height) : cContainer(width, height) {}
  virtual ~cRootContainer() {}

  //{{{
  void prox (int16_t x, int16_t y, bool controlled) {
    mProxWidget = picked (x, y, 0);
    if (mProxWidget)
      mProxWidget->prox (x - mProxWidget->getPixX(), y - mProxWidget->getPixY(), controlled);
    }
  //}}}
  //{{{
  void press (int pressCount, int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc, bool controlled) {
    if (!pressCount) {
      mPressedWidget = picked (x, y, z);
      if (mPressedWidget)
        mPressedWidget->pressed (x - mPressedWidget->getPixX(), y - mPressedWidget->getPixY(), controlled);
      }
    else if (mPressedWidget)
      mPressedWidget->moved (x - mPressedWidget->getPixX(), y - mPressedWidget->getPixY(), z, xinc, yinc, controlled);
    }
  //}}}
  //{{{
  void release() {
    if (mPressedWidget) {
      mPressedWidget->released();
      mPressedWidget = nullptr;
      }
    }
  //}}}

  //{{{
  virtual void render (iDraw* draw) {
    if (mOn)
      draw->drawRect (COL_DARKGREEN, mX, mY, mWidth, mHeight);
    cContainer::render (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
