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

  void onDown (float x, float y) {
    cWidget::onDown (x, y);
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

    draw->drawEllipseSolid (loading ? COL_DARKORANGE : loaded ? COL_DARKERGREEN : COL_DARKRED,
      cPoint(mX + 2.f + ((mWidth-2.f) / 2.f), mY + (chunk * getBoxHeight()) + mHeight / 6.f), 
      cPoint((mWidth-2.f) / 2.f, (mWidth-2.f) / 2.f));

    if (loaded || loading)
      draw->drawText (COL_LIGHTGREY, getSmallFontHeight(), dec(offset),
                      cPoint(mX + mWidth / 3.f, mY + (chunk * getBoxHeight()) + mWidth/6.f), cPoint(mWidth, mWidth));
    }

  cHls* mHls;
  };
//}}}
//{{{
class cHlsPeakWidget : public cWidget {
public:
  cHlsPeakWidget (cHls* hls, float width, float height) : cWidget (COL_BLUE, width, height), mHls(hls) {}
  virtual ~cHlsPeakWidget() {}

  void onDown (float x, float y) {
    cWidget::onDown (x, y);
    mMove = 0;
    mPressInc = x - (mWidth/2.f);
    mHls->setScrub();
    setZoomAnim (1.0f + ((mWidth/2.f) - abs(x - (mWidth/2.f))) / (mWidth/6.f), 4);
    }

  void onMove (float x, float y, float xinc, float yinc) {
    cWidget::onMove (x, y, xinc, yinc);
    mMove += abs(xinc) + abs(yinc);
    mHls->incPlaySample ((-xinc * kSamplesPerFrame) / mZoom);
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
    float midx = mX + (mWidth/2.f);
    float samplesPerPixF = kSamplesPerFrame / mZoom;
    float pixPerSec = kSamplesPerSecondF / samplesPerPixF;
    double sample = mHls->getPlaySample() - ((mWidth/2.f) + mAnimation) * samplesPerPixF;

    float y = mY + mHeight - getBoxHeight()/2.f;
    int secs = int (sample / kSamplesPerSecond);
    float subSecSamples = (float)fmod (sample, kSamplesPerSecondD);
    float nextxF = (kSamplesPerSecondF - subSecSamples) / samplesPerPixF;

    //{{{  draw waveform rects
    auto context = draw->getVg();

    context->setFillColour (kVgDarkGrey);
    context->beginPath();
    for (float x = 0; x < mWidth; secs++, nextxF += pixPerSec) {
      if (secs & 1)
        context->rect (cPoint(mX+x, y), cPoint(nextxF - x, getBoxHeight()/2.0f));
      x = nextxF;
      }
    context->triangleFill();

    y = mY + mHeight - getBigFontHeight();

    context->setFontSize ((float)getBigFontHeight());
    context->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
    context->setFillColour (kVgWhite);
    context->text (cPoint(midx-60.0f+3.0f, y+1.0f), getTimeString (mHls->getPlayTzSeconds()));

    float midy = (float)mY + (mHeight/2);
    float midWidth = midx + int(mHls->getPlaying() == cHls::eScrub ? kScrubFrames*mZoom : mZoom);

    context->beginPath();
    uint8_t* samples = nullptr;
    uint32_t numSamples = 0;
    for (float x = mX; x < mX+mWidth; x++) {
      if (!numSamples) {
        samples = mHls->getPeakSamples (sample, numSamples, mZoom);
        if (samples)
          sample += numSamples * samplesPerPixF;
        }

      if (samples) {
        if (x == midx) {
          context->setFillColour (nvgRGBA32 (COL_BLUE));
          context->triangleFill();
          context->beginPath();
          }
        else if (x == midWidth) {
          context->setFillColour (nvgRGBA32 (COL_DARKGREEN));
          context->triangleFill();
          context->beginPath();
          }

        auto left = (*samples++ * mHeight) / 0x100;
        auto right = (*samples++ * mHeight) / 0x100;
        context->rect (cPoint(x, midy - left), cPoint(1.f, (float)(left+right)));
        numSamples--;
        }
      else
        sample += samplesPerPixF;
      }

    context->setFillColour (nvgRGBA32 (COL_DARKGREY));
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

  root->addTopLeft (new cDecodePicWidget (r1x80, sizeof(r1x80), 3, 3, 1, hls->mChan, hls->mChanChanged));
  root->add (new cDecodePicWidget (r2x80, sizeof(r2x80), 3, 3, 2, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r3x80, sizeof(r3x80), 3, 3, 3, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r4x80, sizeof(r4x80), 3, 3, 4, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r5x80, sizeof(r5x80), 3, 3, 5, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r6x80, sizeof(r6x80), 3, 3, 6, hls->mChan,  hls->mChanChanged));

  root->add (new cHlsDotsBox (hls, 1, 3));
  root->addAt (new cHlsPeakWidget (hls, 0, -4), 0, 4);
  }
