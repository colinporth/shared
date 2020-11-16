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
  virtual void layout (const cPointF& size) {

    cWidget::layout (size);

    for (auto& widgetLayout : mWidgetLayouts)
      widgetLayout.mWidget->layout (getPixSize());

    layoutWidgets();
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
  // simple layoutWidgets rules

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
          else if (prevWidget->getPixOrg().x + prevWidget->getPixWidth() + widgetLayout.mWidget->getPixWidth() < getPixWidth())
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
      if (widgetLayout.mWidget->getPixOrg().x + widgetLayout.mWidget->getPixWidth() > boundingSize.x)
        boundingSize.x = widgetLayout.mWidget->getPixOrg().x + widgetLayout.mWidget->getPixWidth();
      if (widgetLayout.mWidget->getPixOrg().y + widgetLayout.mWidget->getPixHeight() > boundingSize.y)
        boundingSize.y = widgetLayout.mWidget->getPixOrg().x + widgetLayout.mWidget->getPixWidth();

      prevWidget = widgetLayout.mWidget;
      }
    }
  //}}}

  std::vector <cWidgetLayout> mWidgetLayouts;
  };
