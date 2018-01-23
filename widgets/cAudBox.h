// cAudBox.h
//{{{  includes
#pragma once
#include <string>
#include <array>
#include "cWidget.h"

#include "../utils/utils.h"
//}}}
const float kBarsScale = 3.0f;  // need to work out power scaling derivation

class cAudBox : public cWidget {
public:
  cAudBox (std::array<float,6>& power, int& chans, float width, float height) :
    cWidget (width, height), mPower(power), mChans(chans) {}
  virtual ~cAudBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    if (mChans == 2) {
      drawChanBar (draw, mX + 1, mY, mWidth/2 -1, mHeight, mPower[0]);
      drawChanBar (draw, mX + 1 + mWidth/2 -1, mY, mWidth/2, mHeight, mPower[1]);
      }
    else {
      drawChanBar (draw, mX + 1, mY, mWidth/5 -1, mHeight, mPower[4]);
      drawChanBar (draw, mX + 1 + mWidth/5, mY, mWidth/5 -1, mHeight, mPower[0]);
      drawChanBar (draw, mX + 1 + mWidth*2/5, mY, mWidth/5 -1, mHeight, mPower[2]);
      drawChanBar (draw, mX + 1 + mWidth*3/5, mY, mWidth/5 -1, mHeight, mPower[1]);
      drawChanBar (draw, mX + 1 + mWidth*4/5, mY, mWidth/5 -1, mHeight, mPower[5]);
      drawWooferBar (draw, mX + 1, mY, mWidth-2, mHeight, mPower[3]);
      }
    }
  //}}}

private:
  //{{{
  void drawChanBar (iDraw* draw, int16_t x, int16_t y, uint16_t width, uint16_t height, float value) {

    y = y + height - int(value * kBarsScale * height);
    draw->rectClipped (COL_GREEN, x, y, width, height-y);
    }
  //}}}
  //{{{
  void drawWooferBar (iDraw* draw, int16_t x, int16_t y, uint16_t width, uint16_t height, float value) {

    y = y + height - int(value * kBarsScale * height);
    draw->rectClipped (0x80FF0000, x, y, width, height-y);
    }
  //}}}

  std::array<float,6>& mPower;
  int& mChans;
  };
