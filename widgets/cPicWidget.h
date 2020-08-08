// cPicWidget.h
//{{{  includes
#pragma once

#include "../utils/cLog.h"
#include "cWidget.h"
//}}}

class cPicWidget : public cWidget {
public:
  //{{{
  cPicWidget (int selectValue, int& value, bool& changed)
    : cWidget(), mSelectValue(selectValue), mValue(value), mChanged(changed) {}
  //}}}
  //{{{
  cPicWidget (float width, float height, int selectValue, int& value, bool& changed)
    : cWidget(width, height), mSelectValue(selectValue), mValue(value), mChanged(changed) {}
  //}}}
  virtual ~cPicWidget() {}

  virtual void setFileName (std::string fileName) {}
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
  //{{{
  void setSelectValue (int selectValue) {
    mSelectValue = selectValue;
    }
  //}}}

  //{{{
  virtual void onDown (float x, float y) {
    cWidget::onDown (x, y);
    if (mSelectValue != mValue) {
      mValue = mSelectValue;
      mChanged = true;
      }
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {

    mScale = isOn() ? 0.7f : (mSelectValue == mValue) ? 0.85f : 1.0f;
    uint16_t width = int((mWidth-1) * mScale);
    uint16_t height = int((mHeight-1) * mScale);

    if (mPic) {
      auto context = draw->getContext();
      int imageFlags = cVg::IMAGE_GENERATE_MIPMAPS;

      if (mImage == -1)
        mImage = context->createImageRGBA (mPicWidth, mPicHeight, imageFlags, mPic);
      else if (mUpdateTexture) {
        //nvgUpdateImage (context, mImage, mPic);
        context->deleteImage (mImage);
        mImage = context->createImageRGBA (mPicWidth, mPicHeight, imageFlags, mPic);
        }
      mUpdateTexture = false;

      float x = mScale >= 1.0 ? mX : mX + (mWidth - width)/2.f;
      float y = mScale >= 1.0 ? mY : mY + (mHeight - height)/2.f;

      auto imgPaint = context->imagePattern (x, y, width, height, mAngle / 180.0f * PI, mImage, 1.0f);
      context->beginPath();
      context->rect (x, y, width, height);
      context->fillPaint (imgPaint);
      context->fill();
      }
    }
  //}}}

private:
  uint8_t* mPic = nullptr;
  uint16_t mPicWidth = 0;
  uint16_t mPicHeight = 0;
  uint16_t mPicComponents = 0;

  float mScale = 1.0f;
  int mSelectValue;
  int& mValue;
  bool& mChanged;

  int mImage = -1;
  bool mUpdateTexture = false;
  float mAngle = 0;
  };
