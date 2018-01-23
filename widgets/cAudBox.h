// cAudBox.h
//{{{  includes
#pragma once
#include <string>
#include <array>
#include "cWidget.h"

#include "../utils/utils.h"
//}}}
const float kBarsScale = 3.0f;  

class cAudBox : public cWidget {
public:
  cAudBox (std::array<float,6>& power, int& chans, float width, float height) :
    cWidget (width, height), mPower(power), mChans(chans) {}
  virtual ~cAudBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    if (mChans == 2) {
      drawBar (draw, 0xC000FF00, mX + 1,              mY, (mWidth/2)-1, mHeight, mPower[0]);
      drawBar (draw, 0xC000FF00, mX + 1 + mWidth/2,   mY, (mWidth/2)-1, mHeight, mPower[1]);
      }
    else {
      drawBar (draw, 0xC000FF00, mX + 1,              mY, (mWidth/5)-1, mHeight, mPower[4]);
      drawBar (draw, 0xC000FF00, mX + 1 + mWidth/5,   mY, (mWidth/5)-1, mHeight, mPower[0]);
      drawBar (draw, 0xC000FF00, mX + 1 + mWidth*2/5, mY, (mWidth/5)-1, mHeight, mPower[2]);
      drawBar (draw, 0xC000FF00, mX + 1 + mWidth*3/5, mY, (mWidth/5)-1, mHeight, mPower[1]);
      drawBar (draw, 0xC000FF00, mX + 1 + mWidth*4/5, mY, (mWidth/5)-1, mHeight, mPower[5]);
      drawBar (draw, 0x80FF0000, mX + 1,              mY,     mWidth-2, mHeight, mPower[3]);
      }
    }
  //}}}

private:
  //{{{
  void drawBar (iDraw* draw, uint32_t colour, int16_t x, int16_t y, uint16_t width, uint16_t height, float value) {

    y += height - int(value * kBarsScale * height);
    draw->rectClipped (colour, x, y, width, height-y);
    }
  //}}}

  std::array<float,6>& mPower;
  int& mChans;
  };
