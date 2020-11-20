// cRootContainer.h - singleton root widget
//{{{  includes
#pragma once

#include "cContainer.h"

#include "../fmt/core.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"
//}}}

class cRootContainer : public cContainer {
public:
  //{{{
  cRootContainer (float width, float height, const std::string& id = "rootContainer")
    : cContainer(width, height, id) {}
  //}}}
  virtual ~cRootContainer() {}

  //{{{
  void setSize (cPointF size) {

    mSize = size;

    // update size of our subWidgets
    for (auto& layout : mWidgetLayouts)
      layout->getWidget()->updateSize (mSize);

    updateOrg (mOrg, mSize);
    }
  //}}}

  //{{{
  void toggleDebug() {

    mDebug = !mDebug;

    if (mDebug) {
      cLog::log (LOGINFO, "--------------------- widgetDebug on ---------------------------");
      debug (0);
      cLog::log (LOGINFO, "----------------------------------------------------------------");
      }
    else
      cLog::log (LOGINFO, "--------------------- widgetDebug off --------------------------");
    }
  //}}}

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

    if (mDebug)
      cContainer::onDrawDebug (draw, mProxWidget);
    else
      cContainer::onDraw (draw);
    }
  //}}}

private:
  cWidget* mProxWidget = nullptr;
  cWidget* mPressedWidget = nullptr;
  };
