// cVideoDecodeWidget.h
#pragma once
//{{{  includes
#include "../utils/cVideoDecode.h"
#include "cWidget.h"
//}}}

class cVideoDecodeWidget : public cWidget {
public:
  cVideoDecodeWidget (cVideoDecode* videoDecode, float width, float height)
    : cWidget (COL_BLACK, width, height), mVideoDecode(videoDecode) {}
  virtual ~cVideoDecodeWidget() {}

  void onDraw (iDraw* draw) {
    auto frame = mVideoDecode->findPlayFrame();
    if (frame) {
      auto context = draw->getContext();
      if (frame->getPts() != mPts) {
        // newFrame, update image
        mPts = frame->getPts();
        if (mImage == -1)
          mImage = context->createImageRGBA (frame->getWidth(), frame->getHeight(), 0, (uint8_t*)frame->get32());
        else
          context->updateImage (mImage, (uint8_t*)frame->get32());
        }

      // paint image rect
      context->beginPath();
      context->rect (0, 0, mWidth, mHeight);
      context->fillPaint (context->imagePattern (0, 0, mWidth, mHeight, 0.f, mImage, 1.f));
      context->triangleFill();
      }
    }

private:
  cVideoDecode* mVideoDecode;
  uint64_t mPts = 0;
  int mImage = -1;
  };
