// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>
#include "cWidget.h"
//}}}

class cContainer : public cWidget {
public:
  //{{{
  cContainer (float width, float height, const std::string& debugName = "cContainer")
    : cWidget (kBlackF, width, height, debugName) {}
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
      widgetLayout.mWidget->layout();

    layoutWidgets();
    }
  //}}}
  //{{{
  virtual void layout() {

    cWidget::layout();

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout.mWidget->layout();

    layoutWidgets();
    }
  //}}}

  //{{{
  void add (cWidget* widget, float offset = 0.f) {
    addWidget (widget, cWidgetLayout::eLayout::eNext, cPointF (offset, offset));
    }
  //}}}
  //{{{
  void addTopLeft (cWidget* widget) {
    addWidget (widget, cWidgetLayout::eLayout::eAt, cPointF());
    }
  //}}}
  //{{{
  void addAt (cWidget* widget, const cPointF& offset) {
    addWidget (widget, cWidgetLayout::eLayout::eAt, offset);
    }
  //}}}
  //{{{
  void addBelowLeft (cWidget* widget, float offset = 0.f) {
    addWidget (widget, cWidgetLayout::eLayout::eBelowLeft, cPointF(0.f, offset));
    }
  //}}}

  //{{{
  virtual cWidget* isPicked (const cPointF& point) {

    if (cWidget::isPicked (point)) {
      if (mWidgetLayouts.empty())
        return nullptr;
      else {
        for (auto it = mWidgetLayouts.rbegin(); it != mWidgetLayouts.rend(); ++it) {
          cWidget* pickedWidget = (*it).mWidget->isPicked (point);
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
    enum class eLayout { eAt, eNext, eBelowLeft };
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
    widget->setParent (this);
    layoutWidgets();
    }
  //}}}
  //{{{
  void layoutWidgets() {

    cPointF boundingSize;
    cWidget* prevWidget = nullptr;

    for (auto& widgetLayout : mWidgetLayouts) {
      switch (widgetLayout.mLayout) {
        case cWidgetLayout::eLayout::eAt:
          widgetLayout.mWidget->setOrg (widgetLayout.mOffset);
          break;

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
        }

      // calc boundingSize
      boundingSize.x = std::max (boundingSize.x, widgetLayout.mWidget->getEndX());
      boundingSize.y = std::max (boundingSize.y, widgetLayout.mWidget->getEndY());

      prevWidget = widgetLayout.mWidget;
      }

    //cLog::log (LOGINFO, "layoutWidgets " + getDebugName() +
    //                    " " + dec (boundingSize.x) + "," + dec(boundingSize.y));
    }
  //}}}

  std::vector <cWidgetLayout> mWidgetLayouts;
  };
