// cHlsPlayerWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"

#include "../hls/cHlsPlayer.h"
//}}}

class cHlsPlayerWidget : public cWidget {
public:
  cHlsPlayerWidget (cHlsPlayer* hlsPlayer, cPointF size)
    : cWidget (kBlackF, size.x, size.y), mHlsPlayer(hlsPlayer) {}
  virtual ~cHlsPlayerWidget() {}

  //{{{
  void onDraw (iDraw* draw) {

    cVg* vg = draw->getVg();

    auto frame = mHlsPlayer->getVideoDecode()->findPlayFrame();
    if (frame) {
      if (frame->getPts() != mImagePts) {
        mImagePts = frame->getPts();
        if (mImageId > -1)
          vg->updateImage (mImageId, (uint8_t*)frame->get32());
        else
          mImageId = vg->createImageRGBA (mHlsPlayer->getVideoDecode()->getWidth(),
                                          mHlsPlayer->getVideoDecode()->getHeight(),
                                          0, (uint8_t*)frame->get32());
        }

      // paint image rect
      vg->beginPath();
      vg->rect (cPointF(), mPixSize);
      vg->setFillPaint (vg->setImagePattern (cPointF(), mPixSize, 0.f, mImageId, 1.f));
      vg->triangleFill();
      }

    // draw progress spinners
    cPointF centre (mPixSize.x-20.f, 20.f);
    float loadFrac = mHlsPlayer->getLoadFrac();
    drawSpinner (vg, centre, 18.f,12.f, 0.f, loadFrac,
                 sColourF(0.f,1.f,0.f,0.f), sColourF(0.f,1.f,0.f,0.75f));
    drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - mHlsPlayer->getVideoFrac()), loadFrac,
                 sColourF(1.f, 0.f, 0.f, 0.f), sColourF(1.f, 0.f, 0.f, 0.75f));
    drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - mHlsPlayer->getAudioFrac()), loadFrac,
                 sColourF(0.f, 0.f, 1.f, 0.f), sColourF(0.f, 0.f, 1.f, 0.75f));

    drawInfo (vg);
    }
  //}}}

private:
  //{{{
  void drawSpinner (cVg* vg, cPointF centre, float outerRadius, float innerRadius,
                    float fracFrom, float fracTo, const sColourF& colourFrom, const sColourF& colourTo) {

    if ((fracTo - fracFrom)  > 0.f) {
      float angleFrom = -kPiDiv2 + (fracFrom * k2Pi);
      float angleTo = -kPiDiv2 + (fracTo * k2Pi);

      vg->beginPath();
      vg->arc (centre, outerRadius, angleFrom, angleTo, cVg::eClockWise);
      vg->arc (centre, innerRadius, angleTo, angleFrom, cVg::eCounterClockWise);
      vg->closePath();

      float midRadius = (outerRadius + innerRadius) / 2.f;
      vg->setFillPaint (
        vg->setLinearGradient (centre + cPointF (cosf (angleFrom), sinf (angleFrom)) * midRadius,
                               centre + cPointF (cosf (angleTo), sinf (angleTo)) * midRadius,
                               colourFrom, colourTo));
      vg->fill();
      }
    }
  //}}}
  //{{{
  void drawInfo (cVg* vg) {
  // info text

    std::string infoString = mHlsPlayer->getChannelName() +
                             " - " + dec(mHlsPlayer->getVidBitrate()) +
                             ":" + dec(mHlsPlayer->getAudBitrate()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getWidth()) +
                             "x" + dec(mHlsPlayer->getVideoDecode()->getHeight()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getFramePoolSize());
    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
    vg->setFillColour (kBlackF);
    vg->text (mPixOrg + cPointF(2.f, 2.f), infoString);
    vg->setFillColour (kWhiteF);
    vg->text (mPixOrg, infoString);
    }
  //}}}

  cHlsPlayer* mHlsPlayer;
  uint64_t mImagePts = 0;
  int mImageId = -1;
  };
