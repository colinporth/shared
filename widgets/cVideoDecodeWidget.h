// cVideoDecodeWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"
#include "../utils/cVideoDecode.h"
//}}}

class cVideoDecodeWidget : public cWidget {
public:
  cVideoDecodeWidget (cVideoDecode* videoDecode, float width, float height)
    : cWidget (COL_BLACK, width, height), mVideoDecode(videoDecode) {}
  virtual ~cVideoDecodeWidget() {}

  void onDraw (iDraw* draw) {
    auto frame = mVideoDecode->findPlayFrame();
    if (frame) {
      cVg* vg = draw->getVg();
      if (frame->getPts() != mPts) {
        mPts = frame->getPts();
        if (mImageId == -1)
          mImageId = vg->createImageRGBA (mVideoDecode->getWidth(), mVideoDecode->getHeight(), 0, (uint8_t*)frame->get32());
        else
          vg->updateImage (mImageId, (uint8_t*)frame->get32());
        }

      // paint image rect
      vg->beginPath();
      vg->rect (0.f,0.f, mWidth, mHeight);
      vg->fillPaint (vg->imagePattern (0.f,0.f, mWidth, mHeight, 0.f, mImageId, 1.f));
      vg->triangleFill();

      float frac = mVideoDecode->getDecodeFrac();
      if ((frac > 0.f) && (frac < 1.f))
        vg->drawSpinner (mWidth - 24.f, mHeight-24.f, 20.f, frac, nvgRGBA(0,0,0,0), nvgRGBA(32,255,32,192));
      }
    }

private:
  cVideoDecode* mVideoDecode;
  uint64_t mPts = 0;
  int mImageId = -1;
  };
