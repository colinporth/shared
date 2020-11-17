// cTextBox.h
//{{{  includes
#pragma once
#include <string>
#include "cWidget.h"
//}}}

class cTextBox : public cWidget {
public:
  //{{{
  cTextBox (std::string& text, float width, const std::string id = "cTextBox")
    : cWidget (width, kBox, id), mText(text) {}
  //}}}
  //{{{
  cTextBox (std::string& text, const sColourF& colour, float width, float height, const std::string id = "cTextBox") :
    cWidget (colour, width, height, id), mText(text) {}
  //}}}
  virtual ~cTextBox() {}

  virtual std::string getDebugName() { return mId + " - " + mText; }

  virtual void setText (std::string& text) { mText = text; }
  virtual void setTextColour (const sColourF& colour) { mTextColour = colour; }

  //{{{
  void onDraw (iDraw* draw) {

    cWidget::onDraw (draw);
    draw->drawText (mTextColour, kFontHeight, mText, mOrg + cPointF(2.f, 1.f), mSize - cPointF(3.f, 1.f));
    }
  //}}}

protected:
  std::string& mText;
  sColourF mTextColour = kDarkGreyF;
  };
