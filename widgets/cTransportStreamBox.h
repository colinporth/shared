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

    auto y = 0;
    auto lineHeight = getFontHeight()*2/3;

    if (mTs->mPidInfoMap.size()) {
      for (auto &pidInfo : mTs->mPidInfoMap) {
        mMaxPidPackets = max (mMaxPidPackets, pidInfo.second.mPackets);
        auto frac = (float)pidInfo.second.mPackets / (float)mMaxPidPackets;

        auto str = dec (pidInfo.second.mPackets,mPacketDigits) +
                   (mContDigits ? (":" + dec(pidInfo.second.mDisContinuity, mContDigits)) : "") +
                   " " + dec(pidInfo.first, 4) +
                   " " + getFullPtsString (pidInfo.second.mPts) +
                   " " + pidInfo.second.getTypeString();
        auto width = draw->drawText (COL_LIGHTGREY, lineHeight, str,
                                     mX+2, mY+y+1, mWidth-3, lineHeight);

        draw->rectClipped (COL_DARKORANGE, mX+2 + width + lineHeight/2, mY+y+1,
                           int((mWidth - width) * frac), lineHeight - 1);

        draw->drawText (COL_LIGHTGREY, lineHeight,
                        pidInfo.second.getInfoString() +
                        " " + dec(frac) +
                        " " + dec(width) +
                        " " + dec(mWidth) +
                        " " + dec(mMaxPidPackets),
                        mX + 2 + width + lineHeight/2, mY+y+1, mWidth-3, lineHeight);
        y += lineHeight;
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

  int mContDigits = 0;
  int mPacketDigits = 1;
  int mMaxPidPackets = 0;
  };
