// cTimecodeBox.h
//{{{  includes
#pragma once
#include <string>
#include "cWidget.h"

#include "../utils/utils.h"
//}}}

class cTimecodeBox : public cWidget {
public:
  cTimecodeBox (double& playOmxPts, double& lengthOmxPts, float width, float height) :
    cWidget (width, height), mPlayOmxPts(playOmxPts), mLengthOmxPts(lengthOmxPts), mScale(height) {}
  virtual ~cTimecodeBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    draw->rectClipped (0x40000000, mX, mY, mWidth, mHeight);
    string str = getOmxPtsString(mPlayOmxPts) + " " + getOmxPtsString(mLengthOmxPts);
    draw->drawText (COL_WHITE, getFontHeight() * mScale, str, mX+2, mY+1, mWidth-3, mHeight-1);
    }
  //}}}

private:
  double& mPlayOmxPts;
  double& mLengthOmxPts;
  float mScale;
  };
