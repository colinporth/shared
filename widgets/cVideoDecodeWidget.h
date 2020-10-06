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
    cVg* vg = draw->getVg();

    auto frame = mVideoDecode->findPlayFrame();
    if (frame) {
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
      }

    float frac = mVideoDecode->getDecodeFrac();
    if ((frac > 0.f) && (frac < 1.f))
      vg->drawSpinner (mWidth - 24.f, mHeight-24.f, 20.f, frac, nvgRGBA(0,0,0,0), nvgRGBA(32,255,32,192));

    std::string infoString = dec(mVideoDecode->getWidth()) +
                             "x" + dec(mVideoDecode->getHeight()) +
                              " " + dec(mVideoDecode->getFramePoolSize());

    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
    vg->fillColor (kVgBlack);
    vg->text (mX+2.f, mY+2.f, infoString);
    vg->fillColor (kVgWhite);
    vg->text (mX, mY, infoString);
    }

private:
  cVideoDecode* mVideoDecode;
  uint64_t mPts = 0;
  int mImageId = -1;
  };
