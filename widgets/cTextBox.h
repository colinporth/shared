// cTextBox.h
#pragma once
#include <string>
#include "cWidget.h"

class cTextBox : public cWidget {
public:
  cTextBox (std::string& text, float width) : cWidget (width), mText(text) {}
  cTextBox (std::string& text, uint32_t colour, uint16_t width, uint16_t height) : cWidget (colour, width, height), mText(text) {}
  virtual ~cTextBox() {}

  void setText (std::string& text) { mText = text; }
  void setTextColour (uint32_t colour) { mTextColour = colour; }

  //{{{

  void onDraw (iDraw* draw) {
    cWidget::onDraw (draw);
    draw->drawText (mTextColour, getFontHeight(), mText, mX+2, mY+1, mWidth-3, mHeight-1);
    }
  //}}}

protected:
  std::string& mText;
  uint32_t mTextColour = COL_DARKGREY;
  };
