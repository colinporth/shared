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

  //{{{
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
      vg->rect (cPoint(0.f,0.f), mWidth, mHeight);
      vg->fillPaint (vg->imagePattern (0.f,0.f, mWidth, mHeight, 0.f, mImageId, 1.f));
      vg->triangleFill();
      }

    // progress spinner
    drawSpinner (vg, cPoint(mWidth - 26.f, 26.f), 20.f, 10.f, 
                 mHlsPlayer->getLoadFrac(), mHlsPlayer->getAudioDecodeFrac(), mHlsPlayer->getVideoDecodeFrac());

    // info text
    std::string infoString = mHlsPlayer->getChannel() +
                             " - " + dec(mHlsPlayer->getVidBitrate()) +
                             ":" + dec(mHlsPlayer->getAudBitrate()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getWidth()) +
                             "x" + dec(mHlsPlayer->getVideoDecode()->getHeight()) +
                             " " + dec(mHlsPlayer->getVideoDecode()->getFramePoolSize());
    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
    vg->fillColour (kVgBlack);
    vg->text (mX+2.f, mY+2.f, infoString);
    vg->fillColour (kVgWhite);
    vg->text (mX, mY, infoString);
    }
  //}}}

private:
  //{{{
  void drawSpinner (cVg* vg, cPoint centre, float inner, float outer,
                    float loadFrac, float vidFrac, float audFrac) {

    float angle[4] = { 0.f, loadFrac * k2Pi, vidFrac * k2Pi, audFrac * k2Pi } ;

    float x[4];
    float y[4];
    for (int i = 0; i < 4; i++) {
      x[i] = centre.x + cosf (angle[i]) * ((outer + inner) / 2.f);
      y[i] = centre.y + sinf (angle[i]) * ((outer + inner) / 2.f);
      }

    sVgColour colours[4] = { nvgRGBA(0,0,0,0), nvgRGBA(32,255,32,192), nvgRGBA(32,32,255,192), nvgRGBA(255,32,32,192) };

    vg->saveState();

    vg->beginPath();
    vg->arc (centre, outer, angle[0], angle[1], cVg::eHOLE);
    vg->arc (centre, inner, angle[1], angle[0], cVg::eSOLID);
    vg->closePath();
    vg->fillPaint (vg->linearGradient (x[0],y[0], x[1],y[1], colours[0], colours[1]));
    vg->fill();

    vg->beginPath();
    vg->arc (centre, outer, angle[0], angle[2], cVg::eHOLE);
    vg->arc (centre, inner, angle[2], angle[0], cVg::eSOLID);
    vg->closePath();
    vg->fillPaint (vg->linearGradient (x[0],y[0], x[2],y[2], colours[0], colours[3]));
    vg->fill();

    vg->beginPath();
    vg->arc (centre, outer, angle[0], angle[3], cVg::eHOLE);
    vg->arc (centre, inner, angle[3], angle[0], cVg::eSOLID);
    vg->closePath();
    vg->fillPaint (vg->linearGradient (x[0],y[0], x[3],y[3], colours[0], colours[3]));
    vg->fill();


    vg->restoreState();
    }
  //}}}

  cHlsPlayer* mHlsPlayer;
  uint64_t mPts = 0;
  int mImageId = -1;
  };
