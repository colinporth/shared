// cLoaderPlayerWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"

#include "../utils/cLoaderPlayer.h"
//}}}

class cLoaderPlayerWidget : public cWidget {
public:
  cLoaderPlayerWidget (cLoaderPlayer* loaderPlayer, cPointF size)
    : cWidget (kBlackF, size.x, size.y), mLoaderPlayer(loaderPlayer) {}
  virtual ~cLoaderPlayerWidget() {}

  //{{{
  void onDraw (iDraw* draw) {

    cVg* vg = draw->getVg();
    auto videoDecode = mLoaderPlayer->getVideoDecode();

    if (videoDecode) {
      auto frame = videoDecode->findPlayFrame();
      if (frame) {
        if (frame->getPts() != mImagePts) {
          mImagePts = frame->getPts();
          if (mImageId > -1)
            vg->updateImage (mImageId, (uint8_t*)frame->getBuffer());
          else
            mImageId = vg->createImageRGBA (videoDecode->getWidth(), videoDecode->getHeight(),
                                            0, (uint8_t*)frame->getBuffer());
          }

        // paint image rect
        vg->beginPath();
        vg->rect (cPointF(), mPixSize);
        vg->setFillPaint (vg->setImagePattern (cPointF(), mPixSize, 0.f, mImageId, 1.f));
        vg->triangleFill();
        }
      }

    std::string infoString = mLoaderPlayer->getChannelName() +
                             " - " + dec(mLoaderPlayer->getVidBitrate()) +
                             ":" + dec(mLoaderPlayer->getAudBitrate());
    if (videoDecode)
      infoString += " " + dec(videoDecode->getWidth()) +
                    "x" + dec(videoDecode->getHeight()) +
                    " " + dec(videoDecode->getFramePoolSize());
    drawInfo (vg, infoString);

    // draw progress spinners
    float loadFrac;
    float videoFrac;
    float audioFrac;
    if (mLoaderPlayer->getFrac (loadFrac, videoFrac, audioFrac)) {
      cPointF centre (mPixSize.x-20.f, 20.f);
      drawSpinner (vg, centre, 18.f,12.f, 0.1f, loadFrac,
                   sColourF(0.f,1.f,0.f,0.f), sColourF(0.f,1.f,0.f,0.75f));
      if (videoFrac > 0.f)
        drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - videoFrac), loadFrac,
                     sColourF(1.f, 0.f, 0.f, 0.25f), sColourF(1.f, 0.f, 0.f, 0.75f));
      if (audioFrac > 0.f)
        drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - audioFrac), loadFrac,
                     sColourF(0.f, 0.f, 1.f, 0.25f), sColourF(0.f, 0.f, 1.f, 0.75f));
      }
    }
  //}}}

private:
  //{{{
  void drawInfo (cVg* vg, const std::string& infoString) {
  // info text

    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
    vg->setFillColour (kBlackF);
    vg->text (mPixOrg + cPointF(2.f, 2.f), infoString);
    vg->setFillColour (kWhiteF);
    vg->text (mPixOrg, infoString);
    }
  //}}}
  //{{{
  void drawSpinner (cVg* vg, const cPointF& centre, float outerRadius, float innerRadius,
                    float fracFrom, float fracTo,
                    const sColourF& colourFrom, const sColourF& colourTo) {

    if ((fracTo - fracFrom)  > 0.f) {
      float angleFrom = -kPiDiv2 + (fracFrom * k2Pi);
      float angleTo = -kPiDiv2 + (fracTo * k2Pi);

      vg->beginPath();
      vg->arc (centre, outerRadius, angleFrom, angleTo, cVg::eClockWise);
      vg->arc (centre, innerRadius, angleTo, angleFrom, cVg::eCounterClockWise);
      vg->closePath();

      float midRadius = (outerRadius + innerRadius) / 2.f;
      vg->setFillPaint (vg->setLinearGradient (centre + cPointF (cosf (angleFrom), sinf (angleFrom)) * midRadius,
                                               centre + cPointF (cosf (angleTo), sinf (angleTo)) * midRadius,
                                               colourFrom, colourTo));
      vg->fill();
      }
    }
  //}}}

  cLoaderPlayer* mLoaderPlayer;
  uint64_t mImagePts = 0;
  int mImageId = -1;
  };
