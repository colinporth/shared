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

    int numServices = mDvb->getNumServices();
    if (numServices) {
      auto context = draw->getContext();

      float y = mY;
      int widgetLine = 0;
      for (int serviceIndex = 0; serviceIndex < numServices; serviceIndex++) {
        cSubtitle* subtitle = mDvb->getSubtitle (serviceIndex);
        if (subtitle) {
          if (!subtitle->mRects.empty()) {
            for (size_t subtitleLine = 0; subtitleLine < subtitle->mRects.size(); subtitleLine++) {
              int subWidth = subtitle->mRects[subtitleLine]->mWidth;
              int subHeight = subtitle->mRects[subtitleLine]->mHeight;
              float dstWidth = mWidth;
              float dstHeight = float(subHeight * mWidth) / subWidth;

              if (mImage[widgetLine] == -1)
                mImage[widgetLine] = context->createImageRGBA (subWidth, subHeight, 0, subtitle->mRects[subtitleLine]->mPixData);
              else if (subtitle->mChanged)
                context->updateImage (mImage[widgetLine], subtitle->mRects[subtitleLine]->mPixData);

              auto imagePaint = context->imagePattern (mX, y, dstWidth, dstHeight, 0.f, mImage[widgetLine], 1.f);

              context->beginPath();
              context->rect (mX, y, dstWidth, dstHeight);
              context->fillPaint (imagePaint);
              context->fill();

              y += dstHeight;
              widgetLine++;
              }
            }
          subtitle->mChanged = false;
          }
        }
      }
    }

private:
  cDvb* mDvb;
  int mImage[20] =  { -1 };
  };
