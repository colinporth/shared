// cFileNameContainer.h - fileName container
#pragma once
#include "cWidget.h"

class cListWidget : public cWidget {
public:
  //{{{
  cListWidget (std::vector <std::string>& names, int& index, bool& indexChanged, float width, float height)
    : cWidget (COL_WHITE, width, height), mNames(names), mIndex(index), mIndexChanged(indexChanged) {}
  //}}}
  virtual ~cListWidget() {}

  //{{{
  void onDown (int16_t x, int16_t y) {

    mPressedIndex = ((int)mScroll + y) / getBoxHeight();
    mTextPressed = x < mMeasure[y / getBoxHeight()];

    mMoved = false;
    mMoveInc = 0;
    mScrollInc = 0.0f;
    }
  //}}}
  //{{{
  void onMove (int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc) {

    mMoveInc += yinc;

    if (abs(mMoveInc) > 2)
      mMoved = true;

    if (mMoved)
      incScroll (-(float)yinc);
    }
  //}}}
  //{{{
  void onUp() {

    if (mTextPressed && !mMoved) {
      mIndex = mPressedIndex;
      mIndexChanged = true;
      }

    mTextPressed = false;
    mPressedIndex = -1;
    mMoved = false;
    mMoveInc = 0;
    }
  //}}}

  //{{{
  void onDraw (iDraw* draw) {

    if (!mTextPressed && mScrollInc)
      incScroll (mScrollInc * 0.9f);

    int index = int(mScroll) / getBoxHeight();
    int y = -(int(mScroll) % getBoxHeight());

    for (int i = 0; (y < mHeight-1) && (index < (int)mNames.size()); i++, index++, y += getBoxHeight()) {
      if (i >= (int)mMeasure.size())
        mMeasure.push_back (0);
      mMeasure[i] = draw->drawText (
        mTextPressed && !mMoved && (index == mPressedIndex) ? COL_YELLOW : (index == mIndex) ? mColour : COL_LIGHTGREY,
        getFontHeight(), mNames[index], mX+2, mY+y+1, mWidth-1, mHeight-y-1);
      }
    }
  //}}}

private:
  //{{{
  void incScroll (float inc) {

    mScroll += inc;

    if (mScroll < 0.0f)
      mScroll = 0.0f;
    else if (mScroll > ((mNames.size() * getBoxHeight()) - mHeight))
      mScroll = float(((int)mNames.size() * getBoxHeight()) - mHeight);

    mScrollInc = fabs(inc) < 0.2f ? 0 : inc;
    }
  //}}}

  std::vector <std::string>& mNames;
  int& mIndex;
  bool& mIndexChanged;

  bool mTextPressed = false;
  int mPressedIndex = -1;
  bool mMoved = false;
  int mMoveInc = 0;

  std::vector <int> mMeasure;

  float mScroll = 0.0f;
  float mScrollInc = 0.0f;
  };
