// cWidget.h - base widget, draws as box with width-1, height-1
#pragma once
class cContainer;
#include "iDraw.h"

class cWidget {
public:
	#ifdef USE_NANOVG
		static uint16_t getBoxHeight()       { return 19; }
		static uint16_t getSmallFontHeight() { return 16; }
		static uint16_t getFontHeight()      { return 18; }
		static uint16_t getBigFontHeight()   { return 40; }
	#elif STM32F769I_DISCO
		static uint16_t getBoxHeight()       { return 30; }
		static uint16_t getSmallFontHeight() { return 20; }
		static uint16_t getFontHeight()      { return 26; }
		static uint16_t getBigFontHeight()   { return 52; }
	#else // STM32F746G_DISCO
		static uint16_t getBoxHeight()       { return 19; }
		static uint16_t getSmallFontHeight() { return 12; }
		static uint16_t getFontHeight()      { return 16; }
		static uint16_t getBigFontHeight()   { return 32; }
	#endif

	static float getPixel() { return 1.0f / getBoxHeight(); }

	//{{{  constructors, destructor
	cWidget() {}
	cWidget (float width)
		: mWidth(int(width * getBoxHeight())), mHeight(getBoxHeight()) {}
	cWidget (float width, float height)
		: mWidth(int(width * getBoxHeight())), mHeight(int(height * getBoxHeight())) {}
	cWidget (uint32_t colour, float width, float height)
		: mColour(colour), mWidth(int(width * getBoxHeight())), mHeight(int(height * getBoxHeight())) {}
	virtual ~cWidget() {}
	//}}}
	//{{{  gets
	float getX() { return mX / (float)getBoxHeight(); }
	float getY() { return mY / (float)getBoxHeight(); }
	float getWidth() { return (float)mWidth / (float)getBoxHeight(); }
	float getHeight() { return (float)mHeight / (float)getBoxHeight(); }

	int16_t getPixX() { return mX; }
	int16_t getPixY() { return mY; }
	int16_t getPixWidth() { return mWidth; }
	int16_t getPixHeight() { return mHeight; }

	int getPressedCount() { return mPressedCount; }
	bool isPressed() { return mPressedCount > 0; }
	bool isOn() { return mOn; }
	bool isVisible() { return mVisible; }
	//}}}
	//{{{  sets
	//{{{
	void setXY (float x, float y) {
		setPixXY (int (x*getBoxHeight()), int (y*getBoxHeight()));
		}
	//}}}

	//{{{
	void setPixXY (int16_t x, int16_t y) {
		mX = x;
		mY = y;
		}
	//}}}
	//{{{
	void setPixWidth (int16_t width) {
		mWidth = width;
		}
	//}}}
	//{{{
	void setPixHeight (int16_t height) {
		mHeight = height;
		}
	//}}}
	//{{{
	void setPixSize (int16_t width, int16_t height) {
		mWidth = width;
		mHeight = height;
		}
	//}}}

	void setColour (uint32_t colour) { mColour = colour; }
	void setParent (cContainer* parent) { mParent = parent; }

	void setOn (bool on) { mOn = on; }
	void setVisible (bool visible) { mVisible = visible; }

	void setOverPick (float overPick) { mOverPick = int(overPick * getBoxHeight()); }
	//}}}

	//{{{
	virtual void prox (int16_t x, int16_t y, bool controlled) {
		}
	//}}}
	//{{{
	virtual cWidget* picked (int16_t x, int16_t y, uint16_t z) {
		return (x >= mX - mOverPick) && (x < mX + mWidth + mOverPick) &&
					 (y >= mY - mOverPick) && (y < mY + mHeight + mOverPick) ? this : nullptr;
		}
	//}}}
	//{{{
	virtual void pressed (int16_t x, int16_t y, bool controlled) {
		if (!mPressedCount)
			mOn = true;
		mPressedCount++;
		}
	//}}}
	//{{{
	virtual void moved (int16_t x, int16_t y, uint16_t z, int16_t xinc, int16_t yinc, bool controlled) {
		}
	//}}}
	//{{{
	virtual void released() {
		mPressedCount = 0;
		mOn = false;
		}
	//}}}
	//{{{
	virtual void render (iDraw* draw) {
		draw->rectClipped (mOn ? COL_LIGHTRED : mColour, mX+1, mY+1, mWidth-1, mHeight-1);
		}
	//}}}

protected:
	uint32_t mColour = COL_LIGHTGREY;

	int16_t mX = 0;
	int16_t mY = 0;
	int16_t mWidth = 0;
	int16_t mHeight = 0;

	int mPressedCount = 0;
	bool mOn = false;
	uint16_t mOverPick = 0;

	cContainer* mParent = nullptr;
	bool mVisible = true;
	};
