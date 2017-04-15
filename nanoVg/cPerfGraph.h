#pragma once

class cPerfGraph {
public:
  enum eRenderStyle { GRAPH_RENDER_FPS, GRAPH_RENDER_MS, GRAPH_RENDER_PERCENT };
  //{{{
  cPerfGraph (eRenderStyle style, std::string name) : mStyle(style), mName(name) {
    reset();
    }
  //}}}

  //{{{
  void reset() {
    mHead = 0;
    mNumValues = 0;
    for (int i = 0; i < kGRAPH_HISTORY_COUNT; i++)
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
    mHead = (mHead+1) % kGRAPH_HISTORY_COUNT;
    mValues[mHead] = time - mStartTime;
    mNumValues++;

    mStartTime = time;
    }
  //}}}
  //{{{
  void updateValue (float value) {
    mHead = (mHead+1) % kGRAPH_HISTORY_COUNT;
    mValues[mHead] = value;
    mNumValues++;
    }
  //}}}

  //{{{
  void render (cVg* vg, float x, float y, float w, float h) {

    // bgnd
    vg->beginPath();
    vg->rect (x,y, w,h);
    vg->fillColor (nvgRGBA (0,0,0,128));
    vg->fill();

    // graph
    vg->beginPath();
    vg->moveTo (x, y+h);
    if (mStyle == GRAPH_RENDER_FPS) {
      //{{{  fps graph
      for (int i = 0; i < kGRAPH_HISTORY_COUNT; i++) {
        float v =  mValues[(mHead+i) % kGRAPH_HISTORY_COUNT] ?
                     1.0f / (0.00001f + mValues[(mHead+i) % kGRAPH_HISTORY_COUNT]) : 0;
        if (v > 100.0f)
          v = 100.0f;

        float vx = x + ((float)i/(kGRAPH_HISTORY_COUNT-1)) * w;
        float vy = y + h - ((v / 100.0f) * h);

        vg->lineTo (vx, vy);
        }
      }
      //}}}
    else if (mStyle == GRAPH_RENDER_PERCENT) {
      //{{{  percent graph
      for (int i = 0; i < kGRAPH_HISTORY_COUNT; i++) {
        float v = mValues[(mHead+i) % kGRAPH_HISTORY_COUNT] * 1.0f;
        if (v > 100.0f)
          v = 100.0f;

        float vx = x + ((float)i / (kGRAPH_HISTORY_COUNT-1)) * w;
        float vy = y + h - ((v / 100.0f) * h);

        vg->lineTo (vx, vy);
        }
      }
      //}}}
    else {
      //{{{  ms graph
      for (int i = 0; i < kGRAPH_HISTORY_COUNT; i++) {
        float v = mValues[(mHead+i) % kGRAPH_HISTORY_COUNT] * 1000.0f;
        if (v > 20.0f)
          v = 20.0f;

        float vx = x + ((float)i / (kGRAPH_HISTORY_COUNT-1)) * w;
        float vy = y + h - ((v / 20.0f) * h);

        vg->lineTo (vx, vy);
        }
      }
      //}}}
    vg->lineTo (x+w, y+h);
    vg->fillColor (nvgRGBA (255,192,0,128));
    vg->fill();

    // title text
    vg->fontFace ("sans");
    if (!mName.empty()) {
      //{{{  name graph
      vg->fontSize (14.0f);
      vg->textAlign (cVg::ALIGN_LEFT | cVg::ALIGN_TOP);
      vg->fillColor (nvgRGBA(240,240,240,192));
      vg->text (x+3, y+1, mName);
      }
      //}}}

    // average text
    float avg = getGraphAverage();
    if (mStyle == GRAPH_RENDER_FPS) {
      //{{{  fps graph
      vg->fontSize (18.0f);
      vg->textAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
      vg->fillColor (nvgRGBA (240,240,240,255));

      char str[64];
      sprintf (str, "%.2ffps", 1.0f / avg);
      vg->text (x+w-3,y+1, str);

      vg->fontSize (15.0f);
      vg->textAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_BOTTOM);
      vg->fillColor (nvgRGBA (240,240,240,160));

      sprintf (str, "%.2fms", avg * 1000.0f);
      vg->text (x+w-3, y+h-1, str);
      }
      //}}}
    else if (mStyle == GRAPH_RENDER_PERCENT) {
      //{{{  percent graph
      vg->fontSize (18.0f);
      vg->textAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
      vg->fillColor (nvgRGBA (240,240,240,255));

      char str[64];
      sprintf (str, "%.1f%%", avg * 1.0f);
      vg->text (x+w-3, y+1, str);
      }
      //}}}
    else {
      //{{{  ms graph
      vg->fontSize (18.0f);
      vg->textAlign (cVg::ALIGN_RIGHT | cVg::ALIGN_TOP);
      vg->fillColor (nvgRGBA (240,240,240,255));

      char str[64];
      sprintf (str, "%.2fms", avg * 1000.0f);
      vg->text (x+w-3, y+1, str);
      }
      //}}}
    }
  //}}}

private:
  static const int kGRAPH_HISTORY_COUNT = 100;

  //{{{
  float getGraphAverage() {

    int numValues = mNumValues < kGRAPH_HISTORY_COUNT ? mNumValues : kGRAPH_HISTORY_COUNT;
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
  float mValues[kGRAPH_HISTORY_COUNT];

  float mStartTime = 0.0f;
  };
