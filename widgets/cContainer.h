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

    for (auto& widgetLayout : mWidgetLayouts)
      delete widgetLayout.mWidget;

    mWidgetLayouts.clear();
    }
  //}}}

  //{{{
  void setSize (const cPointF& size) {
  // only used by rootContainer ??

    mSize = size;

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout.mWidget->layoutSize (mSize);

    layoutWidgets (mOrg);
    }
  //}}}
  //{{{
  virtual cWidget* isPicked (const cPointF& point) {

    if (cWidget::isPicked (point)) {
      if (mWidgetLayouts.empty())
        return nullptr;
      else {
        // pick reverse of display order
        for (auto it = mWidgetLayouts.rbegin(); it != mWidgetLayouts.rend(); ++it) {
          cWidget* pickedWidget = (*it).mWidget->isPicked (point); // - mOrg ??
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
  cWidget* add (cWidget* widget, float offset = 0.f) {
    addWidget (widget, cWidgetLayout::eLayout::eNext, cPointF (offset, offset));
    return widget;
    }
  //}}}
  //{{{
  cWidget* addTopLeft (cWidget* widget) {
    addWidget (widget, cWidgetLayout::eLayout::eAt, cPointF());
    return widget;
    }
  //}}}
  //{{{
  cWidget* addAt (cWidget* widget, const cPointF& offset) {
    addWidget (widget, cWidgetLayout::eLayout::eAt, offset);
    return widget;
    }
  //}}}
  //{{{
  cWidget* addBelowLeft (cWidget* widget, float offset = 0.f) {
    addWidget (widget, cWidgetLayout::eLayout::eBelowLeft, cPointF(0.f, offset));
    return widget;
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {

    if (mDrawBgnd)
      draw->drawRect (mColour, mOrg, mSize);

    for (auto& widgetLayout : mWidgetLayouts)
      if (widgetLayout.mWidget->isVisible())
        widgetLayout.mWidget->onDraw (draw);
    }
  //}}}

protected:
  //{{{
  class cWidgetLayout {
  public:
    enum class eLayout { eNext, eBelowLeft, eAt };
    //{{{
    cWidgetLayout (cWidget* widget, eLayout layout, cPointF offset)
      : mWidget(widget), mLayout(layout), mOffset(offset) {}
    //}}}
    ~cWidgetLayout() {}

    cWidget* mWidget;
    eLayout mLayout;
    cPointF mOffset;
    };
  //}}}

  std::vector <cWidgetLayout> mWidgetLayouts;

private:
  //{{{
  void addWidget (cWidget* widget, cWidgetLayout::eLayout layout, const cPointF& offset) {

    mWidgetLayouts.push_back (cWidgetLayout(widget, layout, offset));
    layoutSize (mSize);
    }
  //}}}
  //{{{
  void layoutWidgets (const cPointF& org) {
  // layout sub widgets in this container using layout rules

    cPointF maxEnd = org;
    cWidget* prevWidget = nullptr;

    for (auto& widgetLayout : mWidgetLayouts) {
      switch (widgetLayout.mLayout) {
        case cWidgetLayout::eLayout::eNext:
          if (!prevWidget)
            // topLeft
            widgetLayout.mWidget->mOrg = org + widgetLayout.mOffset;
          else if (prevWidget->getEndX() + widgetLayout.mWidget->mSize.x < mSize.x) {
            // next right
            widgetLayout.mWidget->mOrg.x = prevWidget->mOrg.x + prevWidget->mSize.x + widgetLayout.mOffset.x;
            widgetLayout.mWidget->mOrg.y = prevWidget->mOrg.y;
            }
          else {
            // belowLeft
            widgetLayout.mWidget->mOrg.x = org.x;
            widgetLayout.mWidget->mOrg.y = maxEnd.y + widgetLayout.mOffset.y;
            }
          break;

        case cWidgetLayout::eLayout::eBelowLeft:
          // belowLeft
          widgetLayout.mWidget->mOrg.x = org.x;
          widgetLayout.mWidget->mOrg.y = maxEnd.y + widgetLayout.mOffset.y;
          break;

        case cWidgetLayout::eLayout::eAt:
          widgetLayout.mWidget->mOrg = org + widgetLayout.mOffset;
          break;
        }

      // calc maxEnd
      maxEnd.x = std::max (maxEnd.x, widgetLayout.mWidget->getEndX());
      maxEnd.y = std::max (maxEnd.y, widgetLayout.mWidget->getEndY());

      prevWidget = widgetLayout.mWidget;
      }

    if (mSelfSize)
      mSize = maxEnd - org;

    //cLog::log (LOGINFO, "layoutWidgets " + getId() + " " + dec (boundingSize.x) + "," + dec(boundingSize.y));
    }
  //}}}
  //{{{
  virtual void layoutSize (const cPointF& parentSize) {

    cWidget::layoutSize (parentSize);

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout.mWidget->layoutSize (mSize);

    layoutWidgets (mOrg);
    }
  //}}}

  // vars
  const bool mDrawBgnd;
  const bool mSelfSize;
  };
