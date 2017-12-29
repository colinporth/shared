// hls.h
#pragma once
//{{{  includes
#include "../utils/utils.h"
#include "../utils/cLog.h"

#include "r1x80.h"
#include "r2x80.h"
#include "r3x80.h"
#include "r4x80.h"
#include "r5x80.h"
#include "r6x80.h"
//}}}
//{{{  static const
static const int kScrubFrames = 3;
//}}}

//{{{
//class cHlsPicWidget : public cDecodePicWidget {
//public:
  //cHlsPicWidget(cHls* hls, float width, float height)
    //: cDecodePicWidget(width, height, -1, mMyValue, mMyChanged), mHls(hls) {}
  //virtual ~cHlsPicWidget() {}

  //virtual void pressed (int16_t x, int16_t y) {
    //cDecodePicWidget::pressed (x, y);
    //mHls->togglePlaying();
    //}

  //virtual void render (iDraw* draw) {
    //setOn (!mHls->getPlaying());

    //if (mHls->mContent) {
      //setPic (mHls->mContent, mHls->mContentSize);
      //bigFree (mHls->mContent);
      //mHls->mContent = nullptr;
      //}

    //cDecodePicWidget::render (draw);
    //}

//private:
  //cHls* mHls;
  //bool mMyChanged;
  //int mMyValue;
  //};
//}}}
//{{{
class cHlsDotsBox : public cWidget {
public:
  cHlsDotsBox (cHls* hls, float width, float height) : cWidget(width, height), mHls(hls) {}
  virtual ~cHlsDotsBox() {}

  //{{{
  void onDown (int16_t x, int16_t y) {
    cWidget::onDown (x, y);
    }
  //}}}
  //{{{
  virtual void onDraw (iDraw* draw) {
    for (auto i = 0; i < 3; i++)
      drawDot (draw, i);
    }
  //}}}

private:
  //{{{
  void drawDot (iDraw* draw, uint16_t chunk) {
    bool loaded;
    bool loading;
    int offset;
    mHls->getChunkInfo (chunk, loaded, loading, offset);

    draw->ellipseSolid (loading ? COL_DARKORANGE : loaded ? COL_DARKERGREEN : COL_DARKRED,
                   mX + 2 + ((mWidth-2) / 2), mY + (chunk * getBoxHeight()) + mHeight / 6, (mWidth-2) / 2, (mWidth-2) / 2);

    if (loaded || loading)
      draw->drawText(COL_LIGHTGREY, getSmallFontHeight(), dec(offset),
                  mX + mWidth / 3, mY + (chunk * getBoxHeight()) + mWidth/6, mWidth, mWidth);
    }

  //}}}
  cHls* mHls;
  };
//}}}
//{{{
class cHlsTextWidget : public cWidget {
public:
  cHlsTextWidget (cHls* hls, float width, float height) : cWidget(width, height), mHls(hls) {}
  virtual ~cHlsTextWidget() {}

  //{{{
  void onDraw (iDraw* draw) {
    //auto item = mHls->findItem (mHls->getPlayTzSec());
    //if (item)
    //  draw->drawText (COL_WHITE, getFontHeight(), item->mTitle + " - " + item->mSynopsis, mX, mY+1, mWidth-1, mHeight-1);
    //else
    //  draw->drawText (COL_WHITE, getFontHeight(), "no schedule", mX, mY+1, mWidth-1, mHeight-1);
    }
  //}}}

private:
  cHls* mHls;
  };
//}}}
//{{{
class cHlsPeakWidget : public cWidget {
public:
  cHlsPeakWidget (cHls* hls, float width, float height) : cWidget (COL_BLUE, width, height), mHls(hls) {}
  virtual ~cHlsPeakWidget() {}

  //{{{
  void onDown (int16_t x, int16_t y) {

    cWidget::onDown (x, y);

    mMove = 0;
    mPressInc = x - (mWidth/2);

    mHls->setScrub();

    setZoomAnim (1.0f + ((mWidth/2) - abs(x - (mWidth/2))) / (mWidth/6), 4);
    }
  //}}}
  //{{{
  void onMove (int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc) {

    cWidget::onMove (x, y, z, xinc, yinc);
    mMove += abs(xinc) + abs(yinc);

    mHls->incPlaySample ((-xinc * kSamplesPerFrame) / mZoom);
    }
  //}}}
  //{{{
  void onUp() {

    cWidget::onUp();

    if (mMove < getBoxHeight()/2) {
      mAnimation = mPressInc;
      mHls->incPlaySample (mPressInc * kSamplesPerFrame / kNormalZoom);
      mHls->setPlay();
      }
    mHls->setPause();

    setZoomAnim (kNormalZoom, 3);
    }
  //}}}

  //{{{
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

    uint16_t midx = mX + (mWidth/2);
    float samplesPerPixF = kSamplesPerFrame / mZoom;
    float pixPerSec = kSamplesPerSecondF / samplesPerPixF;
    double sample = mHls->getPlaySample() - ((mWidth/2) + mAnimation) * samplesPerPixF;

    int16_t y = mY + mHeight - getBoxHeight()/2;
    int secs = int (sample / kSamplesPerSecond);
    float subSecSamples = (float)fmod (sample, kSamplesPerSecondD);
    float nextxF = (kSamplesPerSecondF - subSecSamples) / samplesPerPixF;

    #ifdef NANOVG
      //{{{  nanoVg rects waveform
      auto context = draw->getContext();

      context->fillColor (kDarkGrey);
      context->beginPath();
      for (float x = 0; x < mWidth; secs++, nextxF += pixPerSec) {
        if (secs & 1)
          context->rect (mX+x, y, nextxF - x, getBoxHeight()/2.0f);
        x = nextxF;
        }
      context->triangleFill();

      y = mY + mHeight - getBigFontHeight();

      context->fontSize ((float)getBigFontHeight());
      context->textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
      context->fillColor (kWhite);
      context->text (midx-60.0f+3.0f, y+1.0f, getTimeString (mHls->getPlayTzSeconds()));

      float midy = (float)mY + (mHeight/2);
      uint16_t midWidth = midx + int(mHls->getPlaying() == cHls::eScrub ? kScrubFrames*mZoom : mZoom);

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
            context->fillColor (nvgRGB32 (COL_BLUE));
            context->triangleFill();
            context->beginPath();
            }
          else if (x == midWidth) {
            context->fillColor (nvgRGB32 (COL_DARKGREEN));
            context->triangleFill();
            context->beginPath();
            }

          auto left = (*samples++ * mHeight) / 0x100;
          auto right = (*samples++ * mHeight) / 0x100;
          context->rect (x, midy - left, 1, (float)(left+right));
          numSamples--;
          }
        else
          sample += samplesPerPixF;
        }

      context->fillColor (nvgRGB32 (COL_DARKGREY));
      context->triangleFill();
      //}}}
    #else
      //{{{  iDraw waveform
      // iterate seconds
      for (int16_t x = 0; x < mWidth; secs++, nextxF += pixPerSec) {
        int16_t nextx = int16_t(nextxF);
        if (secs & 1)
          draw->drawRect (secs & 1 ? COL_DARKGREY : COL_DARKERGREY, mX+x, y, nextx - x, getBoxHeight()/2);
        x = nextx;
        }

      y = mY + mHeight - getBigFontHeight();
      draw->drawText (COL_LIGHTGREY, getBigFontHeight(), getTimeString (mHls->getPlayTzSec())),
                  midx - 60, y, mWidth/2, getBigFontHeight());

      uint16_t midWidth = midx + int(mHls->getScrubbing() ? kScrubFrames*mZoom : mZoom);

      uint16_t midy = mY + (mHeight/2);

      uint32_t colour = COL_DARKBLUE;
      uint8_t* samples = nullptr;
      uint32_t numSamples = 0;
      for (auto x = mX; x < mX+mWidth; x++) {
        if (!numSamples) {
          samples = mHls->getPeakSamples (sample, numSamples, mZoom);
          if (samples)
            sample += numSamples * samplesPerPixF;
          }
        if (samples) {
          if (x == midx)
            colour = COL_DARKGREEN;
          else if (x == midWidth)
            colour = COL_DARKERGREY;
          auto left = (*samples++ * mHeight) / 0x100;
          draw->drawRect (colour, x, midy - left, 1, left + (*samples++ * mHeight) / 0x100);
          numSamples--;
          }
        else
          sample += samplesPerPixF;
        }
      //}}}
    #endif
    }
  //}}}

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
  int16_t mPressInc = 0;
  int16_t mAnimation = 0;

  float mZoom = kNormalZoom;
  float mZoomInc = 0;
  float mTargetZoom = kNormalZoom;
  };
//}}}

//{{{
void hlsMenu (cRootContainer* root, cHls* hls) {

  root->addTopLeft (new cDecodePicWidget (r1x80, sizeof(r1x80), 3, 3, 1, hls->mChan, hls->mChanChanged));
  root->add (new cDecodePicWidget (r2x80, sizeof(r2x80), 3, 3, 2, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r3x80, sizeof(r3x80), 3, 3, 3, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r4x80, sizeof(r4x80), 3, 3, 4, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r5x80, sizeof(r5x80), 3, 3, 5, hls->mChan,  hls->mChanChanged));
  root->add (new cDecodePicWidget (r6x80, sizeof(r6x80), 3, 3, 6, hls->mChan,  hls->mChanChanged));

  //root->add (new cHlsPicWidget (hls, 16/3.0f, 3));
  root->add (new cHlsDotsBox (hls, 1, 3));
  root->addAt (new cHlsPeakWidget (hls, 0, -4), 0, 4);
  root->addAt (new cHlsTextWidget (hls, 0, 3), 0, 3);

  root->addTopRight (new cValueBox (hls->mVolume, hls->mVolumeChanged, COL_YELLOW, 0.5, 0))->setOverPick (1.5);
  }
//}}}
