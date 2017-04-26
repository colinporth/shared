// cWholeWaveWidget.h
#pragma once
#include "cWidget.h"

class cWaveOverviewWidget : public cWaveWidget {
public:
  //{{{
  cWaveOverviewWidget (uint8_t* wave, int& curFrame, int& loadedFrame, int& maxFrame, bool& changed, float width, float height)
    : cWaveWidget (wave, curFrame, loadedFrame, maxFrame, changed, width, height) {
    mSummedWave = (uint8_t*)bigMalloc (mWidth * 2 * sizeof(int16_t), "summedWave");
    }
  //}}}
  //{{{
  virtual ~cWaveOverviewWidget() {
    bigFree (mSummedWave);
    }
  //}}}

  //{{{
  virtual cWidget* picked (int16_t x, int16_t y, uint16_t z) {

    if (cWidget::picked (x, y, z)) {
      int frame = (x * mMaxFrame) / mWidth;
      mCurFrame = frame;
      mChanged = true;
      return this;
      }
    else
      return nullptr;
    }
  //}}}
  //{{{
  virtual void render (iDraw* draw) {

    render (draw, mColour, 0, mMaxSummedX);
    }
  //}}}

protected:
  //{{{
  void makeSummedWave() {

    auto scale = getMaxValue() * 2;
    if (mSummedFrame != mLoadedFrame) {
      // wave changed, cache wave values summed to width, scaled to height
      mSummedFrame = mLoadedFrame;
      auto startFrame = 0;
      auto summedWavePtr = mSummedWave;
      mMaxSummedX = 0;
      for (auto x = 0; x < mWidth; x++) {
        int frame = (x * mMaxFrame) / mWidth;
        if (frame > mLoadedFrame)
          break;

        auto wavePtr = mWave + 1 + (frame * 2);
        int sumL = *wavePtr++;
        int sumR = *wavePtr++;
        if (frame > startFrame) {
          int num = 1;
          for (auto i = startFrame; i <= frame; i++) {
            sumL += *wavePtr++;
            sumR += *wavePtr++;
            num++;
            }
          sumL /= num;
          sumR /= num;
          }
        *summedWavePtr++ = (sumL * mHeight) / scale;
        *summedWavePtr++ = (sumR * mHeight) / scale;
        mMaxSummedX = x;
        startFrame = frame + 1;
        }
      }
    }
  //}}}
  //{{{
  void render (iDraw* draw, uint32_t colour, int firstX, int lastX) {

    makeSummedWave();

    // draw cached graphic
    auto summedWavePtr = mSummedWave + (firstX * 2);
    auto curFrameX = mMaxFrame > 0 ? (mCurFrame * mWidth) / mMaxFrame : 0;
    auto centreY = getCentreY();
    auto scale = getMaxValue() * 2;

  #ifdef USE_NANOVG
    //{{{  draw wave
    auto context = draw->getContext();

    context->beginPath();
    context->fillColor (nvgRGB32 (colour));

    for (auto x = firstX; x < lastX; x++) {
      if (x == curFrameX) {
        auto wave = mWave + 1 + (mCurFrame * 2);
        auto valueL = (*wave++ * mHeight) / scale;
        auto valueR = (*wave++ * mHeight) / scale;

        context->triangleFill();

        context->beginPath();
        context->rect (mX+x, centreY - valueL-2.0f, 1.0f, valueL + valueR + 4.0f);
        context->fillColor (nvgRGB32 (COL_YELLOW));
        context->triangleFill();

        context->beginPath();
        }
      else {
        auto valueL = *summedWavePtr++;
        auto valueR = *summedWavePtr++;
        context->rect (mX+x, centreY - valueL-2.0f, 1.0f, valueL + valueR + 4.0f);
        }
      }

    context->fillColor (nvgRGB32 (mColourAfter));
    context->triangleFill();
    //}}}
  #else
    //{{{  draw wave
    for (auto x = firstX; x <= lastX; x++) {
      if (x == curFrameX) {
        auto wave = mWave + 1 + (mCurFrame * 2);
        auto valueL = (*wave++ * mHeight) / scale;
        auto valueR = (*wave++ * mHeight) / scale;
        draw->rectClipped (COL_YELLOW, mX+x, centreY - valueL-2, 1, valueL + valueR + 4);
        colour = mColourAfter;
        }
      else {
        auto valueL = *summedWavePtr++;
        auto valueR = *summedWavePtr++;
        draw->rectClipped (colour, mX+x, centreY - valueL-2, 1, valueL + valueR + 4);
        }
      }
    //}}}
  #endif
    }
  //}}}

private:
  int mMaxSummedX = 0;
  uint8_t* mSummedWave;
  int mSummedFrame = 0;
  };
