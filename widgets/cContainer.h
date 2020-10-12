// cContainer.h - widget container, no background draw
//{{{  includes
#pragma once

#include <vector>
#include "cWidget.h"
//}}}

class cContainer : public cWidget {
public:
  cContainer() : cWidget() { mColour = kBlackF; }
  cContainer (float width, float height) : cWidget (kBlackF, width, height) {}
  virtual ~cContainer() {}

  //{{{
  cWidget* add (cWidget* widget) { // first addTopLeft, else addRight

    adjustWidthHeight (widget);

    if (mSubWidgets.empty())
      return addAtPix (widget, 0, 0);
    else if (mSubWidgets.back()->getPixX() + mSubWidgets.back()->getPixWidth() + widget->getPixWidth() <= getPixWidth())
      return addAtPix (widget, mSubWidgets.back()->getPixX() + mSubWidgets.back()->getPixWidth(), mSubWidgets.back()->getPixY());
    else if (mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight() + widget->getPixHeight() <= getPixHeight())
      return addAtPix (widget, 0, mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight());
    else
      return widget;
    }
  //}}}
  //{{{
  cWidget* addAt (cWidget* widget, float x, float y) {

    mSubWidgets.push_back (widget);
    widget->setParent (this);

    widget->setXY (x, y);
    adjustWidthHeight (widget);

    return widget;
    }
  //}}}
  //{{{
  cWidget* addAtPix (cWidget* widget, float x, float y) {

    widget->setParent (this);
    mSubWidgets.push_back (widget);

    widget->setPixXY (x, y);
    adjustWidthHeight (widget);

    return widget;
    }
  //}}}

  //{{{
  cWidget* addTopLeft (cWidget* widget) {
    return addAtPix (widget, 0, 0);
    }
  //}}}
  //{{{
  cWidget* addTopRight (cWidget* widget) {
    return addAtPix (widget, mWidth - widget->getPixWidth(), 0);
    }
  //}}}
  //{{{
  cWidget* addBottomLeft (cWidget* widget) {
    return addAtPix (widget, 0, mHeight - widget->getPixHeight());
    }
  //}}}
  //{{{
  cWidget* addBottomRight (cWidget* widget) {
    return addAtPix (widget, mWidth - widget->getPixWidth(),  mHeight - widget->getPixHeight());
    }
  //}}}

  //{{{
  cWidget* addBelow (cWidget* widget) {
    return addAtPix (widget,
                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX(),
                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY() + mSubWidgets.back()->getPixHeight());
    }
  //}}}
  //{{{
  cWidget* addLeft (cWidget* widget) {
    return addAtPix (widget,
                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX() - mSubWidgets.back()->getPixWidth(),
                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY());
    }
  //}}}
  //{{{
  cWidget* addAbove (cWidget* widget) {
    return addAtPix (widget,
                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixX(),
                     mSubWidgets.empty() ? 0 : mSubWidgets.back()->getPixY() - mSubWidgets.back()->getPixHeight());
    }
  //}}}

  //{{{
  virtual cWidget* isPicked (float x, float y) {

    if (cWidget::isPicked (x, y)) {
      if (mSubWidgets.empty())
        return nullptr;
      else {
        int i = (int)mSubWidgets.size();
        while (--i >= 0) {
          cWidget* pickedWidget = mSubWidgets[i]->isPicked (x, y);
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
  virtual void layout (float width, float height) {

    setPixSize (width, height);
    for (auto widget : mSubWidgets)
      widget->layout (width, height);
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
      widget->setPixWidth (mWidth + widget->getPixWidth());
    if (widget->getPixHeight() <= 0) // non +ve height means parent height minus our -ve height
      widget->setPixHeight (mHeight + widget->getPixHeight());
    }
  //}}}

  std::vector<cWidget*> mSubWidgets;
  };
