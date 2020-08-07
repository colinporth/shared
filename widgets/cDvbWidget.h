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

  //{{{
  virtual void onDown (float x, float y) {

    cLog::log (LOGINFO, "dvbWidget onDown " + dec(x) + " " + dec(y));
    }
  //}}}
  //{{{
  virtual void onMove (float x, float y, float z, float xinc, float yinc) {
    cLog::log (LOGINFO, "dvbWidget onMove " + dec(x) + "," + dec(y) + "," + dec(z) +
                         " " + dec(xinc) + "," + dec(yinc));
    mScroll += yinc; 
    }
  //}}}
  //{{{
  virtual void onUp() {

    cLog::log (LOGINFO, "dvbWidget onUp");
    }
  //}}}

  //{{{
  virtual void onWheel (float delta) {
    cLog::log (LOGINFO, "onwheel " + dec(delta));
    }
  //}}}

  virtual void onDraw (iDraw* draw) {

    int lastSid = 0;
    int imageIndex = 0;
    auto context = draw->getContext();

    float x = mX + 2.f;
    float y = mY + 1.f + mScroll;
    float lineHeight = getBoxHeight() * 4.f/5.f;
    for (auto &pidInfoItem : mDvb->getTransportStream()->mPidInfoMap) {
      // draw pidInfo
      cPidInfo& pidInfo = pidInfoItem.second;
      int pid = pidInfo.mPid;

      if (pidInfo.mSid != lastSid) {
        //{{{  spacer on change of service
        lastSid = pidInfo.mSid;
        draw->drawRect (COL_GREY, x, y, mWidth, 1.f);
        }
        //}}}

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
          float x3 = x2 + 44.f;
          float ySub = y - lineHeight;
          for (int line = (int)subtitle->mRects.size()-1; line >= 0; line--) {
            float dstWidth = mWidth - x3;
            float dstHeight = float(subtitle->mRects[line]->mHeight * dstWidth) / subtitle->mRects[line]->mWidth;
            if (dstHeight > lineHeight) {
              float scaleh = lineHeight / dstHeight;
              dstHeight = lineHeight;
              dstWidth *= scaleh;
              }

            if (mImage[imageIndex] == -1)
              mImage[imageIndex] = context->createImageRGBA (subtitle->mRects[line]->mWidth,
                                                             subtitle->mRects[line]->mHeight,
                                                             0, subtitle->mRects[line]->mPixData);
            else if (subtitle->mChanged)
              context->updateImage (mImage[imageIndex], subtitle->mRects[line]->mPixData);

            auto imagePaint = context->imagePattern (x3, ySub, dstWidth, dstHeight, 0.f, mImage[imageIndex], 1.f);
            imageIndex++;

            context->beginPath();
            context->rect (x3, ySub, dstWidth, dstHeight);
            context->fillPaint (imagePaint);
            context->fill();

            std::string text = dec(subtitle->mRects[line]->mX) +
                               "," + dec(subtitle->mRects[line]->mY);
            draw->drawText (COL_GREY, lineHeight, text, x3 + dstWidth - 55.f, ySub, dstWidth, dstHeight);

            for (int i = 0; i < subtitle->mRects[line]->mClutSize; i++) {
              float cx = x3 + (i % 8) * lineHeight / 2.f;
              float cy = ySub + (i / 8) * lineHeight / 2.f;
              draw->drawRect (subtitle->mRects[line]->mClut[i],
                              cx, cy, (lineHeight/2.f)-1.f, (lineHeight / 2.f) - 1.f);
              }

            ySub += lineHeight;
            }
          subtitle->mChanged = false;
          }
        }
        //}}}

      mMaxPidPackets = std::max (mMaxPidPackets, (float)pidInfo.mPackets);
      float frac = pidInfo.mPackets / mMaxPidPackets;
      draw->drawRect (COL_DARKORANGE,x2, y, (mWidth - textWidth) * frac, lineHeight-1.f);
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

  float mScroll = 0.f;
  int mImage[20] =  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  };
