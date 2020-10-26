// cPicWidget.h
//{{{  includes
#pragma once

#include "../utils/cLog.h"
#include "cWidget.h"
//}}}

class cPicWidget : public cWidget {
public:
 cPicWidget (float width, float height, std::function<void (cPicWidget* widget)> hitCallback = [](cPicWidget*){})
    : cWidget(width, height), mHitCallback(hitCallback) {}
  virtual ~cPicWidget() {}

  //{{{
  void setPic (uint8_t* pic, uint16_t picWidth, uint16_t picHeight, uint16_t components) {

    free (mPic);

    mPic = pic;
    mPicWidth = picWidth;
    mPicHeight = picHeight;
    mPicComponents = components;

    mUpdateTexture = true;
    }
  //}}}
  void setAngle (float angle) { mAngle = angle; }

  //{{{
  virtual void onDown (const cPointF& p) {

    cWidget::onDown (p);
    mHitCallback (this);
    }
  //}}}
  //{{{
  virtual void onDraw (iDraw* draw) {

    mScale = isOn() ? 0.7f : mSelected ? 0.85f : 1.0f;
    uint16_t width = int((mPixSize.x-1) * mScale);
    uint16_t height = int((mPixSize.y -1) * mScale);

    if (mPic) {
      auto vg = draw->getVg();

      int imageFlags = cVg::eImageGenerateMipmaps;
      if (mImage == -1)
        mImage = vg->createImageRGBA (mPicWidth, mPicHeight, imageFlags, mPic);
      else if (mUpdateTexture) {
        //nvgUpdateImage (vg, mImage, mPic);
        vg->deleteImage (mImage);
        mImage = vg->createImageRGBA (mPicWidth, mPicHeight, imageFlags, mPic);
        }
      mUpdateTexture = false;

      float x = mScale >= 1.0 ? mPixOrg.x : mPixOrg.x + (mPixSize.x - width)/2.f;
      float y = mScale >= 1.0 ? mPixOrg.y : mPixOrg.y + (mPixSize.y - height)/2.f;

      auto imgPaint = vg->setImagePattern (cPointF(x, y), cPointF(width, height), mAngle / k180Pi, mImage, 1.0f);
      vg->beginPath();
      vg->rect (cPointF(x, y), cPointF(width, height));
      vg->setFillPaint (imgPaint);
      vg->fill();
      }
    }
  //}}}

private:
  std::function <void (cPicWidget* widget)> mHitCallback;
  bool mSelected = false;

  uint8_t* mPic = nullptr;
  uint16_t mPicWidth = 0;
  uint16_t mPicHeight = 0;
  uint16_t mPicComponents = 0;

  float mScale = 1.0f;
  int mImage = -1;
  bool mUpdateTexture = false;
  float mAngle = 0;
  };
