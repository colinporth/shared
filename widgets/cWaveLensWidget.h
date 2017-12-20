// cWaveLensWidget.h
#pragma once
#include "cWaveOverviewWidget.h"

class cWaveLensWidget : public cWaveOverviewWidget {
public:
  //{{{
  cWaveLensWidget (uint8_t* wave, int& curFrame, int& loadedFrame, int& maxFrame, bool& changed, float width, float height)
    : cWaveOverviewWidget (wave, curFrame, loadedFrame, maxFrame, changed, width, height) {}
  //}}}
  virtual ~cWaveLensWidget() {}

  //{{{
  virtual void onDraw (iDraw* draw) {

    if (!mOn && (mWindow <= 0)) {
      mWindow = 0;
      cWaveOverviewWidget::onDraw (draw);
      return;
      }

    if (!mOn)
      mWindow /= 2;
    else if (mWindow < mWidth / 16)
      mWindow +=  (mWidth / 16) / 6;

    int curFrameX = mMaxFrame > 0 ? (mCurFrame * mWidth) / mMaxFrame : 0;
    int firstLensX = curFrameX - mWindow;
    int lastLensX = curFrameX + mWindow;
    if (firstLensX < 0) {
      lastLensX -= firstLensX;
      firstLensX = 0;
      }
    else
      cWaveOverviewWidget::drawCommon (draw, mColour, 0, firstLensX);

    if (lastLensX > mWidth) {
      firstLensX -= lastLensX - mWidth;
      lastLensX = mWidth;
      }
    else
      cWaveOverviewWidget::drawCommon (draw, mColourAfter, lastLensX, mWidth);

    cWaveWidget::drawCommon (draw, COL_LIGHTBLUE, COL_LIGHTGREY, mCurFrame - mWindow, getMaxValue(), firstLensX, lastLensX);

    draw->rectOutline (COL_YELLOW, mX+firstLensX, mY+1, lastLensX - firstLensX, mHeight-2, 1);
    }
  //}}}

private:
  int mWindow = 0;
  };
