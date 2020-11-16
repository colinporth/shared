// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>
#include "cWidget.h"
//}}}

class cContainer : public cWidget {
public:
  //{{{
  cContainer (uint16_t widthInPix, uint16_t heightInPix, const std::string& debugName = "cContainer")
    : cWidget (kBlackF, widthInPix, heightInPix, debugName) {}
  //}}}
  //{{{
  cContainer (float widthInBoxes, float heightInBoxes, const std::string& debugName = "cContainer")
    : cWidget (kBlackF, widthInBoxes, heightInBoxes, debugName) {}
  //}}}
  //{{{
  virtual ~cContainer() {

    for (auto& widgetLayout : mWidgetLayouts)
      delete widgetLayout.mWidget;

    mWidgetLayouts.clear();
    }
  //}}}

  //{{{
  virtual void setPixSize (const cPointF& size) {

    cWidget::setPixSize (size);

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
          widgetLayout.mWidget->setPixOrg (widgetLayout.mOffset);
          break;

        case cWidgetLayout::eLayout::eNext:
          if (!prevWidget)
            // topLeft
            widgetLayout.mWidget->setPixOrg (widgetLayout.mOffset);
          else if (prevWidget->getPixEndX() + widgetLayout.mWidget->getPixWidth() < getPixWidth())
            // next right
            widgetLayout.mWidget->setPixOrg (
              prevWidget->getPixOrg() + cPointF (prevWidget->getPixWidth() + widgetLayout.mOffset.x, 0.f));
          else
            // belowLeft
            widgetLayout.mWidget->setPixOrg (cPointF(0.f, boundingSize.y + widgetLayout.mOffset.y));
          break;

        case cWidgetLayout::eLayout::eBelowLeft:
          // belowLeft
          widgetLayout.mWidget->setPixOrg (cPointF(0.f, boundingSize.y + widgetLayout.mOffset.y));
          break;
        }

      // calc boundingSize
      boundingSize.x = std::max (boundingSize.x, widgetLayout.mWidget->getPixEndX());
      boundingSize.y = std::max (boundingSize.y, widgetLayout.mWidget->getPixEndY());

      prevWidget = widgetLayout.mWidget;
      }

    //cLog::log (LOGINFO, "layoutWidgets " + getDebugName() +
    //                    " " + dec (boundingSize.x) + "," + dec(boundingSize.y));
    }
  //}}}

  std::vector <cWidgetLayout> mWidgetLayouts;
  };
