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
  void render (cVg* vg, float x, float y, float w, float h) {

    // bgnd
    vg->beginPath();
    vg->rect (cPoint(x,y), cPoint(w,h));
    vg->fillColour (nvgRGBA (0,0,0,128));
    vg->fill();

    // graph
    vg->beginPath();
    vg->moveTo (cPoint(x, y+h));
    if (mStyle == eRenderFps) {
      //{{{  fps graph
      for (int i = 0; i < kGraphHistorySize; i++) {
        float v =  mValues[(mHead+i) % kGraphHistorySize] ?
                     1.f / (0.00001f + mValues[(mHead+i) % kGraphHistorySize]) : 0;
        if (v > 100.f)
          v = 100.f;

        float vx = x + ((float)i/(kGraphHistorySize -1)) * w;
        float vy = y + h - ((v / 100.f) * h);

        vg->lineTo (cPoint(vx, vy));
        }
      }
      //}}}
    else if (mStyle == eRenderPercent) {
      //{{{  percent graph
      for (int i = 0; i < kGraphHistorySize; i++) {
        float v = mValues[(mHead+i) % kGraphHistorySize] * 1.f;
        if (v > 100.f)
          v = 100.f;

        float vx = x + ((float)i / (kGraphHistorySize -1)) * w;
        float vy = y + h - ((v / 100.f) * h);

        vg->lineTo (cPoint(vx, vy));
        }
      }
      //}}}
    else {
      //{{{  ms graph
      for (int i = 0; i < kGraphHistorySize; i++) {
        float v = mValues[(mHead+i) % kGraphHistorySize] * 1000.f;
        if (v > 20.f)
          v = 20.f;

        float vx = x + ((float)i / (kGraphHistorySize -1)) * w;
        float vy = y + h - ((v / 20.f) * h);

        vg->lineTo (cPoint(vx, vy));
        }
      }
      //}}}
    vg->lineTo (cPoint(x+w, y+h));
    vg->fillColour (nvgRGBA (255,192,0,128));
    vg->fill();

    // title text
    vg->setFontByName ("sans");
    if (!mName.empty()) {
      //{{{  name graph
      vg->setFontSize (14.f);
      vg->setTextAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
      vg->fillColour (nvgRGBA(240,240,240,192));
      vg->text (x+3, y+1, mName);
      }
      //}}}

    // average text
    float avg = getGraphAverage();
    if (mStyle == eRenderFps) {
      //{{{  fps graph
      vg->setFontSize (18.f);
      vg->setTextAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
      vg->fillColour (nvgRGBA (240,240,240,255));

      char str[64];
      sprintf (str, "%.2ffps", 1.f / avg);
      vg->text (x+w-3,y+1, str);

      vg->setFontSize (15.f);
      vg->setTextAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_BOTTOM);
      vg->fillColour (nvgRGBA (240,240,240,160));

      sprintf (str, "%.2fms", avg * 1000.f);
      vg->text (x+w-3, y+h-1, str);
      }
      //}}}
    else if (mStyle == eRenderPercent) {
      //{{{  percent graph
      vg->setFontSize (18.f);
      vg->setTextAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
      vg->fillColour (nvgRGBA (240,240,240,255));

      char str[64];
      sprintf (str, "%.1f%%", avg * 1.f);
      vg->text (x+w-3, y+1, str);
      }
      //}}}
    else {
      //{{{  ms graph
      vg->setFontSize (18.f);
      vg->setTextAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
      vg->fillColour (nvgRGBA (240,240,240,255));

      char str[64];
      sprintf (str, "%.2fms", avg * 1000.f);
      vg->text (x+w-3, y+1, str);
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
