// cHlsPlayerWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"

#include "../hls/cHlsPlayer.h"
#include "../utils/cVideoDecode.h"
//}}}

class cHlsPlayerWidget : public cWidget {
public:
  cHlsPlayerWidget (cHlsPlayer* hlsPlayer, float width, float height)
    : cWidget (COL_BLACK, width, height), mHlsPlayer(hlsPlayer) {}
  virtual ~cHlsPlayerWidget() {}

  void onDraw (iDraw* draw) {
    cVg* vg = draw->getVg();

    auto frame =  mHlsPlayer->getVideoDecode()->findPlayFrame();
    if (frame) {
      if (frame->getPts() != mPts) {
        mPts = frame->getPts();
        if (mImageId == -1)
          mImageId = vg->createImageRGBA (mHlsPlayer->getVideoDecode()->getWidth(),  mHlsPlayer->getVideoDecode()->getHeight(),
                                          0, (uint8_t*)frame->get32());
        else
          vg->updateImage (mImageId, (uint8_t*)frame->get32());
        }

      // paint image rect
      vg->beginPath();
      vg->rect (0.f,0.f, mWidth, mHeight);
      vg->fillPaint (vg->imagePattern (0.f,0.f, mWidth, mHeight, 0.f, mImageId, 1.f));
      vg->triangleFill();
      }

    float frac = mHlsPlayer->getLoadFrac();
    if ((frac > 0.f) && (frac < 1.f))
      vg->drawSpinner (mWidth - 26.f, 26.f, 24.f, 20.f, frac, nvgRGBA(0,0,0,0), nvgRGBA(32,255,32,192));

    float vidFrac = mHlsPlayer->getVideoDecodeFrac();
    if ((vidFrac > 0.f) && (vidFrac < 1.f))
      vg->drawSpinner (mWidth - 26.f, 26.f, 20.f, 16.f, frac + vidFrac, nvgRGBA(0,0,0,0), nvgRGBA(255,32,32,192));

    float audFrac = mHlsPlayer->getAudioDecodeFrac();
    if ((audFrac > 0.f) && (audFrac < 1.f))
      vg->drawSpinner (mWidth - 26.f, 26.f, 20.f, 16.f, frac + audFrac, nvgRGBA(0,0,0,0), nvgRGBA(32,32,255,192));

    std::string infoString = mHlsPlayer->getChannel() + 
                             " - " + dec(mHlsPlayer->getVidBitrate()) +
                             ":" + dec(mHlsPlayer->getAudBitrate()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getWidth()) +
                             "x" + dec(mHlsPlayer->getVideoDecode()->getHeight()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getFramePoolSize());

    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
    vg->fillColor (kVgBlack);
    vg->text (mX+2.f, mY+2.f, infoString);
    vg->fillColor (kVgWhite);
    vg->text (mX, mY, infoString);
    }

private:
  cHlsPlayer* mHlsPlayer;
  uint64_t mPts = 0;
  int mImageId = -1;
  };
