// cSubtitleWidget.h
//{{{  includes
#pragma once

#include "../utils/cLog.h"
#include "../dvb/cSubtitleDecoder.h"

#include "cWidget.h"
//}}}

class cSubtitleWidget : public cWidget {
public:
  cSubtitleWidget (cSubtitle& subtitle, float height, float width)
    : cWidget(width, height), mSubtitle(subtitle)  {}

  virtual ~cSubtitleWidget() {
    // should delete images, no context pointer ???
    }

  void onDraw (iDraw* draw) {

    float y = mY;
    if (!mSubtitle.mRects.empty()) {
      auto context = draw->getContext();

      for (unsigned int i = 0; i < mSubtitle.mRects.size(); i++) {
        int subWidth = mSubtitle.mRects[i]->mWidth;
        int subHeight = mSubtitle.mRects[i]->mHeight;

        float dstWidth = mWidth;
        float dstHeight = float(subHeight * mWidth) / subWidth;

        if (mImage[i] == -1)
          mImage[i] = context->createImageRGBA (subWidth, subHeight, 0, mSubtitle.mRects[i]->mPixelData);
        else if (mSubtitle.mChanged)
          context->updateImage (mImage[i], mSubtitle.mRects[i]->mPixelData);

        auto imgPaint = context->imagePattern (mX, y, dstWidth, dstHeight, 0.f, mImage[i], 1.f);
        context->beginPath();
        context->rect (mX, y, dstWidth, dstHeight);
        context->fillPaint (imgPaint);
        context->fill();
        y += dstHeight;
        }
      }
    mSubtitle.mChanged = false;
    }

private:
  cSubtitle& mSubtitle;

  int mImage[2] =  { -1 };
  };
