// cPerfGraph.h
#pragma once

class cPerfGraph {
public:
  enum eRenderStyle { eRenderFps, eRenderMs, eRenderPercent };
  //{{{
  cPerfGraph (eRenderStyle style, std::string name) : mStyle(style), mName(name) {
    clear();
    }
  //}}}

  //{{{
  void clear() {

    mHead = 0;
    mNumValues = 0;
    for (int i = 0; i < kGraphHistorySize; i++)
      mValues[i] = 0.0f;
    }
  //}}}
  //{{{
  void start (float startTime) {
    mStartTime = startTime;
    }
  //}}}

  //{{{
  void updateTime (float time) {

    mHead = (mHead+1) % kGraphHistorySize;
    mValues[mHead] = time - mStartTime;
    mNumValues++;
    mStartTime = time;
    }
  //}}}
  //{{{
  void updateValue (float value) {

    mHead = (mHead+1) % kGraphHistorySize;
    mValues[mHead] = value;
    mNumValues++;
    }
  //}}}

  //{{{
  void render (cVg* vg, cPointF org, cPointF size) {

    // bgnd
    vg->beginPath();
    vg->rect (org, size);
    vg->setFillColour (kSemiOpaqueBlackF);
    vg->fill();

    // graph
    vg->beginPath();
    vg->moveTo (org + cPointF(0.f, size.y));
    if (mStyle == eRenderFps) {
      //{{{  fps graph
      for (int i = 0; i < kGraphHistorySize; i++) {
        float v =  mValues[(mHead+i) % kGraphHistorySize] ? 
          1.f / (0.00001f + mValues[(mHead+i) % kGraphHistorySize]) : 0;
        if (v > 100.f)
          v = 100.f;

        vg->lineTo (org + cPointF (((float)i/(kGraphHistorySize -1)) * size.x, size.y - ((v / 100.f) * size.y)));
        }
      }
      //}}}
    else if (mStyle == eRenderPercent) {
      //{{{  percent graph
      for (int i = 0; i < kGraphHistorySize; i++) {
        float v = mValues[(mHead+i) % kGraphHistorySize] * 1.f;
        if (v > 100.f)
          v = 100.f;

        vg->lineTo (org +cPointF (((float)i / (kGraphHistorySize -1)) * size.x, size.y - ((v / 100.f) * size.y)));
        }
      }
      //}}}
    else {
      //{{{  ms graph
      for (int i = 0; i < kGraphHistorySize; i++) {
        float v = mValues[(mHead+i) % kGraphHistorySize] * 1000.f;
        if (v > 20.f)
          v = 20.f;

        vg->lineTo (org + cPointF (((float)i / (kGraphHistorySize -1)) * size.x, size.y - ((v / 20.f) * size.y)));
        }
      }
      //}}}
    vg->lineTo (org + size);
    vg->setFillColour (sColourF(1.f,0.75f,0.f,0.5f));
    vg->fill();

    // text
    vg->setFillColour (kWhiteF);

    // title text
    vg->setFontByName ("sans");
    if (!mName.empty()) {
      //{{{  name graph
      vg->setFontSize (14.f);
      vg->setTextAlign (cVg::eAlignLeft | cVg::eAlignTop);
      vg->text (org + cPointF (3.f,1.f), mName);
      }
      //}}}

    // average text
    float avg = getGraphAverage();
    if (mStyle == eRenderFps) {
      //{{{  fps graph
      vg->setFontSize (18.f);
      vg->setTextAlign (cVg::eAlignRight | cVg::eAlignTop);

      char str[64];
      sprintf (str, "%.2ffps", 1.f / avg);
      vg->text (org + cPointF (size.x-3.f, 1.f), str);

      vg->setFontSize (15.f);
      vg->setTextAlign (cVg::eAlignRight | cVg::eAlignBottom);
      sprintf (str, "%.2fms", avg * 1000.f);
      vg->text (org + size + cPointF (-3.f, -1.f), str);
      }
      //}}}
    else if (mStyle == eRenderPercent) {
      //{{{  percent graph
      vg->setFontSize (18.f);
      vg->setTextAlign (cVg::eAlignRight | cVg::eAlignTop);

      char str[64];
      sprintf (str, "%.1f%%", avg * 1.f);
      vg->text (org + cPointF (size.x-3.f, 1.f), str);
      }
      //}}}
    else {
      //{{{  ms graph
      vg->setFontSize (18.f);
      vg->setTextAlign (cVg::eAlignRight | cVg::eAlignTop);

      char str[64];
      sprintf (str, "%.2fms", avg * 1000.f);
      vg->text (org + cPointF(size.x-3.f, 1.f), str);
      }
      //}}}
    }
  //}}}

private:
  static constexpr int kGraphHistorySize = 100;
  //{{{
  float getGraphAverage() {

    int numValues = mNumValues < kGraphHistorySize ? mNumValues : kGraphHistorySize;
    if (!numValues)
      return 0.0f;

    float avg = 0;
    for (int i = 0; i < numValues; i++)
      avg += mValues[i];

    return avg / (float)numValues;
    }
  //}}}

  eRenderStyle mStyle;
  std::string mName;

  int mHead = 0;
  int mNumValues = 0;
  float mValues[kGraphHistorySize];

  float mStartTime = 0.0f;
  };
