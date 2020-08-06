// cSubtitleWidget.h
//{{{  includes
#pragma once

#include "../utils/cLog.h"
#include "../dvb/cDvb.h"
#include "../dvb/cSubtitleDecoder.h"

#include "cWidget.h"
//}}}

class cSubtitleWidget : public cWidget {
public:
  cSubtitleWidget (cDvb* dvb, float height, float width)
    : cWidget(width, height), mDvb(dvb)  {}

  virtual ~cSubtitleWidget() {
    // should delete images, no context pointer ???
    }

  void onDraw (iDraw* draw) {

    int numServices = mDvb->getNumSubtitleServices();
    if (numServices) {
      auto context = draw->getContext();

      float y = mY;
      int widgetLine = 0;
      for (int serviceIndex = 0; serviceIndex < numServices; serviceIndex++) {
        std::string debugString;
        cSubtitle* subtitle = mDvb->getSubtitle (serviceIndex, debugString);
        if (!subtitle->mRects.empty()) {
          for (size_t subtitleLine = 0; subtitleLine < subtitle->mRects.size(); subtitleLine++) {
            int subWidth = subtitle->mRects[subtitleLine]->mWidth;
            int subHeight = subtitle->mRects[subtitleLine]->mHeight;
            float dstWidth = mWidth;
            float dstHeight = float(subHeight * mWidth) / subWidth;
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

            auto imagePaint = context->imagePattern (mX, y, dstWidth, dstHeight, 0.f, mImage[widgetLine], 1.f);
            context->beginPath();
            context->rect (mX, y, dstWidth, dstHeight);
            context->fillPaint (imagePaint);
            context->fill();

            uint16_t lineHeight = 15;
            std::string text = dec(subtitle->mRects[subtitleLine]->mX) +
                               "," + dec(subtitle->mRects[subtitleLine]->mY) +
                               " " + debugString;
            draw->drawText (COL_GREY, lineHeight, text, mX + uint16_t(dstWidth) -130, (int16_t)y, uint16_t(dstWidth), uint16_t(dstHeight));

            y += dstHeight;
            widgetLine++;
            }
          }
        subtitle->mChanged = false;
        }
      }
    }

private:
  cDvb* mDvb;
  int mImage[16] =  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  };
