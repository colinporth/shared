// cLoaderWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"
#include "../utils/cLoader.h"
#include "../utils/cSong.h"
#include "../utils/iVideoPool.h"
//}}}
constexpr bool kVideoPoolDebug = true;

class cLoaderWidget : public cWidget {
public:
  //{{{
  cLoaderWidget (cLoader* loader, iClockTime* clockTime, cPointF size, const std::string& debugName = "cLoaderWidget")
    : cWidget (kBlackF, size.x, size.y, debugName), mLoader(loader), mClockTime(clockTime) {}
  //}}}
  virtual ~cLoaderWidget() {}

  void setShowOverview (bool showOverview) { mShowOverview = showOverview; }
  void toggleGraphics() { mShowGraphics = ! mShowGraphics; }

  //{{{
  virtual void onDown (const cPointF& point) {

    cWidget::onDown (point);

    cSong* song = mLoader->getSong();

    //std::shared_lock<std::shared_mutex> lock (song->getSharedMutex());
    if (point.y > mDstOverviewTop) {
      int64_t frameNum = song->getFirstFrameNum() + int((point.x * song->getTotalFrames()) / getPixWidth());
      song->setPlayPts (song->getPtsFromFrameNum (frameNum));
      mOverviewPressed = true;
      }

    else if (point.y > mDstRangeTop) {
      mPressedFrameNum = song->getPlayFrameNum() + ((point.x - (getPixWidth()/2.f)) * mFrameStep / mFrameWidth);
      song->getSelect().start (int64_t(mPressedFrameNum));
      mRangePressed = true;
      //mWindow->changed();
      }

    else
      mPressedFrameNum = double(song->getFrameNumFromPts (int64_t(song->getPlayPts())));
    }
  //}}}
  //{{{
  virtual void onMove (const cPointF& point, const cPointF& inc) {

    cWidget::onMove (point, inc);

    cSong* song = mLoader->getSong();
    //std::shared_lock<std::shared_mutex> lock (song.getSharedMutex());
    if (mOverviewPressed)
      song->setPlayPts (
        song->getPtsFromFrameNum (
          song->getFirstFrameNum() + int64_t(point.x * song->getTotalFrames() / getPixWidth())));

    else if (mRangePressed) {
      mPressedFrameNum += (inc.x / mFrameWidth) * mFrameStep;
      song->getSelect().move (int64_t(mPressedFrameNum));
      //mWindow->changed();
      }

    else {
      mPressedFrameNum -= (inc.x / mFrameWidth) * mFrameStep;
      song->setPlayPts (song->getPtsFromFrameNum (int64_t(mPressedFrameNum)));
      }
    }
  //}}}
  //{{{
  virtual void onUp() {

    cWidget::onUp();
    mLoader->getSong()->getSelect().end();
    mOverviewPressed = false;
    mRangePressed = false;
    }
  //}}}
  //{{{
  virtual void onWheel (float delta) {

    //if (getShow())
    setZoom (mZoom - (int)delta);
    }
  //}}}
  //{{{
  virtual void layout (const cPointF& size) {

    mOverviewTotalFrames = 0;
    mOverviewLastFrame = 0;
    mOverviewFirstFrame = 0;
    mOverviewValueScale = 0.f;

    cWidget::layout (size);
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {

    //{{{  layout
    mWaveHeight = 100.f;
    mOverviewHeight = mShowOverview ? 100.f : 0.f;
    mRangeHeight = 8.f;
    mFreqHeight = mPixSize.y - mRangeHeight - mWaveHeight - mOverviewHeight;

    mDstFreqTop = 0.f;
    mDstWaveTop = mDstFreqTop + mFreqHeight;
    mDstRangeTop = mDstWaveTop + mWaveHeight;
    mDstOverviewTop = mDstRangeTop + mRangeHeight;

    mDstWaveCentre = mDstWaveTop + (mWaveHeight/2.f);
    mDstOverviewCentre = mDstOverviewTop + (mOverviewHeight/2.f);
    //}}}
    cVg* vg = draw->getVg();
    cSong* song = mLoader->getSong();
    iVideoPool* videoPool = mLoader->getVideoPool();

    bool foundPiccy = false;
    int64_t playPts = -1;
    if (song && videoPool) {
      //{{{  find and draw draw piccy
      playPts = song->getPlayPts();
      if (playPts >= 0) {
        auto videoFrame = videoPool->findFrame (playPts);
        if (videoFrame) {
          if (videoFrame->getPts() != mImagePts) {
            mImagePts = videoFrame->getPts();
            if (mImageId > -1)
              vg->updateImage (mImageId, (uint8_t*)videoFrame->getBuffer8888());
            else
              mImageId = vg->createImageRGBA (videoPool->getWidth(), videoPool->getHeight(),
                                              0, (uint8_t*)videoFrame->getBuffer8888());
            }

          // paint image rect
          vg->beginPath();
          vg->rect (cPointF(), mPixSize);
          vg->setFillPaint (vg->setImagePattern (cPointF(), mPixSize, 0.f, mImageId, 1.f));
          vg->triangleFill();
          foundPiccy = true;
          }
        }
      }
      //}}}
    if (!foundPiccy)
      draw->clear (kBlackF);

    if (mShowGraphics) {
      drawInfo (vg, videoPool);
      //{{{  draw progress spinners
      float loadFrac;
      float videoFrac;
      float audioFrac;
      mLoader->getFracs (loadFrac, audioFrac, videoFrac);
      if ((loadFrac > 0.f) &&
          ((loadFrac < 1.f) || (audioFrac > 0.f) || (videoFrac > 0.f))) {
        cPointF centre (mPixSize.x-20.f, 20.f);
        drawSpinner (vg, centre, 18.f,12.f, 0.1f, loadFrac,
                     sColourF(0.f,1.f,0.f,0.f), sColourF(0.f,1.f,0.f,0.75f));
        if (videoFrac > 0.f)
          drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - videoFrac), loadFrac,
                       sColourF(1.f, 0.f, 0.f, 0.25f), sColourF(1.f, 0.f, 0.f, 0.75f));
        if (audioFrac > 0.f)
          drawSpinner (vg, centre, 18.f,12.f, loadFrac * (1.f - audioFrac), loadFrac,
                       sColourF(0.f, 0.f, 1.f, 0.25f), sColourF(0.f, 0.f, 1.f, 0.75f));
        }
      //}}}

      // draw song
      if (song) {
        { // locked scope
        std::shared_lock<std::shared_mutex> lock (song->getSharedMutex());

        // wave left right frames, clip right not left
        int64_t playFrame = song->getPlayFrameNum();
        int64_t leftWaveFrame = playFrame - (((int(mPixSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
        int64_t rightWaveFrame = playFrame + (((int(mPixSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
        rightWaveFrame = std::min (rightWaveFrame, song->getLastFrameNum());

        drawRange (vg, song, playFrame, leftWaveFrame, rightWaveFrame);
        if (song->getNumFrames()) {
          bool mono = song->getNumChannels() == 1;
          drawWave (vg, song, playFrame, leftWaveFrame, rightWaveFrame, mono);
          if (mShowOverview)
            drawOverview (vg, song, playFrame, mono);
          drawFreq (vg, song, playFrame);
          }
        }

        drawTime (vg, song->getTimeString (song->getFirstPts(), mClockTime->getDayLightSeconds()),
                      song->getTimeString (song->getPlayPts(), mClockTime->getDayLightSeconds()),
                      song->getTimeString (song->getLastPts(), mClockTime->getDayLightSeconds()));
        }

      if (kVideoPoolDebug && videoPool && (playPts >= 0))
        drawVideoPool (vg, song, videoPool, playPts);
      }
    }
  //}}}

private:
  //{{{
  void setZoom (int zoom) {

    mZoom = std::min (std::max (zoom, mMinZoom), mMaxZoom);

    // zoomIn expanding frame to mFrameWidth pix
    mFrameWidth = (mZoom < 0) ? -mZoom+1 : 1;

    // zoomOut summing mFrameStep frames per pix
    mFrameStep = (mZoom > 0) ? mZoom+1 : 1;
    }
  //}}}

  // draws
  //{{{
  void drawSpinner (cVg* vg, const cPointF& centre, float outerRadius, float innerRadius,
                    float fracFrom, float fracTo,
                    const sColourF& colourFrom, const sColourF& colourTo) {

    if ((fracTo - fracFrom)  > 0.f) {
      float angleFrom = -kPiDiv2 + (fracFrom * k2Pi);
      float angleTo = -kPiDiv2 + (fracTo * k2Pi);

      vg->beginPath();
      vg->arc (centre, outerRadius, angleFrom, angleTo, cVg::eClockWise);
      vg->arc (centre, innerRadius, angleTo, angleFrom, cVg::eCounterClockWise);
      vg->closePath();

      float midRadius = (outerRadius + innerRadius) / 2.f;
      vg->setFillPaint (vg->setLinearGradient (centre + cPointF (cosf (angleFrom), sinf (angleFrom)) * midRadius,
                                               centre + cPointF (cosf (angleTo), sinf (angleTo)) * midRadius,
                                               colourFrom, colourTo));
      vg->fill();
      }
    }
  //}}}
  //{{{
  void drawInfo (cVg* vg, iVideoPool* videoPool) {
  // info text

    std::string infoString;

    if (videoPool)
      infoString += " " + dec(videoPool->getWidth()) + "x" + dec(videoPool->getHeight());

    int loadSize;
    int videoQueueSize;
    int audioQueueSize;
    mLoader->getSizes (loadSize, audioQueueSize, videoQueueSize);
    infoString += " " + dec(loadSize/1000) + "Kbytes";

    if (videoPool) {
      infoString += " a" + dec(audioQueueSize);
      infoString += " v" + dec(videoQueueSize);
      }

    vg->setFontSize ((float)getFontHeight());
    vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
    vg->setFillColour (kBlackF);
    vg->text (mPixOrg + cPointF(2.f, 2.f), infoString);
    vg->setFillColour (kWhiteF);
    vg->text (mPixOrg, infoString);
    }
  //}}}
  //{{{
  void drawVideoPool (cVg* vg, cSong* song, iVideoPool* videoPool, int64_t playerPts) {

    cPointF org { getPixCentre().x, mDstWaveCentre };
    const float ptsPerPix = float((90 * song->getSamplesPerFrame()) / 48);
    constexpr float kPesSizeScale = 1000.f;

    //{{{  draw B frames yellow
    vg->beginPath();

    for (auto frame : videoPool->getFramePool()) {
      if (frame.second->getFrameType() == 'B')  {
        float pix = floor ((frame.second->getPts() - playerPts) / ptsPerPix);
        float pes = frame.second->getPesSize() / kPesSizeScale;
        vg->rect (org + cPointF (pix, -pes), cPointF(1.f, pes));
        }
      }

    vg->setFillColour (kYellowF);
    vg->triangleFill();
    //}}}
    //{{{  draw P frames cyan
    vg->beginPath();

    for (auto frame : videoPool->getFramePool()) {
      if (frame.second->getFrameType() == 'P')  {
        float pix = floor ((frame.second->getPts() - playerPts) / ptsPerPix);
        float pes = frame.second->getPesSize() / kPesSizeScale;
        vg->rect (org + cPointF (pix, -pes), cPointF(1.f, pes));
        }
      }

    vg->setFillColour (kCyanF);
    vg->triangleFill();
    //}}}
    //{{{  draw I frames white
    vg->beginPath();

    for (auto frame : videoPool->getFramePool()) {
      if (frame.second->getFrameType() == 'I')  {
        float pix = floor ((frame.second->getPts() - playerPts) / ptsPerPix);
        float pes = frame.second->getPesSize() / kPesSizeScale;
        vg->rect (org + cPointF (pix, -pes), cPointF(1.f, pes));
        }
      }

    vg->setFillColour (kWhiteF);
    vg->triangleFill();
    //}}}
    }
  //}}}

  //{{{
  void drawRange (cVg* vg, cSong* song, int64_t playFrame, int64_t leftFrame, int64_t rightFrame) {

    //vg->beginPath();
    //vg->rect (mPixOrg + cPointF(0.f, mDstRangeTop), cPointF(mPixSize.x, mRangeHeight));
    //vg->setFillColour (kDarkGreyF);
    //vg->triangleFill();

    //vg->beginPath();
    //for (auto &item : song->getSelect().getItems()) {
    //  auto firstx = (getPixWidth()/2.f) + (item.getFirstFrame() - playFrame) * mFrameWidth / mFrameStep;
    //  float lastx = item.getMark() ? firstx + 1.f :
    //                                 (getPixWidth()/2.f) + (item.getLastFrame() - playFrame) * mFrameWidth / mFrameStep;
    //  vg->rect (mPixOrg + cPointF(firstx, mDstRangeTop), cPointF(lastx - firstx, mRangeHeight));

    //  auto title = item.getTitle();
    //  if (!title.empty()) {
        //dstRect = { mRect.left + firstx + 2.f, mDstRangeTop + mRangeHeight - mWindow->getTextFormat()->GetFontSize(),
        //            mRect.right, mDstRangeTop + mRangeHeight + 4.f };
        //dc->DrawText (std::wstring (title.begin(), title.end()).data(), (uint32_t)title.size(), mWindow->getTextFormat(),
        //              dstRect, mWindow->getWhiteBrush(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
     //   }
     // }

    //vg->setFillColour (kWhiteF);
    //vg->triangleFill();
    }
  //}}}
  //{{{
  void drawWave (cVg* vg, cSong* song, int64_t playFrame, int64_t leftFrame, int64_t rightFrame, bool mono) {

    float values[2] = { 0.f };

    float peakValueScale = mWaveHeight / song->getMaxPeakValue() / 2.f;
    float powerValueScale = mWaveHeight / song->getMaxPowerValue() / 2.f;

    float xlen = (float)mFrameStep;
    if (mFrameStep == 1) {
      //{{{  draw all peak values
      float xorg = mPixOrg.x;

      vg->beginPath();

      for (auto frame = leftFrame; frame < rightFrame; frame += mFrameStep) {
        auto framePtr = song->findFrameByFrameNum (frame);
        if (framePtr) {
          // draw frame peak values scaled to maxPeak
          if (framePtr->getPowerValues()) {
            auto peakValuesPtr = framePtr->getPeakValues();
            for (auto i = 0; i < 2; i++)
              values[i] = *peakValuesPtr++ * peakValueScale;
            }
          vg->rect (cPointF(xorg, mPixOrg.y + mDstWaveCentre - values[0]), cPointF(xlen, values[0] + values[1]));
          }
        xorg += xlen;
        }

      vg->setFillColour (kDarkGreyF);
      vg->triangleFill();
      }
      //}}}

    float xorg = mPixOrg.x;
    //{{{  draw powerValues before playFrame, summed if zoomed out
    vg->beginPath();

    for (auto frame = leftFrame; frame < playFrame; frame += mFrameStep) {
      auto framePtr = song->findFrameByFrameNum (frame);
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
            auto framePtr = song->findFrameByFrameNum (sumFrame);
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
        vg->rect (cPointF(xorg, mPixOrg.y + mDstWaveCentre - values[0]), cPointF(xlen, values[0] + values[1]));
        }

      xorg += xlen;
      }

    vg->setFillColour (kBlueF);
    vg->triangleFill();
    //}}}
    //{{{  draw powerValues playFrame, no sum
    // power scaled to maxPeak
    vg->beginPath();

    auto framePtr = song->findFrameByFrameNum (playFrame);
    if (framePtr) {
      //  draw play frame power scaled to maxPeak
      if (framePtr->getPowerValues()) {
        auto powerValuesPtr = framePtr->getPowerValues();
        for (auto i = 0; i < 2; i++)
          values[i] = *powerValuesPtr++ * peakValueScale;
        }
      vg->rect (cPointF(xorg, mPixOrg.y + mDstWaveCentre - values[0]), cPointF(xlen, values[0] + values[1]));
      }

    xorg += xlen;

    vg->setFillColour (kWhiteF);
    vg->triangleFill();
    //}}}
    //{{{  draw powerValues after playFrame, summed if zoomed out
    vg->beginPath();

    for (auto frame = playFrame+mFrameStep; frame < rightFrame; frame += mFrameStep) {
      auto framePtr = song->findFrameByFrameNum (frame);
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
            auto framePtr = song->findFrameByFrameNum (sumFrame);
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
        vg->rect (cPointF(xorg, mPixOrg.y + mDstWaveCentre - values[0]), cPointF(xlen, values[0] + values[1]));
        }

      xorg += xlen;
      }

    vg->setFillColour (kGreyF);
    vg->triangleFill();
    //}}}

    //{{{  copy reversed spectrum column to bitmap, clip high freqs to height
    //int freqSize = std::min (song->getNumFreqBytes(), (int)mFreqHeight);
    //int freqOffset = song->getNumFreqBytes() > (int)mFreqHeight ? song->getNumFreqBytes() - (int)mFreqHeight : 0;

    // bitmap sampled aligned to mFrameStep, !!! could sum !!! ?? ok if neg frame ???
    //auto alignedFromFrame = fromFrame - (fromFrame % mFrameStep);
    //for (auto frame = alignedFromFrame; frame < toFrame; frame += mFrameStep) {
      //auto framePtr = song->getAudioFramePtr (frame);
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
  void drawFreq (cVg* vg, cSong* song, int64_t playFrame) {

    float valueScale = 100.f / 255.f;

    vg->beginPath();

    float xorg = mPixOrg.x;
    auto framePtr = song->findFrameByFrameNum (playFrame);
    if (framePtr && framePtr->getFreqValues()) {
      auto freqValues = framePtr->getFreqValues();
      for (auto i = 0; (i < song->getNumFreqBytes()) && ((i*2) < int(mPixSize.x)); i++) {
        auto value =  freqValues[i] * valueScale;
        if (value > 1.f)
          vg->rect (cPointF(xorg, mPixOrg.y + mPixSize.y - value), cPointF(2.f, value));
        xorg += 2.f;
        }
      }

    vg->setFillColour (kYellowF);
    vg->triangleFill();
    }
  //}}}
  //{{{
  void drawTime (cVg* vg, const std::string& firstFrameString,
                 const std::string& playFrameString, const std::string& lastFrameString) {

    vg->setFillColour (kWhiteF);

    // small lastFrameString, white, right
    vg->setFontSize (getFontHeight());
    vg->setTextAlign (cVg::eAlignRight | cVg::eAlignBottom);
    vg->text (mPixSize, lastFrameString);

    // small firstFrameString, white, left
    vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignBottom);
    vg->text (cPointF(0.f, mPixSize.y), firstFrameString);

    // big playFrameString, white, centred
    vg->setFontSize (getBigFontHeight());
    vg->setTextAlign (cVg::eAlignCentre | cVg::eAlignBottom);
    vg->text (cPointF(mPixSize.x/2.f, mPixSize.y), playFrameString);
    }
  //}}}
  //{{{
  void drawOverviewWave (cVg* vg, cSong* song, int64_t firstFrame, int64_t playFrame, float playFrameX, float valueScale, bool mono) {
  // simple overview cache, invalidate if anything changed

    int64_t lastFrame = song->getLastFrameNum();
    int64_t totalFrames = song->getTotalFrames();

    bool changed = (mOverviewTotalFrames != totalFrames) ||
                   (mOverviewLastFrame != lastFrame) ||
                   (mOverviewFirstFrame != firstFrame) ||
                   (mOverviewValueScale != valueScale);

    float values[2] = { 0.f };

    vg->beginPath();
    float xorg = mPixOrg.x;
    float xlen = 1.f;
    for (auto x = 0; x < int(mPixSize.x); x++) {
      // iterate widget width
      if (changed) {
        int64_t frame = firstFrame + ((x * totalFrames) / int(mPixSize.x));
        int64_t toFrame = firstFrame + (((x+1) * totalFrames) / int(mPixSize.x));
        if (toFrame > lastFrame)
          toFrame = lastFrame+1;

        auto framePtr = song->findFrameByFrameNum (frame);
        if (framePtr && framePtr->getPowerValues()) {
          // accumulate frame, handle silence better
          float* powerValues = framePtr->getPowerValues();
          values[0] = powerValues[0];
          values[1] = mono ? 0 : powerValues[1];

          if (frame < toFrame) {
            int numSummedFrames = 1;
            frame++;
            while (frame < toFrame) {
              framePtr = song->findFrameByFrameNum (frame);
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

      vg->rect (cPointF(xorg, mPixOrg.y + mDstOverviewCentre - mOverviewValuesL[x]), cPointF(xlen,  mOverviewValuesR[x]));
      xorg += 1.f;
      }
    vg->setFillColour (kGreyF);
    vg->triangleFill();

    // possible cache to stop recalc
    mOverviewTotalFrames = totalFrames;
    mOverviewLastFrame = lastFrame;
    mOverviewFirstFrame = firstFrame;
    mOverviewValueScale = valueScale;
    }
  //}}}
  //{{{
  void drawOverviewLens (cVg* vg, cSong* song, int64_t playFrame, float centreX, float width, bool mono) {
  // draw frames centred at playFrame -/+ width in pixels, centred at centreX

    cLog::log (LOGINFO, "drawOverviewLens %d %f %f", playFrame, centreX, width);

    // cut hole and frame it
    vg->beginPath();
    vg->rect (mPixOrg + cPointF(centreX - width, mDstOverviewTop), cPointF(width * 2.f, mOverviewHeight));
    vg->setFillColour (kBlackF);
    vg->triangleFill();
    // frame in yellow

    // calc leftmost frame, clip to valid frame, adjust firstX which may overlap left up to mFrameWidth
    float leftFrame = playFrame - width;
    float firstX = centreX - (playFrame - leftFrame);
    if (leftFrame < 0) {
      firstX += -leftFrame;
      leftFrame = 0;
      }

    int64_t rightFrame = playFrame + (int64_t)width;
    rightFrame = std::min (rightFrame, song->getLastFrameNum());

    // calc lens max power
    float maxPowerValue = 0.f;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = song->findFrameByFrameNum (frame);
      if (framePtr && framePtr->getPowerValues()) {
        auto powerValues = framePtr->getPowerValues();
        maxPowerValue = std::max (maxPowerValue, powerValues[0]);
        if (!mono)
          maxPowerValue = std::max (maxPowerValue, powerValues[1]);
        }
      }

    // draw unzoomed waveform, start before playFrame
    vg->beginPath();
    float xorg = mPixOrg.x + firstX;
    float valueScale = mOverviewHeight / 2.f / maxPowerValue;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = song->findFrameByFrameNum (frame);
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
          vg->setFillColour (kBlueF);
          vg->triangleFill();
          vg->beginPath();
          }
          //}}}

        auto powerValues = framePtr->getPowerValues();
        float yorg = mono ? mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f) :
                            mDstOverviewCentre - (powerValues[0] * valueScale);
        float ylen = mono ? powerValues[0] * valueScale * 2.f  :
                            (powerValues[0] + powerValues[1]) * valueScale;
        vg->rect (cPointF(xorg, mPixOrg.y + yorg), cPointF(1.f, ylen));

        if (frame == playFrame) {
          //{{{  finish playFrame, start after playFrame
          vg->setFillColour (kWhiteF);
          vg->triangleFill();
          vg->beginPath();
          }
          //}}}
        }

      xorg += 1.f;
      }
    // finish after playFrame
    vg->setFillColour (kGreyF);
    vg->triangleFill();
    }
  //}}}
  //{{{
  void drawOverview (cVg* vg, cSong* song, int64_t playFrame, bool mono) {

    if (!song->getTotalFrames())
      return;

    int64_t firstFrame = song->getFirstFrameNum();
    float playFrameX = ((playFrame - firstFrame) * mPixSize.x) / song->getTotalFrames();
    float valueScale = mOverviewHeight / 2.f / song->getMaxPowerValue();
    drawOverviewWave (vg, song, firstFrame, playFrame, playFrameX, valueScale, mono);

    if (mOverviewPressed) {
      //{{{  animate on
      if (mOverviewLens < getPixWidth() / 16.f) {
        mOverviewLens += getPixWidth() / 16.f / 6.f;
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
      else if (overviewLensCentreX + mOverviewLens > mPixSize.x)
        overviewLensCentreX = mPixSize.x - mOverviewLens;

      drawOverviewLens (vg, song, playFrame, overviewLensCentreX, mOverviewLens-1.f, mono);
      }

    else {
      //  draw playFrame
      auto framePtr = song->findFrameByFrameNum (playFrame);
      if (framePtr && framePtr->getPowerValues()) {
        vg->beginPath();
        auto powerValues = framePtr->getPowerValues();
        float yorg = mono ? (mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f)) :
                            (mDstOverviewCentre - (powerValues[0] * valueScale));
        float ylen = mono ? (powerValues[0] * valueScale * 2.f) : ((powerValues[0] + powerValues[1]) * valueScale);
        vg->rect (mPixOrg + cPointF(playFrameX, yorg), cPointF(1.f, ylen));
        vg->setFillColour (kWhiteF);
        vg->triangleFill();
        }
      }
    }
  //}}}

  //{{{  vars
  cLoader* mLoader;
  iClockTime* mClockTime;
  int64_t mImagePts = 0;
  int mImageId = -1;

  bool mShowGraphics = true;

  float mMove = 0;
  bool mMoved = false;
  float mPressInc = 0;

  // zoom - 0 unity, > 0 zoomOut framesPerPix, < 0 zoomIn pixPerFrame
  int mZoom = 0;
  int mMinZoom = -8;
  int mMaxZoom = 8;
  int mFrameWidth = 1;
  int mFrameStep = 1;

  double mPressedFrameNum = 0;
  bool mOverviewPressed = false;
  bool mRangePressed = false;

  bool mShowOverview = false;
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
  int64_t mOverviewTotalFrames = 0;
  int64_t mOverviewLastFrame = 0;
  int64_t mOverviewFirstFrame = 0;
  float mOverviewValueScale = 0.f;

  float mOverviewValuesL [1920] = { 0.f };
  float mOverviewValuesR [1920] = { 0.f };
  //}}}
  };
