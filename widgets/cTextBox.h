// cTextBox.h
//{{{  includes
#pragma once
#include <string>
#include "cWidget.h"
//}}}

class cTextBox : public cWidget {
public:
  cTextBox (std::string& text, float width, const std::string name = "cTextBox")
    : cWidget (width, name), mText(text) {}
  cTextBox (std::string& text, const sColourF& colour, uint16_t width, uint16_t height, const std::string name = "cTextBox") :
    cWidget (colour, width, height, name), mText(text) {}
  virtual ~cTextBox() {}

  virtual void setText (std::string& text) { mText = text; }
  virtual void setTextColour (const sColourF& colour) { mTextColour = colour; }

  //{{{
  void onDraw (iDraw* draw) {

    cWidget::onDraw (draw);
    draw->drawText (mTextColour, getFontHeight(), mText, mPixOrg + cPointF(2.f, 1.f), mPixSize - cPointF(3.f, 1.f));
    }
  //}}}

protected:
  std::string& mText;
  sColourF mTextColour = kDarkGreyF;
  };
