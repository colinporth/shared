// cTransportStreamBox.h
//{{{  includes
#pragma once

#include <string>
#include <algorithm>
#include "../utils/utils.h"
#include "../decoders/cTransportStream.h"

#include "cWidget.h"
//}}}

class cTransportStreamBox : public cWidget {
public:
  //{{{
  cTransportStreamBox (cTransportStream* ts, float height, float width) :
    cWidget (height, width), mTs(ts) {}
  //}}}
  virtual ~cTransportStreamBox() {}

  //{{{
  void onDraw (iDraw* draw) {

    auto y = mY+1;
    auto lineHeight = getFontHeight()*2/3;

    if (mTs->mPidInfoMap.size()) {
      for (auto &pidInfo : mTs->mPidInfoMap) {
        mMaxPidPackets = std::max (mMaxPidPackets, pidInfo.second.mPackets);
        auto frac = (float)pidInfo.second.mPackets / (float)mMaxPidPackets;
        auto x = mX+2;

        auto str = dec (pidInfo.second.mPackets,mPacketDigits) +
                   (mContDigits ? (":" + dec(pidInfo.second.mDisContinuity, mContDigits)) : "") +
                   " " + dec(pidInfo.first, 4) +
                   " " + getFullPtsString (pidInfo.second.mPts) +
                   " " + pidInfo.second.getTypeString();

        auto width = draw->drawText (COL_LIGHTGREY, lineHeight, str,
                                     x, y, mWidth-3, lineHeight);
        draw->rectClipped (COL_DARKORANGE,
                           x + width + lineHeight/2, y, int((mWidth - width) * frac), lineHeight - 1);
        draw->drawText (COL_LIGHTGREY, lineHeight, pidInfo.second.getInfoString(),
                        x + width + lineHeight/2, y, mWidth-width, lineHeight);

        y += lineHeight;
        if (y > mY + mHeight)
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

  int mContDigits = 0;
  int mPacketDigits = 1;
  int mMaxPidPackets = 0;
  };
