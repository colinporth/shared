// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>
#include "cWidget.h"
//}}}

class cContainer : public cWidget {
public:
  //{{{
  cContainer (const std::string& id = "cContainerUnsized")
    : cWidget (kBlackF, 0.f, 0.f, id), mDrawBgnd(false), mSelfSize(true) {}
  //}}}
  //{{{
  cContainer (const sColourF& colour, const std::string& id = "cContainerUnsizedBgnd")
    : cWidget (colour, 0.f, 0.f, id), mDrawBgnd(true), mSelfSize(true) {}
  //}}}
  //{{{
  cContainer (float width, float height, const std::string& id = "cContainerSized")
    : cWidget (kBlackF, width, height, id), mDrawBgnd(false), mSelfSize(false) {}
  //}}}
  //{{{
  cContainer (const sColourF& colour, float width, float height, const std::string& id = "cContainerSizedBgnd")
    : cWidget (colour, width, height, id), mDrawBgnd(true), mSelfSize(false) {}
  //}}}
  //{{{
  virtual ~cContainer() {

    for (auto widgetLayout : mWidgetLayouts)
      delete widgetLayout;

    mWidgetLayouts.clear();
    }
  //}}}

  //{{{
  void setSize (cPointF size) {
  // only used by rootContainer ??

    mSize = size;

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout->getWidget()->layoutSize (mSize);

    layoutWidgets (mOrg);
    }
  //}}}
  //{{{
  virtual cWidget* isPicked (cPointF point) {

    if (cWidget::isPicked (point)) {
      if (mWidgetLayouts.empty())
        return nullptr;
      else {
        // pick reverse of display order
        for (auto it = mWidgetLayouts.rbegin(); it != mWidgetLayouts.rend(); ++it) {
          cWidget* pickedWidget = (*it)->getWidget()->isPicked (point); // - mOrg ??
          if (pickedWidget)
            return pickedWidget;
          }
        }
      return this;
      }
    else
      return nullptr;
    }
  //}}}

  //{{{
  class cWidgetLayout {
  public:
    cWidgetLayout (cWidget* widget) : mWidget(widget) {}
    virtual ~cWidgetLayout() { delete mWidget; }

    cWidget* getWidget() { return mWidget; }
    virtual cPointF layout (cWidget* prevWidget, cPointF org, cPointF size, cPointF maxEnd) = 0;

  protected:
     //{{{
     cPointF calcMaxEnd (cPointF maxEnd) {
      maxEnd.x = std::max (maxEnd.x, mWidget->getEndX());
      maxEnd.y = std::max (maxEnd.y, mWidget->getEndY());
      return maxEnd;
      }
     //}}}

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
    layoutSize (mSize);
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {

    if (mDrawBgnd)
      draw->drawRect (mColour, mOrg, mSize);

    for (auto widgetLayout : mWidgetLayouts)
      if (widgetLayout->getWidget()->isVisible())
        widgetLayout->getWidget()->onDraw (draw);
    }
  //}}}

protected:
  std::vector <cWidgetLayout*> mWidgetLayouts;

private:
  //{{{
  class cLayoutNext : public cWidgetLayout {
  public:
    cLayoutNext (cWidget* widget, float offset) : cWidgetLayout (widget), mOffset(offset) {}
    virtual ~cLayoutNext() {}

    virtual cPointF layout (cWidget* prevWidget, cPointF org, cPointF size, cPointF maxEnd) {
      if (!prevWidget)
        // topLeft
        mWidget->mOrg = org;
      else if (prevWidget->getEndX() + mWidget->mSize.x < size.x) {
        // next right
        mWidget->mOrg.x = prevWidget->mOrg.x + prevWidget->mSize.x + mOffset;
        mWidget->mOrg.y = prevWidget->mOrg.y;
        }
      else {
        // belowLeft
        mWidget->mOrg.x = org.x;
        mWidget->mOrg.y = maxEnd.y + mOffset;
        }

      return calcMaxEnd (maxEnd);
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

    virtual cPointF layout (cWidget* prevWidget, cPointF org, cPointF size, cPointF maxEnd) {
      mWidget->mOrg = org + mOffset;
      return calcMaxEnd (maxEnd);
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

    virtual cPointF layout (cWidget* prevWidget, cPointF org, cPointF size, cPointF maxEnd) {
      mWidget->mOrg.x = org.x;
      mWidget->mOrg.y = maxEnd.y + mOffset;
      return calcMaxEnd (maxEnd);
      }

  private:
    float mOffset;
    };
  //}}}

  //{{{
  void layoutWidgets (cPointF org) {
  // layout sub widgets in this container using widgetLayout layout rule

    cPointF maxEnd = org;
    cWidget* prevWidget = nullptr;

    for (auto widgetLayout : mWidgetLayouts) {
      maxEnd = widgetLayout->layout (prevWidget, org, mSize, maxEnd);
      prevWidget = widgetLayout->getWidget();
      }

    if (mSelfSize)
      mSize = maxEnd - org;

    //cLog::log (LOGINFO, "layoutWidgets " + getId() + " " + dec (boundingSize.x) + "," + dec(boundingSize.y));
    }
  //}}}
  //{{{
  virtual void layoutSize (cPointF parentSize) {

    cWidget::layoutSize (parentSize);

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout->getWidget()->layoutSize (mSize);

    layoutWidgets (mOrg);
    }
  //}}}

  // vars
  const bool mDrawBgnd;
  const bool mSelfSize;
  };
