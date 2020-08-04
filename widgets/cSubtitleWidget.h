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
  virtual ~cSubtitleWidget() {}

  void onDraw (iDraw* draw) {

    if (!mSubtitle.mRects.empty()) {
      //int i = int(mSubtitle.mRects.size()) - 1;
      int i = 0;
      int subWidth = mSubtitle.mRects[i]->mWidth;
      int subHeight = mSubtitle.mRects[i]->mHeight;

      mScale = float(mWidth) / subWidth;
      float dstWidth = mWidth;
      float dstHeight = subHeight * mScale;

      auto context = draw->getContext();
      int imageFlags = 0; //cVg::IMAGE_GENERATE_MIPMAPS;
      if (mImage == -1)
        mImage = context->createImageRGBA (mSubtitle.mRects[i]->mWidth, mSubtitle.mRects[i]->mHeight,
                                           imageFlags, mSubtitle.mRects[i]->mPixelData);
      else if (mSubtitle.mChanged)
        context->updateImage (mImage, mSubtitle.mRects[i]->mPixelData);
      mSubtitle.mChanged = false;

      auto imgPaint = context->imagePattern (0, 0, dstWidth, dstHeight, 0.f, mImage, 1.f);
      context->beginPath();
      context->rect (mX, mX, dstWidth, dstHeight);
      context->fillPaint (imgPaint);
      context->fill();
      }
    }

private:
  cSubtitle& mSubtitle;

  int mImage = -1;
  float mScale = 1.f;
  };
