// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>

#include "cWidget.h"

#include "../fmt/core.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"
//}}}

class cContainer : public cWidget {
public:
  //{{{
  cContainer (float width, float height, const std::string& id = "cContainerUnsized")
    : cWidget (kBlackF, width, height, id) {}
  //}}}
  //{{{
  cContainer (const std::string& id = "cContainerUnsized")
    : cWidget (kBlackF, 0.f, 0.f, id) {}
  //}}}
  //{{{
  cContainer (const sColourF& colour, const std::string& id = "cContainerUnsizedBgnd")
    : cWidget (colour, 0.f, 0.f, id) {}
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
  // - cannot pick the container

    for (auto it = mWidgetLayouts.rbegin(); it != mWidgetLayouts.rend(); ++it) {
      cWidget* pickedWidget = (*it)->getWidget()->isPicked (point);
      if (pickedWidget) {
        if (mDebug)
          cLog::log (LOGINFO, fmt::format ("pickedWidget - {} picked", pickedWidget->getId()));
        return pickedWidget;
        }
      }

    return nullptr;
    }
  //}}}

  //{{{
  virtual void debug (int level) {

    std::string str;
    for (int i = 0; i < level; i++)
      str += "  ";

    str += fmt::format ("org:{},{} size:{},{} layout:{},{} - {}",
                        mOrg.x, mOrg.y, mSize.x, mSize.y, mLayoutSize.x, mLayoutSize.y, getId());

    cLog::log (LOGINFO, str);

    for (auto& layout : mWidgetLayouts)
      layout->getWidget()->debug (level+1);
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

    for (auto& layout : mWidgetLayouts)
      if (layout->getWidget()->isVisible())
        layout->getWidget()->onDraw (draw);
    }
  //}}}

protected:
  //{{{
  void updateOrg (cPointF parentOrg, cPointF parentSize) {
  // update org of our subWidgets using their layout rule

    cWidget* prevWidget = nullptr;
    for (auto& layout : mWidgetLayouts) {
      layout->layout (prevWidget, parentOrg, parentSize);
      prevWidget = layout->getWidget();
      }

    cPointF end;
    for (auto& layout : mWidgetLayouts) 
      end = end.max (layout->getWidget()->getEnd());

    mSize = end - parentOrg;
    if (mDebug)
      cLog::log (LOGINFO, "updateOrg " + getId() +
                          " parentOrg" + dec (parentOrg.x) + "," + dec(parentOrg.y) +
                          " parentSize" + dec (parentSize.x) + "," + dec(parentSize.y) +
                          " mSize:" + dec (mSize.x) + "," + dec(mSize.y));
    }
  //}}}
  std::vector <cWidgetLayout*> mWidgetLayouts;

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
      mWidget->mOrg.y = prevWidget->mOrg.y + prevWidget->mSize.y + mOffset;
      }

  private:
    float mOffset;
    };
  //}}}

  //{{{
  virtual void updateSize (cPointF parentSize) {

    //mSize = parentSize;
    for (auto& layout : mWidgetLayouts)
      layout->getWidget()->updateSize (parentSize);

    // set org of subWidgets
    updateOrg (mOrg, parentSize);
    }
  //}}}
  };
