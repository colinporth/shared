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

    bigFree (mPic);

    mPic = pic;
    mPicWidth = picWidth;
    mPicHeight = picHeight;
    mPicComponents = components;

    #ifdef NANOVG
      mUpdateTexture = true;
    #else
      bigFree (mSizedRgb888);
      mSizedRgb888 = nullptr;
      mSizedWidth = 0;
      mSizedHeight = 0;
      mSizedAllocWidth = 0;
      mSizedAllocHeight = 0;
    #endif
    }
  //}}}
  //{{{
  void setSelectValue (int selectValue) {
    mSelectValue = selectValue;
    }
  //}}}

  //{{{
  void onDown (int16_t x, int16_t y) {
    cWidget::onDown (x, y);
    if (mSelectValue != mValue) {
      mValue = mSelectValue;
      mChanged = true;
      }
    }
  //}}}
  //{{{
  void onDraw (iDraw* draw) {

    mScale = isOn() ? 0.7f : (mSelectValue == mValue) ? 0.85f : 1.0f;
    uint16_t width = int((mWidth-1) * mScale);
    uint16_t height = int((mHeight-1) * mScale);

    #ifdef NANOVG
      //{{{  GL render
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

        uint16_t x = mScale >= 1.0 ? mX : mX + (mWidth - width)/2;
        uint16_t y = mScale >= 1.0 ? mY : mY + (mHeight - height)/2;

        auto imgPaint = context->imagePattern (x, y, width, height, mAngle / 180.0f * PI, mImage, 1.0f);
        context->beginPath();
        context->rect (x, y, width, height);
        context->fillPaint (imgPaint);
        context->fill();
        }
      //}}}
    #else
      //{{{  STM32 render
      if (mPic) {
        auto buf = mPic;
        if ((mPicWidth != width) || (mPicHeight != height)) {
          // sized, not already sized or new size
          if (!mSizedRgb888 || (width > mSizedAllocWidth) || (height > mSizedAllocHeight)) {
            // not already sized or new size bigger than allocated size
            bigFree (mSizedRgb888);
            mSizedRgb888 = (uint8_t*)bigMalloc (width * height * 3, "picWidgetSized");
            mSizedAllocWidth = width;
            mSizedAllocHeight = height;
            }

          // bilinear resize pic RGB888 to dst RGB888
          buf = mSizedRgb888;
          uint32_t xStep16 = ((mPicWidth - 1) << 16) / (width - 1);
          uint32_t yStep16 = ((mPicHeight - 1) << 16) / (height - 1);
          uint32_t ySrcOffset = mPicWidth * mPicComponents;
          for (uint32_t y16 = 0; y16 < height * yStep16; y16 += yStep16) {
            uint8_t yweight2 = (y16 >> 9) & 0x7F;
            uint8_t yweight1 = 0x80 - yweight2;
            const uint8_t* srcy = mPic + (y16 >> 16) * ySrcOffset;
            for (uint32_t x16 = 0; x16 < width * xStep16; x16 += xStep16) {
              uint8_t xweight2 = (x16 >> 9) & 0x7F;
              uint8_t xweight1 = 0x80 - xweight2;
              const uint8_t* srcy1x1 = srcy + (x16 >> 16) * mPicComponents;
              const uint8_t* srcy1x2 = srcy1x1 + mPicComponents;
              const uint8_t* srcy2x1 = srcy1x1 + ySrcOffset;
              const uint8_t* srcy2x2 = srcy2x1 + mPicComponents;
              for (auto dstComponent = 0; dstComponent < 3; dstComponent++)
                *buf++ = (((*srcy1x1++ * xweight1 + *srcy1x2++ * xweight2) * yweight1) +
                           (*srcy2x1++ * xweight1 + *srcy2x2++ * xweight2) * yweight2) >> 14;
              }
            }
          buf = mSizedRgb888;
          }

        draw->copy (buf, mScale >= 1 ? mX : mX+(mWidth-width)/2, mScale >= 1 ? mY : mY+(mHeight-height)/2, width, height);
        mSizedWidth = width;
        mSizedHeight = height;
        }
      //}}}
    #endif
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

//{{{
#ifdef NANOVG
  int mImage = -1;
  bool mUpdateTexture = false;
  float mAngle = 0;
//}}}
//{{{
#else
  uint8_t* mSizedRgb888 = nullptr;
  uint16_t mSizedWidth = 0;
  uint16_t mSizedHeight = 0;
  uint16_t mSizedAllocWidth = 0;
  uint16_t mSizedAllocHeight = 0;
#endif
//}}}
  };
