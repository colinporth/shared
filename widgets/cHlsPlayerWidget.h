// cHlsPlayerWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"

#include "../hls/cHlsPlayer.h"
#include "../utils/cVideoDecode.h"
//}}}

class cHlsPlayerWidget : public cWidget {
public:
  cHlsPlayerWidget (cHlsPlayer* hlsPlayer, cPoint size)
    : cWidget (kBlackF, size.x, size.y), mHlsPlayer(hlsPlayer) {}
  virtual ~cHlsPlayerWidget() {}

  //{{{
  void onDraw (iDraw* draw) {

    cVg* vg = draw->getVg();

    auto frame = mHlsPlayer->getVideoDecode()->findPlayFrame();
    if (frame) {
      if (frame->getPts() != mPts) {
        mPts = frame->getPts();
        if (mImageId == -1)
          mImageId = vg->createImageRGBA (mHlsPlayer->getVideoDecode()->getWidth(), mHlsPlayer->getVideoDecode()->getHeight(),
                                          0, (uint8_t*)frame->get32());
        else
          vg->updateImage (mImageId, (uint8_t*)frame->get32());
        }

      // paint image rect
      vg->beginPath();
      vg->rect (cPoint(0.f,0.f), cPoint(mWidth, mHeight));
      vg->setFillPaint (vg->setImagePattern (cPoint(0.f,0.f), cPoint(mWidth, mHeight), 0.f, mImageId, 1.f));
      vg->triangleFill();
      }

    // draw progress spinner
    vg->saveState();
    cPoint centre (mWidth-20.f,20.f);
    float loadFrac = mHlsPlayer->getLoadFrac();
    drawSpinner (vg, centre, 18.f,12.f, 0.f, loadFrac,
                 sColourF(0.f,1.f,0.f,0.f), sColourF(0.f,1.f,0.f,0.75f));
    drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - mHlsPlayer->getVideoFrac()), loadFrac,
                 sColourF(1.f, 0.f, 0.f, 0.f), sColourF(1.f, 0.f, 0.f, 0.75f));
    drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - mHlsPlayer->getAudioFrac()), loadFrac,
                 sColourF(0.f, 0.f, 1.f, 0.f), sColourF(0.f, 0.f, 1.f, 0.75f));
    vg->restoreState();

    // info text
    std::string infoString = mHlsPlayer->getChannel() +
                             " - " + dec(mHlsPlayer->getVidBitrate()) +
                             ":" + dec(mHlsPlayer->getAudBitrate()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getWidth()) +
                             "x" + dec(mHlsPlayer->getVideoDecode()->getHeight()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getFramePoolSize());
    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
    vg->setFillColour (kBlackF);
    vg->text (cPoint(mX+2.f, mY+2.f), infoString);
    vg->setFillColour (kWhiteF);
    vg->text (cPoint(mX, mY), infoString);
    }
  //}}}

private:
  //{{{
  void drawSpinner (cVg* vg, cPoint centre, float outerRadius, float innerRadius,
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
        vg->setLinearGradient (centre + cPoint (cosf (angleFrom), sinf (angleFrom)) * midRadius,
                               centre + cPoint (cosf (angleTo), sinf (angleTo)) * midRadius,
                               colourFrom, colourTo));
      vg->fill();
      }
    }
  //}}}

  cHlsPlayer* mHlsPlayer;
  uint64_t mPts = 0;
  int mImageId = -1;
  };
