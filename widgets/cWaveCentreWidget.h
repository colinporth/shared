// cWaveCentredWidget.h
#pragma once
#include "cWaveWidget.h"

class cWaveCentreWidget : public cWaveWidget {
public:
  cWaveCentreWidget (uint8_t* wave, int& curFrame, int& loadedFrame, int& maxFrame, bool& changed,
                     float width, float height) :
      cWaveWidget(wave, curFrame, loadedFrame, maxFrame, changed, width, height) {}

  virtual ~cWaveCentreWidget() {}

  virtual void render (iDraw* draw) {
    cWaveWidget::render (draw, mColour, mColourAfter, mCurFrame - mWidth/2, getMaxValue()*2, 0 , mWidth);
    }
  };
