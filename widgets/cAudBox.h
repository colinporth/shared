// cAudBox.h
//{{{  includes
#pragma once
#include <string>
#include "cWidget.h"

#include "../utils/utils.h"
//}}}
const float kBarsScale = 3.0f;  // need to work out power scaling derivation

class cAudBox : public cWidget {
public:
  cAudBox (float*& power, int& chans, float width, float height) :
    cWidget (width, height), mPower(power), mChans(chans) {}
  virtual ~cAudBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    if (mPower) {
      if (mChans == 2) {
        drawChanBar (draw, mX + 1, mY, mWidth/2, mHeight, mPower[0]);
        drawChanBar (draw, mX + 1 + mWidth/2, mY, mWidth/2, mHeight, mPower[1]);
        }
      else {
        drawChanBar (draw, mX + 1, mY, mWidth/5, mHeight, mPower[4]);
        drawChanBar (draw, mX + 1 + mWidth/5, mY, mWidth/5, mHeight, mPower[0]);
        drawChanBar (draw, mX + 1 + mWidth*2/5, mY, mWidth/5, mHeight, mPower[2]);
        drawChanBar (draw, mX + 1 + mWidth*3/5, mY, mWidth/5, mHeight, mPower[1]);
        drawChanBar (draw, mX + 1 + mWidth*4/5, mY, mWidth/5, mHeight, mPower[5]);
        //// W overlay
        //r.left = mRect.left + 1.f;
        //r.right = mRect.right - 1.f;
        //drawWooferBar (dc, r, audio->getMixedW(), mAudFrame->mPower[3]);
        }
      }
    }
  //}}}

private:
  //{{{
  void drawChanBar (iDraw* draw, int x, int y, int width, int height, float value) {

    y = y + height - int(value * kBarsScale * height);
    draw->rectClipped (COL_GREEN, x, y, width, height-y);
    }
  //}}}
  //{{{
  //void drawWooferBar (iDraw* draw, cRect& r, bool selected, float value) {

    //r.top = r.bottom - value / kBarsScale;
    //dc->FillRectangle (r, selected ? mBrush : mWindow->getGreyBrush());
    //}
  //}}}

  float*& mPower;
  int& mChans;
  };
