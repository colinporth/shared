// cPowerMapBox.h
//{{{  includes
#pragma once
#include <string>
#include <array>
#include "cWidget.h"

#include "../utils/utils.h"
//}}}

class cPowerMapBox : public cWidget {
public:
  cPowerMapBox (double& pts, std::map <uint64_t,std::array<float,6>>*& powerMap, int& chans,
                float width, float height) :
    cWidget (width, height), mPts(pts), mPowerMap(powerMap), mChans(chans) {}
  virtual ~cPowerMapBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    if (mPowerMap) {
      draw->rectClipped (COL_WHITE, mX + mWidth/2, mY + mHeight/4, 1, mHeight/2);

      auto context = draw->getContext();
      context->fillColor (nvgRGB32 (0xC00000FF));
      context->beginPath();

      float x = mX;
      float y = mY + mHeight/2;

      uint64_t uint64Pts = uint64_t(40.0 * mPts / 1000000.0);
      if (uint64Pts < (unsigned int)mWidth/4) {
        x += (mWidth/2) - (uint64Pts*2);
        uint64Pts = 0;
        }
      else
        uint64Pts -= mWidth/4;

      while (x < mX + mWidth) {
        auto powerIt = mPowerMap->find (uint64Pts);
        if (powerIt != mPowerMap->end()) {
          auto l = powerIt->second[0] * mHeight;
          auto r = powerIt->second[1] * mHeight;
          context->rect (x, y-l, 2.f, l+r);
          }
        uint64Pts++;
        x += 2.f;
        }

      context->triangleFill();
      }
    }
  //}}}

private:
  double& mPts;
  std::map <uint64_t,std::array<float,6>>*& mPowerMap;
  int& mChans;
  };
