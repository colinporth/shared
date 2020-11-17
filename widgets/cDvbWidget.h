// cDvbWidget.h
//{{{  includes
#pragma once

#include <string>
#include <algorithm>

#include "../dvb/cDvb.h"

#include "../utils/utils.h"
#include "../dvb/cTransportStream.h"
#include "../dvb/cSubtitle.h"

#include "cWidget.h"
//}}}

class cDvbWidget: public cWidget {
public:
  //{{{
  cDvbWidget (cDvb* dvb, float width, float height, const std::string& debugName = "cDvbWidget") :
    cWidget (width, height, debugName), mDvb(dvb) {}
  //}}}
  virtual ~cDvbWidget() {}

  //{{{
  virtual void onMove (const cPointF& point, const cPointF& inc) {
  // inc and clip mScroll to top

    mScroll = std::max (mScroll - inc.y, -1.f);

    // clip mScroll to bottom
    if (mLastHeight > mSize.y)
      mScroll = std::min (mScroll, mLastHeight - mSize.y);
    else
      mScroll = 0.f;
    }
  //}}}
  //{{{
  virtual void onWheel (float delta) {
  // inc and clip lineHeight

    mZoom  = std::min (5.f, std::max (mZoom + delta, -5.f));
    }
  //}}}

  //{{{
  virtual void onDraw (iDraw* draw) {

    auto vg = draw->getVg();
    vg->scissor (mOrg, mSize);

    int lastSid = 0;
    int imageIndex = 0;
    float lineHeight = mZoom + (kBox * 4.f / 5.f);

    float x = mOrg.x + 2.f;
    float y = mOrg.y - mScroll;
    for (auto& pidInfoItem : mDvb->getTransportStream()->mPidInfoMap) { // iteratepidInfo
      cPidInfo& pidInfo = pidInfoItem.second;
      int pid = pidInfo.mPid;

      if ((pidInfo.mSid != lastSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        // spacer on change of service
        draw->drawRect (kGreyF, cPointF(x, y), cPointF(mSize.x, 1.f));

      auto pidString = dec (pidInfo.mPackets,mPacketDigits) +
                       (mContDigits ? (":" + dec(pidInfo.mErrors, mContDigits)) : "") +
                       " " + dec(pid, 4) +
                       " " + getFullPtsString (pidInfo.mPts) +
                       " " + pidInfo.getTypeString();
      float textWidth = draw->drawText (kLightGreyF, lineHeight, pidString, 
                                        cPointF(x,y), cPointF(mSize.x-3.f, lineHeight));
      float visx = x + textWidth + lineHeight/2.f;

      if (pidInfo.mStreamType == 6) {
        //{{{  draw subtitle
        cSubtitle* subtitle = mDvb->getSubtitleBySid (pidInfo.mSid);

        if (subtitle) {
          if (!subtitle->mRects.empty()) {
            // subttitle with some rects
            float ySub = y - lineHeight;

            for (int line = (int)subtitle->mRects.size()-1; line >= 0; line--) {
              // iterate rects
              float dstWidth = mSize.x - visx;
              float dstHeight = float(subtitle->mRects[line]->mHeight * dstWidth) / subtitle->mRects[line]->mWidth;
              if (dstHeight > lineHeight) {
                // scale to fit line
                float scaleh = lineHeight / dstHeight;
                dstHeight = lineHeight;
                dstWidth *= scaleh;
                }

              // draw bgnd
              draw->drawRect (kDarkGreyF, cPointF(visx, ySub), cPointF(dstWidth, dstHeight));

              // create or update rect image
              if (mImage[imageIndex] == -1) {
                if (imageIndex < 20)
                  mImage[imageIndex] = vg->createImageRGBA (
                    subtitle->mRects[line]->mWidth, subtitle->mRects[line]->mHeight, 0, (uint8_t*)subtitle->mRects[line]->mPixData);
                else
                  cLog::log (LOGERROR, "too many cDvbWidget images, fixit");
                }
              else if (subtitle->mChanged)  // !!! assumes image is same size as before !!!
                vg->updateImage (mImage[imageIndex], (uint8_t*)subtitle->mRects[line]->mPixData);

              // draw rect image
              auto imagePaint = vg->setImagePattern (cPointF(visx, ySub), cPointF(dstWidth, dstHeight), 0.f, mImage[imageIndex], 1.f);
              vg->beginPath();
              vg->rect (cPointF(visx, ySub), cPointF(dstWidth, dstHeight));
              vg->setFillPaint (imagePaint);
              vg->fill();

              imageIndex++;

              // draw rect position
              std::string text = dec(subtitle->mRects[line]->mX) + "," + dec(subtitle->mRects[line]->mY,3);
              float posWidth = draw->drawTextRight (kWhiteF, lineHeight, text, cPointF(mOrg.x + mSize.x,  ySub), cPointF(mSize.x - mOrg.x, dstHeight));

              // draw clut
              float clutX = mSize.x - mOrg.x - posWidth - lineHeight * 4.f;
              for (int i = 0; i < subtitle->mRects[line]->mClutSize; i++) {
                float cx = clutX + (i % 8) * lineHeight / 2.f;
                float cy = ySub + (i / 8) * lineHeight / 2.f;
                draw->drawRect (sColourF(subtitle->mRects[line]->mClut[i]), cPointF(cx, cy), cPointF((lineHeight/2.f)-1.f, (lineHeight / 2.f) - 1.f));
                }

              // next subtitle line
              ySub += lineHeight;
              }
            }

          // reset changed flag
          subtitle->mChanged = false;
          }
        }
        //}}}

      mMaxPidPackets = std::max (mMaxPidPackets, (float)pidInfo.mPackets);
      float frac = pidInfo.mPackets / mMaxPidPackets;
      draw->drawRect (kDarkOrangeF, cPointF(visx, y), cPointF(frac * (mSize.x - textWidth), lineHeight-1.f));

      std::string str;
      if ((pidInfo.mStreamType == 0) && (pidInfo.mSid > 0))
        str += dec(pidInfo.mSid) + " ";
      str += pidInfo.getInfoString();
      draw->drawText (kLightGreyF, lineHeight, str, cPointF(visx,  y), cPointF(mSize.x - textWidth, lineHeight));

      if (pidInfo.mPackets > pow (10, mPacketDigits))
        mPacketDigits++;

      lastSid = pidInfo.mSid;
      y += lineHeight;
      }
    mLastHeight = y + mScroll;

    if (mDvb->getTransportStream()->getErrors() > pow (10, mContDigits))
      mContDigits++;
    }
  //}}}

private:
  cDvb* mDvb;
  int mContDigits = 0;
  int mPacketDigits = 1;
  float mMaxPidPackets = 0;

  float mScroll = -1.f;
  float mZoom  = 0.f;
  float mLastHeight = 0.f;

  int mImage[20] =  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
  };
