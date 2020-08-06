// cDvbWidget.h
//{{{  includes
#pragma once

#include <string>
#include <algorithm>
#include "../utils/utils.h"

#include "../dvb/cDvb.h"
#include "../dvb/cTransportStream.h"
#include "../dvb/cSubtitle.h"


#include "cWidget.h"
//}}}

class cDvbWidget: public cWidget {
public:
  //{{{
  cDvbWidget (cDvb* dvb, float height, float width) :
    cWidget (height, width), mDvb(dvb) {}
  //}}}
  virtual ~cDvbWidget() {}

  void onDraw (iDraw* draw) {

    int imageIndex = 0;
    auto context = draw->getContext();

    float x = mX + 2.f;
    float y = mY + 1.f;
    float lineHeight = getBoxHeight() * 4.f/5.f;
    for (auto &pidInfoItem : mDvb->getTransportStream()->mPidInfoMap) {
      // draw pidInfo

      cPidInfo& pidInfo = pidInfoItem.second;
      int pid = pidInfo.mPid;

      mMaxPidPackets = std::max (mMaxPidPackets, (float)pidInfo.mPackets);
      float frac = pidInfo.mPackets / mMaxPidPackets;

      auto pidString = dec (pidInfo.mPackets,mPacketDigits) +
                       (mContDigits ? (":" + dec(pidInfo.mErrors, mContDigits)) : "") +
                       " " + dec(pid, 4) +
                       " " + getFullPtsString (pidInfo.mPts) +
                       " " + pidInfo.getTypeString();
      float textWidth = draw->drawText (COL_LIGHTGREY, lineHeight, pidString, x, y, mWidth-3.f, lineHeight);

      float x2 = x + textWidth + lineHeight/2;
      if (pidInfo.mStreamType == 6) {
        //{{{  draw subtitle
        cSubtitle* subtitle = mDvb->getSubtitleBySid (pidInfo.mSid);
        if (subtitle && !subtitle->mRects.empty()) {
          float x3 = x2 + 40.f;
          float y2 = y;
          for (size_t subtitleLine = 0; subtitleLine < subtitle->mRects.size(); subtitleLine++) {
            int subWidth = subtitle->mRects[subtitleLine]->mWidth;
            int subHeight = subtitle->mRects[subtitleLine]->mHeight;
            float dstWidth = mWidth - x3;
            float dstHeight = float(subtitle->mRects[subtitleLine]->mHeight * dstWidth) / subtitle->mRects[subtitleLine]->mWidth;
            if (dstHeight > lineHeight) {
              float scaleh = lineHeight / dstHeight;
              dstHeight = lineHeight;
              dstWidth *= scaleh;
              }

            if (mImage[imageIndex] == -1)
              mImage[imageIndex] = context->createImageRGBA (subtitle->mRects[subtitleLine]->mWidth,
                                                             subtitle->mRects[subtitleLine]->mHeight,
                                                             0, subtitle->mRects[subtitleLine]->mPixData);
            else if (subtitle->mChanged)
              context->updateImage (mImage[imageIndex], subtitle->mRects[subtitleLine]->mPixData);

            auto imagePaint = context->imagePattern (x3, y2, dstWidth, dstHeight, 0.f, mImage[imageIndex], 1.f);
            imageIndex++;

            context->beginPath();
            context->rect (x3, y2, dstWidth, dstHeight);
            context->fillPaint (imagePaint);
            context->fill();

            std::string text = dec(subtitle->mRects[subtitleLine]->mX) +
                               "," + dec(subtitle->mRects[subtitleLine]->mY);
            draw->drawText (COL_GREY, lineHeight, text, x3 + dstWidth - 55.f, y2, dstWidth, dstHeight);

            y2 += lineHeight;
            }
          subtitle->mChanged = false;
          }
        }
        //}}}

      draw->rectClipped (COL_DARKORANGE,x2, y, (mWidth - textWidth) * frac, lineHeight-1);
      draw->drawText (COL_LIGHTGREY, lineHeight, pidInfo.getInfoString(), x2, y, mWidth - textWidth, lineHeight);

      if (pidInfo.mPackets > pow (10, mPacketDigits))
        mPacketDigits++;

      y += lineHeight;
      }

    if (mDvb->getTransportStream()->getErrors() > pow (10, mContDigits))
      mContDigits++;
    }

private:
  cDvb* mDvb;
  int mContDigits = 0;
  int mPacketDigits = 1;
  float mMaxPidPackets = 0;
  int mImage[16] =  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  };
