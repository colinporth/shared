// cBmpWidget.h
//{{{  includes
#pragma once

#include "cWidget.h"
#include "../utils/cLog.h"

extern "C" {
  uint8_t* stbi_load_from_memory (uint8_t* const* buffer, int len, int* x, int* y, int* channels_in_file, int desired_channels);
  void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);
  }
//}}}

class cImageWidget : public cWidget {
public:
  cImageWidget (const uint8_t* bmp, int bmpSize, bool flip, float width, float height,
              std::function<void (cImageWidget* widget)> hitCallback = [](cImageWidget*) {})
      : cWidget(width, height), mHitCallback(hitCallback) {

   stbi_set_flip_vertically_on_load (flip);
   mPic  = stbi_load_from_memory ((uint8_t* const*)bmp, bmpSize, &mPicWidth, &mPicHeight, &mPicComponents, 4);
   }

  virtual ~cImageWidget() {}

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
      mImage = vg->createImageRGBA (mPicWidth, mPicHeight, 0, mPic);
      //mImage = vg->createImageRGBA (mPicWidth, mPicHeight, cVg::eImageGenerateMipmaps, mPic);

    float x = mPixOrg.x + (mPixSize.x - width)/2.f;
    float y = mPixOrg.y + (mPixSize.y - height)/2.f;

    auto imgPaint = vg->setImagePattern (cPointF(x, y), cPointF(width, height), 0.f, mImage, 1.0f);
    vg->beginPath();
    vg->rect (cPointF(x, y), cPointF(width, height));
    vg->setFillPaint (imgPaint);
    vg->fill();
    }
  //}}}

private:
  std::function <void (cImageWidget* widget)> mHitCallback;

  bool mSelected = false;

  uint8_t* mPic = nullptr;
  int mPicWidth = 0;
  int mPicHeight = 0;
  int mPicComponents = 0;

  int mImage = -1;
  float mScale = 1.0f;
  };
