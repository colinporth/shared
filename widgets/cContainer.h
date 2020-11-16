// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>
#include "cWidget.h"
//}}}

class cContainer : public cWidget {
public:
  cContainer() : cWidget() { mColour = kBlackF; }
  //{{{
  cContainer (uint16_t widthInPixels, uint16_t heightInPixels, const std::string& name = "cContainer")
    : cWidget (kBlackF, widthInPixels, heightInPixels, name) {}
  //}}}
  //{{{
  cContainer (float widthInBoxes, float heightInBoxes, const std::string& name = "cContainer") 
    : cWidget (kBlackF, widthInBoxes, heightInBoxes, name) {}
  //}}}
  virtual ~cContainer() {}

  //{{{
  cWidget* add (cWidget* widget) { // first addTopLeft, else addRight

    adjustWidthHeight (widget);

    if (mSubWidgets.empty())
      return addAtPix (widget, cPointF());
    else if (mSubWidgets.back()->getPixX() + mSubWidgets.back()->getPixWidth() + widget->getPixWidth() <= getPixWidth())
      return addAtPix (widget, cPointF (mSubWidgets.back()->getPixX() + mSubWidgets.back()->getPixWidth(), mSubWidgets.back()->getPixY()));
    else if (mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight() + widget->getPixHeight() <= getPixHeight())
      return addAtPix (widget, cPointF(0.f, mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight()));
    else
      return widget;
    }
  //}}}
  //{{{
  cWidget* addAtBox (cWidget* widget, float x, float y) {

    mSubWidgets.push_back (widget);
    widget->setParent (this);

    widget->setPixOrg (cPointF (x * getBoxHeight(), y * getBoxHeight()));
    adjustWidthHeight (widget);

    return widget;
    }
  //}}}
  //{{{
  cWidget* addAtPix (cWidget* widget, const cPointF& p) {

    mSubWidgets.push_back (widget);
    widget->setParent (this);

    widget->setPixOrg (p);
    adjustWidthHeight (widget);

    return widget;
    }
  //}}}

  //{{{
  cWidget* addTopLeft (cWidget* widget) {
    return addAtPix (widget, cPointF());
    }
  //}}}
  //{{{
  cWidget* addTopRight (cWidget* widget) {
    return addAtPix (widget, cPointF(mPixSize.x - widget->getPixWidth(), 0.f));
    }
  //}}}
  //{{{
  cWidget* addBottomLeft (cWidget* widget) {
    return addAtPix (widget, cPointF(0.f, mPixSize.y - widget->getPixHeight()));
    }
  //}}}
  //{{{
  cWidget* addBottomRight (cWidget* widget) {
    return addAtPix (widget, cPointF(mPixSize.x - widget->getPixWidth(), mPixSize.y - widget->getPixHeight()));
    }
  //}}}

  //{{{
  cWidget* addBelow (cWidget* widget) {
    return addAtPix (widget, cPointF(mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX(),
                                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight()));
    }
  //}}}
  //{{{
  cWidget* addLeft (cWidget* widget) {
    return addAtPix (widget, cPointF(mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX() - mSubWidgets.back()->getPixWidth(),
                                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY()));
    }
  //}}}
  //{{{
  cWidget* addAbove (cWidget* widget) {
    return addAtPix (widget, cPointF(mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX(),
                                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY() - mSubWidgets.back()->getPixHeight()));
    }
  //}}}

  //{{{
  virtual cWidget* isPicked (const cPointF& p) {

    if (cWidget::isPicked (p)) {
      if (mSubWidgets.empty())
        return nullptr;
      else {
        int i = (int)mSubWidgets.size();
        while (--i >= 0) {
          cWidget* pickedWidget = mSubWidgets[i]->isPicked (p);
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

protected:
  //{{{
  void adjustWidthHeight (cWidget* widget) {

    if (widget->getPixWidth() <= 0) // non +ve width means parent width minus our -ve width
      widget->setPixWidth (mPixSize.x + widget->getPixWidth());
    if (widget->getPixHeight() <= 0) // non +ve height means parent height minus our -ve height
      widget->setPixHeight (mPixSize.y + widget->getPixHeight());
    }
  //}}}

  std::vector<cWidget*> mSubWidgets;
  };
