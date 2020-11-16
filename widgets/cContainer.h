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
  virtual ~cContainer() {}

  //{{{
  cWidget* add (cWidget* widget) { // first addTopLeft, else addRight

    adjustWidthHeight (widget);

    if (mSubWidgets.empty())
      return addAt (widget, cPointF());
    else if (mSubWidgets.back()->getPixX() + mSubWidgets.back()->getPixWidth() + widget->getPixWidth() <= getPixWidth())
      return addAt (widget, cPointF (mSubWidgets.back()->getPixX() + mSubWidgets.back()->getPixWidth(), mSubWidgets.back()->getPixY()));
    else if (mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight() + widget->getPixHeight() <= getPixHeight())
      return addAt (widget, cPointF(0.f, mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight()));
    else
      return widget;
    }
  //}}}
  //{{{
  cWidget* addAt (cWidget* widget, const cPointF& p) {

    mSubWidgets.push_back (widget);
    widget->setParent (this);

    widget->setPixOrg (p);
    adjustWidthHeight (widget);

    return widget;
    }
  //}}}

  //{{{
  cWidget* addTopLeft (cWidget* widget) {
    return addAt (widget, cPointF());
    }
  //}}}
  //{{{
  cWidget* addTopRight (cWidget* widget) {
    return addAt (widget, cPointF(mPixSize.x - widget->getPixWidth(), 0.f));
    }
  //}}}
  //{{{
  cWidget* addBottomLeft (cWidget* widget) {
    return addAt (widget, cPointF(0.f, mPixSize.y - widget->getPixHeight()));
    }
  //}}}
  //{{{
  cWidget* addBottomRight (cWidget* widget) {
    return addAt (widget, cPointF(mPixSize.x - widget->getPixWidth(), mPixSize.y - widget->getPixHeight()));
    }
  //}}}

  //{{{
  cWidget* addBelow (cWidget* widget) {
    return addAt (widget, cPointF(mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX(),
                                  mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight()));
    }
  //}}}
  //{{{
  cWidget* addLeft (cWidget* widget) {
    return addAt (widget, cPointF(mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX() - mSubWidgets.back()->getPixWidth(),
                                 mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY()));
    }
  //}}}
  //{{{
  cWidget* addAbove (cWidget* widget) {
    return addAt (widget, cPointF(mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX(),
                                  mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY() - mSubWidgets.back()->getPixHeight()));
    }
  //}}}

  //{{{
  virtual cWidget* isPicked (const cPointF& point) {

    if (cWidget::isPicked (point)) {
      if (mSubWidgets.empty())
        return nullptr;
      else {
        for (auto it = mSubWidgets.rbegin(); it != mSubWidgets.rend(); ++it) {
          cWidget* pickedWidget = (*it)->isPicked (point);
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

    setPixSize (size);
    for (auto widget : mSubWidgets)
      widget->layout (size);
    }
  //}}}
  //{{{
  virtual void onDraw (iDraw* draw) {

    for (auto widget : mSubWidgets)
      if (widget->isVisible())
        widget->onDraw (draw);
    }
  //}}}

private:
  //{{{
  void adjustWidthHeight (cWidget* widget) {

    if (widget->getPixWidth() <= 0) // non +ve width means parent width minus our -ve width
      widget->setPixWidth (mPixSize.x + widget->getPixWidth());

    if (widget->getPixHeight() <= 0) // non +ve height means parent height minus our -ve height
      widget->setPixHeight (mPixSize.y + widget->getPixHeight());
    }
  //}}}

  std::vector <cWidget*> mSubWidgets;
  };
