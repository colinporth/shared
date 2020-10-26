// cBmpWidget.h
//{{{  includes
#pragma once

#include "cWidget.h"
#include "../utils/cLog.h"

extern "C" {
  uint8_t* stbi_load_from_memory (uint8_t* const* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels);
  }
//}}}

class cBmpWidget : public cWidget {
public:
  cBmpWidget (const uint8_t* bmp, int bmpSize, float angle, float width, float height,
              std::function<void (cBmpWidget* widget)> hitCallback = [](cBmpWidget*) {})
      : cWidget(width, height), mAngle(angle), mHitCallback(hitCallback) {

   mPic  = stbi_load_from_memory ((uint8_t* const*)bmp, bmpSize, &mPicWidth, &mPicHeight, &mPicComponents, 4);
   }

  virtual ~cBmpWidget() {}

  void setAngle (float angle) { mAngle = angle; }

  //{{{
  virtual void onDown (const cPointF& p) {

    cWidget::onDown (p);
    mHitCallback (this);
    }
  //}}}
  //{{{
  virtual void onDraw (iDraw* draw) {

    cVg* vg = draw->getVg();

    mScale = isOn() ? 0.7f : mSelected ? 0.85f : 1.0f;
    uint16_t width = int((mPixSize.x-1) * mScale);
    uint16_t height = int((mPixSize.y -1) * mScale);

    if (mImage == -1)
      mImage = vg->createImageRGBA (mPicWidth, mPicHeight, cVg::eImageGenerateMipmaps, mPic);

    float x = mScale >= 1.0 ? mPixOrg.x : mPixOrg.x + (mPixSize.x - width)/2.f;
    float y = mScale >= 1.0 ? mPixOrg.y : mPixOrg.y + (mPixSize.y - height)/2.f;

    auto imgPaint = vg->setImagePattern (cPointF(x, y), cPointF(width, height), mAngle * k2Pi, mImage, 1.0f);
    vg->beginPath();
    vg->rect (cPointF(x, y), cPointF(width, height));
    vg->setFillPaint (imgPaint);
    vg->fill();
    }
  //}}}

private:
  float mAngle = 0;
  std::function <void (cBmpWidget* widget)> mHitCallback;

  bool mSelected = false;

  uint8_t* mPic = nullptr;
  int mPicWidth = 0;
  int mPicHeight = 0;
  int mPicComponents = 0;

  float mScale = 1.0f;
  int mImage = -1;
  };
