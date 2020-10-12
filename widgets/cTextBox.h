// cTextBox.h
//{{{  includes
#pragma once
#include <string>
#include "cWidget.h"
//}}}

class cTextBox : public cWidget {
public:
  cTextBox (std::string& text, float width) : cWidget (width), mText(text) {}
  cTextBox (std::string& text, const sColourF& colour, uint16_t width, uint16_t height) :
    cWidget (colour, width, height), mText(text) {}
  virtual ~cTextBox() {}

  void setText (std::string& text) { mText = text; }
  void setTextColour (const sColourF& colour) { mTextColour = colour; }

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
