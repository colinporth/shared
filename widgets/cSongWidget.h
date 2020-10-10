// cSongWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"
#include "../utils/cSong.h"
//}}}

class cSongWidget : public cWidget {
public:
  cSongWidget (cSong* song, float width, float height) : cWidget (COL_BLUE, width, height), mSong(song) {}
  virtual ~cSongWidget() {}

  //{{{
  void onDown (float x, float y) {

    cWidget::onDown (x, y);

    //std::shared_lock<std::shared_mutex> lock (mSong->getSharedMutex());
    if (y > mDstOverviewTop) {
      auto frame = mSong->getFirstFrame() + int((x * mSong->getTotalFrames()) / getWidth());
      mSong->setPlayFrame (frame);
      mOverviewPressed = true;
      }

    else if (y > mDstRangeTop) {
      mPressedFrame = mSong->getPlayFrame() + ((x - (getWidth()/2.f)) * mFrameStep / mFrameWidth);
      mSong->getSelect().start (int(mPressedFrame));
      mRangePressed = true;
      //mWindow->changed();
      }

    else
      mPressedFrame = (float)mSong->getPlayFrame();
    }
  //}}}
  //{{{
  void onMove (float x, float y, float xinc, float yinc) {

    cWidget::onMove (x, y, xinc, yinc);

    //std::shared_lock<std::shared_mutex> lock (mSong.getSharedMutex());
    if (mOverviewPressed)
      mSong->setPlayFrame (mSong->getFirstFrame() + int((x * mSong->getTotalFrames()) / getWidth()));

    else if (mRangePressed) {
      mPressedFrame += (xinc / mFrameWidth) * mFrameStep;
      mSong->getSelect().move ((int)mPressedFrame);
      //mWindow->changed();
      }

    else {
      mPressedFrame -= (xinc / mFrameWidth) * mFrameStep;
      mSong->setPlayFrame ((int)mPressedFrame);
      }
    }
  //}}}
  //{{{
  void onUp() {
    cWidget::onUp();
    mSong->getSelect().end();
    mOverviewPressed = false;
    mRangePressed = false;
    }
  //}}}
  //{{{
  void onWheel (float delta) {

    //if (getShow())
    setZoom (mZoom - (int)delta);
    }
  //}}}

  //{{{
  void onDraw (iDraw* draw) {

    mWaveHeight = 100.f;
    mOverviewHeight = 100.f;
    mRangeHeight = 8.f;
    mFreqHeight = mHeight - mOverviewHeight - mRangeHeight - mWaveHeight;

    mDstFreqTop = 0;
    mDstWaveTop = mDstFreqTop + mFreqHeight;
    mDstRangeTop = mDstWaveTop + mWaveHeight;
    mDstOverviewTop = mDstRangeTop + mRangeHeight;
    mDstWaveCentre = mDstWaveTop + (mWaveHeight/2.f);
    mDstOverviewCentre = mDstOverviewTop + (mOverviewHeight/2.f);

    // lock
    std::shared_lock<std::shared_mutex> lock (mSong->getSharedMutex());

    // wave left right frames, clip right not left
    auto playFrame = mSong->getPlayFrame();
    auto leftWaveFrame = playFrame - (((int(mWidth)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
    auto rightWaveFrame = playFrame + (((int(mWidth)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
    rightWaveFrame = std::min (rightWaveFrame, mSong->getLastFrame());

    auto vg = draw->getVg();
    drawRange (vg, playFrame, leftWaveFrame, rightWaveFrame);
    if (mSong->getNumFrames()) {
      bool mono = mSong->getNumChannels() == 1;
      drawWave (vg, playFrame, leftWaveFrame, rightWaveFrame, mono);
      drawOverview (vg, playFrame, mono);
      drawFreq (vg, playFrame);
      }
    drawTime (vg,
              mSong->hasHlsBase() ? getFrameString (mSong->getFirstFrame()) : "",
              getFrameString (mSong->getPlayFrame()),
              mSong->hasHlsBase() ? getFrameString (mSong->getLastFrame()): getFrameString (mSong->getTotalFrames()));
    }
  //}}}

private:
  //{{{
  std::string getFrameString (int frame) {

    if (mSong->getSamplesPerFrame() && mSong->getSampleRate()) {
      // can turn frame into seconds
      auto value = ((uint64_t)frame * mSong->getSamplesPerFrame()) / (mSong->getSampleRate() / 100);
      auto subSeconds = value % 100;

      value /= 100;
      //value += mWindow->getDayLightSeconds();
      auto seconds = value % 60;

      value /= 60;
      auto minutes = value % 60;

      value /= 60;
      auto hours = value % 60;

      // !!! must be a better formatter lib !!!
      return (hours > 0) ? (dec (hours) + ':' + dec (minutes, 2, '0') + ':' + dec(seconds, 2, '0')) :
               ((minutes > 0) ? (dec (minutes) + ':' + dec(seconds, 2, '0') + ':' + dec(subSeconds, 2, '0')) :
                 (dec(seconds) + ':' + dec(subSeconds, 2, '0')));
      }
    else
      return ("--:--:--");
    }
  //}}}

  //{{{
  void setZoom (int zoom) {

    mZoom = std::min (std::max (zoom, mMinZoom), mMaxZoom);

    // zoomIn expanding frame to mFrameWidth pix
    mFrameWidth = (mZoom < 0) ? -mZoom+1 : 1;

    // zoomOut summing mFrameStep frames per pix
    mFrameStep = (mZoom > 0) ? mZoom+1 : 1;
    }
  //}}}

  //{{{
  void drawRange (cVg* vg, int playFrame, int leftFrame, int rightFrame) {

    vg->beginPath();
    vg->rect (cPoint(mX, mY + mDstRangeTop), cPoint(mWidth, mRangeHeight));
    vg->fillColour (kVgDarkGrey);
    vg->triangleFill();

    vg->beginPath();
    for (auto &item : mSong->getSelect().getItems()) {
      auto firstx = (getWidth()/2.f) + (item.getFirstFrame() - playFrame) * mFrameWidth / mFrameStep;
      float lastx = item.getMark() ? firstx + 1.f :
                                     (getWidth()/2.f) + (item.getLastFrame() - playFrame) * mFrameWidth / mFrameStep;
      vg->rect (cPoint(mX  + firstx, mY + mDstRangeTop), cPoint(lastx - firstx, mRangeHeight));

      auto title = item.getTitle();
      if (!title.empty()) {
        //dstRect = { mRect.left + firstx + 2.f, mDstRangeTop + mRangeHeight - mWindow->getTextFormat()->GetFontSize(),
        //            mRect.right, mDstRangeTop + mRangeHeight + 4.f };
        //dc->DrawText (std::wstring (title.begin(), title.end()).data(), (uint32_t)title.size(), mWindow->getTextFormat(),
        //              dstRect, mWindow->getWhiteBrush(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
      }

    vg->fillColour (kVgWhite);
    vg->triangleFill();
    }
  //}}}
  //{{{
  void drawWave (cVg* vg, int playFrame, int leftFrame, int rightFrame, bool mono) {

    float values[2] = { 0.f };

    float peakValueScale = mWaveHeight / mSong->getMaxPeakValue() / 2.f;
    float powerValueScale = mWaveHeight / mSong->getMaxPowerValue() / 2.f;

    float xlen = (float)mFrameStep;
    if (mFrameStep == 1) {
      //{{{  draw all peak values
      float xorg = mX;

      vg->beginPath();

      for (auto frame = leftFrame; frame < rightFrame; frame += mFrameStep) {
        auto framePtr = mSong->getAudioFramePtr (frame);
        if (framePtr) {
          // draw frame peak values scaled to maxPeak
          if (framePtr->getPowerValues()) {
            auto peakValuesPtr = framePtr->getPeakValues();
            for (auto i = 0; i < 2; i++)
              values[i] = *peakValuesPtr++ * peakValueScale;
            }
          vg->rect (cPoint(xorg, mY + mDstWaveCentre - values[0]), cPoint(xlen, values[0] + values[1]));
          }
        xorg += xlen;
        }

      vg->fillColour (kVgDarkGrey);
      vg->triangleFill();
      }
      //}}}

    float xorg = mX;
    //{{{  draw powerValues before playFrame, summed if zoomed out
    vg->beginPath();

    for (auto frame = leftFrame; frame < playFrame; frame += mFrameStep) {
      auto framePtr = mSong->getAudioFramePtr (frame);
      if (framePtr) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          if (framePtr->getPowerValues()) {
           auto powerValuesPtr = framePtr->getPowerValues();
            for (auto i = 0; i < 2; i++)
              values[i] = *powerValuesPtr++ * peakValueScale;
            }
          }
        else {
          // sum mFrameStep frames, mFrameStep aligned, scaled to maxPower
          for (auto i = 0; i < 2; i++)
            values[i] = 0.f;

          auto alignedFrame = frame - (frame % mFrameStep);
          auto toSumFrame = std::min (alignedFrame + mFrameStep, rightFrame);
          for (auto sumFrame = alignedFrame; sumFrame < toSumFrame; sumFrame++) {
            auto framePtr = mSong->getAudioFramePtr (sumFrame);
            if (framePtr) {
              if (framePtr->getPowerValues()) {
                auto powerValuesPtr = framePtr->getPowerValues();
                for (auto i = 0; i < 2; i++)
                  values[i] += *powerValuesPtr++ * powerValueScale;
                }
              }
            }

          for (auto i = 0; i < 2; i++)
            values[i] /= toSumFrame - alignedFrame + 1;
          }
        vg->rect (cPoint(xorg, mY + mDstWaveCentre - values[0]), cPoint(xlen, values[0] + values[1]));
        }

      xorg += xlen;
      }

    vg->fillColour (kVgBlue);
    vg->triangleFill();
    //}}}
    //{{{  draw powerValues playFrame, no sum
    // power scaled to maxPeak
    vg->beginPath();

    auto framePtr = mSong->getAudioFramePtr (playFrame);
    if (framePtr) {
      //  draw play frame power scaled to maxPeak
      if (framePtr->getPowerValues()) {
        auto powerValuesPtr = framePtr->getPowerValues();
        for (auto i = 0; i < 2; i++)
          values[i] = *powerValuesPtr++ * peakValueScale;
        }
      vg->rect (cPoint(xorg, mY + mDstWaveCentre - values[0]), cPoint(xlen, values[0] + values[1]));
      }

    xorg += xlen;

    vg->fillColour (kVgWhite);
    vg->triangleFill();
    //}}}
    //{{{  draw powerValues after playFrame, summed if zoomed out
    vg->beginPath();

    for (auto frame = playFrame+mFrameStep; frame < rightFrame; frame += mFrameStep) {
      auto framePtr = mSong->getAudioFramePtr (frame);
      if (framePtr) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          if (framePtr->getPowerValues()) {
            auto powerValuesPtr = framePtr->getPowerValues();
            for (auto i = 0; i < 2; i++)
              values[i] = *powerValuesPtr++ * peakValueScale;
            }
          }
        else {
          // sum mFrameStep frames, mFrameStep aligned, scaled to maxPower
          for (auto i = 0; i < 2; i++)
            values[i] = 0.f;

          auto alignedFrame = frame - (frame % mFrameStep);
          auto toSumFrame = std::min (alignedFrame + mFrameStep, rightFrame);
          for (auto sumFrame = alignedFrame; sumFrame < toSumFrame; sumFrame++) {
            auto framePtr = mSong->getAudioFramePtr (sumFrame);
            if (framePtr) {
              if (framePtr->getPowerValues()) {
                auto powerValuesPtr = framePtr->getPowerValues();
                for (auto i = 0; i < 2; i++)
                  values[i] += *powerValuesPtr++ * powerValueScale;
                }
              }
            }

          for (auto i = 0; i < 2; i++)
            values[i] /= toSumFrame - alignedFrame + 1;
          }
        vg->rect (cPoint(xorg, mY + mDstWaveCentre - values[0]), cPoint(xlen, values[0] + values[1]));
        }

      xorg += xlen;
      }

    vg->fillColour (kVgGrey);
    vg->triangleFill();
    //}}}

    //{{{  copy reversed spectrum column to bitmap, clip high freqs to height
    //int freqSize = std::min (mSong->getNumFreqBytes(), (int)mFreqHeight);
    //int freqOffset = mSong->getNumFreqBytes() > (int)mFreqHeight ? mSong->getNumFreqBytes() - (int)mFreqHeight : 0;

    // bitmap sampled aligned to mFrameStep, !!! could sum !!! ?? ok if neg frame ???
    //auto alignedFromFrame = fromFrame - (fromFrame % mFrameStep);
    //for (auto frame = alignedFromFrame; frame < toFrame; frame += mFrameStep) {
      //auto framePtr = mSong->getAudioFramePtr (frame);
      //if (framePtr) {
        //if (framePtr->getFreqLuma()) {
          //uint32_t bitmapIndex = getSrcIndex (frame);
          //D2D1_RECT_U bitmapRectU = { bitmapIndex, 0, bitmapIndex+1, (UINT32)freqSize };
          //mBitmap->CopyFromMemory (&bitmapRectU, framePtr->getFreqLuma() + freqOffset, 1);
          //}
        //}
      //}
    //}}}
    }
  //}}}
  //{{{
  void drawFreq (cVg* vg, int playFrame) {

    float valueScale = 100.f / 255.f;

    vg->beginPath();

    float xorg = mX;
    auto framePtr = mSong->getAudioFramePtr (playFrame);
    if (framePtr && framePtr->getFreqValues()) {
      auto freqValues = framePtr->getFreqValues();
      for (auto i = 0; (i < mSong->getNumFreqBytes()) && ((i*2) < int(mWidth)); i++) {
        auto value =  freqValues[i] * valueScale;
        if (value > 1.f)
          vg->rect (cPoint(xorg, mY + mHeight - value), cPoint(2.f, value));
        xorg += 2.f;
        }
      }

    vg->fillColour (kVgYellow);
    vg->triangleFill();
    }
  //}}}
  //{{{
  void drawTime (cVg* vg, const std::string& firstFrameString,
                 const std::string& playFrameString, const std::string& lastFrameString) {

    // small lastFrameString, coloured, right
    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_BOTTOM);
    if (mSong->getHlsLoad() == cSong::eHlsIdle)
      vg->fillColour (kVgWhite);
    else if (mSong->getHlsLoad() == cSong::eHlsFailed)
      vg->fillColour (kVgRed);
    else
      vg->fillColour (kVgGreen);
    vg->text (mWidth, mHeight, lastFrameString);

    // small firstFrameString, white, left
    vg->fillColour (kVgWhite);
    vg->setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_BOTTOM);
    vg->text (0.f, mHeight, firstFrameString);

    // big playFrameString, white, centred
    vg->setFontSize ((float)getBigFontHeight());
    vg->setTextAlign (cVg::ALIGN_CENTER | cVg::ALIGN_BOTTOM);
    vg->text (mWidth/2.f, mHeight, playFrameString);
    }
  //}}}

  //{{{
  void drawOverviewWave (cVg* vg, int firstFrame, int playFrame, float playFrameX, float valueScale, bool mono) {
  // simple overview cache, invalidate if anything changed

    int lastFrame = mSong->getLastFrame();
    int totalFrames = mSong->getTotalFrames();

    bool changed = (mOverviewTotalFrames != totalFrames) ||
                   (mOverviewLastFrame != lastFrame) ||
                   (mOverviewFirstFrame != firstFrame) ||
                   (mOverviewValueScale != valueScale);

    float values[2] = { 0.f };

    vg->beginPath();
    float xorg = mX;
    float xlen = 1.f;
    for (auto x = 0; x < int(mWidth); x++) {
      // iterate widget width
      if (changed) {
        int frame = firstFrame + ((x * totalFrames) / int(mWidth));
        int toFrame = firstFrame + (((x+1) * totalFrames) / int(mWidth));
        if (toFrame > lastFrame)
          toFrame = lastFrame+1;

        auto framePtr = mSong->getAudioFramePtr (frame);
        if (framePtr && framePtr->getPowerValues()) {
          // accumulate frame, handle silence better
          float* powerValues = framePtr->getPowerValues();
          values[0] = powerValues[0];
          values[1] = mono ? 0 : powerValues[1];

          if (frame < toFrame) {
            int numSummedFrames = 1;
            frame++;
            while (frame < toFrame) {
              framePtr = mSong->getAudioFramePtr (frame);
              if (framePtr) {
                if (framePtr->getPowerValues()) {
                  auto powerValues = framePtr->getPowerValues();
                  values[0] += powerValues[0];
                  values[1] += mono ? 0 : powerValues[1];
                  numSummedFrames++;
                  }
                }
              frame++;
              }
            values[0] /= numSummedFrames;
            values[1] /= numSummedFrames;
            }
          values[0] *= valueScale;
          values[1] *= valueScale;
          mOverviewValuesL[x] = values[0];
          mOverviewValuesR[x] = values[0] + values[1];
          }
        }

      vg->rect (cPoint(xorg, mY + mDstOverviewCentre - mOverviewValuesL[x]), cPoint(xlen,  mOverviewValuesR[x]));
      xorg += 1.f;
      }
    vg->fillColour (kVgGrey);
    vg->triangleFill();

    // possible cache to stop recalc
    mOverviewTotalFrames = totalFrames;
    mOverviewLastFrame = lastFrame;
    mOverviewFirstFrame = firstFrame;
    mOverviewValueScale = valueScale;
    }
  //}}}
  //{{{
  void drawOverviewLens (cVg* vg, int playFrame, float centreX, float width, bool mono) {
  // draw frames centred at playFrame -/+ width in pixels, centred at centreX

    cLog::log (LOGINFO, "drawOverviewLens %d %f %f", playFrame, centreX, width);

    // cut hole and frame it
    vg->beginPath();
    vg->rect (cPoint(mX + centreX - width, mY + mDstOverviewTop), cPoint(width * 2.f, mOverviewHeight));
    vg->fillColour (kVgBlack);
    vg->triangleFill();
    // frame in yellow

    // calc leftmost frame, clip to valid frame, adjust firstX which may overlap left up to mFrameWidth
    float leftFrame = playFrame - width;
    float firstX = centreX - (playFrame - leftFrame);
    if (leftFrame < 0) {
      firstX += -leftFrame;
      leftFrame = 0;
      }

    int rightFrame = (int)(playFrame + width);
    rightFrame = std::min (rightFrame, mSong->getLastFrame());

    // calc lens max power
    float maxPowerValue = 0.f;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = mSong->getAudioFramePtr (frame);
      if (framePtr && framePtr->getPowerValues()) {
        auto powerValues = framePtr->getPowerValues();
        maxPowerValue = std::max (maxPowerValue, powerValues[0]);
        if (!mono)
          maxPowerValue = std::max (maxPowerValue, powerValues[1]);
        }
      }

    // draw unzoomed waveform, start before playFrame
    vg->beginPath();
    float xorg = mX + firstX;
    float valueScale = mOverviewHeight / 2.f / maxPowerValue;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = mSong->getAudioFramePtr (frame);
      if (framePtr && framePtr->getPowerValues()) {
        //if (framePtr->hasTitle()) {
          //{{{  draw song title yellow bar and text
          //cRect barRect = { dstRect.left-1.f, mDstOverviewTop, dstRect.left+1.f, mRect.bottom };
          //dc->FillRectangle (barRect, mWindow->getYellowBrush());

          //auto str = framePtr->getTitle();
          //dc->DrawText (std::wstring (str.begin(), str.end()).data(), (uint32_t)str.size(), mWindow->getTextFormat(),
                        //{ dstRect.left+2.f, mDstOverviewTop, getWidth(), mDstOverviewTop + mOverviewHeight },
                        //mWindow->getWhiteBrush());
          //}
          //}}}
        //if (framePtr->isSilence()) {
          //{{{  draw red silence
          //dstRect.top = mDstOverviewCentre - 2.f;
          //dstRect.bottom = mDstOverviewCentre + 2.f;
          //dc->FillRectangle (dstRect, mWindow->getRedBrush());
          //}
          //}}}

        if (frame == playFrame) {
          //{{{  finish before playFrame
          vg->fillColour (kVgBlue);
          vg->triangleFill();
          vg->beginPath();
          }
          //}}}

        auto powerValues = framePtr->getPowerValues();
        float yorg = mono ? mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f) :
                            mDstOverviewCentre - (powerValues[0] * valueScale);
        float ylen = mono ? powerValues[0] * valueScale * 2.f  :
                            (powerValues[0] + powerValues[1]) * valueScale;
        vg->rect (cPoint(xorg, mY + yorg), cPoint(1.f, ylen));

        if (frame == playFrame) {
          //{{{  finish playFrame, start after playFrame
          vg->fillColour (kVgWhite);
          vg->triangleFill();
          vg->beginPath();
          }
          //}}}
        }

      xorg += 1.f;
      }
    // finish after playFrame
    vg->fillColour (kVgGrey);
    vg->triangleFill();
    }
  //}}}
  //{{{
  void drawOverview (cVg* vg, int playFrame, bool mono) {

    if (!mSong->getTotalFrames())
      return;

    int firstFrame = mSong->getFirstFrame();
    float playFrameX = ((playFrame - firstFrame) * mWidth) / mSong->getTotalFrames();
    float valueScale = mOverviewHeight / 2.f / mSong->getMaxPowerValue();
    drawOverviewWave (vg, firstFrame, playFrame, playFrameX, valueScale, mono);

    if (mOverviewPressed) {
      //{{{  animate on
      if (mOverviewLens < getWidth() / 16.f) {
        mOverviewLens += getWidth() / 16.f / 6.f;
        //mWindow->changed();
        }
      }
      //}}}
    else {
      //{{{  animate off
      if (mOverviewLens > 1.f) {
        mOverviewLens /= 2.f;
        //mWindow->changed();
        }
      else if (mOverviewLens > 0.f) {
        // finish animate
        mOverviewLens = 0.f;
        //mWindow->changed();
        }
      }
      //}}}

    if (mOverviewLens > 0.f) {
      float overviewLensCentreX = (float)playFrameX;
      if (overviewLensCentreX - mOverviewLens < 0.f)
        overviewLensCentreX = (float)mOverviewLens;
      else if (overviewLensCentreX + mOverviewLens > mWidth)
        overviewLensCentreX = mWidth - mOverviewLens;

      drawOverviewLens (vg, playFrame, overviewLensCentreX, mOverviewLens-1.f, mono);
      }

    else {
      //  draw playFrame

      auto framePtr = mSong->getAudioFramePtr (playFrame);
      if (framePtr && framePtr->getPowerValues()) {
        vg->beginPath();
        auto powerValues = framePtr->getPowerValues();
        float yorg = mono ? (mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f)) :
                            (mDstOverviewCentre - (powerValues[0] * valueScale));
        float ylen = mono ? (powerValues[0] * valueScale * 2.f) : ((powerValues[0] + powerValues[1]) * valueScale);
        vg->rect (cPoint(mX+playFrameX, mY+yorg), cPoint(1.f, ylen));
        vg->fillColour (kVgWhite);
        vg->triangleFill();
        }
      }
    }
  //}}}

  //{{{  vars
  cSong* mSong;
  float mMove = 0;
  bool mMoved = false;
  float mPressInc = 0;

  // zoom - 0 unity, > 0 zoomOut framesPerPix, < 0 zoomIn pixPerFrame
  int mZoom = 0;
  int mMinZoom = -8;
  int mMaxZoom = 8;
  int mFrameWidth = 1;
  int mFrameStep = 1;

  float mPressedFrame = 0.f;
  bool mOverviewPressed = false;
  bool mRangePressed = false;

  float mOverviewLens = 0.f;

  // vertical layout
  float mFreqHeight = 0.f;
  float mWaveHeight = 0.f;
  float mRangeHeight = 0.f;
  float mOverviewHeight = 0.f;

  float mDstFreqTop = 0.f;
  float mDstWaveTop = 0.f;
  float mDstRangeTop = 0.f;
  float mDstOverviewTop = 0.f;

  float mDstWaveCentre = 0.f;
  float mDstOverviewCentre = 0.f;

  // mOverview cache
  int mOverviewTotalFrames = 0;
  int mOverviewLastFrame = 0;
  int mOverviewFirstFrame = 0;
  float mOverviewValueScale = 0.f;

  float mOverviewValuesL [1920] = { 0.f };
  float mOverviewValuesR [1920] = { 0.f };
  //}}}
  };
