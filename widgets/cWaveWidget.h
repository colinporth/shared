// cWaveformWidget.h
//{{{  includes
#pragma once
#include "cWidget.h"
//}}}

class cWaveWidget : public cWidget {
public:
  //{{{
  cWaveWidget (uint8_t* wave, int& curFrame, int& loadedFrame, int& maxFrame, bool& changed, float width, float height) :
      cWidget (COL_BLUE, width, height),
      mWave(wave), mCurFrame(curFrame), mLoadedFrame(loadedFrame), mMaxFrame(maxFrame), mChanged(changed) {

    mChanged = false;
    mSrc = (uint8_t*)bigMalloc (mHeight, "waveSrc");
    memset (mSrc, 0xC0, mHeight);
    }
  //}}}
  //{{{
  virtual ~cWaveWidget() {
    bigFree (mSrc);
    }
  //}}}

  //{{{
  void onDraw (iDraw* draw) {
    drawCommon (draw, mColour, mColourAfter, mCurFrame - mWidth, getMaxValue()*2, 0, mWidth);
    }
  //}}}

protected:
  int getMaxValue() { return *mWave; }
  int getCentreY() { return mY + (mHeight/2); }

  //{{{
  void drawCommon (iDraw* draw, uint32_t colour, uint32_t colourAfter, int frame, int scale, int x, int lastX) {

    if (frame < x) { // not enough front frames
      x = -frame;
      frame = 0;
      }
    if (x + mLoadedFrame - frame < lastX) // not enough back frames
      lastX = x + mLoadedFrame - frame;

    // draw frames
    auto wave = mWave + 1 + (frame * 2);
    auto centreY = getCentreY();

    auto context = draw->getContext();

    context->fillColor (nvgRGB32 (colour));
    context->beginPath();

    for (; x < lastX; x++, frame++) {
      auto valueL = (*wave++ * mHeight) / scale;
      auto valueR = (*wave++ * mHeight) / scale;
      if (frame == mCurFrame) {
        context->triangleFill();

        context->beginPath();
        context->rect (float(mX+x), float(centreY - valueL), 1.0f, float(valueL + valueR));
        context->fillColor (nvgRGB32 (COL_WHITE));
        context->triangleFill();

        context->beginPath();
        }
      else
        context->rect (float(mX+x), float(centreY - valueL), 1.0f, float(valueL + valueR));
      }

    context->fillColor (nvgRGB32 (colourAfter));
    context->triangleFill();
    }
  //}}}

  uint8_t* mWave;
  int& mCurFrame;
  int& mLoadedFrame;
  int& mMaxFrame;
  bool& mChanged;

  uint32_t mColourAfter = COL_GREY;
  uint8_t* mSrc;
  };
