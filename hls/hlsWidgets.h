// hls.h
#pragma once
#include "../widgets/cDecodePicWidget.h"
#include "../utils/utils.h"
#include "../utils/cLog.h"

#include "r1x80.h"
#include "r2x80.h"
#include "r3x80.h"
#include "r4x80.h"
#include "r5x80.h"
#include "r6x80.h"

static const int kScrubFrames = 3;

//{{{
class cHlsDotsBox : public cWidget {
public:
  cHlsDotsBox (cHls* hls, float width, float height) : cWidget(width, height), mHls(hls) {}
  virtual ~cHlsDotsBox() {}

  void onDown (const cPointF& p) {
    cWidget::onDown (p);
    }

  virtual void onDraw (iDraw* draw) {
    for (auto i = 0; i < 3; i++)
      drawDot (draw, i);
    }

private:
  void drawDot (iDraw* draw, uint16_t chunk) {
    bool loaded;
    bool loading;
    int offset;
    mHls->getChunkInfo (chunk, loaded, loading, offset);

    draw->drawEllipseSolid (loading ? kDarkOrangeF : loaded ? kDarkGreenF : kDarkRedF,
      mPixOrg + cPointF(2.f + ((mPixSize.x-2.f) / 2.f), (chunk * getBoxHeight()) + mPixSize.y / 6.f),
      cPointF((mPixSize.x-2.f) / 2.f, (mPixSize.x-2.f) / 2.f));

    if (loaded || loading)
      draw->drawText (kLightGreyF, getSmallFontHeight(), dec(offset),
                      mPixOrg + cPointF(mPixSize.x / 3.f, (chunk * getBoxHeight()) + mPixSize.x /6.f), cPointF(mPixSize.x, mPixSize.x));
    }

  cHls* mHls;
  };
//}}}
//{{{
class cHlsPeakWidget : public cWidget {
public:
  cHlsPeakWidget (cHls* hls, float width, float height) : cWidget (kBlueF, width, height), mHls(hls) {}
  virtual ~cHlsPeakWidget() {}

  void onDown (const cPointF& p) {
    cWidget::onDown (p);
    mMove = 0;
    mPressInc = p.x - (mPixSize.x/2.f);
    mHls->setScrub();
    setZoomAnim (1.0f + ((mPixSize.x /2.f) - abs(p.x - (mPixSize.x/2.f))) / (mPixSize.x/6.f), 4);
    }

  void onMove (const cPointF& p, const cPointF& inc) {
    cWidget::onMove (p, inc);
    mMove += abs(inc.x) + abs(inc.y);
    mHls->incPlaySample ((-inc.x * kSamplesPerFrame) / mZoom);
    }

  void onUp() {
    cWidget::onUp();
    if (mMove < getBoxHeight()/2.f) {
      mAnimation = mPressInc;
      mHls->incPlaySample (mPressInc * kSamplesPerFrame / kNormalZoom);
      mHls->setPlay();
      }
    mHls->setPause();
    setZoomAnim (kNormalZoom, 3);
    }

  void onDraw (iDraw* draw) {
    //{{{  animate
    mAnimation /= 2;

    if (mZoomInc > 0) {
      if (mZoom + mZoomInc > mTargetZoom)
        mZoom = mTargetZoom;
      else
        mZoom += mZoomInc;
      }
    else  {
      if (mZoom + mZoomInc < mTargetZoom)
        mZoom = mTargetZoom;
      else
        mZoom += mZoomInc;
      }
    //}}}
    float midx = mPixOrg.x + (mPixSize.x /2.f);
    float samplesPerPixF = kSamplesPerFrame / mZoom;
    float pixPerSec = kSamplesPerSecondF / samplesPerPixF;
    double sample = mHls->getPlaySample() - ((mPixSize.x /2.f) + mAnimation) * samplesPerPixF;

    float y = mPixOrg.y + mPixSize.y - getBoxHeight()/2.f;
    int secs = int (sample / kSamplesPerSecond);
    float subSecSamples = (float)fmod (sample, kSamplesPerSecondD);
    float nextxF = (kSamplesPerSecondF - subSecSamples) / samplesPerPixF;

    //{{{  draw waveform rects
    auto context = draw->getVg();

    context->setFillColour (kDarkGreyF);
    context->beginPath();
    for (float x = 0; x < mPixSize.x; secs++, nextxF += pixPerSec) {
      if (secs & 1)
        context->rect (cPointF(mPixOrg.x + x, y), cPointF(nextxF - x, getBoxHeight()/2.0f));
      x = nextxF;
      }
    context->triangleFill();

    y = mPixOrg.y + mPixSize.y - getBigFontHeight();

    context->setFontSize ((float)getBigFontHeight());
    context->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
    context->setFillColour (kWhiteF);
    context->text (cPointF(midx-60.0f+3.0f, y+1.0f), getTimeString (mHls->getPlayTzSeconds()));

    float midy = (float)mPixOrg.y + (mPixSize.y /2.f);
    float midWidth = midx + int(mHls->getPlaying() == cHls::eScrub ? kScrubFrames*mZoom : mZoom);

    context->beginPath();
    uint8_t* samples = nullptr;
    uint32_t numSamples = 0;
    for (float x = mPixOrg.x; x < mPixOrg.x+mPixSize.x; x++) {
      if (!numSamples) {
        samples = mHls->getPeakSamples (sample, numSamples, mZoom);
        if (samples)
          sample += numSamples * samplesPerPixF;
        }

      if (samples) {
        if (x == midx) {
          context->setFillColour (kBlueF);
          context->triangleFill();
          context->beginPath();
          }
        else if (x == midWidth) {
          context->setFillColour (kDarkGreenF);
          context->triangleFill();
          context->beginPath();
          }

        auto left = (*samples++ * mPixSize.y) / 0x100;
        auto right = (*samples++ * mPixSize.y) / 0x100;
        context->rect (cPointF(x, midy - left), cPointF(1.f, (float)(left+right)));
        numSamples--;
        }
      else
        sample += samplesPerPixF;
      }

    context->setFillColour (kDarkGreyF);
    context->triangleFill();
    //}}}
    }

private:
  //{{{
  void setZoomAnim (float zoom, int16_t frames) {

    mTargetZoom = zoom;
    mZoomInc = (mTargetZoom - mZoom) / frames;
    }
  //}}}

  cHls* mHls;

  float mMove = 0;
  bool mMoved = false;
  float mPressInc = 0;
  float mAnimation = 0;

  float mZoom = kNormalZoom;
  float mZoomInc = 0;
  float mTargetZoom = kNormalZoom;
  };
//}}}

void hlsMenu (cRootContainer* root, cHls* hls) {

  root->addTopLeft (new cDecodePicWidget (r1x80, sizeof(r1x80), 3, 3, 1,
    [&](cPicWidget* widget, int value) noexcept { hls->mChan = value; hls->mChanChanged = true; } ));
  root->add (new cDecodePicWidget (r2x80, sizeof(r2x80), 3, 3, 2,
    [&](cPicWidget* widget, int value) noexcept { hls->mChan = value; hls->mChanChanged = true; } ));
  root->add (new cDecodePicWidget (r3x80, sizeof(r3x80), 3, 3, 3,
    [&](cPicWidget* widget, int value) noexcept { hls->mChan = value; hls->mChanChanged = true; } ));
  root->add (new cDecodePicWidget (r4x80, sizeof(r4x80), 3, 3, 4,
    [&](cPicWidget* widget, int value) noexcept { hls->mChan = value; hls->mChanChanged = true; } ));
  root->add (new cDecodePicWidget (r5x80, sizeof(r5x80), 3, 3, 5,
    [&](cPicWidget* widget, int value) noexcept { hls->mChan = value; hls->mChanChanged = true; } ));
  root->add (new cDecodePicWidget (r6x80, sizeof(r6x80), 3, 3, 6,
    [&](cPicWidget* widget, int value) noexcept { hls->mChan = value; hls->mChanChanged = true; } ));

  root->add (new cHlsDotsBox (hls, 1, 3));
  root->addAtBox (new cHlsPeakWidget (hls, 0, -4), 0.f, 4.f);
  }
