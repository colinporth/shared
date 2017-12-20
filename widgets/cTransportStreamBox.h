// cTransportStreamBox.h
//{{{  includes
#pragma once
#include <string>
#include "../utils/utils.h"
#include "../decoders/cTransportStream.h"

#include "cWidget.h"
//}}}

class cTransportStreamBox : public cWidget {
public:
  //{{{
  cTransportStreamBox (float height, float width, cTransportStream* ts) :
    cWidget (height, width), mTs(ts) {}
  //}}}
  virtual ~cTransportStreamBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    //draw->drawText (mTextColour, getFontHeight(), mText, mX+2, mY+1, mWidth-3, mHeight-1);
    //draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX+1, mY+1, mWidth-1, mHeight-1);

    //auto serviceWidth = 0.f;
    auto y = mY;
    if (mTs->mPidInfoMap.size()) {
      auto maxPidPackets = 10000;
      for (auto &pidInfo : mTs->mPidInfoMap)
        maxPidPackets = max (maxPidPackets, pidInfo.second.mPackets);

      for (auto &pidInfo : mTs->mPidInfoMap) {
        auto str = dec (pidInfo.second.mPackets,mPacketDigits) +
                   (mContDigits ? (":" + dec(pidInfo.second.mDisContinuity, mContDigits)) : "") +
                   " " + dec(pidInfo.first, 4) +
                   " " + getFullPtsString (pidInfo.second.mPts) +
                   " " + pidInfo.second.getTypeString();
        auto width = draw->drawText (mTextColour, getFontHeight(), str, mX+2, y+1, mWidth-3, mHeight-1);

        auto x = mX + width;
        draw->drawText (mTextColour, getFontHeight()/2, pidInfo.second.getInfoString(), x+2, y+1, mWidth-3, mHeight-1);
        y += getFontHeight()/2;
        if (y > mHeight)
          break;

        if (pidInfo.second.mPackets > pow (10, mPacketDigits))
          mPacketDigits++;
        }

      if (mTs->getDiscontinuity() > pow (10, mContDigits))
        mContDigits++;
      }
    }
  //}}}

private:
  cTransportStream* mTs;
  uint32_t mTextColour = COL_DARKGREY;

  int mContDigits = 0;
  int mPacketDigits = 1;
  };
