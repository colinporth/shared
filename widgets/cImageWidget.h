// cImageWidget.h
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
  //{{{
  cImageWidget (const uint8_t* image, int imageSize,
                float width, float height,
                std::function<void (cImageWidget* widget)> hitCallback = [](cImageWidget*) {},
                const std::string& debugName = "cImageWidget")
      : cWidget(width, height, debugName), mHitCallback(hitCallback) {
   stbi_set_flip_vertically_on_load (true);
   mPiccy = stbi_load_from_memory ((uint8_t* const*)image, imageSize, &mPicWidth, &mPicHeight, &mPicComponents, 4);
   //auto ptr = mPic;
   //for (int i = 0; i < mPicWidth * mPicHeight; i++)
   //  ptr[(i*4) + 3] = 0x80;
   }
  //}}}
  virtual ~cImageWidget() {}

  virtual void onDown (const cPointF& p) {
    cWidget::onDown (p);
    mHitCallback (this);
    }

  virtual void onDraw (iDraw* draw) {
    cVg* vg = draw->getVg();

    // createImage firstTime
    if (mImage == -1)
      mImage = vg->createImageRGBA (mPicWidth, mPicHeight, 0, mPiccy);  // cVg::eImageGenerateMipmaps, mPiccy);

    // calc draw pos,size
    mScale = isPressed() ? 0.7f : 1.0f;
    cPointF drawSize = (mPixSize - kBorder) * mScale;
    cPointF drawCentre = mPixOrg + (mPixSize - drawSize)/2.f;

    // draw it
    auto imgPaint = vg->setImagePattern (drawCentre, drawSize, 0.f, mImage, 1.0f);
    vg->beginPath();
    vg->rect (drawCentre, drawSize);
    vg->setFillPaint (imgPaint);
    vg->fill();
    }

private:
  const cPointF kBorder = { 1.f, 1.f };
  std::function <void (cImageWidget* widget)> mHitCallback;

  const uint8_t* mPiccy = nullptr;
  int mPicWidth = 0;
  int mPicHeight = 0;
  int mPicComponents = 0;

  int mImage = -1;
  float mScale = 1.0f;
  };
