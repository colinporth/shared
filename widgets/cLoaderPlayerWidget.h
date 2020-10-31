// cLoaderPlayerWidget.h
#pragma once
//{{{  includes
#include "cWidget.h"
#include "../utils/cLoaderPlayer.h"
#include "../utils/cSong.h"
//}}}
constexpr bool kVideoPoolDebug = true;

class cLoaderPlayerWidget : public cWidget {
public:
  cLoaderPlayerWidget (cLoaderPlayer* loaderPlayer, iClockTime* clockTime, cPointF size)
    : cWidget (kBlackF, size.x, size.y), mLoaderPlayer(loaderPlayer), mClockTime(clockTime) {}
  virtual ~cLoaderPlayerWidget() {}

  void setShowOverview (bool showOverview) { mShowOverview = showOverview; }

  //{{{
  virtual void onDown (const cPointF& p) {

    cWidget::onDown (p);

    auto song = mLoaderPlayer->getSong();
    //std::shared_lock<std::shared_mutex> lock (song->getSharedMutex());
    if (p.y > mDstOverviewTop) {
      auto frame = song->getFirstFrame() + int((p.x * song->getTotalFrames()) / getPixWidth());
      song->setPlayFrame (frame);
      mOverviewPressed = true;
      }

    else if (p.y > mDstRangeTop) {
      mPressedFrame = song->getPlayFrame() + ((p.x - (getPixWidth()/2.f)) * mFrameStep / mFrameWidth);
      song->getSelect().start (int(mPressedFrame));
      mRangePressed = true;
      //mWindow->changed();
      }

    else
      mPressedFrame = (float)song->getPlayFrame();
    }
  //}}}
  //{{{
  virtual void onMove (const cPointF& p, const cPointF& inc) {

    cWidget::onMove (p, inc);

    auto song = mLoaderPlayer->getSong();
    //std::shared_lock<std::shared_mutex> lock (song.getSharedMutex());
    if (mOverviewPressed)
      song->setPlayFrame (song->getFirstFrame() + int((p.x * song->getTotalFrames()) / getPixWidth()));

    else if (mRangePressed) {
      mPressedFrame += (inc.x / mFrameWidth) * mFrameStep;
      song->getSelect().move ((int)mPressedFrame);
      //mWindow->changed();
      }

    else {
      mPressedFrame -= (inc.x / mFrameWidth) * mFrameStep;
      song->setPlayFrame ((int)mPressedFrame);
      }
    }
  //}}}
  //{{{
  virtual void onUp() {

    cWidget::onUp();
    mLoaderPlayer->getSong()->getSelect().end();
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

    cVg* vg = draw->getVg();
    auto videoDecode = mLoaderPlayer->getVideoDecode();

    if (videoDecode) {
      //{{{  draw piccy
      auto frame = videoDecode->findPlayFrame();
      if (frame) {
        if (frame->getPts() != mImagePts) {
          mImagePts = frame->getPts();
          if (mImageId > -1)
            vg->updateImage (mImageId, (uint8_t*)frame->getBuffer8888());
          else
            mImageId = vg->createImageRGBA (videoDecode->getWidth(), videoDecode->getHeight(),
                                            0, (uint8_t*)frame->getBuffer8888());
          }

        // paint image rect
        vg->beginPath();
        vg->rect (cPointF(), mPixSize);
        vg->setFillPaint (vg->setImagePattern (cPointF(), mPixSize, 0.f, mImageId, 1.f));
        vg->triangleFill();
        }
      else
        draw->clear (kBlackF);
      }
      //}}}
    else
      draw->clear (kBlackF);

    drawInfo (vg, videoDecode);
    //{{{  draw progress spinners
    float loadFrac;
    float videoFrac;
    float audioFrac;
    mLoaderPlayer->getFrac (loadFrac, videoFrac, audioFrac);
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

    // layout
    mWaveHeight = 100.f;
    mOverviewHeight = mShowOverview ? 100.f : 0.f;
    mRangeHeight = 8.f;
    mFreqHeight = mPixSize.y - mRangeHeight - mWaveHeight - mOverviewHeight;
    mDstFreqTop = 0;
    mDstWaveTop = mDstFreqTop + mFreqHeight;
    mDstRangeTop = mDstWaveTop + mWaveHeight;
    mDstOverviewTop = mDstRangeTop + mRangeHeight;
    mDstWaveCentre = mDstWaveTop + (mWaveHeight/2.f);
    mDstOverviewCentre = mDstOverviewTop + (mOverviewHeight/2.f);

    auto song = mLoaderPlayer->getSong();

    if (kVideoPoolDebug && videoDecode)
      drawVideoPool (vg, song, videoDecode);

    if (song) {
      { // locked scope
      std::shared_lock<std::shared_mutex> lock (song->getSharedMutex());

      // wave left right frames, clip right not left
      int playFrame = song->getPlayFrame();
      int leftWaveFrame = playFrame - (((int(mPixSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
      int rightWaveFrame = playFrame + (((int(mPixSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
      rightWaveFrame = std::min (rightWaveFrame, song->getLastFrame());

      drawRange (vg, song, playFrame, leftWaveFrame, rightWaveFrame);
      if (song->getNumFrames()) {
        bool mono = song->getNumChannels() == 1;
        drawWave (vg, song, playFrame, leftWaveFrame, rightWaveFrame, mono);
        if (mShowOverview)
          drawOverview (vg, song, playFrame, mono);
        drawFreq (vg, song, playFrame);
        }
      }

      drawTime (vg, song->hasHlsBase() ? getFrameString (song, song->getFirstFrame()) : "",
                    getFrameString (song, song->getPlayFrame()),
                    song->hasHlsBase() ? getFrameString (song, song->getLastFrame()) : getFrameString (song, song->getTotalFrames()));
      }
    }
  //}}}

private:
  //{{{
  std::string getFrameString (cSong* song, int frame) {

    if (song->getSamplesPerFrame() && song->getSampleRate()) {
      // can turn frame into seconds
      auto value = ((uint64_t)frame * song->getSamplesPerFrame()) / (song->getSampleRate() / 100);
      auto subSeconds = value % 100;

      value /= 100;
      value += mClockTime->getDayLightSeconds();
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
  void drawInfo (cVg* vg, iVideoDecode* videoDecode) {
  // info text

    std::string infoString;

    if (videoDecode)
      infoString += " " + dec(videoDecode->getWidth()) + "x" + dec(videoDecode->getHeight()) +
                    ":" + dec(videoDecode->getFramePoolSize());

    int loadSize;
    int videoQueueSize;
    int audioQueueSize;
    mLoaderPlayer->getSizes (loadSize, videoQueueSize, audioQueueSize);
    infoString += " " + dec(loadSize/1000) + "Kbytes";

    if (videoDecode) {
      infoString += " v" + dec(videoQueueSize);
      infoString += " a" + dec(audioQueueSize);
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
  void drawVideoPool (cVg* vg, cSong* song, iVideoDecode* videoDecode) {

    cPointF org { getPixCentre().x, getPixSize().y - 100.f };
    float ptsPerPix = float((90 * song->getSamplesPerFrame()) / 48);

    // get playFrame pts
    auto framePtr = song->getFramePtr (song->getPlayFrame());
    if (framePtr) {
      int64_t playPts = framePtr->mPts;

      // draw pool num
      vg->beginPath();
      int i = 0;
      for (auto frame : videoDecode->getFramePool()) {
        float pix = floor ((frame->getPts() - playPts) / ptsPerPix);
        float pixEnd = floor ((frame->getPtsEnd() - playPts) / ptsPerPix);
        vg->rect (org + cPointF (pix, -i/2.f), cPointF (pixEnd - pix, i/2.f));
        i++;
        }
      vg->setFillColour (kSemiOpaqueGreenF);
      vg->triangleFill();

      // draw pool frame num
      vg->beginPath();
      for (auto frame : videoDecode->getFramePool()) {
        float pix = floor ((frame->getPts() - playPts) / ptsPerPix);
        float pixEnd = floor ((frame->getPtsEnd() - playPts) / ptsPerPix);
        vg->rect (org + cPointF (pix, -frame->getNum()/2.f), cPointF(pixEnd - pix, frame->getNum()/2.f));
        }
      vg->setFillColour (kSemiOpaqueBlueF);
      vg->triangleFill();

      // draw pool frame pesSize
      vg->beginPath();
      for (auto frame : videoDecode->getFramePool()) {
        float pix = floor ((frame->getPts() - playPts) / ptsPerPix);
        float pixEnd = floor ((frame->getPtsEnd() - playPts) / ptsPerPix);
        float pes = frame->getPesSize() / 1000.f;
        vg->rect (org + cPointF (pix, -pes), cPointF(pixEnd - pix, pes));
        }
      vg->setFillColour (kSemiOpaqueWhiteF);
      vg->triangleFill();
      }

    }
  //}}}

  //{{{
  void drawRange (cVg* vg, cSong* song, int playFrame, int leftFrame, int rightFrame) {

    vg->beginPath();
    vg->rect (mPixOrg + cPointF(0.f, mDstRangeTop), cPointF(mPixSize.x, mRangeHeight));
    vg->setFillColour (kDarkGreyF);
    vg->triangleFill();

    vg->beginPath();
    for (auto &item : song->getSelect().getItems()) {
      auto firstx = (getPixWidth()/2.f) + (item.getFirstFrame() - playFrame) * mFrameWidth / mFrameStep;
      float lastx = item.getMark() ? firstx + 1.f :
                                     (getPixWidth()/2.f) + (item.getLastFrame() - playFrame) * mFrameWidth / mFrameStep;
      vg->rect (mPixOrg + cPointF(firstx, mDstRangeTop), cPointF(lastx - firstx, mRangeHeight));

      auto title = item.getTitle();
      if (!title.empty()) {
        //dstRect = { mRect.left + firstx + 2.f, mDstRangeTop + mRangeHeight - mWindow->getTextFormat()->GetFontSize(),
        //            mRect.right, mDstRangeTop + mRangeHeight + 4.f };
        //dc->DrawText (std::wstring (title.begin(), title.end()).data(), (uint32_t)title.size(), mWindow->getTextFormat(),
        //              dstRect, mWindow->getWhiteBrush(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
      }

    vg->setFillColour (kWhiteF);
    vg->triangleFill();
    }
  //}}}
  //{{{
  void drawWave (cVg* vg, cSong* song, int playFrame, int leftFrame, int rightFrame, bool mono) {

    float values[2] = { 0.f };

    float peakValueScale = mWaveHeight / song->getMaxPeakValue() / 2.f;
    float powerValueScale = mWaveHeight / song->getMaxPowerValue() / 2.f;

    float xlen = (float)mFrameStep;
    if (mFrameStep == 1) {
      //{{{  draw all peak values
      float xorg = mPixOrg.x;

      vg->beginPath();

      for (auto frame = leftFrame; frame < rightFrame; frame += mFrameStep) {
        auto framePtr = song->getFramePtr (frame);
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
      auto framePtr = song->getFramePtr (frame);
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
            auto framePtr = song->getFramePtr (sumFrame);
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

    auto framePtr = song->getFramePtr (playFrame);
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
      auto framePtr = song->getFramePtr (frame);
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
            auto framePtr = song->getFramePtr (sumFrame);
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
  void drawFreq (cVg* vg, cSong* song, int playFrame) {

    float valueScale = 100.f / 255.f;

    vg->beginPath();

    float xorg = mPixOrg.x;
    auto framePtr = song->getFramePtr (playFrame);
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
  void drawOverviewWave (cVg* vg, cSong* song, int firstFrame, int playFrame, float playFrameX, float valueScale, bool mono) {
  // simple overview cache, invalidate if anything changed

    int lastFrame = song->getLastFrame();
    int totalFrames = song->getTotalFrames();

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
        int frame = firstFrame + ((x * totalFrames) / int(mPixSize.x));
        int toFrame = firstFrame + (((x+1) * totalFrames) / int(mPixSize.x));
        if (toFrame > lastFrame)
          toFrame = lastFrame+1;

        auto framePtr = song->getFramePtr (frame);
        if (framePtr && framePtr->getPowerValues()) {
          // accumulate frame, handle silence better
          float* powerValues = framePtr->getPowerValues();
          values[0] = powerValues[0];
          values[1] = mono ? 0 : powerValues[1];

          if (frame < toFrame) {
            int numSummedFrames = 1;
            frame++;
            while (frame < toFrame) {
              framePtr = song->getFramePtr (frame);
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
  void drawOverviewLens (cVg* vg, cSong* song, int playFrame, float centreX, float width, bool mono) {
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

    int rightFrame = (int)(playFrame + width);
    rightFrame = std::min (rightFrame, song->getLastFrame());

    // calc lens max power
    float maxPowerValue = 0.f;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = song->getFramePtr (frame);
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
      auto framePtr = song->getFramePtr (frame);
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
  void drawOverview (cVg* vg, cSong* song, int playFrame, bool mono) {

    if (!song->getTotalFrames())
      return;

    int firstFrame = song->getFirstFrame();
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

      auto framePtr = song->getFramePtr (playFrame);
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
  cLoaderPlayer* mLoaderPlayer;
  iClockTime* mClockTime;
  int64_t mImagePts = 0;
  int mImageId = -1;

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
  int mOverviewTotalFrames = 0;
  int mOverviewLastFrame = 0;
  int mOverviewFirstFrame = 0;
  float mOverviewValueScale = 0.f;

  float mOverviewValuesL [1920] = { 0.f };
  float mOverviewValuesR [1920] = { 0.f };
  //}}}
  };
