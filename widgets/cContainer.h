// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>

#include "cWidget.h"

#include "../fmt/core.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

class cRootContainer;
//}}}

class cContainer : public cWidget {
friend cRootContainer;
public:
  cContainer (const std::string& id = "container") : cWidget(id), mSelfSize(true) {}
  //{{{
  cContainer (float width, float height, const std::string& id = "container")
    : cWidget (kBlackF, width, height, id), mSelfSize(false) {}
  //}}}
  //{{{
  virtual ~cContainer() {

    for (auto& layout : mWidgetLayouts)
      delete layout;

    mWidgetLayouts.clear();
    }
  //}}}

  //{{{
  virtual cWidget* isPicked (cPointF point) {
  // pick subWidgets in reverse of display order
  // widget enabled & visible only tested in container

    for (auto it = mWidgetLayouts.rbegin(); it != mWidgetLayouts.rend(); ++it) {
      cWidget* widget = (*it)->getWidget();
      if (widget->isEnabled() && widget->isVisible()) {
        cWidget* pickedWidget = widget->isPicked (point);
        if (pickedWidget) {
          if (mDebug)
            cLog::log (LOGINFO, fmt::format ("pickedWidget - {} picked", pickedWidget->getId()));
          return pickedWidget;
          }
        }
      }

    return nullptr;
    }
  //}}}

  //{{{
  class cWidgetLayout {
  public:
    cWidgetLayout (cWidget* widget) : mWidget(widget) {}
    virtual ~cWidgetLayout() { delete mWidget; }

    cWidget* getWidget() { return mWidget; }
    virtual void layout (cWidget* prevWidget, cPointF org, cPointF parentSize) = 0;

  protected:
    cWidget* mWidget;
    };
  //}}}
  //{{{
  cWidget* addTopLeft (cWidget* widget) {
  // add topleft of container, useful for adding more than one overlaid topLeft widgets

    addWidgetLayout (new cLayoutAt (widget, cPointF()));
    return widget;
    }
  //}}}
  //{{{
  cWidget* add (cWidget* widget) {
  // add next right until end of container, then start new row

    addWidgetLayout (new cLayoutNext (widget, 0.f));
    return widget;
    }
  //}}}
  //{{{
  cWidget* add (cWidget* widget, float offset) {
  // add with offset next right until end of container, then start new row

    addWidgetLayout (new cLayoutNext (widget, offset));
    return widget;
    }
  //}}}
  //{{{
  cWidget* add (cWidget* widget, cPointF offset) {
  // add at absolute offset into container

    addWidgetLayout (new cLayoutAt (widget, offset));
    return widget;
    }
  //}}}
  //{{{
  cWidget* addBelowLeft (cWidget* widget, float offset = 0.f) {
  // add belowLeft of container widgets so far

    addWidgetLayout (new cLayoutBelowLeft (widget, offset));
    return widget;
    }
  //}}}
  //{{{
  void addWidgetLayout (cWidgetLayout* widgetLayout) {

    mWidgetLayouts.push_back (widgetLayout);
    updateSize (mSize);
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {
  // widget enabled & visible only tested in container

    for (auto& layout : mWidgetLayouts)
      if (layout->getWidget()->isEnabled() && layout->getWidget()->isVisible())
        layout->getWidget()->onDraw (draw);
    }
  //}}}
  //{{{
  virtual void onDrawDebug (iDraw* draw, cWidget* widget) {
  // widget enabled & visible only tested in container

    for (auto& layout : mWidgetLayouts)
      if (layout->getWidget()->isEnabled() && layout->getWidget()->isVisible())
        layout->getWidget()->onDrawDebug (draw, widget);
    }
  //}}}

private:
  //{{{
  class cLayoutNext : public cWidgetLayout {
  public:
    cLayoutNext (cWidget* widget, float offset) : cWidgetLayout (widget), mOffset(offset) {}
    virtual ~cLayoutNext() {}

    virtual void layout (cWidget* prevWidget, cPointF parentOrg, cPointF parentSize) {
      if (!prevWidget)
        // topLeft
        mWidget->mOrg = parentOrg;
      else if (prevWidget->getEndX() + mWidget->mSize.x < parentSize.x) {
        // next right
        mWidget->mOrg.x = prevWidget->mOrg.x + prevWidget->mSize.x + mOffset;
        mWidget->mOrg.y = prevWidget->mOrg.y;
        }
      else {
        // belowLeft
        mWidget->mOrg.x = parentOrg.x;
        mWidget->mOrg.y = prevWidget->mOrg.y + prevWidget->mSize.y + mOffset;
        }
      }

  private:
    float mOffset;
    };
  //}}}
  //{{{
  class cLayoutAt: public cWidgetLayout {
  public:
    cLayoutAt (cWidget* widget, cPointF offset) : cWidgetLayout (widget), mOffset(offset) {}
    virtual ~cLayoutAt() {}

    virtual void layout (cWidget* prevWidget, cPointF parentOrg, cPointF parentSize) {
      mWidget->mOrg = parentOrg + mOffset;
      }

  private:
    cPointF mOffset;
    };
  //}}}
  //{{{
  class cLayoutBelowLeft: public cWidgetLayout {
  public:
    cLayoutBelowLeft (cWidget* widget, float offset) : cWidgetLayout (widget), mOffset(offset) {}
    virtual ~cLayoutBelowLeft() {}

    virtual void layout (cWidget* prevWidget, cPointF parentOrg, cPointF parentSize) {
      mWidget->mOrg.x = parentOrg.x;
      if (prevWidget)
        mWidget->mOrg.y = prevWidget->mOrg.y + prevWidget->mSize.y + mOffset;
      else
        mWidget->mOrg.y = parentOrg.y + mOffset;
      }

  private:
    float mOffset;
    };
  //}}}

  //{{{
  void updateOrg (cPointF parentOrg, cPointF parentSize) {
  // update org of our subWidgets using their layout rule

    cWidget* prevWidget = nullptr;
    for (auto& layout : mWidgetLayouts) {
      if (layout->getWidget()->isEnabled()) {
        layout->layout (prevWidget, parentOrg, parentSize);
        prevWidget = layout->getWidget();
        }
      }

    cPointF end;
    for (auto& layout : mWidgetLayouts)
      if (layout->getWidget()->isEnabled())
        end = end.max (layout->getWidget()->getEnd());

    if (mSelfSize)
      mSize = end - parentOrg;

    if (mDebug)
      cLog::log (LOGINFO, fmt::format ("updateOrg {} - parentOrg:{} parentSize:{}{} size:{}",
                                       getId(), parentOrg, parentSize, mSelfSize ? " selfSize" : "", mSize));
    }
  //}}}
  //{{{
  virtual void updateSize (cPointF parentSize) {

    if (isEnabled()) {
      for (auto& layout : mWidgetLayouts)
        layout->getWidget()->updateSize (parentSize);

      // set org of subWidgets
      updateOrg (mOrg, parentSize);
      }
    }
  //}}}

  //{{{
  virtual void debug (int indent) {

    cLog::log (LOGINFO, fmt::format ("{:{}}org:{}size:{} layout:{}{}{}{} - {}",
                                     indent ? " ": "", indent,
                                     mOrg, mSize, mLayoutSize,
                                     mEnabled ? "" : " disabled",
                                     mVisible ? "" : " invisble",
                                     mOn ? " on" : "", getId()));

    indent += 2;
    for (auto& layout : mWidgetLayouts)
      layout->getWidget()->debug (indent);
    }
  //}}}

  bool mSelfSize;
  std::vector <cWidgetLayout*> mWidgetLayouts;
  };
