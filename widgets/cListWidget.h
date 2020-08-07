// cListWidget.h - sring vector select
//{{{  includes
#pragma once
#include "cWidget.h"
//}}}

class cListWidget : public cWidget {
public:
  //{{{
  cListWidget (std::vector <std::string>& names, unsigned int& index, bool& indexChanged, float width, float height)
    : cWidget (COL_WHITE, width, height), mNames(names), mIndex(index), mIndexChanged(indexChanged) {}
  //}}}
  virtual ~cListWidget() {}

  //{{{
  void onDown (float x, float y) {

    mPressedIndex = ((int)mScroll + y) / getBoxHeight();
    mTextPressed = x < mMeasure[y / getBoxHeight()];

    mMoved = false;
    mMoveInc = 0;
    mScrollInc = 0.0f;
    }
  //}}}
  //{{{
  void onMove (float x, float y, float z, float xinc, float yinc) {

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
    mPressedIndex = 0;
    mMoved = false;
    mMoveInc = 0;
    }
  //}}}

  //{{{
  void onDraw (iDraw* draw) {

    if (!mTextPressed && mScrollInc)
      incScroll (mScrollInc * 0.9f);

    unsigned int index = int(mScroll) / getBoxHeight();
    int y = -(int(mScroll) % getBoxHeight());

    for (unsigned int i = 0; (y < mHeight-1) && (index < mNames.size()); i++, index++, y += getBoxHeight()) {
      if (i >= mMeasure.size())
        mMeasure.push_back (0);

      draw->rectClipped (0x40000000, mX, mY+y, mMeasure[i] + getBoxHeight()/2, getBoxHeight());

      mMeasure[i] = draw->drawText (
        (!mMoved && mTextPressed && (index == mPressedIndex)) ?
          COL_YELLOW : (index == mIndex) ? mColour : COL_LIGHTBLUE,
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
  unsigned int& mIndex;
  bool& mIndexChanged;

  bool mTextPressed = false;
  unsigned int mPressedIndex = 0;
  bool mMoved = false;
  int mMoveInc = 0;

  std::vector <int> mMeasure;

  float mScroll = 0.0f;
  float mScrollInc = 0.0f;
  };
