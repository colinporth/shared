// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>
#include "cWidget.h"
//}}}

class cContainer : public cWidget {
public:
  //{{{
  cContainer (const std::string& id = "cContainer - unsized")
    : cWidget (kBlackF, 0.f, 0.f, id), mSized(false) {}
  //}}}
  //{{{
  cContainer (float width, float height, const std::string& id = "cContainer - sized")
    : cWidget (kBlackF, width, height, id), mSized(true) {}
  //}}}
  //{{{
  virtual ~cContainer() {

    for (auto& widgetLayout : mWidgetLayouts)
      delete widgetLayout.mWidget;

    mWidgetLayouts.clear();
    }
  //}}}

  //{{{
  virtual void setSize (const cPointF& size) {

    cWidget::setSize (size);

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout.mWidget->layoutWidgetSize (mSize);

    layoutWidgetsInContainer();
    }
  //}}}
  //{{{
  virtual void layoutWidgetSize (const cPointF& parentSize) {

    cWidget::layoutWidgetSize (mSize);

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout.mWidget->layoutWidgetSize (mSize);

    layoutWidgetsInContainer();
    }
  //}}}

  //{{{
  cWidget* add (cWidget* widget, float offset = 0.f) {
    addWidget (widget, cWidgetLayout::eLayout::eNext, cPointF (offset, offset));
    return this;
    }
  //}}}
  //{{{
  cWidget* addTopLeft (cWidget* widget) {
    addWidget (widget, cWidgetLayout::eLayout::eAt, cPointF());
    return this;
    }
  //}}}
  //{{{
  cWidget* addAt (cWidget* widget, const cPointF& offset) {
    addWidget (widget, cWidgetLayout::eLayout::eAt, offset);
    return this;
    }
  //}}}
  //{{{
  cWidget* addBelowLeft (cWidget* widget, float offset = 0.f) {
    addWidget (widget, cWidgetLayout::eLayout::eBelowLeft, cPointF(0.f, offset));
    return this;
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
  virtual void onDraw (iDraw* draw) {

    for (auto& widgetLayout : mWidgetLayouts)
      if (widgetLayout.mWidget->isVisible())
        widgetLayout.mWidget->onDraw (draw);
    }
  //}}}

private:
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
  //{{{
  void addWidget (cWidget* widget, cWidgetLayout::eLayout layout, const cPointF& offset) {

    mWidgetLayouts.push_back (cWidgetLayout(widget, layout, offset));
    layoutWidgetSize (mSize);
    }
  //}}}
  //{{{
  void layoutWidgetsInContainer() {

    cPointF boundingSize;
    cWidget* prevWidget = nullptr;

    for (auto& widgetLayout : mWidgetLayouts) {
      switch (widgetLayout.mLayout) {
        case cWidgetLayout::eLayout::eNext:
          if (!prevWidget)
            // topLeft
            widgetLayout.mWidget->setOrg (widgetLayout.mOffset);
          else if (prevWidget->getEndX() + widgetLayout.mWidget->getWidth() < getWidth())
            // next right
            widgetLayout.mWidget->setOrg (
              prevWidget->getOrg() + cPointF (prevWidget->getWidth() + widgetLayout.mOffset.x, 0.f));
          else
            // belowLeft
            widgetLayout.mWidget->setOrg (cPointF(0.f, boundingSize.y + widgetLayout.mOffset.y));
          break;

        case cWidgetLayout::eLayout::eBelowLeft:
          // belowLeft
          widgetLayout.mWidget->setOrg (cPointF(0.f, boundingSize.y + widgetLayout.mOffset.y));
          break;

        case cWidgetLayout::eLayout::eAt:
          widgetLayout.mWidget->setOrg (widgetLayout.mOffset);
          break;
        }

      // calc boundingSize
      boundingSize.x = std::max (boundingSize.x, widgetLayout.mWidget->getEndX());
      boundingSize.y = std::max (boundingSize.y, widgetLayout.mWidget->getEndY());

      prevWidget = widgetLayout.mWidget;
      }

    if (!mSized) {
      mSize = boundingSize;
      mLayoutSize = boundingSize;
      }

    cLog::log (LOGINFO, "layoutWidgetsInContainer " + getId() +
                        " " + dec (boundingSize.x) + "," + dec(boundingSize.y));
    }
  //}}}

  const bool mSized;
  std::vector <cWidgetLayout> mWidgetLayouts;
  };
