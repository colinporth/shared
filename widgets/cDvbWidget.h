// cDvbWidget.h
//{{{  includes
#pragma once

#include <string>
#include <algorithm>
#include "../utils/utils.h"

#include "../dvb/cDvb.h"
#include "../dvb/cTransportStream.h"

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

    int numServices = mDvb->getNumSubtitleServices();
    if (numServices) {
      auto context = draw->getContext();

      float x = mX + 12.f * getBoxHeight();
      float y = mY;
      int widgetLine = 0;
      for (int serviceIndex = 0; serviceIndex < numServices; serviceIndex++) {
        //{{{  draw service subtitle
        std::string debugString;
        cSubtitle* subtitle = mDvb->getSubtitle (serviceIndex, debugString);
        if (!subtitle->mRects.empty()) {
          for (size_t subtitleLine = 0; subtitleLine < subtitle->mRects.size(); subtitleLine++) {
            int subWidth = subtitle->mRects[subtitleLine]->mWidth;
            int subHeight = subtitle->mRects[subtitleLine]->mHeight;
            float dstWidth = mWidth - x;
            float dstHeight = float(subHeight * dstWidth) / subWidth;
            if (dstHeight > mHeight) {
              float scaleh = mHeight / dstHeight;
              dstHeight = mHeight;
              dstWidth *= scaleh;
              }

            if (mImage[widgetLine] == -1)
              mImage[widgetLine] = context->createImageRGBA (
                subWidth, subHeight, 0, subtitle->mRects[subtitleLine]->mPixData);
            else if (subtitle->mChanged)
              context->updateImage (mImage[widgetLine], subtitle->mRects[subtitleLine]->mPixData);

            auto imagePaint = context->imagePattern (x, y, dstWidth, dstHeight, 0.f, mImage[widgetLine], 1.f);
            context->beginPath();
            context->rect (x, y, dstWidth, dstHeight);
            context->fillPaint (imagePaint);
            context->fill();

            uint16_t lineHeight = 15;
            std::string text = dec(subtitle->mRects[subtitleLine]->mX) +
                               "," + dec(subtitle->mRects[subtitleLine]->mY) +
                               " " + debugString;
            draw->drawText (COL_GREY, lineHeight, text, uint16_t(x+dstWidth) - 130, (int16_t)y, uint16_t(dstWidth), uint16_t(dstHeight));

            y += dstHeight;
            widgetLine++;
            }
          }
        subtitle->mChanged = false;
        }
        //}}}
      }

    int y = mY+1;
    auto lineHeight = getFontHeight()*2/3;
    for (auto &pidInfoItem : mDvb->getTransportStream()->mPidInfoMap) {
      //{{{  draw pidInfo
      int pid = pidInfoItem.first;
      cPidInfo& pidInfo = pidInfoItem.second;

      mMaxPidPackets = std::max (mMaxPidPackets, pidInfo.mPackets);
      auto frac = (float)pidInfo.mPackets / (float)mMaxPidPackets;
      auto x = mX+2;

      auto pidString = dec (pidInfo.mPackets,mPacketDigits) +
                       (mContDigits ? (":" + dec(pidInfo.mErrors, mContDigits)) : "") +
                       " " + dec(pid, 4) +
                       " " + getFullPtsString (pidInfo.mPts) +
                       " " + pidInfo.getTypeString();
      auto width = draw->drawText (COL_LIGHTGREY, lineHeight, pidString, x, y, mWidth-3, lineHeight);

      draw->rectClipped (COL_DARKORANGE, x + width + lineHeight/2, y, int((mWidth - width) * frac), lineHeight-1);
      draw->drawText (COL_LIGHTGREY, lineHeight, pidInfo.getInfoString(),
                      x + width + lineHeight/2, y, mWidth-width, lineHeight);

      if (pidInfo.mPackets > pow (10, mPacketDigits))
        mPacketDigits++;

      y += lineHeight;
      }
      //}}}

    if (mDvb->getTransportStream()->getErrors() > pow (10, mContDigits))
      mContDigits++;
    }

private:
  cDvb* mDvb;
  int mContDigits = 0;
  int mPacketDigits = 1;
  int mMaxPidPackets = 0;
  int mImage[16] =  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  };
